/*
 * This file belongs to the Galois project, a C++ library for exploiting parallelism.
 * The code is being released under the terms of the 3-Clause BSD License (a
 * copy is located in LICENSE.txt at the top-level directory).
 *
 * Copyright (C) 2018, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 */

#ifndef GALOIS_RUNTIME_KDG_SPEC_LOCAL_MIN_H
#define GALOIS_RUNTIME_KDG_SPEC_LOCAL_MIN_H

#include "galois/runtime/IKDGbase.h"
#include "galois/runtime/WindowWorkList.h"
#include "galois/runtime/Executor_ParaMeter.h"

namespace galois {
namespace runtime {

template <typename T, typename Cmp, typename NhFunc, typename OpFunc,
          typename ArgsTuple>
class KDGspecLocalMinExecutor
    : public IKDGbase<T, Cmp, NhFunc, internal::DummyExecFunc, OpFunc,
                      ArgsTuple, TwoPhaseContext<T, Cmp>> {

protected:
  using Ctxt = TwoPhaseContext<T, Cmp>;
  using Base = IKDGbase<T, Cmp, NhFunc, internal::DummyExecFunc, OpFunc,
                        ArgsTuple, Ctxt>;

  using WindowWL =
      typename std::conditional<Base::NEEDS_PUSH, PQwindowWL<T, Cmp>,
                                SortedRangeWindowWL<T, Cmp>>::type;
  using CtxtWL = typename Base::CtxtWL;

  static const bool ENABLE_PROF            = false;
  static const unsigned DEFAULT_CHUNK_SIZE = 8;

  WindowWL winWL;
  PerThreadBag<T> pending;
  CtxtWL scheduled;

  PerThreadBag<Ctxt*> abortList;
  PerThreadBag<Ctxt*> retireList;

public:
  KDGspecLocalMinExecutor(const Cmp& cmp, const NhFunc& nhFunc,
                          const OpFunc& opFunc, const ArgsTuple& argsTuple)
      : Base(cmp, nhFunc, internal::DummyExecFunc(), opFunc, argsTuple),
        winWL(cmp) {}

  ~KDGspecLocalMinExecutor() {

    dumpStats();

    if (Base::ENABLE_PARAMETER) {
      ParaMeter::closeStatsFile();
    }
  }

protected:
  void dumpStats(void) {
    reportStat_Single(Base::loopname, "efficiency %",
                      double(100.0 * Base::totalCommits) / Base::totalTasks);
    reportStat_Single(Base::loopname, "avg. parallelism",
                      double(Base::totalCommits) / Base::rounds);
  }

  GALOIS_ATTRIBUTE_PROF_NOINLINE void abortCtxt(Ctxt* c) {
    assert(c);
    c->cancelIteration();
    c->reset();
    c->~Ctxt();
    Base::ctxtAlloc.deallocate(c, 1);
  }

  GALOIS_ATTRIBUTE_PROF_NOINLINE void retireCtxt(Ctxt* c) {
    assert(c);
    c->commitIteration();
    c->~Ctxt();
    Base::ctxtAlloc.deallocate(c, 1);
  }

  GALOIS_ATTRIBUTE_PROF_NOINLINE void expandNhood(void) {

    galois::runtime::do_all_gen(
        makeLocalRange(pending),
        [this](const T& x) {
          Ctxt* c = Base::ctxtAlloc.allocate(1);
          assert(c);
          Base::ctxtAlloc.construct(c, x, Base::cmp);

          typename Base::UserCtxt& uhand = *Base::userHandles.getLocal();
          uhand.reset();
          runCatching(Base::nhFunc, c, uhand);

          if (c->isSrc()) {
            scheduled.push(c);
          } else {

            if (ENABLE_PROF) {
              abortList.push(c);
            } else {
              abortCtxt(c);
            }
          }

          Base::roundTasks += 1;
        },
        std::make_tuple(galois::loopname("expandNhood"),
                        chunk_size<NhFunc::CHUNK_SIZE>()));

    pending.clear_all_parallel();
  }

  GALOIS_ATTRIBUTE_PROF_NOINLINE void applyOperator(void) {

    galois::optional<T> minElem;

    if (Base::NEEDS_PUSH) {
      if (Base::targetCommitRatio != 0.0 && !winWL.empty()) {
        minElem = *winWL.getMin();
      }
    }

    galois::runtime::do_all_gen(
        makeLocalRange(scheduled),
        [this, &minElem](Ctxt* c) {
          typename Base::UserCtxt& uhand = *Base::userHandles.getLocal();
          uhand.reset();

          if (c->isSrc()) {
            static_assert(!Base::NEEDS_CUSTOM_LOCKING,
                          "operator not allowed to abort");
            Base::opFunc(c->getActive(), uhand);
            Base::roundCommits += 1;

            if (Base::NEEDS_PUSH) {
              for (auto i    = uhand.getPushBuffer().begin(),
                        endi = uhand.getPushBuffer().end();
                   i != endi; ++i) {

                if ((Base::targetCommitRatio == 0.0) || !minElem ||
                    !Base::cmp(*minElem, *i)) {
                  // if *i >= *minElem
                  pending.push(*i);
                  ;
                } else {
                  winWL.push(*i);
                }
              }
            } else {
              assert(uhand.getPushBuffer().begin() ==
                     uhand.getPushBuffer().end());
            }

            if (ENABLE_PROF) {
              retireList.push(c);
            } else {
              retireCtxt(c);
            }

          } else { // not isSrc

            if (ENABLE_PROF) {
              abortList.push(c);
            } else {
              abortCtxt(c);
            }
          }
        },
        std::make_tuple(galois::loopname("applyOperator"),
                        chunk_size<OpFunc::CHUNK_SIZE>()));

    scheduled.clear_all_parallel();
  }

  void serviceAborts(void) {
    if (ENABLE_PROF) {
      galois::runtime::do_all_gen(
          makeLocalRange(abortList), [this](Ctxt* c) { abortCtxt(c); },
          std::make_tuple(galois::loopname("serviceAborts"),
                          chunk_size<DEFAULT_CHUNK_SIZE>()));

      abortList.clear_all_parallel();
    }
  }

  void performCommits(void) {
    if (ENABLE_PROF) {
      galois::runtime::do_all_gen(
          makeLocalRange(retireList), [this](Ctxt* c) { retireCtxt(c); },
          std::make_tuple(galois::loopname("performCommits"),
                          chunk_size<DEFAULT_CHUNK_SIZE>()));

      retireList.clear_all_parallel();
    }
  }

  GALOIS_ATTRIBUTE_PROF_NOINLINE void endRound() {

    if (Base::ENABLE_PARAMETER) {
      ParaMeter::StepStats s(Base::rounds, Base::roundCommits.reduceRO(),
                             Base::roundTasks.reduceRO());
      s.dump(ParaMeter::getStatsFile(), Base::loopname);
    }

    Base::endRound();
  }

public:
  template <typename R>
  void push_initial(const R& range) {

    if (Base::targetCommitRatio == 0.0) {

      galois::runtime::do_all_gen(
          range, [this](const T& x) { pending.push(x); },
          std::make_tuple(galois::loopname("init-fill"),
                          chunk_size<DEFAULT_CHUNK_SIZE>()));

    } else {
      winWL.initfill(range);
    }
  }

  void execute() {

    while (true) {

      Base::t_beginRound.start();
      Base::refillRound(winWL, pending);
      Base::t_beginRound.stop();

      if (pending.empty_all()) {
        break;
      }

      Base::t_expandNhood.start();
      expandNhood();
      Base::t_expandNhood.stop();

      Base::t_applyOperator.start();
      applyOperator();
      Base::t_applyOperator.stop();

      Base::t_serviceAborts.start();
      serviceAborts();
      Base::t_serviceAborts.stop();

      Base::t_performCommits.start();
      performCommits();
      Base::t_performCommits.stop();

      endRound();
    }
  }
};

template <typename R, typename Cmp, typename NhFunc, typename OpFunc,
          typename _ArgsTuple>
void for_each_ordered_kdg_spec_local_min_impl(const R& range, const Cmp& cmp,
                                              const NhFunc& nhFunc,
                                              const OpFunc& opFunc,
                                              const _ArgsTuple& argsTuple) {

  auto argsT = std::tuple_cat(
      argsTuple,
      get_default_trait_values(
          argsTuple, std::make_tuple(loopname_tag{}, enable_parameter_tag{}),
          std::make_tuple(default_loopname{}, enable_parameter<false>{})));
  using ArgsT = decltype(argsT);

  using T = typename R::value_type;

  using Exec = KDGspecLocalMinExecutor<T, Cmp, NhFunc, OpFunc, ArgsT>;

  Exec e(cmp, nhFunc, opFunc, argsT);

  const bool wakeupThreadPool = true;

  if (wakeupThreadPool) {
    substrate::getThreadPool().burnPower(galois::getActiveThreads());
  }

  e.push_initial(range);
  e.execute();

  if (wakeupThreadPool) {
    substrate::getThreadPool().beKind();
  }
}

template <typename R, typename Cmp, typename NhFunc, typename OpFunc,
          typename _ArgsTuple>
void for_each_ordered_kdg_spec_local_min(const R& range, const Cmp& cmp,
                                         const NhFunc& nhFunc,
                                         const OpFunc& opFunc,
                                         const _ArgsTuple& argsTuple) {

  auto tplParam =
      std::tuple_cat(argsTuple, std::make_tuple(enable_parameter<true>()));
  auto tplNoParam =
      std::tuple_cat(argsTuple, std::make_tuple(enable_parameter<false>()));

  if (useParaMeterOpt) {
    for_each_ordered_kdg_spec_local_min_impl(range, cmp, nhFunc, opFunc,
                                             tplParam);
  } else {
    for_each_ordered_kdg_spec_local_min_impl(range, cmp, nhFunc, opFunc,
                                             tplNoParam);
  }
}

} // namespace runtime
} // namespace galois

#endif // GALOIS_RUNTIME_KDG_SPEC_LOCAL_MIN_H
