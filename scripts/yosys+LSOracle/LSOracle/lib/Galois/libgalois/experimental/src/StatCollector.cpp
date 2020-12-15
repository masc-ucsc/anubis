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

#include "galois/runtime/StatCollector.h"

#include <cmath>
#include <new>
#include <map>
#include <mutex>
#include <numeric>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

namespace galois {
namespace runtime {
extern unsigned activeThreads;
}
} // namespace galois

using namespace galois;
using namespace galois::runtime;

static galois::runtime::StatCollector* SC;

void galois::runtime::internal::setStatCollector(
    galois::runtime::StatCollector* sc) {
  GALOIS_ASSERT(!(SC && sc), "StatCollector.cpp: Double Initialization of SC");
  SC = sc;
}

StatCollector::StatCollector(const std::string& outfile) : m_outfile(outfile) {}

StatCollector::~StatCollector() {}

const std::string*
galois::runtime::StatCollector::getSymbol(const std::string& str) const {
  auto ii = symbols.find(str);
  if (ii == symbols.cend())
    return nullptr;
  return &*ii;
}

const std::string*
galois::runtime::StatCollector::getOrInsertSymbol(const std::string& str) {
  auto ii = symbols.insert(str);
  return &*ii.first;
}

unsigned
galois::runtime::StatCollector::getInstanceNum(const std::string& str) const {
  auto s = getSymbol(str);
  if (!s)
    return 0;
  auto ii =
      std::lower_bound(loopInstances.begin(), loopInstances.end(), s,
                       [](const StringPair<unsigned>& s1,
                          const std::string* s2) { return s1.first < s2; });
  if (ii == loopInstances.end() || s != ii->first)
    return 0;
  return ii->second;
}

void galois::runtime::StatCollector::addInstanceNum(const std::string& str) {
  auto s = getOrInsertSymbol(str);
  auto ii =
      std::lower_bound(loopInstances.begin(), loopInstances.end(), s,
                       [](const StringPair<unsigned>& s1,
                          const std::string* s2) { return s1.first < s2; });
  if (ii == loopInstances.end() || s != ii->first) {
    loopInstances.emplace_back(s, 0);
    std::sort(
        loopInstances.begin(), loopInstances.end(),
        [](const StringPair<unsigned>& s1, const StringPair<unsigned>& s2) {
          return s1.first < s2.first;
        });
  } else {
    ++ii->second;
  }
}

// using galois::runtime::StatCollector::RecordTy;

galois::runtime::StatCollector::RecordTy::RecordTy(size_t value)
    : mode(StatCollector::RecordTy::INT), valueInt(value) {}

galois::runtime::StatCollector::RecordTy::RecordTy(double value)
    : mode(StatCollector::RecordTy::DOUBLE), valueDouble(value) {}

galois::runtime::StatCollector::RecordTy::RecordTy(const std::string& value)
    : mode(StatCollector::RecordTy::STR), valueStr(value) {}

size_t StatCollector::RecordTy::intVal(void) const { return valueInt; }
double StatCollector::RecordTy::doubleVal(void) const { return valueDouble; }
const std::string& StatCollector::RecordTy::strVal(void) const {
  return valueStr;
}

galois::runtime::StatCollector::RecordTy::~RecordTy() {
  using string_type = std::string;
  if (mode == RecordTy::STR)
    valueStr.~string_type();
}

galois::runtime::StatCollector::RecordTy::RecordTy(const RecordTy& r)
    : mode(r.mode) {
  switch (mode) {
  case RecordTy::INT:
    valueInt = r.valueInt;
    break;
  case RecordTy::DOUBLE:
    valueDouble = r.valueDouble;
    break;
  case RecordTy::STR:
    new (&valueStr) std::string(r.valueStr);
    break;
  }
}

void galois::runtime::StatCollector::RecordTy::print(std::ostream& out) const {
  switch (mode) {
  case RecordTy::INT:
    out << valueInt;
    break;
  case RecordTy::DOUBLE:
    out << valueDouble;
    break;
  case RecordTy::STR:
    out << valueStr;
    break;
  }
}

