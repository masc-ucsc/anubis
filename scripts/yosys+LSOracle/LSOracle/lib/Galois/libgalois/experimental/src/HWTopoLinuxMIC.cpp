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

#include "galois/substrate/HWTopo.h"
#include "galois/substrate/EnvCheck.h"
#include "galois/gIO.h"

#include <vector>
#include <set>
#include <map>
#include <functional>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <sched.h>

#define GALOIS_USE_MIC_TOPO

using namespace galois::substrate;

namespace {

static const char* sProcInfo = "/proc/cpuinfo";
static const char* sCPUSet   = "/proc/self/cpuset";

// TODO: store number of cores in each socket?
// and number of procs in each socket?

struct cpuinfo {
  // fields filled in from OS files
  unsigned proc;
  unsigned physid;
  unsigned sib;
  unsigned coreid;
  unsigned cpucores;

  // fields filled in by our assignment
  unsigned vpkgid;
  unsigned vcoreid;
  unsigned vtid;
  unsigned leader;
};

//! binds current thread to OS HW context "proc"
static bool bindToProcessor(unsigned proc) {
  cpu_set_t mask;
  /* CPU_ZERO initializes all the bits in the mask to zero. */
  CPU_ZERO(&mask);

  /* CPU_SET sets only the bit corresponding to cpu. */
  // void to cancel unused result warning
  (void)CPU_SET(proc, &mask);

  /* sched_setaffinity returns 0 in success */
  if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
    gWarn("Could not set CPU affinity for thread ", proc, "(", strerror(errno),
          ")");
    return false;
  }
  return true;
}

//! Parse /proc/cpuinfo
static std::vector<cpuinfo> parseCPUInfo() {
  std::vector<cpuinfo> vals;

  const int len = 1024;
  std::array<char, len> line;

  std::ifstream procInfo(sProcInfo);
  if (!procInfo)
    GALOIS_SYS_DIE("failed opening ", sProcInfo);

  int cur = -1;

  while (true) {
    procInfo.getline(line.data(), len);
    if (!procInfo)
      break;

    int num;
    if (sscanf(line.data(), "processor : %d", &num) == 1) {
      assert(cur < num);
      cur = num;
      vals.resize(cur + 1);
      vals.at(cur).proc = num;
    } else if (sscanf(line.data(), "physical id : %d", &num) == 1) {
      vals.at(cur).physid = num;
    } else if (sscanf(line.data(), "siblings : %d", &num) == 1) {
      vals.at(cur).sib = num;
    } else if (sscanf(line.data(), "core id : %d", &num) == 1) {
      vals.at(cur).coreid = num;
    } else if (sscanf(line.data(), "cpu cores : %d", &num) == 1) {
      vals.at(cur).cpucores = num;
    }
  }

  return vals;
}

//! Returns physical ids in current cpuset
static std::vector<int> parseCPUSet() {
  std::vector<int> vals;

  // Parse: /proc/self/cpuset
  std::string name;
  {
    std::ifstream cpuSetName(sCPUSet);
    if (!cpuSetName)
      return vals;

    // TODO: this will fail to read correctly if name contains newlines
    cpuSetName.getline(cpuSetName, name);
    if (!cpuSetName)
      return vals;
  }

  if (name.size() <= 1)
    return vals;

  // Parse: /dev/cpuset/<name>/cpus
  std::string path("/dev/cpuset");
  path += name;
  path += "/cpus";
  std::ifstream cpuSet(path);

  if (!cpuSet)
    return vals;

  std::string buffer;
  getline(cpuSet, buffer);
  if (!cpuSet)
    return vals;

  char* np = buffer.data();
  while (np && strlen(np)) {
    char* c = index(np, ',');
    if (c) { // slice string at comma (np is old string, c is next string)
      *c = '\0';
      ++c;
    }

    char* d = index(np, '-');
    if (d) { // range
      *d = '\0';
      ++d;
      unsigned b = atoi(np);
      unsigned e = atoi(d);
      while (b <= e)
        vals.push_back(b++);
    } else { // singleton
      vals.push_back(atoi(np));
    }
    np = c;
  }

  return vals;
}

class HWTopoLinux : public HWTopo {

  typedef std::vector<unsigned> VecNum;
  typedef std::set<unsigned> SetNum;
  typedef std::vector<VecNum> VecVecNum;
  typedef std::vector<VecVecNum> VecVecVecNum;
  typedef std::map<unsigned, unsigned> MapNum_Num;
  typedef std::map<unsigned, SetNum> MapNum_SetNum;

