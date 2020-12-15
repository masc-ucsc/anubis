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
#include <functional>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <sched.h>

using namespace galois::substrate;

namespace {

struct cpuinfo {
  int proc;
  int physid;
  int sib;
  int coreid;
  int cpucores;
};

static const char* sProcInfo = "/proc/cpuinfo";
static const char* sCPUSet   = "/proc/self/cpuset";

static bool linuxBindToProcessor(int proc) {
  // cpu_set_t mask;
  // /* CPU_ZERO initializes all the bits in the mask to zero. */
  // CPU_ZERO( &mask );

  // /* CPU_SET sets only the bit corresponding to cpu. */
  // // void to cancel unused result warning
  // (void)CPU_SET( proc, &mask );

  // /* sched_setaffinity returns 0 in success */
  // if( sched_setaffinity( 0, sizeof(mask), &mask ) == -1 ) {
  //   gWarn("Could not set CPU affinity for thread ", proc, "(",
  //   strerror(errno), ")"); return false;
  // }
  return false;
}

//! Parse /proc/cpuinfo
static std::vector<cpuinfo> parseCPUInfo() {
  std::vector<cpuinfo> vals;
  vals.reserve(64);

  FILE* f = fopen(sProcInfo, "r");
  if (!f) {
    GALOIS_SYS_DIE("failed opening ", sProcInfo);
    return vals; // Shouldn't get here
  }

  const int len = 1024;
  char* line    = (char*)malloc(len);
  int cur       = -1;

  while (fgets(line, len, f)) {
    int num;
    if (sscanf(line, "processor : %d", &num) == 1) {
      assert(cur < num);
      cur = num;
      vals.resize(cur + 1);
      vals.at(cur).proc = num;
    } else if (sscanf(line, "physical id : %d", &num) == 1) {
      vals.at(cur).physid = num;
    } else if (sscanf(line, "siblings : %d", &num) == 1) {
      vals.at(cur).sib = num;
    } else if (sscanf(line, "core id : %d", &num) == 1) {
      vals.at(cur).coreid = num;
    } else if (sscanf(line, "cpu cores : %d", &num) == 1) {
      vals.at(cur).cpucores = num;
    }
  }

  free(line);
  fclose(f);

  return vals;
}

//! Returns physical ids in current cpuset
std::vector<int> parseCPUSet() {
  std::vector<int> vals;
  vals.reserve(64);

  // PARSE: /proc/self/cpuset
  FILE* f = fopen(sCPUSet, "r");
  if (!f) {
    return vals;
  }

  const int len = 1024;
  char* path    = (char*)malloc(len);
  path[0]       = '/';
  path[1]       = '\0';
  if (!fgets(path, len, f)) {
    fclose(f);
    return vals;
  }
  fclose(f);

  if (char* t = index(path, '\n'))
    *t = '\0';

  if (strlen(path) == 1) {
    free(path);
    return vals;
  }

  char* path2 = (char*)malloc(len);
  strcpy(path2, "/dev/cpuset");
  strcat(path2, path);
  strcat(path2, "/cpus");

  f = fopen(path2, "r");
  if (!f) {
    free(path2);
    free(path);
    GALOIS_SYS_DIE("failed opening ", path2);
    return vals; // Shouldn't get here
  }

  // reuse path
  char* np = path;
  if (!fgets(np, len, f)) {
    fclose(f);
    return vals;
  }
  while (np && strlen(np)) {
    char* c = index(np, ',');
    if (c) { // slice string at comma (np is old string, c is next string
      *c = '\0';
      ++c;
    }

    char* d = index(np, '-');
    if (d) { // range
      *d = '\0';
      ++d;
      int b = atoi(np);
      int e = atoi(d);
      while (b <= e)
        vals.push_back(b++);
    } else { // singleton
      vals.push_back(atoi(np));
    }
    np = c;
  };

  fclose(f);
  free(path2);
  free(path);
  return vals;
}

struct AutoLinuxPolicy {
  // number of hw supported threads
  unsigned numThreads, numThreadsRaw;

  // number of "real" processors
  unsigned numCores, numCoresRaw;

  // number of sockets
  unsigned numSockets, numSocketsRaw;

  std::vector<int> sockets;
  std::vector<int> maxSocket;
  std::vector<int> virtmap;
  std::vector<int> leaders;