void galois::runtime::StatCollector::addToStat(const std::string& loop,
                                               const std::string& category,
                                               size_t value, unsigned TID,
                                               unsigned HostID) {
  MAKE_LOCK_GUARD(StatsLock);
  auto tpl = std::make_tuple(HostID, TID, getOrInsertSymbol(loop),
                             getOrInsertSymbol(category), getInstanceNum(loop));
  auto iip = Stats.insert(std::make_pair(tpl, RecordTy(value)));
  if (iip.second == false) {
    assert(iip.first->second.mode == RecordTy::INT);
    iip.first->second.valueInt += value;
  }
}

void galois::runtime::StatCollector::addToStat(const std::string& loop,
                                               const std::string& category,
                                               double value, unsigned TID,
                                               unsigned HostID) {
  MAKE_LOCK_GUARD(StatsLock);
  auto tpl = std::make_tuple(HostID, TID, getOrInsertSymbol(loop),
                             getOrInsertSymbol(category), getInstanceNum(loop));
  auto iip = Stats.insert(std::make_pair(tpl, RecordTy(value)));
  if (iip.second == false) {
    assert(iip.first->second.mode == RecordTy::DOUBLE);
    iip.first->second.valueDouble += value;
  }
}

void galois::runtime::StatCollector::addToStat(const std::string& loop,
                                               const std::string& category,
                                               const std::string& value,
                                               unsigned TID, unsigned HostID) {
  MAKE_LOCK_GUARD(StatsLock);
  auto tpl = std::make_tuple(HostID, TID, getOrInsertSymbol(loop),
                             getOrInsertSymbol(category), getInstanceNum(loop));
  auto iip = Stats.insert(std::make_pair(tpl, RecordTy(value)));
  if (iip.second == false) {
    assert(iip.first->second.mode == RecordTy::STR);
    iip.first->second.valueStr = value;
  }
}

boost::uuids::uuid galois::runtime::StatCollector::UUID;
boost::uuids::uuid galois::runtime::StatCollector::getUUID() {
  if (UUID.is_nil()) {
    boost::uuids::random_generator generator;
    UUID = generator();
    return UUID;
  } else {
    return UUID;
  }
}
// assumne called serially
void galois::runtime::StatCollector::printStatsForR(std::ostream& out,
                                                    bool json) {
  if (json)
    out << "[\n";
  else
    out << "UUID,LOOP,INSTANCE,CATEGORY,THREAD,HOST,VAL\n";
  MAKE_LOCK_GUARD(StatsLock);
  for (auto& p : Stats) {
    if (json)
      out << "{\"UUID\" : " << getUUID()
          << ", \"LOOP\" : " << *std::get<2>(p.first)
          << " , \"INSTANCE\" : " << std::get<4>(p.first)
          << " , \"CATEGORY\" : " << *std::get<3>(p.first)
          << " , \"HOST\" : " << std::get<0>(p.first)
          << " , \"THREAD\" : " << std::get<1>(p.first) << " , \"VALUE\" : ";
    else
      out << getUUID() << "," << *std::get<2>(p.first) << ","
          << std::get<4>(p.first) << " , " << *std::get<3>(p.first) << ","
          << std::get<0>(p.first) << "," << std::get<1>(p.first) << ",";
    p.second.print(out);
    out << (json ? "}\n" : "\n");
  }
  if (json)
    out << "]\n";
}

void galois::runtime::StatCollector::printStats(void) {
  if (m_outfile == "") {
    printStatsForR(std::cout, false);

  } else {
    std::ofstream outf(m_outfile);
    if (outf.good()) {
      printStatsForR(outf, false);
    } else {
      gWarn("Could not open stats file for writing, file provided:", m_outfile);
      printStats(std::cerr);
    }
  }
}

