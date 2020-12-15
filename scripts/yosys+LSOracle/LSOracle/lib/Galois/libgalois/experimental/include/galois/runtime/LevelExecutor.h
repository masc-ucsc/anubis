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

#ifndef GALOIS_RUNTIME_LEVELEXECUTOR_H
#define GALOIS_RUNTIME_LEVELEXECUTOR_H

#if 0
#include "galois/Reduction.h"
#include "galois/Galois.h"
#include "galois/optional.h"
#include "galois/PriorityQueue.h"
#include "galois/runtime/UserContextAccess.h"
#include "galois/worklists/WorkList.h"
#include "galois/worklists/WorkListWrapper.h"
#include "galois/substrate/ThreadRWlock.h"
#include "galois/runtime/Mem.h"
#include "galois/gIO.h"
#include "galois/substrate/ThreadPool.h"
#include "galois/substrate/PerThreadStorage.h"

#include <map>
#include <vector>

namespace galois {
namespace runtime {
namespace {

#define USE_DENSE_LEVELS 1
// #undef USE_DENSE_LEVELS
//

#define USE_LEVEL_CACHING 1
// #undef USE_LEVEL_CACHING

template <typename Key, typename KeyCmp, typename WL_ty>
class LevelMap {
public:
  using value_type = typename WL_ty::value_type;
  using MapAlloc = FixedSizeAllocator<std::pair<const Key, WL_ty*> >;
  using WLalloc = FixedSizeAllocator<WL_ty>;
  using InternalMap = std::map<Key, WL_ty*, KeyCmp, MapAlloc>;
  using GarbageVec = galois::gdeque<WL_ty*>;
  using Level = std::pair<Key, WL_ty*>;
  using CachedLevel = galois::optional<Level>;

private:
  substrate::ThreadRWlock rwmutex;
  InternalMap levelMap;
  WLalloc wlAlloc;
  GarbageVec removedLevels;
  substrate::PerThreadStorage<CachedLevel> cachedLevels;

public:
  LevelMap (const KeyCmp& kcmp):
    rwmutex (),
    levelMap (kcmp)
  {
  }

  bool empty (void) const {
    return levelMap.empty ();
  }

  std::pair<const Key, WL_ty*> pop (void) {
    assert (!levelMap.empty ());
    auto p =  *levelMap.begin ();

    levelMap.erase (levelMap.begin ());
    removedLevels.push_back (p.second);

    return p;
  }

  // only push called in parallel by multiple threads
  void push (const Key& k, const value_type& x) {
#ifdef USE_LEVEL_CACHING
    CachedLevel& cached = *(cachedLevels.getLocal ());

    if (cached && k == cached->first) { // fast path
      assert (cached->second != nullptr);
      cached->second->push (x);
      return;
    }
#endif
    // debug
    // std::printf ("could not find cached worklist");

    rwmutex.readLock ();
    auto currLevel = levelMap.find (k);

    if (currLevel == levelMap.end ()) {
      rwmutex.readUnlock (); // give up read lock to acquire write lock

      rwmutex.writeLock ();
      // check again after locking
      if (levelMap.find (k) == levelMap.end ()) {
        WL_ty* wl = wlAlloc.allocate (1); // new WL_ty ();
        new (wl) WL_ty;
        levelMap.insert (std::make_pair (k, wl));
      }
      rwmutex.writeUnlock ();

      // read again now
      rwmutex.readLock ();
      currLevel = levelMap.find (k);
    }
    rwmutex.readUnlock ();

    assert (currLevel != levelMap.end ());
    currLevel->second->push (x);
#ifdef USE_LEVEL_CACHING
    cached = *currLevel;
#endif
  }

