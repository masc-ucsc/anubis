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

#include <iostream>
#include <limits>
#include "galois/DistGalois.h"
#include "galois/gstl.h"
#include "DistBenchStart.h"
#include "galois/runtime/CompilerHelperFunctions.h"

#include "galois/runtime/dGraph_edgeCut.h"
#include "galois/runtime/dGraph_cartesianCut.h"
#include "galois/runtime/dGraph_hybridCut.h"

#include "galois/DReducible.h"
#include "galois/runtime/Tracer.h"

#include "galois/runtime/dGraphLoader.h"

#ifdef __GALOIS_HET_CUDA__
#include "galois/cuda/cuda_device.h"
#include "gen_cuda.h"
struct CUDA_Context* cuda_ctx;

enum Personality { CPU, GPU_CUDA, GPU_OPENCL };

std::string personality_str(Personality p) {
  switch (p) {
  case CPU:
    return "CPU";
  case GPU_CUDA:
    return "GPU_CUDA";
  case GPU_OPENCL:
    return "GPU_OPENCL";
  }
  assert(false && "Invalid personality");
  return "";
}
#endif

static const char* const name = "BFS pull - Distributed Heterogeneous";
static const char* const desc = "BFS pull on Distributed Galois.";
static const char* const url  = 0;

/******************************************************************************/
/* Declaration of command line arguments */
/******************************************************************************/

namespace cll = llvm::cl;

static cll::opt<unsigned int> maxIterations("maxIterations",
                                            cll::desc("Maximum iterations: "
                                                      "Default 1000"),
                                            cll::init(1000));

static cll::opt<unsigned long long>
    src_node("startNode", cll::desc("ID of the source node"), cll::init(0));
static cll::opt<bool> verify("verify",
                             cll::desc("Verify results by outputting results "
                                       "to file"),
                             cll::init(false));

#ifdef __GALOIS_HET_CUDA__
static cll::opt<int> gpudevice("gpu",
                               cll::desc("Select GPU to run on, "
                                         "default is to choose automatically"),
                               cll::init(-1));
static cll::opt<Personality>
    personality("personality", cll::desc("Personality"),
                cll::values(clEnumValN(CPU, "cpu", "Galois CPU"),
                            clEnumValN(GPU_CUDA, "gpu/cuda", "GPU/CUDA"),
                            clEnumValN(GPU_OPENCL, "gpu/opencl", "GPU/OpenCL"),
                            clEnumValEnd),
                cll::init(CPU));
static cll::opt<unsigned> scalegpu(
    "scalegpu",
    cll::desc("Scale GPU workload w.r.t. CPU, default is proportionally "
              "equal workload to CPU and GPU (1)"),
    cll::init(1));
static cll::opt<unsigned> scalecpu(
    "scalecpu",
    cll::desc("Scale CPU workload w.r.t. GPU, default is proportionally "
              "equal workload to CPU and GPU (1)"),
    cll::init(1));
static cll::opt<int> num_nodes(
    "num_nodes",
    cll::desc("Num of physical nodes with devices (default = num of hosts): "
              "detect GPU to use for each host automatically"),
    cll::init(-1));
static cll::opt<std::string> personality_set(
    "pset",
    cll::desc("String specifying personality for hosts on each physical "
              "node. 'c'=CPU,'g'=GPU/CUDA and 'o'=GPU/OpenCL"),
    cll::init("c"));
#endif

/******************************************************************************/
/* Graph structure declarations + other initialization */
/******************************************************************************/

const uint32_t infinity = std::numeric_limits<uint32_t>::max() / 4;

struct NodeData {
  uint32_t dist_current;
};

typedef galois::graphs::DistGraph<NodeData, void> Graph;
typedef typename Graph::GraphNode GNode;

#if __OPT_VERSION__ >= 3
galois::DynamicBitSet bitset_dist_current;
#endif

#include "gen_sync.hh"

/******************************************************************************/
/* Algorithm structures */
/******************************************************************************/

struct InitializeGraph {
  const uint32_t& local_infinity;
  cll::opt<unsigned long long>& local_src_node;
  Graph* graph;

  InitializeGraph(cll::opt<unsigned long long>& _src_node,
                  const uint32_t& _infinity, Graph* _graph)
      : local_infinity(_infinity), local_src_node(_src_node), graph(_graph) {}