  typedef std::vector<cpuinfo*> VecPtr;
  typedef std::vector<VecPtr> VecVecPtr;
  typedef std::vector<VecVecPtr> VecVecVecPtr;

  struct OrderByPkg : public std::binary_function<unsigned, unsigned, bool> {
    bool operator()(const cpuinfo* a, const cpuinfo* b) const {
      assert(a != nullptr);
      assert(b != nullptr);
      return (a->physid < b->physid);
    }
  };

  struct OrderByCore : public std::binary_function<unsigned, unsigned, bool> {
    bool operator()(const cpuinfo* a, const cpuinfo* b) const {
      assert(a != nullptr);
      assert(b != nullptr);
      return (a->coreid < b->coreid);
    }
  };

  struct OrderByProc : public std::binary_function<unsigned, unsigned, bool> {
    bool operator()(const cpuinfo* a, const cpuinfo* b) const {
      assert(a != nullptr);
      assert(b != nullptr);
      return (a->proc < b->proc);
    }
  };

  struct OrderByTID : public std::binary_function<unsigned, unsigned, bool> {
    bool operator()(const cpuinfo* a, const cpuinfo* b) const {
      assert(a != nullptr);
      assert(b != nullptr);
      return (a->vtid < b->vtid);
    }
  };

  unsigned numThreadsRaw;
  unsigned numCoresRaw;
  unsigned numSocketsRaw;

  unsigned numThreads;
  unsigned numCores;
  unsigned numSockets;

  std::vector<unsigned> processorMap;
  std::vector<unsigned> coreMap;
  std::vector<unsigned> socketMap;
  std::vector<unsigned> leaderMapThread;

  std::vector<unsigned> maxSocketMap;
  std::vector<unsigned> leaderMapSocket;

  Policy() {
    std::vector<cpuinfo> rawInfo = parseCPUInfo();

    // for MIC only
#ifdef GALOIS_USE_MIC_TOPO
    for (auto i = rawInfoVec.begin(), endi = rawInfoVec.end(); i != endi; ++i) {
      i->physid = i->coreid;
    }
#endif
    std::vector<unsigned> enabledSet;
    parseCPUSet(enabledSet);

    if (EnvCheck("GALOIS_DEBUG_TOPO"))
      printRawConfiguration(rawInfo, enabledSet);

    computeSizes(rawInfoVec, numSocketsRaw, numCoresRaw, numThreadsRaw);

    std::vector<cpuinfo> activeSet;
    std::vector<cpuinfo>* infovec = nullptr;

    if (enabledSet.empty()) {
      infovec    = &rawInfoVec;
      numSockets = numSocketsRaw;
      numCores   = numCoresRaw;
      numThreads = numThreadsRaw;
    } else {
      infovec = &activeSet;

      // XXX: Following code assumes that the Proc IDs are contiguous in
      // [0..numProcs) but not necessarily in that order
      std::vector<bool> enabledFlagVec(rawInfoVec.size(), false);
      for (auto i = enabledSet.cbegin(), endi = enabledSet.cend(); i != endi;
           ++i) {
        enabledFlagVec[*i] = true;
      }

      for (auto i = rawInfoVec.cbegin(), endi = rawInfoVec.cend(); i != endi;
           ++i) {
        if (enabledFlagVec[i->proc]) {
          activeSet.push_back(*i);
        }
      }

      // compute the actual sizes based on activeSet
      computeSizes(activeSet, numSockets, numCores, numThreads);
    }

    assert(infovec != nullptr);
    computeForwardMap(*infovec, numThreads);

    computeReverseMap(*infovec);

    if (EnvCheck("GALOIS_DEBUG_TOPO"))
      printFinalConfiguration();
  }

  static void computeSizes(const std::vector<cpuinfo>& infovec, unsigned& numP,
                           unsigned& numC, unsigned& numT) {
    numT = infovec.size();
    MapNum_SetNum pkg_core_sets;

    for (auto i = infovec.cbegin(), endi = infovec.cend(); i != endi; ++i) {
      if (pkg_core_sets.count(i->physid) == 0) {
        pkg_core_sets.insert(std::make_pair(i->physid, SetNum()));
      }
      pkg_core_sets[i->physid].insert(i->coreid);
    }

    numP = pkg_core_sets.size();
    numC = 0;

    for (auto s = pkg_core_sets.cbegin(), ends = pkg_core_sets.cend();
         s != ends; ++s) {
      assert(s->second.size() > 0);
      numC += s->second.size();
    }
  }

