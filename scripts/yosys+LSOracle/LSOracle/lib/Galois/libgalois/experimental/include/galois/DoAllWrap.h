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

#ifndef GALOIS_DOALL_WRAPPER_H
#define GALOIS_DOALL_WRAPPER_H

#include "galois/Galois.h"
#include "galois/GaloisForwardDecl.h"
#include "galois/OrderedTraits.h"
#include "galois/runtime/Executor_DoAll_Old.h"
#include "galois/runtime/Executor_DoAll.h"
#include "galois/substrate/EnvCheck.h"

#ifdef GALOIS_USE_TBB
#include "tbb/parallel_for_each.h"
#endif

#include "CilkInit.h"
#include <unistd.h>

#include "llvm/Support/CommandLine.h"

namespace galois {

enum DoAllTypes {
  DO_ALL_OLD,
  DO_ALL_OLD_STEAL,
  DOALL_GALOIS_FOREACH,
  DO_ALL,
  DOALL_CILK,
  DOALL_OPENMP
};

namespace cll = llvm::cl;
// extern cll::opt<DoAllTypes> doAllKind;
static cll::opt<DoAllTypes> doAllKind(
    "doAllKind", cll::desc("DoAll Implementation"),
    cll::values(clEnumVal(DO_ALL_OLD, "DO_ALL_OLD"),
                clEnumVal(DO_ALL_OLD_STEAL, "DO_ALL_OLD_STEAL"),
                clEnumVal(DOALL_GALOIS_FOREACH, "DOALL_GALOIS_FOREACH"),
                clEnumVal(DO_ALL, "DO_ALL"),
                clEnumVal(DOALL_CILK, "DOALL_CILK"),
                clEnumVal(DOALL_OPENMP, "DOALL_OPENMP"), clEnumValEnd),
    cll::init(DO_ALL_OLD)); // default is regular DOALL

void setDoAllImpl(const DoAllTypes& type);

DoAllTypes getDoAllImpl(void);

template <DoAllTypes TYPE>
struct DoAllImpl {
  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {
    std::abort();
  }
};

template <>
struct DoAllImpl<DO_ALL_OLD> {
  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {
    galois::runtime::do_all_gen_old(
        range, func, std::tuple_cat(std::make_tuple(steal()), argsTuple));
  }
};

template <>
struct DoAllImpl<DO_ALL_OLD_STEAL> {
  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {
    galois::runtime::do_all_gen_old(
        range, func, std::tuple_cat(std::make_tuple(steal()), argsTuple));
  }
};

template <>
struct DoAllImpl<DOALL_GALOIS_FOREACH> {

  template <typename T, typename _F>
  struct FuncWrap {
    _F func;

    template <typename C>
    void operator()(T& x, C&) {
      func(x);
    }
  };

  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {

    using T = typename R::value_type;

    const unsigned CHUNK_SIZE = 128;
    // const unsigned CHUNK_SIZE = get_type_by_supertype<chunk_size_tag,
    // ArgsTuple>::type::value;

    using WL_ty = galois::worklists::PerThreadChunkLIFO<CHUNK_SIZE, T>;

    galois::runtime::for_each_gen(
        range, FuncWrap<T, F>{func},
        std::tuple_cat(
            std::make_tuple(galois::wl<WL_ty>(), no_pushes(), no_conflicts()),
            argsTuple));
  }
};

template <>
struct DoAllImpl<DO_ALL> {
  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {
    galois::runtime::do_all_gen(range, func, argsTuple);
  }
};

#ifdef HAVE_CILK
template <>
struct DoAllImpl<DOALL_CILK> {
  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {
    CilkInit();
    cilk_for(auto it = range.begin(), end = range.end(); it != end; ++it) {
      func(*it);
    }
  }
};
#else
template <>
struct DoAllImpl<DOALL_CILK> {
  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {
    GALOIS_DIE("Cilk not found\n");
  }
};
#endif

template <>
struct DoAllImpl<DOALL_OPENMP> {
  template <typename R, typename F, typename ArgsTuple>
  static inline void go(const R& range, const F& func,
                        const ArgsTuple& argsTuple) {
    const auto end = range.end();
#pragma omp parallel for schedule(guided)
    for (auto it = range.begin(); it < end; ++it) {
      func(*it);
    }
  }
};

template <typename R, typename F, typename ArgsTuple>
void do_all_choice(const R& range, const F& func, const DoAllTypes& type,
                   const ArgsTuple& argsTuple) {
  switch (type) {
  case DO_ALL_OLD_STEAL:
    DoAllImpl<DO_ALL_OLD_STEAL>::go(range, func, argsTuple);
    break;
  case DOALL_GALOIS_FOREACH:
    DoAllImpl<DOALL_GALOIS_FOREACH>::go(range, func, argsTuple);
    break;
  case DO_ALL_OLD:
    DoAllImpl<DO_ALL_OLD>::go(range, func, argsTuple);
    break;
  case DO_ALL:
    DoAllImpl<DO_ALL>::go(range, func, argsTuple);
    break;
  case DOALL_CILK:
    DoAllImpl<DOALL_CILK>::go(range, func, argsTuple);
    break;
  case DOALL_OPENMP:
    // DoAllImpl<DOALL_OPENMP>::go(range, func, argsTuple);
    std::abort();
    break;
  default:
    abort();
    break;
  }
}

template <typename R, typename F, typename ArgsTuple>
void do_all_choice(const R& range, const F& func, const ArgsTuple& argsTuple) {
  do_all_choice(range, func, doAllKind, argsTuple);
}

} // end namespace galois

#endif //  GALOIS_DOALL_WRAPPER_H
