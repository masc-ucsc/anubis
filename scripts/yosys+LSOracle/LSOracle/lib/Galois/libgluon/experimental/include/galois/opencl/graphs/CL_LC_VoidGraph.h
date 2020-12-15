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

/*
 * CL_LC_Graph.h
 *
 *  Created on: Nov 19, 2015
 *      Author: Rashid Kaleem (rashid.kaleem@gmail.com)
 */
#include "galois/opencl/CL_Header.h"
#include "galois/opencl/CL_Kernel.h"
#include <boost/iterator/counting_iterator.hpp>
//#include "galois/runtime/DistGraph.h"
#ifndef _GDIST_CL_LC_VOID_Graph_H_
#define _GDIST_CL_LC_VOID_Graph_H_

namespace galois {
namespace opencl {
namespace graphs {
#if 0
         /*####################################################################################################################################################*/
         template<typename DataType>
         struct BufferWrapperImpl {
            typedef std::pair<int, DataType> MessageType;
            std::vector<MessageType *> messageBuffers;
            std::vector<cl_mem> gpuMessageBuffer;
            int num_hosts;
            int num_ghosts;
            struct BufferWrapperGPU {
               cl_mem counters;
               cl_mem messages;
               int numHosts;
               cl_mem buffer_ptr;
            };
            BufferWrapperGPU gpu_impl;
            BufferWrapperImpl(int nh, int ng):num_hosts(nh), num_ghosts(ng) {
               allocate();
            }

            void allocate() {
               auto * ctx = galois::opencl::getCLContext();
               cl_int err;
               gpu_impl.counters = clCreateBuffer(ctx->get_default_device()->context(),CL_MEM_READ_WRITE, sizeof(cl_uint) *( num_hosts), 0, &err);
               gpu_impl.messages= clCreateBuffer(ctx->get_default_device()->context(), CL_MEM_READ_WRITE, sizeof(cl_uint) *( num_hosts), 0, &err);
               gpu_impl.buffer_ptr= clCreateBuffer(ctx->get_default_device()->context(),CL_MEM_READ_WRITE, sizeof(BufferWrapperGPU), 0, &err);
               CL_Kernel initBuffer;
               initBuffer.init("buffer_wrapper.cl", "initBufferWrapper");
               initBuffer.set_arg(0, gpu_impl.buffer_ptr);
               initBuffer.set_arg(1, gpu_impl.counters);
               initBuffer.set_arg(2, num_hosts);
               initBuffer.run_task();

               CL_Kernel registerBuffer;
               registerBuffer.init("buffer_wrapper.cl", "registerBuffer");
               registerBuffer.set_arg(0, gpu_impl.buffer_ptr);
               for(int i=0; i < num_hosts; ++i) {
                  MessageType * host_msg_buffer = new MessageType[num_ghosts];
                  messageBuffers.push_back(host_msg_buffer);
                  cl_mem msg_buffer = clCreateBuffer(ctx->get_default_device()->context(),CL_MEM_READ_WRITE, sizeof(MessageType) * (num_ghosts), 0, &err);
                  messageBuffers.push_back(msg_buffer);
                  registerBuffer.set_arg(1, msg_buffer);
                  registerBuffer.set_arg(2, i);
                  registerBuffer.run_task();
               }

            }
            void deallocate() {
               for(auto b : gpuMessageBuffer) {
                  clReleaseMemObject(b);
               }
               for(auto b : messageBuffers) {
                  delete [] b;
               }
               clReleaseMemObject(gpu_impl.counters);
               clReleaseMemObject(gpu_impl.messages);
               clReleaseMemObject(gpu_impl.buffer_ptr);
            }

