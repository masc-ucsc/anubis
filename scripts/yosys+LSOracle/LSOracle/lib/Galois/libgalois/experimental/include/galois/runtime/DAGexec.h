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

#ifndef GALOIS_RUNTIME_DAGEXEC_H
#define GALOIS_RUNTIME_DAGEXEC_H

#include "galois/GaloisForwardDecl.h"
#include "galois/Reduction.h"
#include "galois/Atomic.h"
#include "galois/gdeque.h"
#include "galois/PriorityQueue.h"
#include "galois/Timer.h"
#include "galois/DoAllWrap.h"
#include "galois/PerThreadContainer.h"

#include "galois/runtime/Context.h"
#include "galois/runtime/Executor_DoAll.h"
#include "galois/runtime/OrderedLockable.h"
#include "galois/gIO.h"
#include "galois/runtime/Mem.h"

#include "llvm/Support/CommandLine.h"

#include <atomic>

namespace galois {
namespace runtime {

namespace {

template <typename Ctxt>
struct DAGnhoodItem
    : public OrdLocBase<DAGnhoodItem<Ctxt>, Ctxt, std::less<Ctxt*>> {

  using CtxtCmp = std::less<Ctxt*>;
  using Base    = OrdLocBase<DAGnhoodItem, Ctxt, CtxtCmp>;

public:
  typedef galois::ThreadSafeOrderedSet<Ctxt*, CtxtCmp> SharerSet;

  SharerSet sharers;

public:
  explicit DAGnhoodItem(Lockable* l, const CtxtCmp& c) : Base(l), sharers(c) {}

  void addSharer(Ctxt* ctxt) { sharers.push(ctxt); }
};

template <typename Ctxt>
struct DAGnhoodItemRW
    : public OrdLocBase<DAGnhoodItemRW<Ctxt>, Ctxt, std::less<Ctxt*>> {

  using CtxtCmp = std::less<Ctxt*>;
  using Base    = OrdLocBase<DAGnhoodItemRW, Ctxt, CtxtCmp>;

public:
  typedef galois::ThreadSafeOrderedSet<Ctxt*, CtxtCmp> SharerSet;

  SharerSet writers;
  SharerSet readers;

  explicit DAGnhoodItemRW(Lockable* l, const CtxtCmp& c) : Base(l) {}

  void addReader(Ctxt* ctxt) { readers.push(ctxt); }

  void addWriter(Ctxt* ctxt) { writers.push(ctxt); }
};

template <typename T, typename Derived, typename NItem_tp>
struct DAGcontextBase : public OrderedContextBase<T> {

  using Base = OrderedContextBase<T>;

  typedef NItem_tp NItem;
  typedef PtrBasedNhoodMgr<NItem> NhoodMgr;

public:
  typedef galois::ThreadSafeOrderedSet<Derived*, std::less<Derived*>> AdjSet;
  // TODO: change AdjList to array for quicker iteration
  typedef galois::gdeque<Derived*, 64> AdjList;
  typedef std::atomic<int> ParCounter;

  // GALOIS_ATTRIBUTE_ALIGN_CACHE_LINE ParCounter inDeg;
  ParCounter inDeg;
  int origInDeg;
  NhoodMgr& nhmgr;
  unsigned outDeg;
  Derived** outNeighbors;

  AdjSet adjSet;

public:
  explicit DAGcontextBase(const T& t, NhoodMgr& nhmgr)
      : Base(t), inDeg(0), origInDeg(0), nhmgr(nhmgr), outDeg(0),
        outNeighbors(nullptr) {}

  //! returns true on success
  bool addOutNeigh(Derived* that) { return adjSet.push(that); }

  void addInNeigh(Derived* that) {
    int* x = const_cast<int*>(&origInDeg);
    ++(*x);
    ++inDeg;
  }

  template <typename A>
  void finalizeAdj(A& allocator) {
    assert(outDeg == 0);
    assert(outNeighbors == nullptr);

    outDeg = adjSet.size();

    if (outDeg != 0) {
      outNeighbors = allocator.allocate(outDeg);

      unsigned j = 0;
      for (auto i = adjSet.begin(), i_end = adjSet.end(); i != i_end; ++i) {

        outNeighbors[j++] = *i;
      }

      assert(j == outDeg);
      adjSet.clear();
    }
  }

  Derived** neighbor_begin(void) { return &outNeighbors[0]; }

  Derived** neighbor_end(void) { return &outNeighbors[outDeg]; }

  bool removeLastInNeigh(Derived* that) {
    assert(inDeg >= 0);
    return ((--inDeg) == 0);
  }

