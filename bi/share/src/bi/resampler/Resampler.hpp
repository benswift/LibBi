/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_RESAMPLER_RESAMPLER_HPP
#define BI_RESAMPLER_RESAMPLER_HPP

#include "../state/State.hpp"
#include "../random/Random.hpp"
#include "../misc/location.hpp"

namespace bi {
/**
 * @internal
 *
 * Determine error in particular resampling.
 */
template<class T>
struct resample_error : public std::binary_function<T,int,T> {
  const T lW;
  const T P;
  // ^ oddly, casting o or P in operator()() causes a hang with CUDA 3.1 on
  //   Fermi, so we set the type of P to T instead of int

  /**
   * Constructor.
   */
  CUDA_FUNC_HOST resample_error(const T lW, const int P) : lW(lW),
      P(P) {
    //
  }

  /**
   * Apply functor.
   *
   * @param lw Log-weight for this index.
   * @param o Number of offspring for this index.
   *
   * @return Contribution to error for this index.
   */
  CUDA_FUNC_BOTH T operator()(const T& lw, const int& o) {
    T eps;

    if (bi::is_finite(lw)) {
      eps = bi::exp(lw - lW) - o/P; // P of type T, not int, see note above
      eps *= eps;
    } else {
      eps = 0.0;
    }

    return eps;
  }
};

/**
 * %Resampler for particle filter.
 *
 * @ingroup method_resampler
 */
class Resampler {
public:
  /**
   * @name Low-level interface.
   */
  //@{
  /**
   * Compute offspring vector from ancestors vector.
   *
   * @tparam V1 Integral vector type.
   * @tparam V2 Integral vector type.
   *
   * @param as Ancestors.
   * @param[out] os Offspring.
   */
  template<class V1, class V2>
  static void ancestorsToOffspring(const V1 as, V2 os);

  /**
   * Compute ancestor vector from offspring vector.
   *
   * @tparam V1 Integral vector type.
   * @tparam V2 Integral vector type.
   *
   * @param os Offspring.
   * @param[out] as Ancestors.
   */
  template<class V1, class V2>
  static void offspringToAncestors(const V1 os, V2 as);

  /**
   * Compute already-permuted ancestor vector from offspring vector.
   *
   * @tparam V1 Integral vector type.
   * @tparam V2 Integral vector type.
   *
   * @param os Offspring.
   * @param[out] as Ancestors.
   */
  template<class V1, class V2>
  static void offspringToAncestorsPermute(const V1 os, V2 as);

  /**
   * Compute ancestor vector from cumulative offspring vector.
   *
   * @tparam V1 Integral vector type.
   * @tparam V2 Integral vector type.
   *
   * @param Os Cumulative offspring.
   * @param[out] as Ancestors.
   */
  template<class V1, class V2>
  static void cumulativeOffspringToAncestors(const V1 Os, V2 as);

  /**
   * Compute already-permuted ancestor vector from cumulative offspring
   * vector.
   *
   * @tparam V1 Integral vector type.
   * @tparam V2 Integral vector type.
   *
   * @param Os Cumulative offspring.
   * @param[out] as Ancestors.
   */
  template<class V1, class V2>
  static void cumulativeOffspringToAncestorsPermute(const V1 Os, V2 as);

  /**
   * Permute ancestors to permit in-place copy.
   *
   * @tparam V1 Integral vector type.
   *
   * @param[in,out] as Ancestry.
   */
  template<class V1>
  static void permute(V1 as);

  /**
   * Correct weights after resampling with proposal.
   *
   * @tparam V1 Integral vector type.
   * @tparam V2 Vector type.
   * @tparam V2 Vector type.
   *
   * @param as Ancestry.
   * @param qlws Proposal log-weights.
   * @param[in,out] lws Log-weights.
   *
   * Assuming that a resample has been performed using the weights @p qlws,
   * The weights @p lws are set as importance weights, such that if
   * \f$a^i = p\f$, \f$w^i = w^p/w^{*p}\f$, where \f$w^{*p}\f$ are the
   * proposal weights (@p qlws) and \f$w^p\f$ the particle weights (@p lws).
   */
  template<class V1, class V2, class V3>
  static void correct(const V1 as, const V2 qlws, V3 lws);

  /**
   * In-place copy based on ancestry.
   *
   * @tparam V1 Vector type.
   * @tparam M1 Matrix type.
   *
   * @param as Ancestry.
   * @param[in,out] X Matrix. Rows of the matrix are copied.
   *
   * The copy is performed in-place. For each particle @c i that is to be
   * preserved (i.e. its offspring count is at least 1), @c a[i] should equal
   * @c i. This ensures that all particles are either read or (over)written,
   * but not both. Use permute() to ensure that an ancestry satisfies this
   * constraint.
   */
  template<class V1, class M1>
  static void copy(const V1 as, M1 X);