            template<typename GraphType>
            void prepRecv(GraphType & g, galois::runtime::RecvBuffer& buffer) {
               CL_Kernel applyKernel;
               auto * ctx = galois::opencl::getCLContext();
               auto & net = galois::runtime::getSystemNetworkInterface();
               cl_command_queue queue = ctx->get_default_device()->command_queue();
               unsigned sender=0;
               const uint32_t counter = 0; // get counters from buffers.
               gDeserialize(buffer, sender, counter);
               for(int m=0; m<counter; ++m) {
                  gDeserialize(buffer, messageBuffers[sender][m].first, messageBuffers[sender][m].second);
               }
               int err = clEnqueueWriteBuffer(queue, gpuMessageBuffer[sender], CL_TRUE, 0, sizeof(MessageType)*(num_ghosts), &messageBuffers[sender], 0, NULL, NULL);
               applyKernel.init("buffer_wrapper.cl", "syncApply");
               applyKernel.set_arg(0, g);
               applyKernel.set_arg(1, gpuMessageBuffer[sender]);
               applyKernel.set_work_size(counter);
               applyKernel();

            } //End prepSend
         };
         /*####################################################################################################################################################*/
         enum GRAPH_FIELD_FLAGS {
            NODE_DATA=0x1,EDGE_DATA=0x10,OUT_INDEX=0x100, NEIGHBORS=0x1000,ALL=0x1111, ALL_DATA=0x0011, STRUCTURE=0x1100,
         };
         /*
          std::string vendor_name;
          cl_platform_id m_platform_id;
          cl_device_id m_device_id;
          cl_context m_context;
          cl_command_queue m_command_queue;
          cl_program m_program;
          */
         struct _CL_LC_Graph_GPU {
            cl_mem outgoing_index;
            cl_mem node_data;
            cl_mem neighbors;
            cl_mem edge_data;
            cl_uint num_nodes;
            cl_uint num_edges;
            cl_uint num_owned;
            cl_uint local_offset;
            cl_mem ghostMap;
         };

         static const char * cl_wrapper_str_CL_LC_Graph =
         "\
         typedef struct _ND{int dist_current;} NodeData;\ntypedef unsigned int EdgeData; \n\
      typedef struct _GraphType { \n\
   uint _num_nodes;\n\
   uint _num_edges;\n\
    uint _node_data_size;\n\
    uint _edge_data_size;\n\
    uint _num_owned;\n\
    uint _global_offset;\n\
    __global NodeData *_node_data;\n\
    __global uint *_out_index;\n\
    __global uint *_out_neighbors;\n\
    __global EdgeData *_out_edge_data;\n\
    }GraphType;\
      ";
         static const char * init_kernel_str_CL_LC_Graph =
         "\
      __kernel void initialize_graph_struct(__global uint * res, __global uint * g_meta, __global NodeData *g_node_data, __global uint * g_out_index, __global uint * g_nbr, __global EdgeData * edge_data){ \n \
      __global GraphType * g = (__global GraphType *) res;\n\
      g->_num_nodes = g_meta[0];\n\
      g->_num_edges = g_meta[1];\n\
      g->_node_data_size = g_meta[2];\n\
      g->_edge_data_size= g_meta[3];\n\
      g->_num_owned= g_meta[4];\n\
      g->_global_offset= g_meta[6];\n\
      g->_node_data = g_node_data;\n\
      g->_out_index= g_out_index;\n\
      g->_out_neighbors = g_nbr;\n\
      g->_out_edge_data = edge_data;\n\
      }\n\
      ";
#endif

template <typename NodeDataTy>
struct CL_LC_Graph<NodeDataTy, void> {
  typedef NodeDataTy NodeDataType;
  typedef boost::counting_iterator<uint64_t> NodeIterator;
  typedef NodeIterator GraphNode;
  typedef boost::counting_iterator<uint64_t> EdgeIterator;
  typedef unsigned int NodeIDType;
  typedef unsigned int EdgeIDType;
  typedef CL_LC_Graph<NodeDataType, void> SelfType;