  static void computeForwardMap(std::vector<cpuinfo>& infovec,
                                const unsigned numThreads) {
    VecPtr infoptrs(infovec.size());

    for (unsigned i = 0; i < infovec.size(); ++i) {
      infoptrs[i] = &infovec[i];
    }

    // calls to sort here are not necessary, they just ensure that lower
    // physical id gets lower virtual id
    std::sort(infoptrs.begin(), infoptrs.end(), OrderByPkg());

    MapNum_Num pkg_id_map;
    unsigned pkg_id_cntr = 0;
    for (auto i = infoptrs.cbegin(), endi = infoptrs.cend(); i != endi; ++i) {
      if (pkg_id_map.count((*i)->physid) == 0) {
        pkg_id_map.insert(std::make_pair((*i)->physid, pkg_id_cntr++));
      }
    }

    VecVecPtr pkg_grps(pkg_id_map.size());
    for (auto i = infoptrs.begin(), endi = infoptrs.end(); i != endi; ++i) {
      assert(pkg_id_map.count((*i)->physid) > 0);
      (*i)->vpkgid = pkg_id_map[(*i)->physid];
      pkg_grps[(*i)->vpkgid].push_back(*i);
    }

    // for MIC only
#ifdef GALOIS_USE_MIC_TOPO
    // for (auto p = pkg_grps.begin(), endp = pkg_grps.end(); p != endp; ++p) {
    //
    // unsigned core_cntr = 0;
    // for (auto i = p->begin(), endi = p->end(); i != endi; ++i) {
    // (*i)->coreid = core_cntr++;
    // }
    // }
#endif

    VecVecVecPtr pkg_core_config;
    pkg_core_config.resize(pkg_grps.size());

    for (unsigned p = 0; p < pkg_grps.size(); ++p) {
      std::sort(pkg_grps[p].begin(), pkg_grps[p].end(), OrderByCore());

      MapNum_Num core_id_map;
      unsigned core_id_cntr = 0;

      for (auto i = pkg_grps[p].begin(), endi = pkg_grps[p].end(); i != endi;
           ++i) {
        if (core_id_map.count((*i)->coreid) == 0) {
          core_id_map.insert(std::make_pair((*i)->coreid, core_id_cntr++));
        }
      }

      pkg_core_config[p].resize(core_id_map.size());
      VecVecPtr core_grps(core_id_map.size());
      for (auto i = pkg_grps[p].begin(), endi = pkg_grps[p].end(); i != endi;
           ++i) {
        assert(core_id_map.count((*i)->coreid) > 0);
        (*i)->vcoreid = core_id_map[(*i)->coreid];
        core_grps[(*i)->vcoreid].push_back(*i);
      }

      for (unsigned c = 0; c < core_grps.size(); ++c) {
        std::sort(core_grps[c].begin(), core_grps[c].end(), OrderByProc());
        // pushed in reverse order to make popping more efficient (needed
        // afterwards)
        for (auto i = core_grps[c].rbegin(), endi = core_grps[c].rend();
             i != endi; ++i) {
          pkg_core_config[p][c].push_back(*i);
        }
      }
    }

    for (unsigned vtid_cntr = 0; vtid_cntr < numThreads;) {
      for (unsigned p = 0; p < pkg_core_config.size(); ++p) {
        for (unsigned c = 0; c < pkg_core_config[p].size(); ++c) {
// for MIC only
#ifdef GALOIS_USE_MIC_TOPO
          const unsigned IDEAL_THREADS_PER_CORE = 2;
          for (unsigned _k = 0; _k < IDEAL_THREADS_PER_CORE; ++_k) {
            if (!pkg_core_config[p][c].empty()) {

              cpuinfo* i = pkg_core_config[p][c].back();
              pkg_core_config[p][c].pop_back();
              i->vtid = vtid_cntr++;
            }
          }
#else
          if (!pkg_core_config[p][c].empty()) {
            cpuinfo* i = pkg_core_config[p][c].back();
            pkg_core_config[p][c].pop_back();
            i->vtid = vtid_cntr++;
          }
#endif
        }
      }
    }

    // find and assign leader threads for sockets
    for (unsigned p = 0; p < pkg_grps.size(); ++p) {
      // order by vtid;
      std::sort(pkg_grps[p].begin(), pkg_grps[p].end(), OrderByTID());
      unsigned leader = pkg_grps[p][0]->vtid;
      for (auto i = pkg_grps[p].begin(), endi = pkg_grps[p].end(); i != endi;
           ++i) {
        (*i)->leader = leader;
      }
    }
  }

