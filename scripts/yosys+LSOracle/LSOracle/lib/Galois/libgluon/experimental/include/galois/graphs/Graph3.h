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

#ifndef GALOIS_GRAPH3_H
#define GALOIS_GRAPH3_H

#include <iterator>
#include <deque>

#include "galois/Threads.h"
#include "galois/graphs/Bag.h"
#include "galois/runtime/DistSupport.h"
#include "galois/runtime/Context.h"
#include "galois/runtime/MethodFlags.h"
#include "galois/runtime/PerThreadStorage.h"
#include "galois/runtime/mm/Mem.h"

#include <boost/iterator/filter_iterator.hpp>

#include <iostream>

namespace galois {
namespace graphs {

enum class EdgeDirection { Un, Out, InOut };

template <typename NodeTy, typename EdgeTy, EdgeDirection EDir>
class ThirdGraph;

template <typename NHTy>
class GraphNodeBase {
  NHTy nextNode;
  bool active;

protected:
  GraphNodeBase() : active(false) {}

  NHTy& getNextNode() { return nextNode; }

  void serialize(galois::runtime::SerializeBuffer& s) const {
    gSerialize(s, nextNode, active);
  }
  void deserialize(galois::runtime::DeSerializeBuffer& s) {
    gDeserialize(s, nextNode, active);
  }

  void dump(std::ostream& os) const {
    os << "next: " << nextNode << " active: " << active;
  }

public:
  bool getActive() const { return active; }

  void setActive(bool b) { active = b; }
};

template <typename NodeDataTy>
class GraphNodeData {
  NodeDataTy data;

protected:
  void serialize(galois::runtime::SerializeBuffer& s) const {
    gSerialize(s, data);
  }
  void deserialize(galois::runtime::DeSerializeBuffer& s) {
    gDeserialize(s, data);
  }

  void dump(std::ostream& os) const { os << "data: " << data; }

public:
  template <typename... Args>
  GraphNodeData(Args&&... args) : data(std::forward<Args...>(args...)) {}
  GraphNodeData() : data() {}

  NodeDataTy& getData() { return data; }
};

template <>
class GraphNodeData<void> {};

template <typename NHTy, typename EdgeDataTy, EdgeDirection EDir>
class GraphNodeEdges;

template <typename NHTy, typename EdgeDataTy>
class Edge {
  NHTy dst;
  EdgeDataTy val;

public:
  template <typename... Args>
  Edge(const NHTy& d, Args&&... args)
      : dst(d), val(std::forward<Args...>(args...)) {}

  Edge() {}

  NHTy getDst() const { return dst; }
  EdgeDataTy& getValue() { return val; }

  typedef int tt_has_serialize;
  void serialize(galois::runtime::SerializeBuffer& s) const {
    gSerialize(s, dst, val);
  }
  void deserialize(galois::runtime::DeSerializeBuffer& s) {
    gDeserialize(s, dst, val);
  }

  void dump(std::ostream& os) const {
    os << "<{Edge: dst: " << dst << " dst active: " << dst->getActive()
       << " val: " << val << "}>";
  }

  // Trivially copyable
  typedef int tt_is_copyable;
};

template <typename NHTy>
class Edge<NHTy, void> {
  NHTy dst;

public:
  Edge(const NHTy& d) : dst(d) {}
  Edge() {}

  NHTy getDst() const { return dst; }

  void dump(std::ostream& os) const {
    os << "<{Edge: dst: ";
    dst->dump(os);
    os << " dst active: ";
    os << dst->getActive();
    os << "}>";
  }

  // Trivially copyable
  typedef int tt_is_copyable;
};

template <typename NHTy, typename EdgeDataTy>
class GraphNodeEdges<NHTy, EdgeDataTy, EdgeDirection::Out> {
  typedef Edge<NHTy, EdgeDataTy> EdgeTy;
  typedef gdeque<EdgeTy, 4> EdgeListTy;