  bool isSrc(void) const { return inDeg == 0; }

  void reset(void) { inDeg = origInDeg; }

  template <typename A>
  void reclaimAlloc(A& allocator) {
    if (outDeg != 0) {
      allocator.deallocate(outNeighbors, outDeg);
      outNeighbors = nullptr;
    }
  }
};

template <typename T>
struct DAGcontext
    : public DAGcontextBase<T, DAGcontext<T>, DAGnhoodItem<DAGcontext<T>>> {

  typedef DAGcontextBase<T, DAGcontext, DAGnhoodItem<DAGcontext>> Base;
  typedef typename Base::NItem NItem;
  typedef typename Base::NhoodMgr NhoodMgr;

  explicit DAGcontext(const T& t, typename Base::NhoodMgr& nhmgr)
      : Base(t, nhmgr) {}

  GALOIS_ATTRIBUTE_PROF_NOINLINE
  virtual void subAcquire(Lockable* l, galois::MethodFlag) {
    NItem& nitem = Base::nhmgr.getNhoodItem(l);

    assert(NItem::getOwner(l) == &nitem);

    nitem.addSharer(this);
  }
};

template <typename T>
struct DAGcontextRW : public DAGcontextBase<T, DAGcontextRW<T>,
                                            DAGnhoodItemRW<DAGcontextRW<T>>> {

  typedef DAGcontextBase<T, DAGcontextRW, DAGnhoodItemRW<DAGcontextRW>> Base;
  typedef typename Base::NItem NItem;
  typedef typename Base::NhoodMgr NhoodMgr;

  explicit DAGcontextRW(const T& t, NhoodMgr& nhmgr) : Base(t, nhmgr) {}

  GALOIS_ATTRIBUTE_PROF_NOINLINE
  virtual void subAcquire(Lockable* l, galois::MethodFlag f) {

    NItem& nitem = Base::nhmgr.getNhoodItem(l);

    assert(NItem::getOwner(l) == &nitem);

    switch (f) {
    case galois::MethodFlag::READ:
      nitem.addReader(this);
      break;

    case galois::MethodFlag::WRITE:
      nitem.addWriter(this);
      break;

    default:
      GALOIS_DIE("can't handle flag type\n");
      break;
    }
  }
};

} // namespace

template <typename T, typename Cmp, typename OpFunc, typename NhoodFunc,
          typename Ctxt_tp>
class DAGexecutorBase {

protected:
  using Ctxt         = Ctxt_tp;
  using NhoodMgr     = typename Ctxt::NhoodMgr;
  using NItem        = typename Ctxt::NItem;
  using CtxtCmp      = typename NItem::CtxtCmp;
  using NItemFactory = typename NItem::Factory;

  using CtxtAlloc        = FixedSizeAllocator<Ctxt>;
  using CtxtAdjAlloc     = Pow_2_BlockAllocator<Ctxt*>;
  using CtxtWL           = PerThreadBag<Ctxt*>;
  using UserCtx          = UserContextAccess<T>;
  using PerThreadUserCtx = substrate::PerThreadStorage<UserCtx>;

  struct ApplyOperator {

    typedef int tt_does_not_need_aborts;

    DAGexecutorBase& outer;

    template <typename W>
    void operator()(Ctxt* src, W& wl) {
      assert(src->isSrc());

      // printf ("processing source: %p, item: %d\n", src, src->elem);

      UserCtx& uctx = *(outer.userCtxts.getLocal());
      outer.opFunc(src->getActive(), uctx);

      for (auto i = src->neighbor_begin(), i_end = src->neighbor_end();
           i != i_end; ++i) {

        if ((*i)->removeLastInNeigh(src)) {
          wl.push(*i);
          outer.numPush += 1;
        }
      }
    }
  };

  static const unsigned DEFAULT_CHUNK_SIZE = 16;

  Cmp cmp;
  NhoodFunc nhVisitor;
  OpFunc opFunc;
  std::string loopname;
  NItemFactory nitemFactory;
  NhoodMgr nhmgr;

  CtxtAlloc ctxtAlloc;
  CtxtAdjAlloc ctxtAdjAlloc;
  CtxtWL allCtxts;
  CtxtWL initSources;
  PerThreadUserCtx userCtxts;
  galois::GAccumulator<size_t> numPush;

public:
  DAGexecutorBase(const Cmp& cmp, const NhoodFunc& nhVisitor,
                  const OpFunc& opFunc, const char* loopname)
      : cmp(cmp), nhVisitor(nhVisitor), opFunc(opFunc), loopname(loopname),
        nitemFactory(CtxtCmp()), nhmgr(nitemFactory) {}

