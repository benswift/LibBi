/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_METHOD_BRIDGEPARTICLEFILTER_HPP
#define BI_METHOD_BRIDGEPARTICLEFILTER_HPP

#include "AuxiliaryParticleFilter.hpp"

namespace bi {
/**
 * Particle filter with bridge weighting function.
 *
 * @ingroup method
 *
 * @tparam B Model type.
 * @tparam S Simulator type.
 * @tparam R #concept::Resampler type.
 * @tparam IO1 Output type.
 *
 * @section Concepts
 *
 * #concept::Filter
 */
template<class B, class S, class R, class IO1>
class BridgeParticleFilter: public AuxiliaryParticleFilter<B,S,R,IO1> {
public:
  /**
   * @copydoc ParticleFilter::ParticleFilter()
   */
  BridgeParticleFilter(B& m, S* sim = NULL, R* resam = NULL, IO1* out = NULL);

  /**
   * @name High-level interface.
   *
   * An easier interface for common usage.
   */
  //@{
  /**
   * @copydoc ParticleFilter::filter(Random&, const ScheduleIterator, const ScheduleIterator, State<B,L>&, IO2*)
   */
  template<Location L, class IO2>
  real filter(Random& rng, const ScheduleIterator first,
      const ScheduleIterator last, State<B,L>& s, IO2* inInit);

  /**
   * @copydoc ParticleFilter::filter(Random&, Schedule&, const V1, State<B,L>&)
   */
  template<Location L, class V1>
  real filter(Random& rng, const ScheduleIterator first,
      const ScheduleIterator last, const V1 theta, State<B,L>& s);

  /**
   * @copydoc ParticleFilter::filter(Random&, Schedule&, const V1, State<B,L>&, M1)
   */
  template<Location L, class V1, class M1>
  real filter(Random& rng, const ScheduleIterator first,
      const ScheduleIterator last, const V1 theta, State<B,L>& s, M1 X);
  //@}

  /**
   * @name Low-level interface.
   *
   * Largely used by other features of the library or for finer control over
   * performance and behaviour.
   */
  //@{
  /**
   * Resample, predict and correct.
   *
   * @tparam L Location.
   * @tparam V1 Vector type.
   * @tparam V2 Vector type.
   *
   * @param[in,out] rng Random number generator.
   * @param[in,out] iter Current position in time schedule. Advanced on
   * return.
   * @param last End of time schedule.
   * @param[in,out] s State.
   * @param[in,out] lws Log-weights.
   * @param[in,out] blws Bridge log-weights.
   * @param[out] as Ancestry.
   */
  template<bi::Location L, class V1, class V2>
  real step(Random& rng, ScheduleIterator& iter, const ScheduleIterator last,
      State<B,L>& s, V1 lws, V1 blws, V2 as);

  /**
   * Resample, predict and correct.
   *
   * @tparam L Location.
   * @tparam M1 Matrix type.
   * @tparam V1 Vector type.
   * @tparam V2 Vector type.
   *
   * @param[in,out] rng Random number generator.
   * @param[in,out] iter Current position in time schedule. Advanced on
   * return.
   * @param last End of time schedule.
   * @param[in,out] s State.
   * @param X Path on which to condition. Rows index variables, columns
   * index times.
   * @param[in,out] lws Log-weights.
   * @param[in,out] blws Bridge log-weights.
   * @param[out] as Ancestry after resampling.
   */
  template<bi::Location L, class M1, class V1, class V2>
  real step(Random& rng, ScheduleIterator& iter, const ScheduleIterator last,
      State<B,L>& s, const M1 X, V1 lws, V1 blws, V2 as);

  /**
   * Update particle weights using lookahead.
   *
   * @tparam L Location.
   * @tparam V1 Vector type.
   *
   * @param iter Current position in time schedule.
   * @param last End of time schedule.
   * @param[in,out] s State.
   * @param[in,out] lws Log-weights.
   * @param[in,out] blws Bridge log-weights.
   *
   * @return Normalising constant contribution.
   */
  template<Location L, class V1>
  real bridge(Random& rng, const ScheduleIterator iter,
      const ScheduleIterator last, State<B,L>& s, V1 lws, V1 blws);
  //@}
};

/**
 * Factory for creating BridgeParticleFilter objects.
 *
 * @ingroup method
 *
 * @see BridgeParticleFilter
 */
struct BridgeParticleFilterFactory {
  /**
   * Create auxiliary particle filter.
   *
   * @return BridgeParticleFilter object. Caller has ownership.
   *
   * @see BridgeParticleFilter::BridgeParticleFilter()
   */
  template<class B, class S, class R, class IO1>
  static BridgeParticleFilter<B,S,R,IO1>* create(B& m, S* sim = NULL,
      R* resam = NULL, IO1* out = NULL) {
    return new BridgeParticleFilter<B,S,R,IO1>(m, sim, resam, out);
  }

  /**
   * Create auxiliary particle filter.
   *
   * @return BridgeParticleFilter object. Caller has ownership.
   *
   * @see BridgeParticleFilter::BridgeParticleFilter()
   */
  template<class B, class S, class R>
  static BridgeParticleFilter<B,S,R,ParticleFilterCache<> >* create(B& m,
      S* sim = NULL, R* resam = NULL) {
    return new BridgeParticleFilter<B,S,R,ParticleFilterCache<> >(m, sim,
        resam);
  }
};
}

#include "../math/loc_temp_vector.hpp"
#include "../math/loc_temp_matrix.hpp"
#include "../primitive/vector_primitive.hpp"
#include "../primitive/matrix_primitive.hpp"