  EdgeListTy edges;

protected:
  void serialize(galois::runtime::SerializeBuffer& s) const {
    gSerialize(s, edges);
  }
  void deserialize(galois::runtime::DeSerializeBuffer& s) {
    gDeserialize(s, edges);
  }
  void dump(std::ostream& os) const {
    os << "numedges: " << edges.size();
    for (auto ii = edges.begin(), ee = edges.end(); ii != ee; ++ii) {
      os << " ";
      ii->dump(os);
    }
  }

public:
  typedef typename EdgeListTy::iterator iterator;

  template <typename... Args>
  void createEdge(const NHTy& src, const NHTy& dst, Args&&... args) {
    edges.emplace_back(dst, std::forward<Args...>(args...));
  }

  void createEdge(const NHTy& src, const NHTy& dst) { edges.emplace_back(dst); }

  void clearEdges() { edges.clear(); }

  iterator begin() { return edges.begin(); }

  iterator end() { return edges.end(); }
};

template <typename NHTy, typename EdgeDataTy>
class GraphNodeEdges<NHTy, EdgeDataTy, EdgeDirection::InOut> {
  // FIXME
};

template <typename NHTy>
class GraphNodeEdges<NHTy, void, EdgeDirection::Un> {
  typedef Edge<NHTy, void> EdgeTy;
  typedef gdeque<EdgeTy, 4> EdgeListTy;

  EdgeListTy edges;

protected:
  void serialize(galois::runtime::SerializeBuffer& s) const {
    gSerialize(s, edges);
  }
  void deserialize(galois::runtime::DeSerializeBuffer& s) {
    gDeserialize(s, edges);
  }
  void dump(std::ostream& os) const {
    os << "numedges: " << edges.size();
    for (auto ii = edges.begin(), ee = edges.end(); ii != ee; ++ii) {
      os << " ";
      ii->dump(os);
    }
  }

public:
  typedef typename EdgeListTy::iterator iterator;

  void createEdge(NHTy& src, NHTy& dest) {
    // assert(*src == this);
    dest->edges.emplace_back(src);
    edges.emplace_back(dest);
  }

  void clearEdges() { edges.clear(); }

  iterator begin() { return edges.begin(); }

  iterator end() { return edges.end(); }
};

template <typename NHTy, typename EdgeDataTy>
class GraphNodeEdges<NHTy, EdgeDataTy, EdgeDirection::Un> {
  // FIXME
};

template <typename NodeDataTy, typename EdgeDataTy, EdgeDirection EDir>
class GraphNode
    : public galois::runtime::Lockable,
      public GraphNodeBase<
          galois::runtime::gptr<GraphNode<NodeDataTy, EdgeDataTy, EDir>>>,
      public GraphNodeData<NodeDataTy>,
      public GraphNodeEdges<
          galois::runtime::gptr<GraphNode<NodeDataTy, EdgeDataTy, EDir>>,
          EdgeDataTy, EDir> {
  friend class ThirdGraph<NodeDataTy, EdgeDataTy, EDir>;
  using GraphNodeBase<galois::runtime::gptr<
      GraphNode<NodeDataTy, EdgeDataTy, EDir>>>::getNextNode;

public:
  typedef galois::runtime::gptr<GraphNode<NodeDataTy, EdgeDataTy, EDir>> Handle;
  typedef typename galois::graphs::Edge<Handle, EdgeDataTy> EdgeType;
  typedef
      typename GraphNodeEdges<Handle, EdgeDataTy, EDir>::iterator edge_iterator;

  template <typename... Args>
  GraphNode(Args&&... args)
      : GraphNodeData<NodeDataTy>(std::forward<Args...>(args...)) {}

  GraphNode() = default;
  GraphNode(galois::runtime::DeSerializeBuffer& buf) { deserialize(buf); }

  // serialize
  typedef int tt_has_serialize;
  void serialize(galois::runtime::SerializeBuffer& s) const {
    GraphNodeBase<Handle>::serialize(s);
    GraphNodeData<NodeDataTy>::serialize(s);
    GraphNodeEdges<Handle, EdgeDataTy, EDir>::serialize(s);
  }
  void deserialize(galois::runtime::DeSerializeBuffer& s) {
    GraphNodeBase<Handle>::deserialize(s);
    GraphNodeData<NodeDataTy>::deserialize(s);
    GraphNodeEdges<Handle, EdgeDataTy, EDir>::deserialize(s);
  }
  void dump(std::ostream& os) const {
    os << this << " ";
    os << "<{GN: ";
    GraphNodeBase<Handle>::dump(os);
    os << " ";
    GraphNodeData<NodeDataTy>::dump(os);
    os << " ";
    GraphNodeEdges<Handle, EdgeDataTy, EDir>::dump(os);
    os << "}>";
  }

  friend std::ostream& operator<<(std::ostream& os, const GraphNode& n) {
    n.dump(os);
    return os;
  }
};

/**
 * A Graph
 *
 * @param NodeTy type of node data (may be void)
 * @param EdgeTy type of edge data (may be void)
 * @param IsDir  bool indicated if graph is directed
 *
 */
template <typename NodeTy, typename EdgeTy, EdgeDirection EDir>
class ThirdGraph {
  typedef GraphNode<NodeTy, EdgeTy, EDir> gNode;

