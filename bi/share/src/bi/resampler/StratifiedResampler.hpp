/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_RESAMPLER_STRATIFIEDRESAMPLER_HPP
#define BI_RESAMPLER_STRATIFIEDRESAMPLER_HPP

#include "Resampler.hpp"
#include "../cuda/cuda.hpp"
#include "../math/vector.hpp"
#include "../state/State.hpp"
#include "../random/Random.hpp"
#include "../misc/exception.hpp"

namespace bi {
/**
 * StratifiedResampler implementation on host.
 */
class StratifiedResamplerHost: public ResamplerHost {
public:
  /**
   * @copydoc StratifiedResampler::op
   */
  template<class V1, class V2>
  static void op(Random& rng, const V1 Ws, V2 Os, const int n);
};

/**
 * StratifiedResampler implementation on device.
 */
class StratifiedResamplerGPU: public ResamplerGPU {
public:
  /**
   * @copydoc StratifiedResampler::op
   */
  template<class V1, class V2>
  static void op(Random& rng, const V1 Ws, V2 Os, const int n);
};

/**
 * Stratified resampler for particle filter.
 *
 * @ingroup method_resampler
 *
 * Stratified resampler based on the scheme of
 * @ref Kitagawa1996 "Kitagawa (1996)", with optional pre-sorting.
 */
class StratifiedResampler: public Resampler {
public:
  /**
   * Constructor.
   *
   * @param sort True to pre-sort weights, false otherwise.
   * @param essRel Minimum ESS, as proportion of total number of particles,
   * to trigger resampling.
   */
  StratifiedResampler(const bool sort = true, const double essRel = 0.5);

  /**
   * @name High-level interface
   */
  //@{
  /**
   * @copydoc concept::Resampler::resample(Random&, V1, V2, O1&)
   */
  template<class V1, class V2, class O1>
  bool resample(Random& rng, V1 lws, V2 as, O1& s)
      throw (ParticleFilterDegeneratedException);

  /**
   * @copydoc concept::Resampler::resample(Random&, const V1, V2, V3, O1&)
   */
  template<class V1, class V2, class V3, class O1>
  bool resample(Random& rng, const V1 qlws, V2 lws, V3 as, O1& s)
      throw (ParticleFilterDegeneratedException);

  /**
   * @copydoc concept::Resampler::resample(Random&, const int, const V1, V2, V3, O1&)
   */
  template<class V1, class V2, class V3, class O1>
  bool resample(Random& rng, const int a, const V1 qlws, V2 lws, V3 as, O1& s)
      throw (ParticleFilterDegeneratedException);

  /**
   * @copydoc concept::Resampler::resample(Random&, const int, V1, V2, O1&)
   */
  template<class V1, class V2, class O1>
  bool cond_resample(Random& rng, const int ka, const int k, V1 lws, V2 as,
      O1& s) throw (ParticleFilterDegeneratedException);
  //@}

  /**
   * @name Low-level interface
   */
  //@{
  /**
   * @copydoc concept::Resampler::offspring
   */
  template<class V1, class V2>
  void offspring(Random& rng, const V1 lws, V2 o, const int P)
      throw (ParticleFilterDegeneratedException);

  template<class V1, class V2, class V3, class V4>
  void offspring(Random& rng, const V1 lws, V2 o, const int n, int ka,
      bool sorted, V3 lws1, V4 ps, V3 Ws)
          throw (ParticleFilterDegeneratedException);

  template<class V1, class V2, class V3, class V4>
  void offspring(Random& rng, const V1 lws, V2 os, const int n, bool sorted,
      V3 lws1, V4 ps, V3 Ws) throw (ParticleFilterDegeneratedException);

  /**
   * @copydoc concept::Resampler::cumulativeoOffspring
   */
  template<class V1, class V2>
  void cumulativeOffspring(Random& rng, const V1 lws, V2 Os, const int P)
      throw (ParticleFilterDegeneratedException);