  void freeRemovedLevels () {
    while (!removedLevels.empty ()) {
      WL_ty* wl = removedLevels.front ();
      removedLevels.pop_front ();
      wlAlloc.destroy (wl);
      wlAlloc.deallocate (wl, 1);
    }
  }
};

#ifdef USE_DENSE_LEVELS

template <typename WL_ty>
class LevelMap<unsigned, std::less<unsigned>, WL_ty> {
public:
  using value_type = typename WL_ty::value_type;
  using WLalloc = FixedSizeAllocator<WL_ty>;
  using GarbageVec = galois::gdeque<WL_ty*>;
  using Level = std::pair<unsigned, WL_ty*>;
  using InternalMap = std::deque<Level>;
  using CachedLevel = galois::optional<Level>;

private:
  substrate::ThreadRWlock rwmutex;
  InternalMap levelMap;
  WLalloc wlAlloc;
  GarbageVec removedLevels;
  substrate::PerThreadStorage<CachedLevel> cachedLevels;

  unsigned begLevel = 0;

public:
  LevelMap (const std::less<unsigned>&) {}

  bool empty (void) const {
    return levelMap.empty ();
  }

  std::pair<unsigned, WL_ty*> pop (void) {
    assert (!levelMap.empty ());

    const unsigned BAD_VAL = 1 << 30;
    Level p = std::make_pair (BAD_VAL, nullptr);

    while (!levelMap.empty ()) {
      p = levelMap.front ();
      levelMap.pop_front ();
      ++begLevel;
      assert (p.second != nullptr);
      removedLevels.push_back (p.second);

      if (p.second->size () != 0) {
        break;
      }
    }

    return p;
  }

  void push (const unsigned k, const value_type& x) {
#ifdef USE_LEVEL_CACHING
    if (k < begLevel) {
      GALOIS_DIE ("Can't handle non-monotonic adds");
    }

    CachedLevel cached = *(cachedLevels.getLocal ());

    if (cached && k == cached->first) {
      assert (cached->second != nullptr);
      cached->second->push (x);

    } else {
#else
    {
#endif
      const unsigned index = k - begLevel;

      rwmutex.readLock ();
      if (index >= levelMap.size ()) {
        rwmutex.readUnlock (); // give up read lock to acquire write lock

        rwmutex.writeLock ();
        // check again
        if (index >= levelMap.size ()) {
          // levelMap.resize ((index + 1), std::make_pair (BAD_VAL, nullptr));

          // resize
          for (unsigned i = levelMap.size (); i <= index; ++i) {

            WL_ty* wl = wlAlloc.allocate (1);
            assert (wl != nullptr);
            new (wl) WL_ty;

            levelMap.push_back (std::make_pair (i + begLevel, wl));
          }
        }
        rwmutex.writeUnlock ();

        // read again now
        rwmutex.readLock ();
      }

      assert (levelMap.size () > index);
      auto currLevel = levelMap[index];
      assert (currLevel.first == k);
      rwmutex.readUnlock ();

      currLevel.second->push (x);
#ifdef USE_LEVEL_CACHING
      *cachedLevels.getLocal() = currLevel;
#endif

    }
  }

  void freeRemovedLevels () {
    while (!removedLevels.empty ()) {
      WL_ty* wl = removedLevels.front ();
      removedLevels.pop_front ();
      wlAlloc.destroy (wl);
      wlAlloc.deallocate (wl, 1);

    }
  }

};

#endif // USE_DENSE_LEVELS

template <bool CanAbort>
struct InheritTraits {
  typedef char tt_does_not_need_push;
};

template <>
struct InheritTraits<false> {
  typedef int tt_does_not_need_aborts;
  typedef char tt_does_not_need_push;
};


template <typename T, typename Key, typename KeyFn, typename KeyCmp, typename NhoodFunc, typename OpFunc>
class LevelExecutor {

  static const unsigned CHUNK_SIZE = OpFunc::CHUNK_SIZE;

  // hack to get ChunkMaster base class
  using BaseWL = typename galois::worklists::PerSocketChunkFIFO<CHUNK_SIZE>::template retype<T>::type;
  using WL_ty = galois::worklists::WLsizeWrapper<BaseWL>;
  using LevelMap_ty = LevelMap<Key, KeyCmp, WL_ty>;

