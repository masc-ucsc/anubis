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

#ifndef GALOIS_GRAPH_LCDIST_H
#define GALOIS_GRAPH_LCDIST_H

#include <vector>
#include <iostream>

namespace galois {
namespace graphs {

template <typename NodeTy, typename EdgeTy>
class LC_Dist {

  struct EdgeImplTy;

  struct NodeImplTy : public runtime::Lockable {
    NodeTy data;
    EdgeImplTy* b;
    EdgeImplTy* e;
    unsigned len;
    bool remote;

    NodeImplTy(EdgeImplTy* start, unsigned len)
        : b(start), e(start), len(len), remote(false) {}
    ~NodeImplTy() {
      if (remote)
        delete[] b;
      b = e = nullptr;
    }

    // Serialization support
    typedef int tt_has_serialize;
    NodeImplTy(runtime::DeSerializeBuffer& buf) : remote(true) {
      ptrdiff_t diff;
      gDeserialize(buf, data, len, diff);
      b = e = len ? (new EdgeImplTy[len]) : nullptr;
      while (diff--) {
        runtime::gptr<NodeImplTy> ptr;
        EdgeTy tmp;
        gDeserialize(buf, ptr, tmp);
        append(ptr, tmp);
      }
    }
    void serialize(runtime::SerializeBuffer& buf) const {
      EdgeImplTy* begin = b;
      EdgeImplTy* end   = e;
      ptrdiff_t diff    = end - begin;
      gSerialize(buf, data, len, diff);
      while (begin != end) {
        gSerialize(buf, begin->dst, begin->data);
        ++begin;
      }
    }
    void deserialize(runtime::DeSerializeBuffer& buf) {
      assert(!remote);
      ptrdiff_t diff;
      unsigned _len;
      EdgeImplTy* begin = b;
      gDeserialize(buf, data, _len, diff);
      assert(_len == len);
      assert(diff >= e - b);
      while (diff--) {
        gDeserialize(buf, begin->dst, begin->data);
        ++begin;
      }
      e = begin;
    }

    EdgeImplTy* append(runtime::gptr<NodeImplTy> dst, const EdgeTy& data) {
      e->dst  = dst;
      e->data = data;
      return e++;
    }

    EdgeImplTy* append(runtime::gptr<NodeImplTy> dst) {
      e->dst = dst;
      return e++;
    }

    EdgeImplTy* begin() { return b; }
    EdgeImplTy* end() { return e; }
  };

  struct EdgeImplTy {
    runtime::gptr<NodeImplTy> dst;
    EdgeTy data;
  };

public:
  typedef runtime::gptr<NodeImplTy> GraphNode;

private:
  std::vector<NodeImplTy> Nodes;
  std::vector<EdgeImplTy> Edges;

  std::vector<std::pair<GraphNode, std::atomic<int>>> Starts;
  std::vector<unsigned> Num;
  std::vector<unsigned> PrefixNum;

  runtime::PerHost<LC_Dist> self;

  friend class runtime::PerHost<LC_Dist>;

  LC_Dist(runtime::PerHost<LC_Dist> _self, std::vector<unsigned>& edges)
      : Starts(runtime::NetworkInterface::Num),
        Num(runtime::NetworkInterface::Num),
        PrefixNum(runtime::NetworkInterface::Num), self(_self) {
    // std::cerr << runtime::NetworkInterface::ID << " Construct\n";
    runtime::trace("LC_Dist with % nodes total\n", edges.size());
    // Fill up metadata vectors
    for (unsigned h = 0; h < runtime::NetworkInterface::Num; ++h) {
      auto p = block_range(edges.begin(), edges.end(), h,
                           runtime::NetworkInterface::Num);
      Num[h] = std::distance(p.first, p.second);
    }
    std::partial_sum(Num.begin(), Num.end(), PrefixNum.begin());

    // std::copy(Num.begin(), Num.end(),
    // std::ostream_iterator<unsigned>(std::cout, ",")); std::cout << "\n";
    // std::copy(PrefixNum.begin(), PrefixNum.end(),
    // std::ostream_iterator<unsigned>(std::cout, ",")); std::cout << "\n";

    // Block nodes
    auto p =
        block_range(edges.begin(), edges.end(), runtime::NetworkInterface::ID,
                    runtime::NetworkInterface::Num);
    // Allocate Edges
    unsigned sum = std::accumulate(p.first, p.second, 0);
    Edges.resize(sum);
    EdgeImplTy* cur = &Edges[0];
    // allocate Nodes
    Nodes.reserve(std::distance(p.first, p.second));
    for (auto ii = p.first; ii != p.second; ++ii) {
      Nodes.emplace_back(cur, *ii);
      cur += *ii;
    }
    Starts[runtime::NetworkInterface::ID].first  = GraphNode(&Nodes[0]);
    Starts[runtime::NetworkInterface::ID].second = 2;
  }

  static void getStart(runtime::PerHost<LC_Dist> graph, uint32_t whom) {
    // std::cerr << runtime::NetworkInterface::ID << " getStart " << whom <<
    // "\n";
    runtime::getSystemNetworkInterface().sendAlt(
        whom, putStart, graph, runtime::NetworkInterface::ID,
        graph->Starts[runtime::NetworkInterface::ID].first);
  }

  static void putStart(runtime::PerHost<LC_Dist> graph, uint32_t whom,
                       GraphNode start) {
    // std::cerr << runtime::NetworkInterface::ID << " putStart " << whom <<
    // "\n";
    graph->Starts[whom].first  = start;
    graph->Starts[whom].second = 2;
  }