  /*
   * A wrapper class to return when a request for getData is made.
   * The wrapper ensures that any pending writes are made before the
   * kernel is called on the graph instance. This is done either auto-
   * matically via the destructor or through a "sync" method.
   * */
  class NodeDataWrapper : public NodeDataType {
  public:
    SelfType* const parent;
    NodeIterator id;
    //               NodeDataType cached_data;
    bool isDirty;
    size_t idx_in_vec;
    NodeDataWrapper(SelfType* const p, NodeIterator& _id, bool isD)
        : parent(p), id(_id), isDirty(isD) {
      parent->read_node_data_impl(id, *this);
      if (isDirty) {
        parent->dirtyData.push_back(this);
        idx_in_vec = parent->dirtyData.size();
      }
    }
    NodeDataWrapper(const SelfType* const p, NodeIterator& _id, bool isD)
        : parent(const_cast<SelfType*>(p)), id(_id), isDirty(isD) {
      parent->read_node_data_impl(id, *this);
      if (isDirty) {
        parent->dirtyData.push_back(this);
        idx_in_vec = parent->dirtyData.size();
      }
    }
    NodeDataWrapper(const NodeDataWrapper& other)
        : parent(other.parent), id(other.id), isDirty(other.isDirty),
          idx_in_vec(other.idx_in_vec) {}
    ~NodeDataWrapper() {
      // Write upon cleanup for automatic scope cleaning.
      if (isDirty) {
        //                     fprintf(stderr, "Destructor - Writing to device
        //                     %d\n", *(id));
        parent->write_node_data_impl(id, *this);
        parent->dirtyData[idx_in_vec] = nullptr;
      }
    }
  }; // End NodeDataWrapper

  typedef NodeDataWrapper UserNodeDataType;

protected:
  galois::opencl::CLContext* ctx = getCLContext();
  // CPU Data
  size_t _num_nodes;
  size_t _num_edges;
  uint32_t _num_owned;
  uint64_t _global_offset;
  unsigned int _max_degree;
  const size_t SizeEdgeData;
  const size_t SizeNodeData;
  unsigned int* outgoing_index;
  unsigned int* neighbors;
  NodeDataType* node_data;

  std::vector<UserNodeDataType*> dirtyData;
  // GPU Data
  cl_mem gpu_struct_ptr;
  _CL_LC_Graph_GPU gpu_wrapper;
  cl_mem gpu_meta;