  void static go(Graph& _graph) {
    auto& allNodes = _graph.allNodesRange();
#ifdef __GALOIS_HET_CUDA__
    if (personality == GPU_CUDA) {
      std::string impl_str("InitializeGraph_" + (_graph.get_run_identifier()));
      galois::StatTimer StatTimer_cuda(impl_str.c_str());
      StatTimer_cuda.start();
      InitializeGraph_cuda(*(allNodes.begin()), *(allNodes.end()), infinity,
                           src_node, cuda_ctx);
      StatTimer_cuda.stop();
    } else if (personality == CPU)
#endif
    {
      galois::do_all_local(
          allNodes, InitializeGraph(src_node, infinity, &_graph),
          galois::no_stats(),
          galois::loopname(
              _graph.get_run_identifier("InitializeGraph").c_str()),
          galois::steal());
    }
  }

  void operator()(GNode src) const {
    NodeData& sdata = graph->getData(src);
    sdata.dist_current =
        (graph->getGID(src) == local_src_node) ? 0 : local_infinity;
  }
};

struct BFS {
  Graph* graph;
  galois::DGAccumulator<unsigned int>& DGAccumulator_accum;

  BFS(Graph* _graph, galois::DGAccumulator<unsigned int>& _dga)
      : graph(_graph), DGAccumulator_accum(_dga) {}

  void static go(Graph& _graph, galois::DGAccumulator<unsigned int>& dga) {
    unsigned _num_iterations = 0;

    auto& nodesWithEdges = _graph.allNodesWithEdgesRange();
    do {
      _graph.set_num_round(_num_iterations);
      dga.reset();

#if __OPT_VERSION__ == 5
      _graph.sync_on_demand<readDestination, Reduce_min_dist_current,
                            Bitset_dist_current>(
          Flags_dist_current, "BFS");
#endif

#ifdef __GALOIS_HET_CUDA__
      if (personality == GPU_CUDA) {
        std::string impl_str("BFS_" + (_graph.get_run_identifier()));
        galois::StatTimer StatTimer_cuda(impl_str.c_str());
        StatTimer_cuda.start();
        int __retval = 0;
        BFS_cuda(*nodesWithEdges.begin(), *nodesWithEdges.end(), __retval,
                 cuda_ctx);
        dga += __retval;
        StatTimer_cuda.stop();
      } else if (personality == CPU)
#endif
      {
        galois::do_all_local(
            nodesWithEdges, BFS(&_graph, dga), galois::no_stats(),
            galois::loopname(_graph.get_run_identifier("BFS").c_str()),
            galois::steal());
      }

#if __OPT_VERSION__ == 5
      Flags_dist_current.set_write_src();
#endif

#if __OPT_VERSION__ == 1
      // naive sync of everything after operator
      _graph.sync<writeAny, readAny, Reduce_min_dist_current,
                  Broadcast_dist_current>("BFS");
#elif __OPT_VERSION__ == 2
      // sync of touched fields (same as v1)
      _graph.sync<writeAny, readAny, Reduce_min_dist_current,
                  Broadcast_dist_current>("BFS");
#elif __OPT_VERSION__ == 3
      // with bitset
      _graph.sync<writeAny, readAny, Reduce_min_dist_current,
                  Bitset_dist_current>("BFS");
#elif __OPT_VERSION__ == 4
      // write aware (not read aware, i.e. conservative)
      _graph.sync<writeSource, readAny, Reduce_min_dist_current,
                  Bitset_dist_current>("BFS");
#endif

      // gold standard sync
      //_graph.sync<writeSource, readDestination, Reduce_min_dist_current,
      //            Bitset_dist_current>("BFS");

      galois::runtime::reportStat(
          "(NULL)", "NumWorkItems_" + (_graph.get_run_identifier()),
          (unsigned long)dga.read_local(), 0);
      ++_num_iterations;
    } while ((_num_iterations < maxIterations) &&
             dga.reduce(_graph.get_run_identifier()));

    if (galois::runtime::getSystemNetworkInterface().ID == 0) {
      galois::runtime::reportStat(
          "(NULL)", "NumIterations_" + std::to_string(_graph.get_run_num()),
          (unsigned long)_num_iterations, 0);
    }
  }