  ~DAGexecutorBase(void) {
    galois::runtime::do_all_gen(
        galois::runtime::makeLocalRange(allCtxts),
        [this](Ctxt* ctxt) {
          ctxt->reclaimAlloc(ctxtAdjAlloc);
          ctxtAlloc.destroy(ctxt);
          ctxtAlloc.deallocate(ctxt, 1);
        },
        std::make_tuple(galois::loopname("free_ctx"),
                        galois::chunk_size<DEFAULT_CHUNK_SIZE>()));
  }

  void createEdge(Ctxt* a, Ctxt* b) {
    assert(a != nullptr);
    assert(b != nullptr);

    if (a == b) {
      // no self edges
      return;
    }

    // a < b ? a : b
    Ctxt* src = cmp(a->getActive(), b->getActive()) ? a : b;
    Ctxt* dst = (src == a) ? b : a;

    // avoid adding same edge multiple times
    if (src->addOutNeigh(dst)) {
      dst->addInNeigh(src);
    }
  }

  virtual void createAllEdges(void) = 0;

  template <typename R>
  void initialize(const R& range) {
    //
    // 1. create contexts and expand neighborhoods and create graph nodes
    // 2. go over nhood items and create edges
    // 3. Find initial sources and run for_each
    //

    galois::StatTimer t_init("Time to create the DAG: ");

    std::printf("total number of tasks: %ld\n",
                std::distance(range.begin(), range.end()));

    t_init.start();
    galois::runtime::do_all_gen(
        range,
        [this](const T& x) {
          Ctxt* ctxt = ctxtAlloc.allocate(1);
          assert(ctxt != NULL);
          ctxtAlloc.construct(ctxt, x, nhmgr);

          allCtxts.get().push_back(ctxt);

          galois::runtime::setThreadContext(ctxt);

          UserCtx& uctx = *(userCtxts.getLocal());
          nhVisitor(ctxt->getActive(), uctx);
          galois::runtime::setThreadContext(NULL);

          // printf ("Created context:%p for item: %d\n", ctxt, x);
        },
        std::make_tuple(galois::loopname("create_ctxt"),
                        galois::chunk_size<DEFAULT_CHUNK_SIZE>()));

    // virtual call implemented by derived classes
    createAllEdges();

    galois::runtime::do_all_gen(
        galois::runtime::makeLocalRange(allCtxts),
        [this](Ctxt* ctxt) {
          ctxt->finalizeAdj(ctxtAdjAlloc);
          // std::printf ("ctxt: %p, indegree=%d\n", ctxt, ctxt->origInDeg);
          if (ctxt->isSrc()) {
            initSources.get().push_back(ctxt);
          }
        },
        std::make_tuple(galois::loopname("finalize"),
                        galois::chunk_size<DEFAULT_CHUNK_SIZE>()));

    std::printf("Number of initial sources: %ld\n", initSources.size_all());

    // printStats ();

    t_init.stop();
  }

  void execute(void) {

    StatTimer t_exec("Time to execute the DAG: ");

    typedef galois::worklists::PerSocketChunkFIFO<OpFunc::CHUNK_SIZE, Ctxt*>
        SrcWL_ty;

    t_exec.start();

    galois::for_each(galois::iterate(initSources), ApplyOperator{*this},
                     galois::loopname("apply_operator"),
                     galois::wl<SrcWL_ty>());

    // std::printf ("Number of pushes: %zd\n, (#pushes + #init) = %zd\n",
    // numPush.reduceRO (), numPush.reduceRO () + initSources.size_all  ());
    t_exec.stop();
  }

  void resetDAG(void) {
    galois::StatTimer t_reset("Time to reset the DAG: ");

    t_reset.start();
    galois::runtime::do_all_gen(
        galois::runtime::makeLocalRange(allCtxts),
        [](Ctxt* ctxt) { ctxt->reset(); },
        std::make_tuple(galois::loopname("resetDAG"),
                        galois::chunk_size<DEFAULT_CHUNK_SIZE>()));
    t_reset.stop();
  }