  /**
   * In-place copy based on ancestry.
   *
   * @tparam V1 Vector type.
   * @tparam B Model type.
   * @tparam L Location.
   *
   * @param as Ancestry.
   * @param[in,out] s State.
   */
  template<class V1, class B, Location L>
  static void copy(const V1 as, State<B,L>& s);

  /**
   * In-place copy based on ancestry.
   *
   * @tparam V1 Vector type.
   * @tparam T1 Assignable type.
   *
   * @param as Ancestry,
   * @oaram[in,out] v STL vector.
   */
  template<class V1, class T1>
  static void copy(const V1 as, std::vector<T1*>& v);

  /**
   * Compute effective sample size (ESS) of log-weights.
   *
   * @tparam V1 Vector type.
   *
   * @tparam lws Log-weights.
   *
   * @return ESS.
   */
  template<class V1>
  static typename V1::value_type ess(const V1 lws);

  /**
   * Compute sum of squared errors of ancestry.
   *
   * @tparam V1 Vector type.
   * @tparam V2 Integral vector type.
   *
   * @param lws Log-weights.
   * @param os Offspring.
   *
   * @return Squared error.
   *
   * This computes the sum of squared error in the resampling, as in
   * @ref Kitagawa1996 "Kitagawa (1996)":
   *
   * \f[
   * \xi = \sum_{i=1}^P \left(\frac{o_i}{P} - \frac{w_i}{W}\right)^2\,,
   * \f]
   *
   * where \f$W\f$ is the sum of weights.
   */
  template<class V1, class V2>
  static typename V1::value_type error(const V1 lws, const V2 os);
  //@}
};

/**
 * Resampler implementation on host.
 */
class ResamplerHost {
public:
  /**
   * @copydoc Resampler::ancestorsToOffspring()
   */
  template<class V1, class V2>
  static void ancestorsToOffspring(const V1 as, V2 os);

  /**
   * @copydoc Resampler::offspringToAncestors()
   */
  template<class V1, class V2>
  static void offspringToAncestors(const V1 os, V2 as);

  /**
   * @copydoc Resampler::offspringToAncestorsPermute()
   */
  template<class V1, class V2>
  static void offspringToAncestorsPermute(const V1 os, V2 as);

  /**
   * @copydoc Resampler::cumulativeOffspringToAncestors()
   */
  template<class V1, class V2>
  static void cumulativeOffspringToAncestors(const V1 Os, V2 as);

  /**
   * @copydoc Resampler::cumulativeOffspringToAncestorsPermute()
   */
  template<class V1, class V2>
  static void cumulativeOffspringToAncestorsPermute(const V1 Os, V2 as);

  /**
   * @copydoc Resampler::permute()
   */
  template<class V1>
  static void permute(V1 as);

  /**
   * @copydoc Resampler::copy()
   */
  template<class V1, class M1>
  static void copy(const V1 as, M1 X);

};

/**
 * Resampler implementation on device.
 */
class ResamplerGPU {
public:
  /**
   * @copydoc Resampler::ancestorsToOffspring()
   */
  template<class V1, class V2>
  static void ancestorsToOffspring(const V1 as, V2 os);

  /**
   * @copydoc Resampler::offspringToAncestors()
   */
  template<class V1, class V2>
  static void offspringToAncestors(const V1 os, V2 as);

  /**
   * @copydoc Resampler::offspringToAncestorsPermute()
   */
  template<class V1, class V2>
  static void offspringToAncestorsPermute(const V1 os, V2 as);

  /**
   * Like offspringToAncestorsPermute(), but only performs first stage of
   * permutation. Second stage should be completed with postPermute().
   */
  template<class V1, class V2, class V3>
  static void offspringToAncestorsPrePermute(const V1 os, V2 as, V3 is);

  /**
   * @copydoc Resampler::cumulativeOffspringToAncestors()
   */
  template<class V1, class V2>
  static void cumulativeOffspringToAncestors(const V1 Os, V2 as);

  /**
   * @copydoc Resampler::cumulativeOffspringToAncestorsPermute()
   */
  template<class V1, class V2>
  static void cumulativeOffspringToAncestorsPermute(const V1 Os, V2 as);

  /**
   * Like cumulativeOffspringToAncestorsPermute(), but only performs first
   * stage of permutation. Second stage should be completed with
   * postPermute().
   */
  template<class V1, class V2, class V3>
  static void cumulativeOffspringToAncestorsPrePermute(const V1 Os, V2 as,
      V3 is);

  /**
   * @copydoc Resampler::permute()
   */
  template<class V1>
  static void permute(V1 as);

  /**
   * First stage of permutation.
   *
   * @tparam V1 Integer vector type.
   * @tparam V2 Integer vector type.
   *
   * @param as Input ancestry.
   * @param is[out] Claims.
   */
  template<class V1, class V2>
  static void prePermute(const V1 as, V2 is);

