/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_CUDA_RESAMPLER_RESAMPLERKERNEL_CUH
#define BI_CUDA_RESAMPLER_RESAMPLERKERNEL_CUH

#include "misc.hpp"
#include "../cuda.hpp"

namespace bi {
/**
 * ResamplerGPU::ancestorsToOffspring() kernel.
 *
 * @tparam V1 Integer vector type.
 * @tparam V2 Integer vector type.
 *
 * @param as Ancestry.
 * @param[out] os Offspring. All elements should be initialised to 0.
 */
template<class V1, class V2>
CUDA_FUNC_GLOBAL void kernelAncestorsToOffspring(const V1 as, V2 os);

/**
 * ResamplerGPU::cumulativeOffspringToAncestorsPermute() kernel.
 *
 * @tparam V1 Integer vector type.
 * @tparam V2 Integer vector type.
 * @tparam V3 Integer vector type.
 * @tparam PrePermute Do pre-permute step?
 *
 * @param Os Cumulative offspring.
 * @param[out] as Ancestry.
 * @param[out] is Claims.
 * @param doPrePermute Either ENABLE_PRE_PERMUTE or DISABLE_PRE_PERMUTE to
 * enable or disable pre-permute step, respectively.
 */
template<class V1, class V2, class V3, class PrePermute>
CUDA_FUNC_GLOBAL void kernelCumulativeOffspringToAncestors(
    const V1 Os, V2 as, V3 is, const PrePermute doPrePermute);

/**
 * ResamplerGPU::prePermute() kernel.
 *
 * @tparam V1 Integer vector type.
 * @tparam V2 Integer vector type.
 *
 * @param as Ancestry to permute.
 * @param is[out] Claims.
 */
template<class V1, class V2>
CUDA_FUNC_GLOBAL void bi::kernelResamplerPrePermute(const V1 as, V2 is);

/**
 * ResamplerGPU::postPermute() kernel.
 *
 * @tparam V1 Integer vector type.
 * @tparam V2 Integer vector type.
 * @tparam V3 Integer vector type.
 *
 * @param as Ancestry to permute.
 * @param is[in,out] Workspace vector, in state as returned from
 * #kernelResamplerPrePermute.
 * @param out[out] Permuted ancestry vector.
 *
 * Before calling this kernel, #kernelResamplerPrePermute should be called.
 * The remaining places in @p is are now claimed, and each thread @c i sets
 * <tt>out(is(i)) = as(i)</tt>.
 */
template<class V1, class V2, class V3>
CUDA_FUNC_GLOBAL void kernelResamplerPostPermute(const V1 as, const V2 is,
    V3 out);

/**
 * ResamplerGPU::copy() kernel.
 *
 * @tparam V1 Integer vector type.
 * @tparam M1 Matrix type.
 *
 * @param as Ancestry.
 * @param[in,out] X Matrix to copy.
 */
template<class V1, class M1>
CUDA_FUNC_GLOBAL void kernelResamplerCopy(const V1 as, M1 X);

}

template<class V1, class V2>
CUDA_FUNC_GLOBAL void bi::kernelAncestorsToOffspring(const V1 as, V2 os) {
  const int P = as.size();
  const int p = blockIdx.x*blockDim.x + threadIdx.x;
  if (p < P) {
    atomicAdd(&os(as(p)), 1);
  }
}

template<class V1, class V2, class V3, class PrePermute>
CUDA_FUNC_GLOBAL void bi::kernelCumulativeOffspringToAncestors(const V1 Os,
    V2 as, V3 is, const PrePermute doPrePermute) {
  const int P = Os.size(); // number of trajectories
  const int p = blockIdx.x*blockDim.x + threadIdx.x; // trajectory id

  if (p < P) {
    int O1 = (p > 0) ? Os(p - 1) : 0;
    int O2 = Os(p);
    int o = O2 - O1;

    for (int i = 0; i < o; ++i) {
      as(O1 + i) = p;
    }
    if (doPrePermute) {
      is(p) = (o > 0) ? O1 : P;
    }
  }

//  const int P = as.size(); // number of trajectories
//  const int p = blockIdx.x*blockDim.x + threadIdx.x; // trajectory id
//  const int W = (P + warpSize - 1)/warpSize; // number of warps
//  const int w = p % warpSize; // warp id
//  const int q = threadIdx.x % warpSize; // id in warp
//
//  /* Each warp is responsible for warpSize number of elements in as,
//   * starting from index q*Q. First do binary search of Os to work out where
//   * to start. Note each access of Os(pivot) is the same for each thread of a
//   * warp, so the read is efficiently broadcast to all of its threads. */
//  int start = 0, end = P, O;
//  do {
//    pivot = (end - start)/2; /// @todo Try something dependent on q
//    O = Os(pivot);
//    if (w*W < O) {
//      end = pivot;
//    } else {
//      start = pivot;
//    }
//  } while (start != end);
//
//  /* work out the ancestor for the element corresponding to each thread in
//   * the warp */
//  int rem, a = -1, i = start;
//  do {
//    O = Os(i); // read will always be broadcast
//    rem = O - w*W;
//    if (a == -1 && q < rem) { // a == -1 ensures first satisfactory i taken
//      a = i;
//    }
//    ++i;
//  } while (rem < warpSize);
//  as(p) = a; // write will always be coalesced
}

template<class V1, class V2>
CUDA_FUNC_GLOBAL void bi::kernelResamplerPrePermute(const V1 as, V2 is) {
  const int P = as.size();
  const int p = blockIdx.x*blockDim.x + threadIdx.x;

  if (p < P) {
    atomicMin(&is(as(p)), p);
  }
}

template<class V1, class V2, class V3>
CUDA_FUNC_GLOBAL void bi::kernelResamplerPostPermute(const V1 as, const V2 is,
    V3 out) {
  const int P = as.size();
  const int p = blockIdx.x*blockDim.x + threadIdx.x;

  if (p < P) {
    int a = as(p), next, i;

    next = a;
    i = is(next);
    if (i != p) {
      // claim in pre-permute kernel was unsuccessful, try own spot next
      next = p;
      i = is(next);
      while (i < P) { // and chase tail of rotation until free spot
        next = i;
        i = is(next);
      }
    }

    /* write ancestor into claimed place, note the out vector is required
     * or this would cause a race condition with the read of as(p)
     * above, so this cannot be done in-place */
    out(next) = a;
  }
}

template<class V1, class M1>
CUDA_FUNC_GLOBAL void bi::kernelResamplerCopy(const V1 as, M1 X) {
  const int p = blockIdx.x*blockDim.x + threadIdx.x;
  const int id = blockIdx.y*blockDim.y + threadIdx.y;

  if (p < X.size1() && id < X.size2()/* && as(p) != p*/) {
    // ^ extra condition above would destroy coalesced writes
    X(p, id) = X(as(p), id);
  }
}

#endif
