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

// TODO (amber): This file needs to be merged back with
// libruntime/.../Executor_DoAll.h once the do_all interface has been decided on

#ifndef GALOIS_RUNTIME_EXECUTOR_DOALL_H
#define GALOIS_RUNTIME_EXECUTOR_DOALL_H

#include "galois/gstl.h"
#include "galois/gtuple.h"
#include "galois/Traits.h"
#include "galois/Timer.h"
#include "galois/substrate/Barrier.h"
#include "galois/runtime/Support.h"
#include "galois/runtime/Range.h"

#include <algorithm>
#include <mutex>
#include <tuple>

namespace galois {
namespace runtime {

// TODO(ddn): Tune stealing. DMR suffers when stealing is on
// TODO: add loopname + stats
template <typename FunctionTy, typename RangeTy, typename ArgsTy>
class DoAllExecutor {

  static const bool STEAL = exists_by_supertype<steal_tag, ArgsTy>::value;

  typedef typename RangeTy::local_iterator iterator;
  FunctionTy F;
  RangeTy range;
  const char* loopname;

  /*
    starving queue
    threads without work go in it.  When all threads are in the queue, stop
  */

  unsigned exec(iterator b, iterator e) {
    unsigned n = 0;
    while (b != e) {
      ++n;
      F(*b++);
    }
    return n;
  }

  struct msg {
    std::atomic<bool> ready;
    bool exit;
    iterator b, e;
    msg* next;
  };
  substrate::PtrLock<msg> head;
  std::atomic<unsigned> waiting;

  // return true to continue, false to exit
  bool wait(iterator& b, iterator& e) {
    // else, add ourselves to the queue
    msg self;
    self.b     = b;
    self.e     = e;
    self.exit  = false;
    self.next  = nullptr;
    self.ready = false;
    do {
      self.next = head.getValue();
    } while (!head.CAS(self.next, &self));
    ++waiting;

    // wait for signal
    while (!self.ready) {
      //      std::cerr << waiting << "\n";
      substrate::asmPause();
      if (waiting == activeThreads)
        return false;
      substrate::asmPause();
    }

    b = self.b;
    e = self.e;
    return true;
  }

  unsigned tryDonate(iterator& b, iterator& e) {
    if (std::distance(b, e) < 2)
      return 0;
    if (!head.getValue())
      return 0;
    if (head.try_lock()) {
      msg* other = head.getValue();
      if (other) {
        head.unlock_and_set(other->next);
        --waiting;
        other->next  = nullptr;
        auto mid     = split_range(b, e);
        auto retval  = std::distance(mid, e);
        other->b     = mid;
        other->e     = e;
        e            = mid;
        other->ready = true;
        return retval;
      }
      head.unlock();
    }
    return 0;
  }

public:
  DoAllExecutor(const FunctionTy& _F, const RangeTy& r, const ArgsTy& args)
      : F(_F), range(r),
        loopname(get_by_supertype<loopname_tag>(args).getValue()) {}

  void operator()() {
    // Assume the copy constructor on the functor is readonly
    iterator begin = range.local_begin();
    iterator end   = range.local_end();

    if (!STEAL) {
      while (begin != end) {
        F(*begin++);
      }
    } else {
      int minSteal = std::distance(begin, end) / 8;
      state& tld   = *TLDS.getLocal();
      tld.populateSteal(begin, end);

      do {
        while (begin != end) {
          F(*begin++);
        }
      } while (trySteal(tld, begin, end, minSteal));
    }
  }

  // New operator
  void operator_NEW() {
    // Assume the copy constructor on the functor is readonly
    iterator begin = range.local_begin();
    iterator end   = range.local_end();
    // TODO: Make a separate donation function.

    unsigned long stat_iterations = 0;
    unsigned long stat_donations  = 0;

    do {
      do {
        auto mid = split_range(begin, end);
        stat_iterations += exec(begin, mid);
        begin = mid;
        stat_donations += tryDonate(begin, end);
      } while (begin != end);
    } while (wait(begin, end));

    reportStat(loopname, "Iterations", stat_iterations,
               substrate::ThreadPool::getTID());
    reportStat(loopname, "Donations", stat_donations,
               substrate::ThreadPool::getTID());
  }

  template <typename RangeTy, typename FunctionTy, typename ArgsTy>
  void do_all_impl(const RangeTy& range, const FunctionTy& f,
                   const ArgsTy& args) {
    DoAllExecutor<FunctionTy, RangeTy, ArgsTy> W(f, range, args);
    substrate::getThreadPool().run(activeThreads, std::ref(W));
  }
};

// template<typename RangeTy, typename FunctionTy, typename ArgsTy>
// void do_all_impl(const RangeTy& range, const FunctionTy& f, const ArgsTy&
// args) {
//
// DoAllExecutor<FunctionTy, RangeTy, ArgsTy> W(f, range, args);
// substrate::getThreadPool().run(activeThreads, std::ref(W));

// if (steal) {
// DoAllExecutor<FunctionTy, RangeTy> W(f, range, loopname);
// substrate::getThreadPool().run(activeThreads, std::ref(W));
// } else {
// FunctionTy f_cpy (f);
// substrate::getThreadPool().run(activeThreads, [&f_cpy, &range] () {
// auto begin = range.local_begin();
// auto end = range.local_end();
// while (begin != end)
// f_cpy(*begin++);
// });
// }
// }

template <typename RangeTy, typename FunctionTy, typename TupleTy>
void do_all_gen(const RangeTy& r, const FunctionTy& fn, const TupleTy& tpl) {
  static_assert(!exists_by_supertype<char*, TupleTy>::value, "old loopname");
  static_assert(!exists_by_supertype<char const*, TupleTy>::value,
                "old loopname");
  static_assert(!exists_by_supertype<bool, TupleTy>::value, "old steal");

  auto dtpl = std::tuple_cat(
      tpl, get_default_trait_values(tpl, std::make_tuple(loopname_tag{}),
                                    std::make_tuple(loopname{})));

  do_all_impl(r, fn, dtpl);
}

template <typename RangeTy, typename FunctionTy, galois::StatTimer GTimerTy,
          typename TupleTy>
void do_all_gen(const RangeTy& r, const FunctionTy& fn, GTimerTy& statTimer,
                const TupleTy& tpl) {
  static_assert(!exists_by_supertype<char*, TupleTy>::value, "old loopname");
  static_assert(!exists_by_supertype<char const*, TupleTy>::value,
                "old loopname");
  static_assert(!exists_by_supertype<bool, TupleTy>::value, "old steal");

  auto dtpl = std::tuple_cat(
      tpl, get_default_trait_values(tpl, std::make_tuple(loopname_tag{}),
                                    std::make_tuple(loopname{})));

#if 0
  std::string loopName(get_by_supertype<loopname_tag>(dtpl).value);
  std::string num_run_identifier = get_by_supertype<numrun_tag>(dtpl).value;
  std::string timer_do_all_str("DO_ALL_IMPL_" + loopName + "_" + num_run_identifier);
  galois::StatTimer Timer_do_all_impl(timer_do_all_str.c_str());
  Timer_do_all_impl.start();
#endif

  statTimer.start();
  do_all_impl(r, fn, get_by_supertype<loopname_tag>(dtpl).getValue(),
              exists_by_supertype<steal_tag, decltype(dtpl)>::value);
  statTimer.stop();
}
} // namespace runtime

} // namespace galois
} // end namespace galois

#endif
