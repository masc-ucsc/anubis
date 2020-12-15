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

#include "galois/runtime/PerThreadStorage.h"
#include "galois/runtime/Termination.h"
#include "galois/runtime/Network.h"
#include "galois/runtime/ll/CompilerSpecific.h"

using namespace galois::runtime;

namespace {
class DistTerminationDetection : public TerminationDetection {
  struct TokenHolder {
    friend class TerminationDetection;
    volatile long tokenIsBlack;
    volatile long hasToken;
    long processIsBlack;
    bool lastWasWhite; // only used by the master
  };

  PerThreadStorage<TokenHolder> data;

  static void globalTermLandingPad(RecvBuffer&);
  static void propTokenLandingPad(RecvBuffer&);

  // send token onwards
  void propToken(bool isBlack) {
    unsigned id = LL::getTID();
    assert(id < activeThreads);
    if (id + 1 == activeThreads && NetworkInterface::Num > 1) {
      // remote
      // send message to networkHost + 1
      SendBuffer b;
      gSerialize(b, isBlack);
      getSystemNetworkInterface().send((NetworkInterface::ID + 1) %
                                           NetworkInterface::Num,
                                       propTokenLandingPad, b);
    } else {
      TokenHolder& th = *data.getRemote((id + 1) % activeThreads);
      th.tokenIsBlack = isBlack;
      LL::compilerBarrier();
      th.hasToken = true;
    }
  }

  // receive remote token
  void recvToken(bool isBlack) {
    TokenHolder& th = *data.getRemote(0);
    th.tokenIsBlack = isBlack;
    LL::compilerBarrier();
    th.hasToken = true;
  }

  void propGlobalTerm() {
    if (NetworkInterface::Num > 1) {
      SendBuffer b;
      getSystemNetworkInterface().broadcast(globalTermLandingPad, b);
    }
    globalTerm.data = true;
  }

  bool isSysMaster() const {
    return LL::getTID() == 0 && NetworkInterface::ID == 0;
  }

public:
  DistTerminationDetection() {}

  virtual void initializeThread() {
    TokenHolder& th   = *data.getLocal();
    th.hasToken       = false;
    th.tokenIsBlack   = false;
    th.processIsBlack = true;
    th.lastWasWhite   = true;
    globalTerm.data   = false;
    if (isSysMaster()) {
      th.hasToken = true;
    }
  }

  virtual void localTermination(bool workHappened) {
    assert(!(workHappened && globalTerm.data));
    TokenHolder& th = *data.getLocal();
    th.processIsBlack |= workHappened;
    if (th.hasToken) {
      if (isSysMaster()) {
        bool failed     = th.tokenIsBlack || th.processIsBlack;
        th.tokenIsBlack = th.processIsBlack = false;
        if (th.lastWasWhite && !failed) {
          // This was the second success
          propGlobalTerm();
          return;
        }
        th.lastWasWhite = !failed;
      }
      // Normal thread or recirc by master
      assert(!globalTerm.data &&
             "no token should be in progress after globalTerm");
      bool taint        = th.processIsBlack || th.tokenIsBlack;
      th.processIsBlack = th.tokenIsBlack = false;
      th.hasToken                         = false;
      propToken(taint);
    }
  }
};

static DistTerminationDetection& getDistTermination() {
  static DistTerminationDetection term;
  return term;
}

void DistTerminationDetection::globalTermLandingPad(RecvBuffer&) {
  getDistTermination().globalTerm.data = true;
}
void DistTerminationDetection::propTokenLandingPad(RecvBuffer& b) {
  bool isBlack;
  gDeserialize(b, isBlack);
  getDistTermination().recvToken(isBlack);
}

} // namespace

galois::runtime::TerminationDetection& galois::runtime::getSystemTermination() {
  return getDistTermination();
}