  //! Sort in socket-dense manner
  struct DenseSocketLessThan : public std::binary_function<int, int, bool> {
    const std::vector<cpuinfo>& vals;
    DenseSocketLessThan(const std::vector<cpuinfo>& v) : vals(v) {}
    bool operator()(int a, int b) const {
      if (vals[a].physid < vals[b].physid) {
        return true;
      } else if (vals[a].physid == vals[b].physid) {
        if (vals[a].coreid < vals[b].coreid) {
          return true;
        } else if (vals[a].coreid == vals[b].coreid) {
          return vals[a].proc < vals[b].proc;
        } else {
          return false;
        }
      } else {
        return false;
      }
    }
  };

  struct DenseSocketEqual : public std::binary_function<int, int, bool> {
    const std::vector<cpuinfo>& vals;
    DenseSocketEqual(const std::vector<cpuinfo>& v) : vals(v) {}
    bool operator()(int a, int b) const {
      return vals[a].physid == vals[b].physid &&
             vals[a].coreid == vals[b].coreid;
    }
  };

  AutoLinuxPolicy() {
    std::vector<cpuinfo> vals = parseCPUInfo();
    virtmap                   = parseCPUSet();

    if (virtmap.empty()) {
      // 1-1 mapping for non-cpuset using systems
      for (unsigned i = 0; i < vals.size(); ++i)
        virtmap.push_back(i);
    }

    if (EnvCheck("GALOIS_DEBUG_TOPO"))
      printRawConfiguration(vals);

    // Get thread count
    numThreadsRaw = vals.size();
    numThreads    = virtmap.size();

    // Get socket level stuff
    int maxrawsocket = generateRawSocketData(vals);
    generateSocketData(vals);

    // Sort by socket to get socket-dense mapping
    std::sort(virtmap.begin(), virtmap.end(), DenseSocketLessThan(vals));
    generateHyperthreads(vals);

    // Finally renumber for virtual processor numbers
    finalizeSocketData(vals, maxrawsocket);

    // Get core count
    numCores = generateCoreData(vals);

    // Compute cummulative max socket
    int p = 0;
    maxSocket.resize(sockets.size());
    for (int i = 0; i < (int)sockets.size(); ++i) {
      p            = std::max(sockets[i], p);
      maxSocket[i] = p;
    }

    // Compute first thread in socket
    leaders.resize(numSockets, -1);
    for (int i = 0; i < (int)sockets.size(); ++i)
      if (leaders[sockets[i]] == -1)
        leaders[sockets[i]] = i;

    if (EnvCheck("GALOIS_DEBUG_TOPO"))
      printFinalConfiguration();
  }

  void printRawConfiguration(const std::vector<cpuinfo>& vals) {
    for (unsigned i = 0; i < vals.size(); ++i) {
      const cpuinfo& p = vals[i];
      gPrint("proc ", p.proc, ", physid ", p.physid, ", sib ", p.sib,
             ", coreid ", p.coreid, ", cpucores ", p.cpucores, "\n");
    }
    for (unsigned i = 0; i < virtmap.size(); ++i)
      gPrint(", ", virtmap[i]);
    gPrint("\n");
  }

  void printFinalConfiguration() {
    // DEBUG: PRINT Stuff
    gPrint("Threads: ", numThreads, ", ", numThreadsRaw, " (raw)\n");
    gPrint("Cores: ", numCores, ", ", numCoresRaw, " (raw)\n");
    gPrint("Sockets: ", numSockets, ", ", numSocketsRaw, " (raw)\n");

    for (unsigned i = 0; i < virtmap.size(); ++i) {
      gPrint("T ", i, " P ", sockets[i], " Tr ", virtmap[i], " L? ",
             ((int)i == leaders[sockets[i]] ? 1 : 0));
      if (i >= numCores)
        gPrint(" HT");
      gPrint("\n");
    }
  }

  void finalizeSocketData(const std::vector<cpuinfo>& vals, int maxrawsocket) {
    std::vector<int> mapping(maxrawsocket + 1);
    int nextval = 1;
    for (int i = 0; i < (int)virtmap.size(); ++i) {
      int x = vals[virtmap[i]].physid;
      if (!mapping[x])
        mapping[x] = nextval++;
      sockets.push_back(mapping[x] - 1);
    }
  }