  template<class V1, class V2, class V3, class V4>
  void cumulativeOffspring(Random& rng, const V1 lws, V2 Os, const int n,
      int ka, bool sorted, V3 lws1, V4 ps, V3 Ws)
          throw (ParticleFilterDegeneratedException);

  template<class V1, class V2, class V3, class V4>
  void cumulativeOffspring(Random& rng, const V1 lws, V2 Os, const int n,
      bool sorted, V3 lws1, V4 ps, V3 Ws)
          throw (ParticleFilterDegeneratedException);

  /**
   * @copydoc concept::Resampler::ancestors
   */
  template<class V1, class V2>
  void ancestors(Random& rng, const V1 lws, V2 as)
      throw (ParticleFilterDegeneratedException);

  template<class V1, class V2, class V3, class V4>
  void ancestors(Random& rng, const V1 lws, V2 as, int P, bool sorted,
      V3 lws1, V4 ps, V3 Ws) throw (ParticleFilterDegeneratedException);

  template<class V1, class V2, class V3, class V4>
  void ancestors(Random& rng, const V1 lws, V2 as, int P, int ka, int k,
      bool sorted, V3 lws1, V4 ps, V3 Ws)
          throw (ParticleFilterDegeneratedException);
  //@}

protected:
  /**
   * Compute cumulative offspring vector.
   *
   * @tparam V1 Vector type.
   * @tparam V2 Integer vector type.
   *
   * @param[in,out] rng Random number generator.
   * @param Ws Cumulative weight vector.
   * @param[out] Os Cumulative offspring vector.
   * @param n Number of offspring.
   */
  template<class V1, class V2>
  static void op(Random& rng, const V1 Ws, V2 Os, const int n);

  /**
   * Pre-sort weights?
   */
  bool sort;
};
}

#include "../host/resampler/StratifiedResamplerHost.hpp"
#ifdef __CUDACC__
#include "../cuda/resampler/StratifiedResamplerGPU.cuh"
#endif

#include "../primitive/vector_primitive.hpp"
#include "../primitive/matrix_primitive.hpp"
#include "../misc/location.hpp"
#include "../math/temp_vector.hpp"
#include "../math/sim_temp_vector.hpp"

#include "thrust/sequence.h"
#include "thrust/fill.h"
#include "thrust/extrema.h"
#include "thrust/transform.h"
#include "thrust/reduce.h"
#include "thrust/scan.h"
#include "thrust/transform_scan.h"
#include "thrust/for_each.h"

template<class V1, class V2, class O1>
bool bi::StratifiedResampler::resample(Random& rng, V1 lws, V2 as, O1& s)
    throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(lws.size() == as.size());

  bool r = isTriggered(lws);
  if (r) {
    const int P = lws.size();
    typename sim_temp_vector<V2>::type Os(P);

    cumulativeOffspring(rng, lws, Os, P);
    cumulativeOffspringToAncestorsPermute(Os, as);
    lws.clear();
    copy(as, s);
  } else {
    normalise(lws);
    seq_elements(as, 0);
  }
  return r;
}

template<class V1, class V2, class V3, class O1>
bool bi::StratifiedResampler::resample(Random& rng, const V1 qlws, V2 lws,
    V3 as, O1& s) throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(qlws.size() == lws.size());

  bool r = isTriggered(lws);
  if (r) {
    const int P = lws.size();
    typename sim_temp_vector<V3>::type Os(P);

    cumulativeOffspring(rng, qlws, Os, P);
    cumulativeOffspringToAncestorsPermute(Os, as);
    correct(as, qlws, lws);
    normalise(lws);
    copy(as, s);
  } else {
    normalise(lws);
    seq_elements(as, 0);
  }
  return r;
}