  void computeReverseMap(const std::vector<cpuinfo>& infovec) {
    // now compute reverse mappings
    processorMap.clear();
    coreMap.clear();
    socketMap.clear();
    leaderMapThread.clear();
    maxSocketMap.clear();

    processorMap.resize(numThreads);
    coreMap.resize(numThreads);
    socketMap.resize(numThreads);
    leaderMapThread.resize(numThreads);
    maxSocketMap.resize(numThreads);

    leaderMapSocket.resize(numSockets);
    for (auto i = infovec.cbegin(), endi = infovec.cend(); i != endi; ++i) {
      processorMap[i->vtid]    = i->proc;
      coreMap[i->vtid]         = i->vcoreid;
      socketMap[i->vtid]       = i->vpkgid;
      leaderMapThread[i->vtid] = i->leader;
    }

    unsigned mp = 0;
    for (unsigned i = 0; i < socketMap.size(); ++i) {
      mp              = std::max(socketMap[i], mp);
      maxSocketMap[i] = mp;
    }

    for (unsigned i = 0; i < leaderMapThread.size(); ++i) {
      leaderMapSocket[socketMap[i]] = leaderMapThread[i];
    }
  }

  static void printRawConfiguration(const std::vector<cpuinfo>& vals,
                                    const std::vector<unsigned>& enabledSet) {
    for (unsigned i = 0; i < vals.size(); ++i) {
      const cpuinfo& p = vals[i];
      gPrint("proc ", p.proc, ", physid ", p.physid, ", sib ", p.sib,
             ", coreid ", p.coreid, ", cpucores ", p.cpucores, "\n");
    }
    gPrint("enabled set: ");
    for (unsigned i = 0; i < enabledSet.size(); ++i)
      gPrint(", ", enabledSet[i]);
    gPrint("\n");
  }

  void printFinalConfiguration() const {
    // DEBUG: PRINT Stuff
    gPrint("Threads: ", numThreads, ", ", numThreadsRaw, " (raw)\n");
    gPrint("Cores: ", numCores, ", ", numCoresRaw, " (raw)\n");
    gPrint("Sockets: ", numSockets, ", ", numSocketsRaw, " (raw)\n");

    for (unsigned i = 0; i < numThreads; ++i) {
      gPrint("T ", i, " Proc ", processorMap[i], " Pkg ", socketMap[i],
             " Core ", coreMap[i], " L? ", (i == leaderMapThread[i] ? 1 : 0));

      if (i >= numCores) {
        gPrint(" HT");
      }
      gPrint("\n");
    }
  }
};

static Policy& getPolicy() {
  static Policy A;
  return A;
}

} // namespace

bool galois::runtime::LL::bindThreadToProcessor(int id) {
  assert(size_t(id) < getPolicy().numThreads);
  return linuxBindToProcessor(getPolicy().processorMap[id]);
}

unsigned galois::runtime::LL::getProcessorForThread(int id) {
  assert(size_t(id) < getPolicy().numThreads);
  return getPolicy().processorMap[id];
}

unsigned galois::runtime::LL::getMaxThreads() { return getPolicy().numThreads; }

unsigned galois::runtime::LL::getMaxCores() { return getPolicy().numCores; }

unsigned galois::runtime::LL::getMaxSockets() { return getPolicy().numSockets; }

unsigned galois::runtime::LL::getSocketForThread(int id) {
  assert(size_t(id) < getPolicy().numThreads);
  return getPolicy().socketMap[id];
}

unsigned galois::runtime::LL::getMaxSocketForThread(int id) {
  assert(size_t(id) < getPolicy().numThreads);
  return getPolicy().maxSocketMap[id];
}

bool galois::runtime::LL::isSocketLeader(int id) {
  assert(size_t(id) < getPolicy().numThreads);
  return getPolicy().leaderMapThread[id] == id;
}

unsigned galois::runtime::LL::getLeaderForThread(int id) {
  assert(size_t(id) < getPolicy().numThreads);
  return getPolicy().leaderMapThread[id];
}

unsigned galois::runtime::LL::getLeaderForSocket(int id) {
  assert(size_t(id) < getPolicy().numSockets);
  return getPolicy().leaderMapSocket[id];
}