  typename Bag<gNode>::pointer localStateStore;
  typename Bag<runtime::gptr<gNode>>::pointer localStatePtr;
  runtime::PerThreadDist<ThirdGraph> basePtr;

  struct is_edge : public std::unary_function<typename gNode::EdgeType&, bool> {
    bool operator()(typename gNode::EdgeType& n) const {
      return n.getDst()->getActive();
    }
  };
  struct is_node : public std::unary_function<runtime::gptr<gNode>&, bool> {
    bool operator()(const runtime::gptr<gNode>& g) const {
      acquire(g, MethodFlag::ALL);
      return g->getActive();
    }
  };
  struct makePtrLocal
      : public std::unary_function<gNode&, runtime::gptr<gNode>> {
    runtime::gptr<gNode> operator()(gNode& data) const {
      return runtime::gptr<gNode>(&data);
    }
  };

  static void
  invalidate(typename Bag<runtime::gptr<gNode>>::pointer localStatePtr) {
    for (unsigned x = 0; x < runtime::getSystemThreadPool().getMaxThreads();
         ++x) {
      auto lptr =
          localStatePtr.remote(runtime::NetworkInterface::ID, x).resolve();
      for (auto ii = lptr->local_begin(), ei = lptr->local_end(); ii != ei;
           ++ii) {
        galois::runtime::getLocalDirectory().invalidate(
            static_cast<runtime::fatPointer>(*ii));
      }
    }
  }

public:
  typedef typename gNode::Handle NodeHandle;
  //! Edge iterator
  typedef
      typename boost::filter_iterator<is_edge, typename gNode::edge_iterator>
          edge_iterator;

  template <typename... Args>
  NodeHandle createNode(Args&&... args) {
    NodeHandle N(&*localStateStore->emplace(std::forward<Args...>(args...)));
    localStatePtr->push(N);
    return N;
  }

  NodeHandle createNode() {
    NodeHandle N(&*localStateStore->emplace());
    localStatePtr->push(N);
    return N;
  }

  void addNode(NodeHandle N) { N->setActive(true); }

  void removeNode(NodeHandle N) {
    if (N->getActive()) {
      N->setActive(false);
      // delete all the edges (in the deque)
      N->clearEdges();
    }
  }

  //! Node iterator
  typedef boost::filter_iterator<
      is_node, typename Bag<runtime::gptr<gNode>>::local_iterator>
      local_iterator;

  local_iterator local_begin() {
    return boost::make_filter_iterator<is_node>(localStatePtr->local_begin(),
                                                localStatePtr->local_end());
  }