  void printStats(void) {
    galois::GAccumulator<size_t> numEdges;
    galois::GAccumulator<size_t> numNodes;

    printf("WARNING: timing affected by measuring DAG stats\n");

    galois::runtime::do_all_gen(
        galois::runtime::makeLocalRange(allCtxts),
        [&numNodes, &numEdges](Ctxt* ctxt) {
          numNodes += 1;
          numEdges +=
              std::distance(ctxt->neighbor_begin(), ctxt->neighbor_end());
        },
        std::make_tuple(galois::loopname("dag_stats"),
                        galois::chunk_size<DEFAULT_CHUNK_SIZE>()));

    printf("DAG created with %zd nodes, %zd edges\n", numNodes.reduce(),
           numEdges.reduce());
  }
};

template <typename T, typename Cmp, typename OpFunc, typename NhoodFunc>
struct DAGexecutor
    : public DAGexecutorBase<T, Cmp, OpFunc, NhoodFunc, DAGcontext<T>> {

  typedef DAGexecutorBase<T, Cmp, OpFunc, NhoodFunc, DAGcontext<T>> Base;
  typedef typename Base::NItem NItem;

  DAGexecutor(const Cmp& cmp, const NhoodFunc& nhoodVisitor,
              const OpFunc& opFunc, const char* loopname)
      : Base(cmp, nhoodVisitor, opFunc, loopname) {}

  virtual void createAllEdges(void) {
    galois::runtime::do_all_gen(
        Base::nhmgr.getAllRange(),
        [this](NItem* nitem) {
          // std::printf ("Nitem: %p, num sharers: %ld\n", nitem,
          // nitem->sharers.size ());

          for (auto i = nitem->sharers.begin(), i_end = nitem->sharers.end();
               i != i_end; ++i) {

            auto j = i;
            ++j;
            for (; j != i_end; ++j) {
              Base::createEdge(*i, *j);
            }
          }
        },
        std::make_tuple(galois::loopname("create_ctxt_edges"),
                        galois::chunk_size<Base::DEFAULT_CHUNK_SIZE>()));
  }
};

template <typename T, typename Cmp, typename OpFunc, typename NhoodFunc>
struct DAGexecutorRW
    : public DAGexecutorBase<T, Cmp, OpFunc, NhoodFunc, DAGcontextRW<T>> {

  typedef DAGexecutorBase<T, Cmp, OpFunc, NhoodFunc, DAGcontextRW<T>> Base;
  typedef typename Base::NItem NItem;

  DAGexecutorRW(const Cmp& cmp, const NhoodFunc& nhoodVisitor,
                const OpFunc& opFunc, const char* loopname)
      : Base(cmp, nhoodVisitor, opFunc, loopname) {}

  virtual void createAllEdges(void) {

    galois::runtime::do_all_gen(
        Base::nhmgr.getAllRange(),
        [this](NItem* nitem) {
          // std::printf ("Nitem: %p, num sharers: %ld\n", nitem,
          // nitem->sharers.size ());

          for (auto w = nitem->writers.begin(), w_end = nitem->writers.end();
               w != w_end; ++w) {

            auto v = w;
            ++v;
            for (; v != w_end; ++v) {
              Base::createEdge(*w, *v);
            }

            for (auto r = nitem->readers.begin(), r_end = nitem->readers.end();
                 r != r_end; ++r) {
              Base::createEdge(*w, *r);
            }
          }
        },
        std::make_tuple(galois::loopname("create_ctxt_edges"),
                        galois::chunk_size<Base::DEFAULT_CHUNK_SIZE>()));
  }
};

template <typename R, typename Cmp, typename OpFunc, typename NhoodFunc>
DAGexecutorRW<typename R::value_type, Cmp, OpFunc, NhoodFunc>*
make_dag_executor(const R& range, const Cmp& cmp, const NhoodFunc& nhVisitor,
                  const OpFunc& opFunc, const char* loopname = nullptr) {

  return new DAGexecutorRW<typename R::value_type, Cmp, OpFunc, NhoodFunc>(
      cmp, nhVisitor, opFunc, loopname);
}

template <typename R, typename Cmp, typename OpFunc, typename NhoodFunc>
void destroy_dag_executor(
    DAGexecutorRW<typename R::value_type, Cmp, OpFunc, NhoodFunc>*& exec_ptr) {
  delete exec_ptr;
  exec_ptr = nullptr;
}

template <typename R, typename Cmp, typename OpFunc, typename NhoodFunc>
void for_each_ordered_dag(const R& range, const Cmp& cmp,
                          const NhoodFunc& nhVisitor, const OpFunc& opFunc,
                          const char* loopname = nullptr) {

  typedef typename R::value_type T;
  typedef DAGexecutorRW<T, Cmp, OpFunc, NhoodFunc> Exec_ty;
  // typedef DAGexecutor<T, Cmp, OpFunc, NhoodFunc> Exec_ty;

  Exec_ty exec(cmp, nhVisitor, opFunc, loopname);

  exec.initialize(range);

  exec.execute();
}

} // end namespace runtime
} // end namespace galois

#endif // GALOIS_RUNTIME_DAG_H