  void operator()(GNode src) const {
    NodeData& snode = graph->getData(src);

    for (auto jj = graph->edge_begin(src), ee = graph->edge_end(src); jj != ee;
         ++jj) {
      GNode dst         = graph->getEdgeDst(jj);
      auto& dnode       = graph->getData(dst);
      uint32_t new_dist = dnode.dist_current + 1;
      uint32_t old_dist = galois::min(snode.dist_current, new_dist);
      if (old_dist > new_dist) {
#if __OPT_VERSION__ >= 3
        bitset_dist_current.set(src);
#endif
        DGAccumulator_accum += 1;
      }
    }
  }
};

/******************************************************************************/
/* Sanity check operators */
/******************************************************************************/

/* Prints total number of nodes visited + max distance */
struct BFSSanityCheck {
  const uint32_t& local_infinity;
  Graph* graph;

  galois::DGAccumulator<uint64_t>& DGAccumulator_sum;
  galois::DGAccumulator<uint32_t>& DGAccumulator_max;
  galois::GReduceMax<uint32_t>& current_max;

  BFSSanityCheck(const uint32_t& _infinity, Graph* _graph,
                 galois::DGAccumulator<uint64_t>& dgas,
                 galois::DGAccumulator<uint32_t>& dgam,
                 galois::GReduceMax<uint32_t>& m)
      : local_infinity(_infinity), graph(_graph), DGAccumulator_sum(dgas),
        DGAccumulator_max(dgam), current_max(m) {}

  void static go(Graph& _graph, galois::DGAccumulator<uint64_t>& dgas,
                 galois::DGAccumulator<uint32_t>& dgam,
                 galois::GReduceMax<uint32_t>& m) {
    dgas.reset();
    dgam.reset();

#ifdef __GALOIS_HET_CUDA__
    if (personality == GPU_CUDA) {
      uint32_t sum, max;
      BFSSanityCheck_cuda(sum, max, infinity, cuda_ctx);
      dgas += sum;
      dgam = max;
    } else
#endif
    {
      m.reset();
      galois::do_all(galois::iterate(_graph.masterNodesRange().begin(),
                                     _graph.masterNodesRange().end()),
                     BFSSanityCheck(infinity, &_graph, dgas, dgam, m),
                     galois::no_stats(), galois::loopname("BFSSanityCheck"));
      dgam = m.reduce();
    }

    uint64_t num_visited  = dgas.reduce();
    uint32_t max_distance = dgam.reduce_max();

    // Only host 0 will print the info
    if (galois::runtime::getSystemNetworkInterface().ID == 0) {
      galois::gPrint("Number of nodes visited from source ", src_node, " is ",
                     num_visited, "\n");
      galois::gPrint("Max distance from source ", src_node, " is ",
                     max_distance, "\n");
    }
  }

  void operator()(GNode src) const {
    NodeData& src_data = graph->getData(src);

    if (src_data.dist_current < local_infinity) {
      DGAccumulator_sum += 1;
      current_max.update(src_data.dist_current);
    }
  }
};

/******************************************************************************/
/* Main */
/******************************************************************************/

