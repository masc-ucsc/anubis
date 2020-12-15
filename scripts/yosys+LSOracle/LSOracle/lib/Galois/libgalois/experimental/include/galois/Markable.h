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

#ifndef GALOIS_UTIL_MARKED_H
#define GALOIS_UTIL_MARKED_H

#include <climits>

#include <boost/iterator/counting_iterator.hpp>

#include "galois/Galois.h"

#include "galois/runtime/DoAllCoupled.h"

namespace galois {

template <typename T>
struct Markable {
private:
  static const unsigned MAX_VAL = UINT_MAX;

  T m_val;
  unsigned m_ver;

public:
  explicit Markable(T val) : m_val(val), m_ver(MAX_VAL) {}

  void mark(unsigned v) {
    assert(v < MAX_VAL);
    m_ver = v;
  }

  bool marked() const { return (m_ver < MAX_VAL); }

  unsigned version() const { return m_ver; }

  T& get() { return m_val; }

  const T& get() const { return m_val; }

  operator T&() { return get(); }

  operator const T&() const { return get(); }
};

template <typename T>
struct IsNotMarked {
  bool operator()(const Markable<T>& x) const { return !x.marked(); }
};

template <typename WL>
struct RemoveMarked {

  WL& wl;

  RemoveMarked(WL& _wl) : wl(_wl) {}

  void operator()(unsigned r) {
    assert(r < wl.numRows());

    using T = typename WL::value_type;

    typename WL::local_iterator new_end =
        std::partition(wl[r].begin(), wl[r].end(), IsNotMarked<T>());

    wl[r].erase(new_end, wl[r].end());
  }
};

template <typename WL>
void removeMarked(WL& wl) {

  galois::do_all(galois::iterate(0, wl.numRows()), RemoveMarked<WL>(wl),
                 "remove_marked");
}

template <typename WL>
struct RemoveMarkedStable : public RemoveMarked<WL> {

  using Super_ty = RemoveMarked<WL>;

  RemoveMarkedStable(WL& _wl) : Super_ty(_wl) {}

  void operator()(unsigned r) {
    assert(r < Super_ty::wl.numRows());

    using T = typename WL::value_type;

    typename WL::local_iterator new_end = std::stable_partition(
        Super_ty::wl[r].begin(), Super_ty::wl[r].end(), IsNotMarked<T>());

    Super_ty::wl[r].erase(new_end, Super_ty::wl[r].end());
  }
};

template <typename WL>
void removeMarkedStable(WL& wl) {

  galois::do_all(galois::iterate(0, wl.numRows()), RemoveMarkedStable<WL>(wl),
                 "remove_marked_stable");
}

} // end namespace galois

#endif // GALOIS_UTIL_MARKED_H