template<class V1, class V2, class V3, class O1>
bool bi::StratifiedResampler::resample(Random& rng, const int a,
    const V1 qlws, V2 lws, V3 as, O1& s)
        throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(qlws.size() == lws.size());

  bool r = isTriggered(lws);
  if (r) {
    const int P = lws.size();
    typename sim_temp_vector<V3>::type Os(P);

    cumulativeOffspring(rng, qlws, Os, P - 1);
    BOOST_AUTO(tail, subrange(Os, a, Os.size() - a));
    addscal_elements(tail, 1, tail);
    cumulativeOffspringToAncestorsPermute(Os, as);
    correct(as, qlws, lws);
    normalise(lws);
    copy(as, s);
  } else {
    normalise(lws);
    seq_elements(as, 0);
  }
  return r;
}

template<class V1, class V2, class O1>
bool bi::StratifiedResampler::cond_resample(Random& rng, const int ka,
    const int k, V1 lws, V2 as, O1& s)
        throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(lws.size() == as.size());
  BI_ASSERT(k >= 0 && k < as.size());
  BI_ASSERT(ka >= 0 && ka < lws.size());
  BI_ASSERT(k == 0 && ka == 0);

  bool r = isTriggered(lws);
  if (r) {
    const int P = lws.size();
    typename sim_temp_vector<V2>::type Os(P);

    int P2;
    if (!sort) {
      // change this?
      P2 = 0;
    } else {
      P2 = s.size();
    }
    typename sim_temp_vector<V1>::type lws1(P2), Ws(P2);
    typename sim_temp_vector<V2>::type ps(P2);

    cumulativeOffspring(rng, lws, Os, P, ka, false, lws1, ps, Ws);
    cumulativeOffspringToAncestorsPermute(Os, as);
    BI_ASSERT(*(as.begin() + k) == ka);
    copy(as, s);
    lws.clear();
  } else {
    normalise(lws);
    seq_elements(as, 0);
  }
  return r;
}

template<class V1, class V2>
void bi::StratifiedResampler::offspring(Random& rng, const V1 lws, V2 os,
    const int n) throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(lws.size() == os.size());

  typedef typename V1::value_type T1;
  typedef typename sim_temp_vector<V1>::type vector_type;
  typedef typename sim_temp_vector<V2>::type int_vector_type;

  const int P = lws.size();

  if (sort) {
    vector_type lws1(P), Ws(P);
    int_vector_type ps(P), Os(P), temp(P);

    lws1 = lws;
    seq_elements(ps, 0);
    bi::sort_by_key(lws1, ps);
    sumexpu_inclusive_scan(lws1, Ws);

    T1 W = *(Ws.end() - 1);  // sum of weights
    if (W > 0) {
      op(rng, Ws, Os, n);
      bi::adjacent_difference(Os, temp);
      bi::scatter(ps, temp, os);

#ifndef NDEBUG
      int m = sum_reduce(os);
      BI_ASSERT_MSG(m == n,
          "Stratified resampler gives " << m << " offspring, should give " << n);
#endif
    } else {
      throw ParticleFilterDegeneratedException();
    }
  } else {
    int_vector_type Os(P);
    cumulativeOffspring(rng, lws, Os, n);
    bi::adjacent_difference(Os, os);
  }
}

template<class V1, class V2, class V3, class V4>
void bi::StratifiedResampler::offspring(Random& rng, const V1 lws, V2 os,
    const int n, bool sorted, V3 lws1, V4 ps, V3 Ws)
        throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(lws.size() == os.size());

  typedef typename V1::value_type T1;
  typedef typename sim_temp_vector<V1>::type vector_type;
  typedef typename sim_temp_vector<V2>::type int_vector_type;

  const int P = lws.size();

  if (sort) {
    int_vector_type Os(P), temp(P);

    if (!sorted) {
      lws1 = lws;
      seq_elements(ps, 0);
      bi::sort_by_key(lws1, ps);
      sumexpu_inclusive_scan(lws1, Ws);
    }

    T1 W = *(Ws.end() - 1);  // sum of weights
    if (W > 0) {
      op(rng, Ws, Os, n);
      bi::adjacent_difference(Os, temp);
      bi::scatter(ps, temp, os);

#ifndef NDEBUG
      int m = sum_reduce(os);
      BI_ASSERT_MSG(m == n,
          "Stratified resampler gives " << m << " offspring, should give " << n);
#endif
    } else {
      throw ParticleFilterDegeneratedException();
    }
  } else {
    int_vector_type Os(P);
    cumulativeOffspring(rng, lws, Os, n, sorted, lws1, ps, Ws);
    bi::adjacent_difference(Os, os);
  }
}