  local_iterator local_end() {
    assert(localStatePtr->local_end() == localStatePtr->local_end());
    return boost::make_filter_iterator<is_node>(localStatePtr->local_end(),
                                                localStatePtr->local_end());
  }

  typedef boost::filter_iterator<is_node,
                                 typename Bag<runtime::gptr<gNode>>::iterator>
      iterator;

  iterator begin() {
    return boost::make_filter_iterator<is_node>(localStatePtr->begin(),
                                                localStatePtr->end());
  }

  iterator end() {
    return boost::make_filter_iterator<is_node>(localStatePtr->end(),
                                                localStatePtr->end());
  }

  //! Returns an iterator to the neighbors of a node
  edge_iterator edge_begin(NodeHandle N) {
    assert(N);
    acquire(N, galois::MethodFlag::ALL);
    // prefetch all the nodes
    for (auto ii = N->begin(), ee = N->end(); ii != ee; ++ii) {
      // ii->getDst().prefetch();
    }
    // lock all the nodes
    for (auto ii = N->begin(), ee = N->end(); ii != ee; ++ii) {
      // NOTE: Andrew thinks acquire may be needed for inactive nodes too
      //       not sure why though. he had to do this in the prev graph
      acquire(ii->getDst(), galois::MethodFlag::ALL);
      if (ii->getDst()->getActive()) {
        // modify the call when local nodes aren't looked up in directory
        //        ii->getDst().acquire();
      }
    }
    return boost::make_filter_iterator(is_edge(), N->begin(), N->end());
  }

  //! Returns the end of the neighbor iterator
  edge_iterator edge_end(NodeHandle N) {
    assert(N);
    acquire(N, galois::MethodFlag::ALL);
    return boost::make_filter_iterator(is_edge(), N->end(), N->end());
  }

  void addEdge(NodeHandle src, NodeHandle dst) {
    assert(src);
    assert(dst);
    src->createEdge(src, dst);
  }

  template <typename... Args>
  void addEdge(NodeHandle src, NodeHandle dst, Args&&... args) {
    assert(src);
    assert(dst);
    src->createEdge(src, dst, std::forward<Args...>(args...));
  }

  NodeHandle getEdgeDst(edge_iterator ii) {
    assert(ii->getDst()->getActive());
    return ii->getDst();
  }

  template <typename EI>
  auto getEdgeData(EI ii) -> decltype(ii->getValue()) {
    return ii->getValue();
  }

  NodeTy& getData(const NodeHandle& N) {
    assert(N);
    return N->getData();
  }

  bool containsNode(const NodeHandle& N) {
    assert(N);
    return N->getActive();
  }

  size_t size() const {
    GALOIS_DIE("Not implemented");
    return 0;
  }

  typedef runtime::PerThreadDist<ThirdGraph> pointer;
  static pointer allocate() {
    return runtime::PerThreadDist<ThirdGraph>::allocate();
  }
  static void deallocate(pointer p) {
    runtime::getSystemNetworkInterface().broadcastAlt(&invalidate,
                                                      p->localStatePtr);
    invalidate(p->localStatePtr);
    Bag<runtime::gptr<gNode>>::deallocate(p->localStatePtr);
    Bag<gNode>::deallocate(p->localStateStore);
    runtime::PerThreadDist<ThirdGraph>::deallocate(p);
  }

  explicit ThirdGraph(pointer p) : basePtr(p) {
    localStateStore = Bag<gNode>::allocate();
    localStatePtr   = Bag<runtime::gptr<gNode>>::allocate();
  }

  explicit ThirdGraph(pointer p, runtime::DeSerializeBuffer& buf) : basePtr(p) {
    gDeserialize(buf, localStateStore, localStatePtr);
    // gDeserialize(buf, localStatePtr);
    assert(localStateStore);
    assert(localStatePtr);
  }

  void getInitData(runtime::SerializeBuffer& buf) {
    gSerialize(buf, localStateStore, localStatePtr);
    // gSerialize(buf, localStatePtr);
  }
};

} // namespace graphs
} // namespace galois

#endif
