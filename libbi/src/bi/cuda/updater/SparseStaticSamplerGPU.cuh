/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev: 2636 $
 * $Date: 2012-05-31 20:44:30 +0800 (Thu, 31 May 2012) $
 */
#ifndef BI_CUDA_UPDATER_SPARSESTATICSAMPLERGPU_CUH
#define BI_CUDA_UPDATER_SPARSESTATICSAMPLERGPU_CUH

#include "../../random/Random.hpp"
#include "../../state/State.hpp"
#include "../../buffer/Mask.hpp"
#include "../../method/misc.hpp"

namespace bi {
/**
 * Sparse static sampling, on device.
 *
 * @ingroup method_updater
 *
 * @tparam B Model type.
 * @tparam S Action type list.
 */
template<class B, class S>
class SparseStaticSamplerGPU {
public:
  static void update(Random& rng, State<B,ON_DEVICE>& s, Mask<ON_DEVICE>& mask);
};
}

#include "SparseStaticSamplerKernel.cuh"
#include "../bind.cuh"
#include "../device.hpp"

template<class B, class S>
void bi::SparseStaticSamplerGPU<B,S>::update(Random& rng,
    State<B,ON_DEVICE>& s, Mask<ON_DEVICE>& mask) {
  const int P = s.size();

  if (mask.size() > 0) {
    dim3 Dg, Db;

    Db.x = deviceIdealThreadsPerBlock();
    Dg.x = (std::min(P, deviceIdealThreads()) + Db.x - 1)/Db.x;

    bind(s);
    Random rng1(rng);
    kernelSparseStaticSampler<B,S><<<Dg,Db>>>(rng1, mask);
    CUDA_CHECK;
    unbind(s);
  }
}

#endif