  using UserCtx = UserContextAccess<T>;
  using PerThreadUserCtx = substrate::PerThreadStorage<UserCtx>;

  struct BodyWrapper;
  using ForEachExec_ty = galois::runtime::ForEachWork<worklists::ExternPtr<WL_ty>, T, BodyWrapper>;

  struct BodyWrapper: public InheritTraits<ForEachTraits<OpFunc>::NeedsAborts> {
    KeyFn& keyFn;
    NhoodFunc& nhVisit;
    OpFunc& opFunc;
    LevelMap_ty& level_map;
    PerThreadUserCtx& userHandles;

    BodyWrapper (
        KeyFn& keyFn,
        NhoodFunc& nhVisit,
        OpFunc& opFunc,
        LevelMap_ty& level_map,
        PerThreadUserCtx& userHandles)
      :
        keyFn (keyFn),
        nhVisit (nhVisit),
        opFunc (opFunc),
        level_map (level_map),
        userHandles (userHandles)
    {}

    template <typename C>
    void operator () (T& x, C&) {

      UserCtx& uhand = *userHandles.getLocal ();

      if (ForEachTraits<OpFunc>::NeedsPush) {
        uhand.reset ();
      }

      nhVisit (x, uhand);
      opFunc (x, uhand);

      if (ForEachTraits<OpFunc>::NeedsPush) {
        for (auto i = uhand.getPushBuffer ().begin ()
            , endi = uhand.getPushBuffer ().end (); i != endi; ++i) {

          // using Galois WL, by ref, automatically handles
          // adds to the current level
          level_map.push (keyFn (*i), *i);
        }
      }
      uhand.reset ();
    }
  };

private:

  KeyFn keyFn;
  KeyCmp kcmp;
  NhoodFunc nhVisit;
  OpFunc opFunc;
  LevelMap_ty level_map;
  WL_ty* dummy;
  PerThreadUserCtx userHandles;
  ForEachExec_ty for_each_exec;
  Barrier& barrier;

  volatile bool done = false;

public:

  LevelExecutor (
      const KeyFn& _keyFn,
      const KeyCmp& _kcmp,
      const NhoodFunc& _nhVisit,
      const OpFunc& _opFunc,
      const char* loopname)
    :
      keyFn (_keyFn),
      kcmp (_kcmp),
      nhVisit (_nhVisit),
      opFunc (_opFunc),
      level_map (kcmp),
      dummy (new WL_ty ()),
      for_each_exec (
          galois::worklists::ExternPtr<WL_ty> (dummy),
          BodyWrapper ( keyFn, nhVisit, opFunc, level_map, userHandles),
          loopname),
      barrier (getSystemBarrier ())

  {}

  ~LevelExecutor (void) {
    delete dummy; dummy = nullptr;
  }

  // parallel
  template <typename R>
  void fill_initial (const R& range) {

    auto rp = range.local_pair ();
    for (auto i = rp.first, i_end = rp.second; i != i_end; ++i) {
      level_map.push (keyFn (*i), *i);
    }

    // galois::do_all (beg, end,
        // [this] (const T& x) {
          // level_map.push (keyFn (x), x);
        // });
  }

  static bool isMasterThread (void) {
    return substrate::ThreadPool::getTID () == 0;
  }

  // parallel
  void go (void) {

    unsigned steps = 0;
    size_t totalWork = 0;

    while (true) {

      if (isMasterThread ()) {
        level_map.freeRemovedLevels ();
        if (level_map.empty ()) {
          done = true;

        } else {

          std::pair<const Key, WL_ty*> currLevel = level_map.pop ();
          ++steps;
          totalWork += currLevel.second->size ();

          for_each_exec.reinit (typename galois::worklists::ExternPtr<WL_ty> (currLevel.second));
        }
      }

      for_each_exec.initThread ();

      barrier.wait ();

      if (done) {
        break;
      }

      for_each_exec ();

      barrier.wait ();

    }

    if (isMasterThread ()) {
      gPrint("Level-by-Level, critical path length: ", steps, ", avg. parallelism: ", totalWork/(double) steps, "\n");
    }

  }

