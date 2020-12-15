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

#include "galois/Galois.h"
#include "galois/runtime/Serialize.h"

#include <iostream>
#include <vector>
#include <algorithm>

using namespace galois::runtime;

int main() {

  auto& net = galois::runtime::getSystemNetworkInterface();

  std::vector<std::vector<std::pair<uint64_t, uint64_t>>>
      assigned_edges_perhost;
  std::vector<std::vector<uint64_t>> assigned_edges_perhost_linear;

  assigned_edges_perhost.resize(net.Num);
  uint64_t n = {0};
  for (auto h = 0; h < net.Num; ++h) {
    assigned_edges_perhost[h].resize(1024 * 100);
    std::generate(assigned_edges_perhost[h].begin(),
                  assigned_edges_perhost[h].end(),
                  [&n] { return std::make_pair(n++, n); });

    std::cout << "[" << net.ID
              << "] Size : " << assigned_edges_perhost[h].size() << "\n";
  }

  assigned_edges_perhost_linear.resize(net.Num);
  for (auto h = 0; h < net.Num; ++h) {
    assigned_edges_perhost_linear[h].resize(2 * 1024 * 1024);
    std::generate(assigned_edges_perhost_linear[h].begin(),
                  assigned_edges_perhost_linear[h].end(), [&n] { return n++; });

    std::cout << "[" << net.ID
              << "] Size : " << assigned_edges_perhost_linear[h].size() << "\n";
  }

  for (unsigned x = 0; x < net.Num; ++x) {
    if (x == net.ID)
      continue;

    galois::runtime::SendBuffer b;
    std::cerr << "[" << net.ID << "]"
              << " serialize start : " << x << "\n";
    gSerialize(b, assigned_edges_perhost_linear[x]);
    std::cerr << "[" << net.ID << "]"
              << " serialize done : " << x << "\n";
    net.sendTagged(x, galois::runtime::evilPhase, b);
    assigned_edges_perhost_linear[x].clear();
    std::stringstream ss;
    ss << " sending from : " << net.ID << " to : " << x
       << " Size should b4 ZERO : " << assigned_edges_perhost_linear[x].size()
       << "\n";
    std::cout << ss.str() << "\n";
  }

  // receive
  for (unsigned x = 0; x < net.Num; ++x) {
    if (x == net.ID)
      continue;

    decltype(net.recieveTagged(galois::runtime::evilPhase, nullptr)) p;
    do {
      net.handleReceives();
      p = net.recieveTagged(galois::runtime::evilPhase, nullptr);
    } while (!p);

    galois::runtime::gDeserialize(p->second,
                                  assigned_edges_perhost_linear[p->first]);
    std::stringstream ss;
    ss << " received on : " << net.ID << " from : " << x
       << " Size : " << assigned_edges_perhost_linear[p->first].size() << "\n";
    std::cout << ss.str();
  }
  ++galois::runtime::evilPhase;
}