  /**
   * Second stage of permutation.
   *
   * @tparam V1 Integer vector type.
   * @tparam V2 Integer vector type.
   * @tparam V3 Integer vector type.
   *
   * @param as Input ancestry.
   * @param is Claims, as output from pre-permute function.
   * @param[out] cs Output, permuted ancestry.
   */
  template<class V1, class V2, class V3>
  static void postPermute(const V1 as, const V2 is, V3 cs);

  /**
   * @copydoc Resampler::copy()
   */
  template<class V1, class M1>
  static void copy(const V1 as, M1 s);
};
}

#include "../host/resampler/ResamplerHost.hpp"
#ifdef __CUDACC__
#include "../cuda/resampler/ResamplerGPU.cuh"
#endif

#include "../primitive/vector_primitive.hpp"

#include "thrust/inner_product.h"

#include "boost/mpl/if.hpp"

template<class V1, class V2>
void bi::Resampler::ancestorsToOffspring(const V1 as, V2 os) {
  typedef typename boost::mpl::if_c<V1::on_device,
      ResamplerGPU,
      ResamplerHost>::type impl;
  impl::ancestorsToOffspring(as, os);
}

template<class V1, class V2>
void bi::Resampler::offspringToAncestors(const V1 os, V2 as) {
  typedef typename boost::mpl::if_c<V1::on_device,
      ResamplerGPU,
      ResamplerHost>::type impl;
  impl::offspringToAncestors(os, as);
}

template<class V1, class V2>
void bi::Resampler::offspringToAncestorsPermute(const V1 os, V2 as) {
  typedef typename boost::mpl::if_c<V1::on_device,
      ResamplerGPU,
      ResamplerHost>::type impl;
  impl::offspringToAncestorsPermute(os, as);
}

template<class V1, class V2>
void bi::Resampler::cumulativeOffspringToAncestors(const V1 Os, V2 as) {
  typedef typename boost::mpl::if_c<V1::on_device,
      ResamplerGPU,
      ResamplerHost>::type impl;
  impl::cumulativeOffspringToAncestors(Os, as);
}

template<class V1, class V2>
void bi::Resampler::cumulativeOffspringToAncestorsPermute(const V1 Os,
    V2 as) {
  typedef typename boost::mpl::if_c<V1::on_device,
      ResamplerGPU,
      ResamplerHost>::type impl;
  impl::cumulativeOffspringToAncestorsPermute(Os, as);
}

template<class V1>
void bi::Resampler::permute(const V1 as) {
  typedef typename boost::mpl::if_c<V1::on_device,
      ResamplerGPU,
      ResamplerHost>::type impl;
  impl::permute(as);
}

template<class V1, class V2, class V3>
void bi::Resampler::correct(const V1 as, const V2 qlws, V3 lws) {
  /* pre-condition */
  BI_ASSERT(qlws.size() == lws.size());

  typedef typename sim_temp_vector<V3>::type vector_type;
  typedef typename V3::value_type T3;

  const int P = as.size();

  vector_type lws1(lws.size());
  lws1 = lws;
  lws.resize(P);

  BOOST_AUTO(iter1, thrust::make_permutation_iterator(lws1.begin(), as.begin()));
  BOOST_AUTO(iter2, thrust::make_permutation_iterator(qlws.begin(), as.begin()));
  thrust::transform(iter1, iter1 + P, iter2, lws.begin(), thrust::minus<T3>());
}

template<class V1, class M1>
void bi::Resampler::copy(const V1 as, M1 s) {
  /* pre-condition */
  BI_ASSERT(as.size() <= s.size1());

  typedef typename boost::mpl::if_c<M1::on_device,
      ResamplerGPU,
      ResamplerHost>::type impl;
  impl::copy(as, s);
}

template<class V1, class B, bi::Location L>
void bi::Resampler::copy(const V1 as, State<B,L>& s) {
  s.setRange(s.start(), bi::max(s.size(), as.size()));
  copy(as, s.getDyn());
  s.setRange(s.start(), as.size());
}

template<class V1, class T1>
void bi::Resampler::copy(const V1 as, std::vector<T1*>& v) {
  /* pre-condition */
  BI_ASSERT(!V1::on_device);

  #pragma omp parallel for
  for (int i = 0; i < as.size(); ++i) {
    int a = as(i);
    if (i != a) {
      v[i]->resize(v[a]->size());
      *v[i] = *v[a];
    }
  }
}

template<class V1>
typename V1::value_type bi::Resampler::ess(const V1 lws) {
  return ess_reduce(lws);
}

template<class V1, class V2>
typename V1::value_type bi::Resampler::error(const V1 lws, const V2 os) {
  real lW = logsumexp_reduce(lws);

  return thrust::inner_product(lws.begin(), lws.end(), os.begin(), BI_REAL(0.0),
      thrust::plus<real>(), resample_error<real>(lW, lws.size()));
}

#endif