  void operator () (void) {
    go ();
  }
};

} // end namespace impl


template <typename R, typename KeyFn, typename KeyCmp, typename NhoodFunc, typename OpFunc>
void for_each_ordered_level (const R& range, const KeyFn& keyFn, const KeyCmp& kcmp, const NhoodFunc& nhVisit, const OpFunc& opFunc, const char* loopname=0) {

  using T = typename R::value_type;
  using K = decltype (keyFn (*(range.begin ()))); // XXX: check
  using Exec_ty = LevelExecutor<T, K, KeyFn, KeyCmp, NhoodFunc, OpFunc>;

  Exec_ty exec (keyFn, kcmp, nhVisit, opFunc, loopname);
  Barrier& barrier = getSystemBarrier ();

  getThreadPool ().run (galois::getActiveThreads (),
    std::bind (&Exec_ty::template fill_initial<R>, std::ref (exec), range),
    std::ref (barrier),
    std::ref (exec));
}

} // end namespace runtime
} // end namespace galois

#else

#include "galois/GaloisForwardDecl.h"
#include "galois/gtuple.h"
#include "galois/runtime/ForEachTraits.h"
#include "galois/worklists/Obim.h"

namespace galois {
namespace runtime {

template <int... Is, typename R, typename OpFn, typename TupleTy>
auto for_each_ordered_level_(int_seq<Is...>, const R& range, const OpFn& opFn,
                             const TupleTy& tpl, int)
    -> decltype(std::declval<typename R::container_type>(), void()) {
  for_each(range.get_container(), opFn, std::get<Is>(tpl)...);
}

template <int... Is, typename R, typename OpFn, typename TupleTy>
auto for_each_ordered_level_(int_seq<Is...>, const R& range, const OpFn& opFn,
                             const TupleTy& tpl, ...) -> void {
  for_each(range.begin(), range.end(), opFn, std::get<Is>(tpl)...);
}

template <typename R, typename KeyFn, typename KeyCmp, typename NhoodFn,
          typename OpFn>
void for_each_ordered_level(const R& range, const KeyFn& keyFn,
                            const KeyCmp& keyCmp, NhoodFn nhoodFn, OpFn opFn,
                            const char* ln = 0) {
  typedef typename R::value_type value_type;
  typedef typename std::result_of<KeyFn(value_type)>::type key_type;
  constexpr bool is_less = std::is_same<KeyCmp, std::less<key_type>>::value;
  constexpr bool is_greater =
      std::is_same<KeyCmp, std::greater<key_type>>::value;
  static_assert((is_less || is_greater) && !(is_less && is_greater),
                "Arbitrary key comparisons not yet supported");
  constexpr unsigned chunk_size = OpFn::CHUNK_SIZE;

  typedef typename worklists::OrderedByIntegerMetric<>::template with_container<
      worklists::PerSocketChunkFIFO<chunk_size>>::type ::
      template with_indexer<KeyFn>::type ::template with_barrier<true>::type ::
          template with_descending<is_greater>::type ::template with_monotonic<
              true>::type WL;
  auto args = std::tuple_cat(
      typename galois::runtime::DEPRECATED::ExtractForEachTraits<
          OpFn>::values_type{},
      std::make_tuple(galois::loopname(ln), galois::wl<WL>(keyFn)));
  for_each_ordered_level_(
      make_int_seq<std::tuple_size<decltype(args)>::value>(), range,
      [&](value_type& x, UserContext<value_type>& ctx) {
        nhoodFn(x, ctx);
        opFn(x, ctx);
      },
      args, 0);
}

} // namespace runtime
} // namespace galois

#endif

#endif // GALOIS_RUNTIME_LEVEL_EXECUTOR_H