template<class B, class S, class R, class IO1>
bi::BridgeParticleFilter<B,S,R,IO1>::BridgeParticleFilter(B& m, S* sim,
    R* resam, IO1* out) :
    AuxiliaryParticleFilter<B,S,R,IO1>(m, sim, resam, out) {
  //
}

template<class B, class S, class R, class IO1>
template<bi::Location L, class IO2>
real bi::BridgeParticleFilter<B,S,R,IO1>::filter(Random& rng,
    const ScheduleIterator first, const ScheduleIterator last, State<B,L>& s,
    IO2* inInit) {
  const int P = s.size();
  bool r = false;
  real ll = 0.0;

  typename loc_temp_vector<L,real>::type lws(P), blws(P);
  typename loc_temp_vector<L,int,-1,1>::type as(P);

  ScheduleIterator iter = first;
  this->init(rng, *iter, s, lws, blws, as, inInit);
  this->output0(s);
  ll = this->correct(*iter, s, lws, blws);
  this->output(*iter, s, r, lws, as);
  while (iter + 1 != last) {
    ll += step(rng, iter, last, s, lws, blws, as);
  }
  this->term();
  this->outputT(ll);

  return ll;
}

template<class B, class S, class R, class IO1>
template<bi::Location L, class V1>
real bi::BridgeParticleFilter<B,S,R,IO1>::filter(Random& rng,
    const ScheduleIterator first, const ScheduleIterator last, const V1 theta,
    State<B,L>& s) {
  // this implementation is (should be) the same as filter() above, but with
  // a different init() call

  const int P = s.size();
  int r = 0;
  real ll;

  typename loc_temp_vector<L,real>::type lws(P), blws(P);
  typename loc_temp_vector<L,int,-1,1>::type as(P);

  ScheduleIterator iter = first;
  this->init(rng, theta, *iter, s, lws, blws, as);
  this->output0(s);
  ll = this->correct(*iter, s, lws, blws);
  this->output(*iter, s, r, lws, as);
  while (iter + 1 != last) {
    ll += step(rng, iter, last, s, lws, blws, as);
  }
  this->term();
  this->outputT(ll);

  return ll;
}

template<class B, class S, class R, class IO1>
template<bi::Location L, class V1, class M1>
real bi::BridgeParticleFilter<B,S,R,IO1>::filter(Random& rng,
    const ScheduleIterator first, const ScheduleIterator last, const V1 theta,
    State<B,L>& s, M1 X) {
  // this implementation is (should be) the same as filter() above, but with
  // a different step() call

  const int P = s.size();
  bool r = false;
  real ll;

  typename loc_temp_vector<L,real>::type lws(P), blws(P);
  typename loc_temp_vector<L,int,-1,1>::type as(P);

  ScheduleIterator iter = first;
  this->init(rng, theta, *iter, s, lws, blws, as);
  row(s.getDyn(), 0) = column(X, 0);
  this->output0(s);
  ll = this->correct(*iter, s, lws, blws);
  this->output(*iter, s, r, lws, as);
  while (iter + 1 != last) {
    ll += step(rng, iter, last, s, X, lws, blws, as);
  }
  this->term();
  this->outputT(ll);

  return ll;
}

template<class B, class S, class R, class IO1>
template<bi::Location L, class V1, class V2>
real bi::BridgeParticleFilter<B,S,R,IO1>::step(Random& rng,
    ScheduleIterator& iter, const ScheduleIterator last, State<B,L>& s,
    V1 lws, V1 blws, V2 as) {
  real ll = 0.0;
  bool r = false;
  do {
    ll += this->bridge(rng, iter, last, s, lws, blws);
    r = this->resample(rng, *iter, s, lws, blws, as);
    ++iter;
    this->predict(rng, *iter, s);
    ll += this->correct(*iter, s, lws, blws);
    this->output(*iter, s, r, lws, as);
  } while (iter + 1 != last && !iter->isObserved());

  return ll;
}

template<class B, class S, class R, class IO1>
template<bi::Location L, class M1, class V1, class V2>
real bi::BridgeParticleFilter<B,S,R,IO1>::step(Random& rng,
    ScheduleIterator& iter, const ScheduleIterator last, State<B,L>& s,
    const M1 X, V1 lws, V1 blws, V2 as) {
  real ll = 0.0;
  bool r = false;
  do {
    ll += this->bridge(rng, iter, last, s, lws, blws);
    r = this->resample(rng, *iter, s, lws, blws, as);
    ++iter;
    this->predict(rng, *iter, s);
    if (iter->hasOutput()) {
      row(s.getDyn(), 0) = column(X, iter->indexOutput());
    }
    ll += this->correct(*iter, s, lws, blws);
    this->output(*iter, s, r, lws, as);
  } while (iter + 1 != last && !iter->isObserved());

  return ll;
}

template<class B, class S, class R, class IO1>
template<bi::Location L, class V1>
real bi::BridgeParticleFilter<B,S,R,IO1>::bridge(Random& rng,
    const ScheduleIterator iter, const ScheduleIterator last, State<B,L>& s,
    V1 lws, V1 blws) {
  /* pre-condition */
  BI_ASSERT(lws.size() == blws.size());

  real ll = 0.0;
  if (iter->hasBridge() && !iter->isObserved()
      && last->indexObs() > iter->indexObs()) {
    axpy(-1.0, blws, lws);
    blws.clear();

    this->m.bridgeLogDensities(s,
        this->getSim()->getObs()->getMask(iter->indexObs()), blws);

    axpy(1.0, blws, lws);
    ll = logsumexp_reduce(lws) - bi::log(static_cast<real>(s.size()));
  }
  return ll;
}

#endif