template<class V1, class V2, class V3, class V4>
void bi::StratifiedResampler::offspring(Random& rng, const V1 lws, V2 os,
    const int n, int ka, bool sorted, V3 lws1, V4 ps, V3 Ws)
        throw (ParticleFilterDegeneratedException) {
  /// @todo May only work if ka == 0

  /* pre-condition */
  BI_ASSERT(lws.size() == os.size());
  BI_ASSERT(ka >= 0 && ka < lws.size());

  typedef typename V1::value_type T1;
  typedef typename sim_temp_vector<V1>::type vector_type;
  typedef typename sim_temp_vector<V2>::type int_vector_type;

  const int P = lws.size();

  if (sort) {
    int_vector_type Os(P), temp(P);

    if (!sorted) {
      lws1 = lws;
      seq_elements(ps, 0);
      bi::sort_by_key(lws1, ps);
      sumexpu_inclusive_scan(lws1, Ws);
    }

    T1 W = *(Ws.end() - 1);  // sum of weights

    if (W > 0) {
      BI_ASSERT_MSG(false, "Not yet implemented");

#ifndef NDEBUG
      int m = sum_reduce(os);
      BI_ASSERT_MSG(m == n,
          "Stratified resampler gives " << m << " offspring, should give " << n);
#endif
    } else {
      throw ParticleFilterDegeneratedException();
    }
  } else {
    int_vector_type Os(P);
    cumulativeOffspring(rng, lws, Os, n, ka, sorted, lws1, ps, Ws);
    bi::adjacent_difference(Os, os);
  }
}

template<class V1, class V2>
void bi::StratifiedResampler::cumulativeOffspring(Random& rng, const V1 lws,
    V2 Os, const int n) throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(lws.size() == Os.size());

  typedef typename V1::value_type T1;
  typedef typename sim_temp_vector<V1>::type vector_type;
  typedef typename sim_temp_vector<V2>::type int_vector_type;

  const int P = lws.size();
  T1 W, a;

  if (sort) {
    int_vector_type os(P);
    offspring(rng, lws, os, n);
    sum_inclusive_scan(os, Os);
  } else {
    vector_type Ws(P);
    sumexpu_inclusive_scan(lws, Ws);

    W = *(Ws.end() - 1);  // sum of weights
    if (W > 0) {
      op(rng, Ws, Os, n);

#ifndef NDEBUG
      int m = *(Os.end() - 1);
      BI_ASSERT_MSG(m == n,
          "Stratified resampler gives " << m << " offspring, should give " << n);
#endif
    } else {
      throw ParticleFilterDegeneratedException();
    }
  }
}

template<class V1, class V2, class V3, class V4>
void bi::StratifiedResampler::cumulativeOffspring(Random& rng, const V1 lws,
    V2 Os, const int n, bool sorted, V3 lws1, V4 ps, V3 Ws)
        throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(lws.size() == Os.size());

  typedef typename V1::value_type T1;
  typedef typename sim_temp_vector<V1>::type vector_type;
  typedef typename sim_temp_vector<V2>::type int_vector_type;

  const int P = lws.size();

  if (sort) {
    int_vector_type os(P);
    offspring(rng, lws, os, n, sorted, lws1, ps, Ws);
    sum_inclusive_scan(os, Os);
  } else {
    sumexpu_inclusive_scan(lws, Ws);
    T1 W = *(Ws.end() - 1);  // sum of weights
    if (W > 0) {
      op(rng, Ws, Os, n);

#ifndef NDEBUG
      int m = *(Os.end() - 1);
      BI_ASSERT_MSG(m == n,
          "Stratified resampler gives " << m << " offspring, should give " << n);
#endif
    } else {
      throw ParticleFilterDegeneratedException();
    }
  }
}