  // blocking
  void fillStarts(uint32_t host) {
    if (Starts[host].second != 2) {
      int ex    = 0;
      bool swap = Starts[host].second.compare_exchange_strong(ex, 1);
      if (swap)
        runtime::getSystemNetworkInterface().sendAlt(
            host, getStart, self, runtime::NetworkInterface::ID);
      while (Starts[host].second != 2) {
        runtime::doNetworkWork();
      }
    }
    assert(Starts[host].first);
  }

  GraphNode generateNodePtr(unsigned x) {
    auto ii = std::upper_bound(PrefixNum.begin(), PrefixNum.end(), x);
    assert(ii != PrefixNum.end());
    unsigned host   = std::distance(PrefixNum.begin(), ii);
    unsigned offset = x - (host == 0 ? 0 : *(ii - 1));
    fillStarts(host);
    return Starts[host].first + offset;
  }

  void acquireNode(runtime::gptr<NodeImplTy> node, galois::MethodFlag mflag) {
    acquire(node, mflag);
  }

public:
  // This is technically a const (non-mutable) iterator
  class iterator
      : public std::iterator<std::random_access_iterator_tag, GraphNode,
                             ptrdiff_t, GraphNode, GraphNode> {
    LC_Dist* g;
    unsigned x;

    friend class LC_Dist;
    iterator(LC_Dist* _g, unsigned _x) : g(_g), x(_x) {}

  public:
    iterator() : g(nullptr), x(0) {}
    iterator(const iterator&) = default;
    bool operator==(const iterator& rhs) const {
      return g == rhs.g & x == rhs.x;
    }
    bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
    bool operator<(const iterator& rhs) const { return x < rhs.x; }
    bool operator>(const iterator& rhs) const { return x > rhs.x; }
    bool operator<=(const iterator& rhs) const { return x <= rhs.x; }
    bool operator>=(const iterator& rhs) const { return x >= rhs.x; }

    GraphNode operator*() { return g->generateNodePtr(x); }
    GraphNode operator->() { return g->generateNodePtr(x); }
    GraphNode operator[](int n) { return g->generateNodePtr(x + n); }

    iterator& operator++() {
      ++x;
      return *this;
    }
    iterator& operator--() {
      --x;
      return *this;
    }
    iterator operator++(int) {
      auto old = *this;
      ++x;
      return old;
    }
    iterator operator--(int) {
      auto old = *this;
      --x;
      return old;
    }
    iterator operator+(int n) {
      auto ret = *this;
      ret.x += n;
      return ret;
    }
    iterator operator-(int n) {
      auto ret = *this;
      ret.x -= n;
      return ret;
    }
    iterator& operator+=(int n) {
      x += n;
      return *this;
    }
    iterator& operator-=(int n) {
      x -= n;
      return *this;
    }
    ptrdiff_t operator-(const iterator& rhs) { return x - rhs.x; }
    // Trivially copyable
    typedef int tt_is_copyable;
  };

  typedef iterator local_iterator;
  typedef EdgeImplTy* edge_iterator;

  //! Creation and destruction
  typedef runtime::PerHost<LC_Dist> pointer;
  static pointer allocate(std::vector<unsigned>& edges) {
    return pointer::allocate(edges);
  }
  static void deallocate(pointer ptr) { pointer::deallocate(ptr); }

  //! Mutation

  template <typename... Args>
  edge_iterator addEdge(GraphNode src, GraphNode dst, const EdgeTy& data,
                        MethodFlag mflag = MethodFlag::ALL) {
    acquire(src, mflag);
    return src->append(dst, data);
  }

  edge_iterator addEdge(GraphNode src, GraphNode dst,
                        MethodFlag mflag = MethodFlag::ALL) {
    acquire(src, mflag);
    return src->append(dst);
  }

  //! Access

  NodeTy& at(GraphNode N, MethodFlag mflag = MethodFlag::ALL) {
    acquire(N, mflag);
    return N->data;
  }

  EdgeTy& at(edge_iterator E, MethodFlag mflag = MethodFlag::ALL) {
    return E->data;
  }

  GraphNode dst(edge_iterator E, MethodFlag mflag = MethodFlag::ALL) {
    return E->dst;
  }

  //! Capacity
  unsigned size() { return *PrefixNum.rbegin(); }

  //! Iterators

  iterator begin() { return iterator(this, 0); }
  iterator end() {
    return iterator(this, PrefixNum[runtime::NetworkInterface::Num - 1]);
  }

  local_iterator local_begin() {
    if (runtime::LL::getTID() == 0)
      return iterator(this, runtime::NetworkInterface::ID == 0
                                ? 0
                                : PrefixNum[runtime::NetworkInterface::ID - 1]);
    else
      return local_end();
  }
  local_iterator local_end() {
    return iterator(this, PrefixNum[runtime::NetworkInterface::ID]);
  }

  edge_iterator edge_begin(GraphNode N, MethodFlag mflag = MethodFlag::ALL) {
    acquire(N, mflag);
    if (mflag != MethodFlag::SRC_ONLY && mflag != MethodFlag::NONE)
      for (edge_iterator ii = N->begin(), ee = N->end(); ii != ee; ++ii) {
        acquireNode(ii->dst, mflag);
      }
    return N->begin();
  }

  edge_iterator edge_end(GraphNode N, MethodFlag mflag = MethodFlag::ALL) {
    acquireNode(N, mflag);
    return N->end();
  }

  template <typename Compare>
  void sort_edges(GraphNode N, Compare comp,
                  MethodFlag mflag = MethodFlag::ALL) {
    std::sort(edge_begin(N, mflag), edge_end(N, mflag),
              [&comp](const EdgeImplTy& e1, const EdgeImplTy& e2) {
                return comp(e1.dst, e1.data, e2.dst, e2.data);
              });
  }
};

} // namespace graphs
} // namespace galois

#endif