  unsigned generateCoreData(const std::vector<cpuinfo>& vals) {
    std::vector<std::pair<int, int>> cores;
    // first get the raw numbers
    for (unsigned i = 0; i < vals.size(); ++i)
      cores.push_back(std::make_pair(vals[i].physid, vals[i].coreid));
    std::sort(cores.begin(), cores.end());
    std::vector<std::pair<int, int>>::iterator it =
        std::unique(cores.begin(), cores.end());
    numCoresRaw = std::distance(cores.begin(), it);
    cores.clear();

    for (unsigned i = 0; i < virtmap.size(); ++i)
      cores.push_back(
          std::make_pair(vals[virtmap[i]].physid, vals[virtmap[i]].coreid));
    std::sort(cores.begin(), cores.end());
    it = std::unique(cores.begin(), cores.end());
    return std::distance(cores.begin(), it);
  }

  void generateHyperthreads(const std::vector<cpuinfo>& vals) {
    // Find duplicates, which are hyperthreads, and place them at the end
    // annoyingly, values after tempi are unspecified for std::unique, so copy
    // in and out instead
    std::vector<int> dense(numThreads);
    std::vector<int>::iterator it = std::unique_copy(
        virtmap.begin(), virtmap.end(), dense.begin(), DenseSocketEqual(vals));
    std::vector<bool> mask(numThreadsRaw);
    for (std::vector<int>::iterator ii = dense.begin(); ii < it; ++ii)
      mask[*ii] = true;
    for (std::vector<int>::iterator ii = virtmap.begin(), ei = virtmap.end();
         ii < ei; ++ii) {
      if (!mask[*ii])
        *it++ = *ii;
    }
    virtmap = dense;
  }

  void generateSocketData(const std::vector<cpuinfo>& vals) {
    std::vector<int> p;
    for (unsigned i = 0; i < virtmap.size(); ++i)
      p.push_back(vals[virtmap[i]].physid);
    std::sort(p.begin(), p.end());
    std::vector<int>::iterator it = std::unique(p.begin(), p.end());
    numSockets                    = std::distance(p.begin(), it);
  }

  int generateRawSocketData(const std::vector<cpuinfo>& vals) {
    std::vector<int> p;
    for (unsigned i = 0; i < vals.size(); ++i)
      p.push_back(vals[i].physid);

    int retval = *std::max_element(p.begin(), p.end());
    std::sort(p.begin(), p.end());
    std::vector<int>::iterator it = std::unique(p.begin(), p.end());
    numSocketsRaw                 = std::distance(p.begin(), it);
    return retval;
  }
};

AutoLinuxPolicy& getPolicy() {
  static AutoLinuxPolicy A;
  return A;
}

} // namespace

bool galois::runtime::LL::bindThreadToProcessor(int id) {
  assert(size_t(id) < getPolicy().virtmap.size());
  return linuxBindToProcessor(getPolicy().virtmap[id]);
}

unsigned galois::runtime::LL::getProcessorForThread(int id) {
  assert(size_t(id) < getPolicy().virtmap.size());
  return getPolicy().virtmap[id];
}

unsigned galois::runtime::LL::getMaxThreads() { return getPolicy().numThreads; }

unsigned galois::runtime::LL::getMaxCores() { return getPolicy().numCores; }

unsigned galois::runtime::LL::getMaxSockets() { return getPolicy().numSockets; }

unsigned galois::runtime::LL::getSocketForThread(int id) {
  assert(size_t(id) < getPolicy().sockets.size());
  return getPolicy().sockets[id];
}

unsigned galois::runtime::LL::getMaxSocketForThread(int id) {
  assert(size_t(id) < getPolicy().maxSocket.size());
  return getPolicy().maxSocket[id];
}

bool galois::runtime::LL::isSocketLeader(int id) {
  assert(size_t(id) < getPolicy().sockets.size());
  return getPolicy().leaders[getPolicy().sockets[id]] == id;
}

unsigned galois::runtime::LL::getLeaderForThread(int id) {
  assert(size_t(id) < getPolicy().sockets.size());
  return getPolicy().leaders[getPolicy().sockets[id]];
}

unsigned galois::runtime::LL::getLeaderForSocket(int id) {
  assert(size_t(id) < getPolicy().leaders.size());
  return getPolicy().leaders[id];
}