// Assume called serially
// still assumes int values
// This ignores HostID.  Thus this is not safe for the network version
void galois::runtime::StatCollector::printStats(std::ostream& out) {
  std::map<std::tuple<const std::string*, unsigned, const std::string*>,
           std::vector<size_t>>
      LKs;
  unsigned maxThreadID = 0;
  // Find all loops and keys
  MAKE_LOCK_GUARD(StatsLock);
  for (auto& p : Stats) {
    auto& v  = LKs[std::make_tuple(std::get<2>(p.first), std::get<4>(p.first),
                                  std::get<3>(p.first))];
    auto tid = std::get<1>(p.first);
    maxThreadID = std::max(maxThreadID, tid);
    if (v.size() <= tid)
      v.resize(tid + 1);
    v[tid] += p.second.valueInt;
  }
  // print header
  out << "STATTYPE,LOOP,INSTANCE,CATEGORY,n,sum";
  for (unsigned x = 0; x <= maxThreadID; ++x)
    out << ",T" << x;
  out << "\n";
  // print all values
  for (auto ii = LKs.begin(), ee = LKs.end(); ii != ee; ++ii) {
    auto& Values = ii->second;
    out << "STAT," << std::get<0>(ii->first)->c_str() << ","
        << std::get<1>(ii->first) << "," << std::get<2>(ii->first)->c_str()
        << "," << maxThreadID + 1 << ","
        << std::accumulate(Values.begin(), Values.end(),
                           static_cast<unsigned long>(0));
    for (unsigned x = 0; x <= maxThreadID; ++x)
      out << "," << (x < Values.size() ? Values.at(x) : 0);
    out << "\n";
  }
}

void galois::runtime::StatCollector::beginLoopInstance(const std::string& str) {
  addInstanceNum(str);
}

void galois::runtime::reportLoopInstance(const char* loopname) {
  SC->beginLoopInstance(std::string(loopname ? loopname : "(NULL)"));
}

void galois::runtime::reportStat(const char* loopname, const char* category,
                                 unsigned long value, unsigned TID) {
  SC->addToStat(std::string(loopname ? loopname : "(NULL)"),
                std::string(category ? category : "(NULL)"), value, TID, 0);
}
void galois::runtime::reportStat(const char* loopname, const char* category,
                                 const std::string& value, unsigned TID) {
  SC->addToStat(std::string(loopname ? loopname : "(NULL)"),
                std::string(category ? category : "(NULL)"), value, TID, 0);
}

void galois::runtime::reportStat(const std::string& loopname,
                                 const std::string& category,
                                 unsigned long value, unsigned TID) {
  SC->addToStat(loopname, category, value, TID, 0);
}

void galois::runtime::reportStat(const std::string& loopname,
                                 const std::string& category,
                                 const std::string& value, unsigned TID) {
  SC->addToStat(loopname, category, value, TID, 0);
}

void galois::runtime::reportStatDist(const std::string& loopname,
                                     const std::string& category,
                                     const size_t value, unsigned TID,
                                     unsigned HostID) {
  SC->addToStat(loopname, category, value, TID, HostID);
}

void galois::runtime::reportStatDist(const std::string& loopname,
                                     const std::string& category,
                                     const double value, unsigned TID,
                                     unsigned HostID) {
  SC->addToStat(loopname, category, value, TID, HostID);
}

void galois::runtime::reportStatDist(const std::string& loopname,
                                     const std::string& category,
                                     const std::string& value, unsigned TID,
                                     unsigned HostID) {
  SC->addToStat(loopname, category, value, TID, HostID);
}

void galois::runtime::reportPageAlloc(const char* category) {
  for (unsigned x = 0; x < galois::runtime::activeThreads; ++x)
    reportStat("(NULL)", category,
               static_cast<unsigned long>(numPagePoolAllocForThread(x)), x);
}

void galois::runtime::reportNumaAlloc(const char* category) {
  int nodes = substrate::getThreadPool().getMaxNumaNodes();
  for (int x = 0; x < nodes; ++x) {
    // auto rStat = Stats.getRemote(x);
    // std::lock_guard<substrate::SimpleLock> lg(rStat->first);
    //      rStat->second.emplace_back(loop, category, numNumaAllocForNode(x));
  }
  //  SC->addNumaAllocToStat(std::string("(NULL)"), std::string(category ?
  //  category : "(NULL)"));
}