template<class V1, class V2, class V3, class V4>
void bi::StratifiedResampler::cumulativeOffspring(Random& rng, const V1 lws,
    V2 Os, const int n, int ka, bool sorted, V3 lws1, V4 ps, V3 Ws)
        throw (ParticleFilterDegeneratedException) {
  /// @todo May only work if ka == 0

  /* pre-condition */
  BI_ASSERT(lws.size() == Os.size());
  BI_ASSERT(ka >= 0 && ka < lws.size());

  typedef typename V1::value_type T1;
  typedef typename sim_temp_vector<V1>::type vector_type;
  typedef typename sim_temp_vector<V2>::type int_vector_type;

  const int P = lws.size();

  if (sort) {
    int_vector_type os(P);
    offspring(rng, lws, os, n, ka, sorted, lws1, ps, Ws);
    sum_inclusive_scan(os, Os);
  } else {
    sumexpu_inclusive_scan(lws, Ws);
    T1 W = *(Ws.end() - 1);  // sum of weights

    if (W > 0) {
      BI_ERROR_MSG(false, "Not yet implemented");

#ifndef NDEBUG
      int m = *(Os.end() - 1);
      BI_ASSERT_MSG(m == n,
          "Stratified resampler gives " << m << " offspring, should give " << n);
#endif
    } else {
      throw ParticleFilterDegeneratedException();
    }
  }
}

template<class V1, class V2>
void bi::StratifiedResampler::ancestors(Random& rng, const V1 lws, V2 as)
    throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(as.size() == lws.size());

  const int P = as.size();

  typename sim_temp_vector<V2>::type Os(P), ps(P);
  typename sim_temp_vector<V1>::type lws1(P), Ws(P);

  cumulativeOffspring(rng, lws, Os, P, false, lws1, ps, Ws);
  cumulativeOffspringToAncestors(Os, as);
}

template<class V1, class V2, class V3, class V4>
void bi::StratifiedResampler::ancestors(Random& rng, const V1 lws, V2 as,
    int P, bool sorted, V3 lws1, V4 ps, V3 Ws)
        throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(as.size() == P);

  typename sim_temp_vector<V2>::type Os(lws.size());

  cumulativeOffspring(rng, lws, Os, P, sorted, lws1, ps, Ws);
  cumulativeOffspringToAncestors(Os, as);
}

template<class V1, class V2, class V3, class V4>
void bi::StratifiedResampler::ancestors(Random& rng, const V1 lws, V2 as,
    int P, int ka, int k, bool sorted, V3 lws1, V4 ps, V3 Ws)
        throw (ParticleFilterDegeneratedException) {
  /* pre-condition */
  BI_ASSERT(as.size() == P);

  typename sim_temp_vector<V2>::type Os(lws.size());

  cumulativeOffspring(rng, lws, Os, P, ka, sorted, lws1, ps, Ws);
  cumulativeOffspringToAncestors(Os, as);

  /* post-condition */
  BI_ASSERT(*(as.begin() + k) == ka);
}

template<class V1, class V2>
void bi::StratifiedResampler::op(Random& rng, const V1 Ws, V2 Os,
    const int n) {
  /* pre-conditions */
  BI_ASSERT(V1::on_device == V2::on_device);

  typedef typename boost::mpl::if_c<V1::on_device,StratifiedResamplerGPU,
      StratifiedResamplerHost>::type impl;
  impl::op(rng, Ws, Os, n);
}

#endif