int main(int argc, char** argv) {

  try {
    galois::DistMemSys G(getStatsFile());
    DistBenchStart(argc, argv, name, desc, url);

    {
      auto& net = galois::runtime::getSystemNetworkInterface();
      if (net.ID == 0) {
        galois::runtime::reportStat("(NULL)", "Max Iterations",
                                    (unsigned long)maxIterations, 0);
        galois::runtime::reportStat("(NULL)", "Source Node ID",
                                    (unsigned long long)src_node, 0);
#if __OPT_VERSION__ == 1
        printf("Version 1 of optimization\n");
#elif __OPT_VERSION__ == 2
        printf("Version 2 of optimization\n");
#elif __OPT_VERSION__ == 3
        printf("Version 3 of optimization\n");
#elif __OPT_VERSION__ == 4
        printf("Version 4 of optimization\n");
#elif __OPT_VERSION__ == 5
        printf("Version 5 of optimization\n");
#endif
      }
      galois::StatTimer StatTimer_init("TIMER_GRAPH_INIT"),
          StatTimer_total("TimerTotal"), StatTimer_hg_init("TIMER_HG_INIT");

      StatTimer_total.start();

      std::vector<unsigned> scalefactor;
#ifdef __GALOIS_HET_CUDA__
      const unsigned my_host_id = galois::runtime::getHostID();
      int gpu_device            = gpudevice;
      // Parse arg string when running on multiple hosts and update/override
      // personality with corresponding value.
      if (num_nodes == -1)
        num_nodes = net.Num;
      assert((net.Num % num_nodes) == 0);
      if (personality_set.length() == (net.Num / num_nodes)) {
        switch (personality_set.c_str()[my_host_id % (net.Num / num_nodes)]) {
        case 'g':
          personality = GPU_CUDA;
          break;
        case 'o':
          assert(0);
          personality = GPU_OPENCL;
          break;
        case 'c':
        default:
          personality = CPU;
          break;
        }
        if ((personality == GPU_CUDA) && (gpu_device == -1)) {
          gpu_device = get_gpu_device_id(personality_set, num_nodes);
        }
        if ((scalecpu > 1) || (scalegpu > 1)) {
          for (unsigned i = 0; i < net.Num; ++i) {
            if (personality_set.c_str()[i % num_nodes] == 'c')
              scalefactor.push_back(scalecpu);
            else
              scalefactor.push_back(scalegpu);
          }
        }
      }
#endif

      StatTimer_hg_init.start();
      Graph* hg = nullptr;

      hg = constructGraph<NodeData, void, false>(scalefactor);

#ifdef __GALOIS_HET_CUDA__
      if (personality == GPU_CUDA) {
        cuda_ctx = get_CUDA_context(my_host_id);
        if (!init_CUDA_context(cuda_ctx, gpu_device))
          return -1;
        MarshalGraph m = (*hg).getMarshalGraph(my_host_id);
        load_graph_CUDA(cuda_ctx, m, net.Num);
      } else if (personality == GPU_OPENCL) {
        // galois::opencl::cl_env.init(cldevice.Value);
      }
#endif
#if __OPT_VERSION__ >= 3
      bitset_dist_current.resize(hg->size());
#endif
      StatTimer_hg_init.stop();

      std::cout << "[" << net.ID << "] InitializeGraph::go called\n";
      StatTimer_init.start();
      InitializeGraph::go((*hg));
      StatTimer_init.stop();
      galois::runtime::getHostBarrier().wait();

      // accumulators for use in operators
      galois::DGAccumulator<unsigned int> DGAccumulator_accum;
      galois::DGAccumulator<uint64_t> DGAccumulator_sum;
      galois::DGAccumulator<uint32_t> DGAccumulator_max;
      galois::GReduceMax<uint32_t> m;

      for (auto run = 0; run < numRuns; ++run) {
        std::cout << "[" << net.ID << "] BFS::go run " << run << " called\n";
        std::string timer_str("Timer_" + std::to_string(run));
        galois::StatTimer StatTimer_main(timer_str.c_str());

        StatTimer_main.start();
        BFS::go(*hg, DGAccumulator_accum);
        StatTimer_main.stop();

        // sanity check
        BFSSanityCheck::go(*hg, DGAccumulator_sum, DGAccumulator_max, m);

        if ((run + 1) != numRuns) {
#ifdef __GALOIS_HET_CUDA__
          if (personality == GPU_CUDA) {
#if __OPT_VERSION__ >= 3
            bitset_dist_current_reset_cuda(cuda_ctx);
#endif
          } else
#endif
#if __OPT_VERSION__ >= 3
            bitset_dist_current.reset();
#endif

#if __OPT_VERSION__ == 5
          Flags_dist_current.clear_all();
#endif

          (*hg).set_num_run(run + 1);
          InitializeGraph::go((*hg));
          galois::runtime::getHostBarrier().wait();
        }
      }

      StatTimer_total.stop();

      // Verify
      if (verify) {
#ifdef __GALOIS_HET_CUDA__
        if (personality == CPU) {
#endif
          for (auto ii = (*hg).begin(); ii != (*hg).end(); ++ii) {
            if ((*hg).isOwned((*hg).getGID(*ii)))
              galois::runtime::printOutput("% %\n", (*hg).getGID(*ii),
                                           (*hg).getData(*ii).dist_current);
          }
#ifdef __GALOIS_HET_CUDA__
        } else if (personality == GPU_CUDA) {
          for (auto ii = (*hg).begin(); ii != (*hg).end(); ++ii) {
            if ((*hg).isOwned((*hg).getGID(*ii)))
              galois::runtime::printOutput(
                  "% %\n", (*hg).getGID(*ii),
                  get_node_dist_current_cuda(cuda_ctx, *ii));
          }
        }
#endif
      }
    }
    galois::runtime::getHostBarrier().wait();
    G.printDistStats();
    galois::runtime::getHostBarrier().wait();

    return 0;
  } catch (const char* c) {
    std::cerr << "Error: " << c << "\n";
    return 1;
  }
}