  //            NodeDataType * getData() {
  //               return node_data;
  //            }
  // Read a single node-data value from the device.
  // Blocking call.
  void read_node_data_impl(NodeIterator& it, NodeDataType& data) {
    int err;
    auto id                = *it;
    cl_command_queue queue = ctx->get_default_device()->command_queue();
    //               fprintf(stderr,  "Data read[%d], offset=%d  \n", id,
    //               sizeof(NodeDataType) * id);
    err = clEnqueueReadBuffer(queue, gpu_wrapper.node_data, CL_TRUE,
                              sizeof(NodeDataType) * (id), sizeof(NodeDataType),
                              &data, 0, NULL, NULL);
    //               fprintf(stderr,  "Data read[%d], offset=%d :: val=%d \n",
    //               id, sizeof(NodeDataType) * id, data.dist_current);
    CHECK_CL_ERROR(err, "Error reading node-data 0\n");
  }
  // Write a single node-data value to the device.
  // Blocking call.
  void write_node_data_impl(NodeIterator& it, NodeDataType& data) {
    int err;
    cl_command_queue queue = ctx->get_default_device()->command_queue();
    err = clEnqueueWriteBuffer(queue, gpu_wrapper.node_data, CL_TRUE,
                               sizeof(NodeDataType) * (*it),
                               sizeof(NodeDataType), &data, 0, NULL, NULL);
    CHECK_CL_ERROR(err, "Error writing node-data 0\n");
  }

public:
  /*
   * Go over all the wrapper instances created and write them to device.
   * Also, cleanup any automatically destroyed wrapper instances.
   * */
  void sync_outstanding_data() {
    fprintf(stderr, "Writing to device %u\n", dirtyData.size());
    std::vector<UserNodeDataType*> purged;
    for (auto i : dirtyData) {
      if (i && i->isDirty) {
        fprintf(stderr, "Writing to device %d\n", *(i->id));
        write_node_data_impl(i->id, i->cached_data);
        purged.push_back(i);
      }
    }
    dirtyData.clear();
    dirtyData.insert(dirtyData.begin(), purged.begin(), purged.end());
  }
  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  CL_LC_Graph()
      : SizeEdgeData(0),
        SizeNodeData(sizeof(NodeDataType) / sizeof(unsigned int)) {
    //      fprintf(stderr, "Created LC_LinearArray_Graph with %d node %d edge
    //      data.", (int) SizeNodeData, (int) SizeEdgeData);
    fprintf(stderr, "Loading devicegraph with copy-optimization.\n");
    _max_degree = _num_nodes = _num_edges = 0;
    _num_owned = _global_offset = 0;
    outgoing_index = neighbors = nullptr;
    node_data                  = nullptr;
    gpu_struct_ptr = gpu_meta = nullptr;
  }
  template <typename HGraph>
  CL_LC_Graph(std::string filename, unsigned myid, unsigned numHost)
      : SizeEdgeData(0),
        SizeNodeData(sizeof(NodeDataType) / sizeof(unsigned int)) {
    //      fprintf(stderr, "Created LC_LinearArray_Graph with %d node %d edge
    //      data.", (int) SizeNodeData, (int) SizeEdgeData);
    fprintf(stderr, "Loading device-graph [%s] with copy-optimization.\n",
            filename.c_str());
    _max_degree = _num_nodes = _num_edges = 0;
    _num_owned = _global_offset = 0;
    outgoing_index = neighbors = nullptr;
    node_data                  = nullptr;
    gpu_struct_ptr = gpu_meta = nullptr;
    //               DistGraph<NodeDataType, void> g(filename, myid, numHost);
    HGraph g(filename, myid, numHost);
    fprintf(stderr, "Loading from hgraph\n");
    load_from_galois(g);
  }
  template <typename HGraph>
  void load_from_hgraph(HGraph /*<NodeDataType,void> */& hg) {
    fprintf(stderr,
            "Loading device-graph from DistGraph with copy-optimization.\n");
    _max_degree = _num_nodes = _num_edges = 0;
    _num_owned = _global_offset = 0;
    outgoing_index = neighbors = nullptr;
    node_data                  = nullptr;
    gpu_struct_ptr = gpu_meta = nullptr;
    fprintf(stderr, "Loading from hgraph\n");
    load_from_galois(hg);
  }
  /******************************************************************
   *
   *******************************************************************/
  template <typename GaloisGraph>
  void load_from_galois(GaloisGraph& ggraph) {
    typedef typename GaloisGraph::GraphNode GNode;
    const size_t gg_num_nodes = ggraph.size();
    const size_t gg_num_edges = ggraph.sizeEdges();
    _num_owned                = ggraph.getNumOwned();
    _global_offset            = ggraph.getGlobalOffset();
    init(gg_num_nodes, gg_num_edges);
    int edge_counter  = 0;
    int node_counter  = 0;
    outgoing_index[0] = 0;
    for (auto n = ggraph.begin(); n != ggraph.end(); n++, node_counter++) {
      int src_node = *n;
      memcpy(&node_data[src_node], &ggraph.getData(*n), sizeof(NodeDataType));
      // TODO - RK - node_data[src_node] = ggraph.getData(*n);
      outgoing_index[src_node] = edge_counter;
      for (auto nbr = ggraph.edge_begin(*n); nbr != ggraph.edge_end(*n);
           ++nbr) {
        GNode dst               = ggraph.getEdgeDst(*nbr);
        neighbors[edge_counter] = dst;
        edge_counter++;
      }
    }
    while (node_counter != gg_num_nodes)
      outgoing_index[node_counter++] = edge_counter;
    outgoing_index[gg_num_nodes] = edge_counter;
    outgoing_index[gg_num_nodes] = edge_counter;
    fprintf(stderr, "Debug :: %d %d \n", node_counter, edge_counter);
    if (node_counter != gg_num_nodes)
      fprintf(stderr, "FAILED EDGE-COMPACTION :: %d, %zu\n", node_counter,
              gg_num_nodes);
    assert(edge_counter == gg_num_edges && "Failed to add all edges.");
    init_graph_struct();
    fprintf(stderr,
            "Loaded from GaloisGraph [V=%zu,E=%zu,ND=%lu,ED=%lu, Owned=%lu, "
            "Offset=%lu].\n",
            gg_num_nodes, gg_num_edges, sizeof(NodeDataType), 0, _num_owned,
            _global_offset);
  }
  /******************************************************************
   *
   *******************************************************************/
  template <typename GaloisGraph>
  void writeback_from_galois(GaloisGraph& ggraph) {
    typedef typename GaloisGraph::GraphNode GNode;
    const size_t gg_num_nodes = ggraph.size();
    const size_t gg_num_edges = ggraph.sizeEdges();
    int edge_counter          = 0;
    int node_counter          = 0;
    outgoing_index[0]         = 0;
    for (auto n = ggraph.begin(); n != ggraph.end(); n++, node_counter++) {
      int src_node = *n;
      //               std::cout<<*n<<", "<<ggraph.getData(*n)<<", "<<
      //               getData()[src_node]<<"\n";
      ggraph.getData(*n) = getData()[src_node];
    }
    fprintf(stderr, "Writeback from GaloisGraph [V=%zu,E=%zu,ND=%lu,ED=%lu].\n",
            gg_num_nodes, gg_num_edges, sizeof(NodeDataType), 0);
  }
  /******************************************************************
   *
   *******************************************************************/
  // TODO RK : fix - might not work with changes in interface.
  template <typename GaloisGraph>
  void load_from_galois(GaloisGraph& ggraph, int gg_num_nodes, int gg_num_edges,
                        int num_ghosts) {
    typedef typename GaloisGraph::GraphNode GNode;
    init(gg_num_nodes + num_ghosts, gg_num_edges);
    fprintf(stderr, "Loading from GaloisGraph [%d,%d,%d].\n", (int)gg_num_nodes,
            (int)gg_num_edges, num_ghosts);
    int edge_counter = 0;
    int node_counter = 0;
    for (auto n = ggraph.begin(); n != ggraph.begin() + gg_num_nodes;
         n++, node_counter++) {
      int src_node             = *n;
      getData(src_node)        = ggraph.getData(*n);
      outgoing_index[src_node] = edge_counter;
      for (auto nbr = ggraph.edge_begin(*n); nbr != ggraph.edge_end(*n);
           ++nbr) {
        GNode dst               = ggraph.getEdgeDst(*nbr);
        neighbors[edge_counter] = dst;
        edge_counter++;
      }
    }
    for (; node_counter < gg_num_nodes + num_ghosts; node_counter++) {
      outgoing_index[node_counter] = edge_counter;
    }
    outgoing_index[gg_num_nodes] = edge_counter;
    if (node_counter != gg_num_nodes)
      fprintf(stderr, "FAILED EDGE-COMPACTION :: %d, %d, %d\n", node_counter,
              gg_num_nodes, num_ghosts);
    assert(edge_counter == gg_num_edges && "Failed to add all edges.");
    init_graph_struct();
    fprintf(stderr, "Loaded from GaloisGraph [V=%d,E=%d,ND=%lu,ED=%lu].\n",
            gg_num_nodes, gg_num_edges, sizeof(NodeDataType), 0);
  }
  /******************************************************************
   *
   *******************************************************************/
  ~CL_LC_Graph() { deallocate(); }
  /******************************************************************
   *
   *******************************************************************/
  uint32_t getNumOwned() const { return this->_num_owned; }
  /******************************************************************
   *
   *******************************************************************/
  uint64_t getGlobalOffset() const { return this->_global_offset; }
  /******************************************************************
   *
   *******************************************************************/
  const cl_mem& device_ptr() { return gpu_struct_ptr; }
  /******************************************************************
   *
   *******************************************************************/
  void read(const char* filename) { readFromGR(*this, filename); }
  const NodeDataType& getData(NodeIterator nid) const {
    return node_data[*nid];
  }
  NodeDataType& getData(NodeIterator nid) { return node_data[*nid]; }
  NodeDataType getDataAfterRead(NodeIterator nid) {
    NodeDataType nData;
    read_node_data_impl(nid, nData);
    return nData;
  }
  void writeNodeData(NodeIterator nid, NodeDataType& nd) {
    write_node_data_impl(nid, nd);
    return;
  }
  const NodeDataWrapper getDataR(NodeIterator id) const {

    return NodeDataWrapper(this, id, false);
  }
  NodeDataWrapper getDataW(NodeIterator id) {
    return NodeDataWrapper(this, id, true);
  }
  unsigned int edge_begin(NodeIterator nid) { return outgoing_index[*nid]; }
  unsigned int edge_end(NodeIterator nid) { return outgoing_index[*nid + 1]; }
  unsigned int num_neighbors(NodeIterator node) {
    return outgoing_index[*node + 1] - outgoing_index[*node];
  }
  unsigned int& getEdgeDst(unsigned int eid) { return neighbors[eid]; }
  NodeIterator begin() { return NodeIterator(0); }
  NodeIterator end() { return NodeIterator(_num_nodes); }
  size_t size() { return _num_nodes; }
  size_t sizeEdges() { return _num_edges; }
  size_t max_degree() { return _max_degree; }
  void init(size_t n_n, size_t n_e) {
    _num_nodes = n_n;
    _num_edges = n_e;
    fprintf(stderr, "Allocating NN: :%d,  , NE %d :\n", (int)_num_nodes,
            (int)_num_edges);
    node_data      = new NodeDataType[_num_nodes];
    outgoing_index = new unsigned int[_num_nodes + 1];
    neighbors      = new unsigned int[_num_edges];
    allocate_on_gpu();
  }
  /******************************************************************
   *
   *******************************************************************/
  void allocate_on_gpu() {
    fprintf(stderr, "Buffer sizes : %d , %d \n", _num_nodes, _num_edges);
    int err;
    cl_mem_flags flags      = 0; // CL_MEM_READ_WRITE  ;
    cl_mem_flags flags_read = 0; // CL_MEM_READ_ONLY ;

    gpu_wrapper.outgoing_index =
        clCreateBuffer(ctx->get_default_device()->context(), flags_read,
                       sizeof(cl_uint) * (_num_nodes + 1), nullptr, &err);
    CHECK_CL_ERROR(err, "Error: clCreateBuffer of SVM - 0\n");
    gpu_wrapper.node_data =
        clCreateBuffer(ctx->get_default_device()->context(), flags,
                       sizeof(NodeDataType) * _num_nodes, nullptr, &err);
    CHECK_CL_ERROR(err, "Error: clCreateBuffer of SVM - 1\n");
    gpu_wrapper.neighbors =
        clCreateBuffer(ctx->get_default_device()->context(), flags_read,
                       sizeof(cl_uint) * _num_edges, nullptr, &err);
    CHECK_CL_ERROR(err, "Error: clCreateBuffer of SVM - 2\n");
    gpu_struct_ptr = clCreateBuffer(ctx->get_default_device()->context(), flags,
                                    sizeof(cl_uint) * 16, nullptr, &err);
    CHECK_CL_ERROR(err, "Error: clCreateBuffer of SVM - 3\n");
    gpu_wrapper.num_nodes = _num_nodes;
    gpu_wrapper.num_edges = _num_edges;

    const int meta_buffer_size = 16;
    gpu_meta =
        clCreateBuffer(ctx->get_default_device()->context(), flags,
                       sizeof(cl_uint) * meta_buffer_size, nullptr, &err);
    CHECK_CL_ERROR(err, "Error: clCreateBuffer of SVM - 5\n");
    int cpu_meta[meta_buffer_size];
    cpu_meta[0] = _num_nodes;
    cpu_meta[1] = _num_edges;
    cpu_meta[2] = SizeNodeData;
    cpu_meta[3] = SizeEdgeData;
    cpu_meta[4] = _num_owned;
    cpu_meta[6] = _global_offset;
    err         = clEnqueueWriteBuffer(
        ctx->get_default_device()->command_queue(), gpu_meta, CL_TRUE, 0,
        sizeof(int) * meta_buffer_size, cpu_meta, NULL, 0, NULL);
    CHECK_CL_ERROR(err, "Error: Writing META to GPU failed- 6\n");
    //               err = clReleaseMemObject(gpu_meta);
    //               CHECK_CL_ERROR(err, "Error: Releasing meta buffer.- 7\n");
    init_graph_struct();
  }
  /******************************************************************
   *
   *******************************************************************/
  void init_graph_struct() {
#if !PRE_INIT_STRUCT_ON_DEVICE
    this->copy_to_device();
    CL_Kernel init_kernel(getCLContext()->get_default_device());
    size_t kernel_len = strlen(cl_wrapper_str_CL_LC_Graph) +
                        strlen(init_kernel_str_CL_LC_Graph) + 1;
    char* kernel_src = new char[kernel_len];
    sprintf(kernel_src, "%s\n%s", cl_wrapper_str_CL_LC_Graph,
            init_kernel_str_CL_LC_Graph);
    //               init_kernel.init_string(kernel_src,
    //               "initialize_graph_struct");
    init_kernel.init("app_header.h", "initialize_void_graph_struct");
    //               init_kernel.set_arg_list(gpu_struct_ptr, gpu_meta,
    //               gpu_wrapper.node_data, gpu_wrapper.outgoing_index,
    //               gpu_wrapper.neighbors, gpu_wrapper.edge_data);
    init_kernel.set_arg_list_raw(
        gpu_struct_ptr, gpu_meta, gpu_wrapper.node_data,
        gpu_wrapper.outgoing_index, gpu_wrapper.neighbors);
    init_kernel.run_task();
#endif
  }
  ////////////##############################################################///////////
  ////////////##############################################################///////////
  /******************************************************************
   *
   *******************************************************************/
  // TODO RK - complete these.
  template <typename FnTy>
  void sync_push() {
#if 0
               char * synKernelString=" ";
               typedef std::pair<unsigned, typename FnTy::ValTy> MsgType;
               std::vector<MsgType> data;
               std::vector<size_t> peerHosts;
               size_t selfID;
               for(int peerID : peerHosts) {
                  if(peerID != selfID) {
                     std::pair<unsigned int, unsigned int> peerRange; // getPeerRange(peerID);
                     for(int i = peerRange.first; i<peerRange.second; ++i) {
                        data.push_back(MsgType(i, getData(i)));
                     }
                  }
               }

#endif
  }
  template <typename FnTy>
  void sync_pull() {}
  ////////////##############################################################///////////
  ////////////##############################################################///////////
  void deallocate(void) {
    delete[] node_data;
    delete[] outgoing_index;
    delete[] neighbors;

    cl_int err = clReleaseMemObject(gpu_wrapper.outgoing_index);
    CHECK_CL_ERROR(err, "Error: clReleaseMemObject of SVM - 0\n");
    err = clReleaseMemObject(gpu_wrapper.node_data);
    CHECK_CL_ERROR(err, "Error: clReleaseMemObject of SVM - 1\n");
    err = clReleaseMemObject(gpu_wrapper.neighbors);
    CHECK_CL_ERROR(err, "Error: clReleaseMemObject of SVM - 3\n");
    err = clReleaseMemObject(gpu_struct_ptr);
    CHECK_CL_ERROR(err, "Error: clReleaseMemObject of SVM - 4\n");
    err = clReleaseMemObject(gpu_meta);
    CHECK_CL_ERROR(err, "Error: clReleaseMemObject of SVM - 5\n");
  }
  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // Done
  void copy_to_host(GRAPH_FIELD_FLAGS flags = GRAPH_FIELD_FLAGS::ALL) {
    int err;
    cl_command_queue queue = ctx->get_default_device()->command_queue();
    if (flags & GRAPH_FIELD_FLAGS::OUT_INDEX) {
      err = clEnqueueReadBuffer(queue, gpu_wrapper.outgoing_index, CL_TRUE, 0,
                                sizeof(cl_uint) * (_num_nodes + 1),
                                outgoing_index, 0, NULL, NULL);
      CHECK_CL_ERROR(err, "Error copying to host 0\n");
    }
    if (flags & GRAPH_FIELD_FLAGS::NODE_DATA) {
      err = clEnqueueReadBuffer(queue, gpu_wrapper.node_data, CL_TRUE, 0,
                                sizeof(NodeDataType) * _num_nodes, node_data, 0,
                                NULL, NULL);
      CHECK_CL_ERROR(err, "Error copying to host 1\n");
    }
    if (flags & GRAPH_FIELD_FLAGS::NEIGHBORS) {
      err = clEnqueueReadBuffer(queue, gpu_wrapper.neighbors, CL_TRUE, 0,
                                sizeof(cl_uint) * _num_edges, neighbors, 0,
                                NULL, NULL);
      CHECK_CL_ERROR(err, "Error copying to host 2\n");
    }
  }
  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // Done
  void copy_to_device(GRAPH_FIELD_FLAGS flags = GRAPH_FIELD_FLAGS::ALL) {
    int err;
    cl_command_queue queue = ctx->get_default_device()->command_queue();
    //               fprintf(stderr, "Buffer sizes : %d , %d \n", _num_nodes,
    //               _num_edges);
    if (flags & GRAPH_FIELD_FLAGS::OUT_INDEX) {
      err = clEnqueueWriteBuffer(queue, gpu_wrapper.outgoing_index, CL_TRUE, 0,
                                 sizeof(cl_uint) * (_num_nodes + 1),
                                 outgoing_index, 0, NULL, NULL);
      CHECK_CL_ERROR(err, "Error copying to device 0\n");
    }
    if (flags & GRAPH_FIELD_FLAGS::NODE_DATA) {
      err = clEnqueueWriteBuffer(queue, gpu_wrapper.node_data, CL_TRUE, 0,
                                 sizeof(NodeDataType) * _num_nodes, node_data,
                                 0, NULL, NULL);
      CHECK_CL_ERROR(err, "Error copying to device 1\n");
    }
    if (flags & GRAPH_FIELD_FLAGS::NEIGHBORS) {
      err = clEnqueueWriteBuffer(queue, gpu_wrapper.neighbors, CL_TRUE, 0,
                                 sizeof(cl_uint) * _num_edges, neighbors, 0,
                                 NULL, NULL);
      CHECK_CL_ERROR(err, "Error copying to device 2\n");
    }
    ////////////Initialize kernel here.
    return;
  }
  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  void print_graph(void) {
    std::cout << "\n====Printing graph (" << _num_nodes << " , " << _num_edges
              << ")=====\n";
    for (size_t i = 0; i < _num_nodes; ++i) {
      print_node(i);
      std::cout << "\n";
    }
    return;
  }
  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  void print_node(unsigned int idx) {
    if (idx < _num_nodes) {
      std::cout << "N-" << idx << "(" << node_data[idx] << ")"
                << " :: [";
      for (size_t i = outgoing_index[idx]; i < outgoing_index[idx + 1]; ++i) {
        std::cout << " " << neighbors[i] << ", ";
      }
      std::cout << "]";
    }
    return;
  }
  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  void print_compact(void) {
    std::cout << "\nOut-index [";
    for (size_t i = 0; i < _num_nodes + 1; ++i) {
      std::cout << " " << outgoing_index[i] << ",";
    }
    std::cout << "]\nNeigh[";
    for (size_t i = 0; i < _num_edges; ++i) {
      std::cout << " " << neighbors[i] << ",";
    }
    std::cout << "]\nEData [";
    std::cout << "]";
  }
  ///////////////////////////////////
  // TODO
  void reset_num_iter(uint32_t runNum) {}
  size_t getGID(const NodeIterator& nt) { return *nt; }

}; // End CL_LC_Graph
} // namespace graphs
} // namespace opencl
} // namespace galois

#endif /* _GDIST_CL_LC_VOID_Graph_H_ */
