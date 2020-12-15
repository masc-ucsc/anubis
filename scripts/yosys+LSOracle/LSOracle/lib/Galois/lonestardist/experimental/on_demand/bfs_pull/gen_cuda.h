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

#pragma once
#include "galois/runtime/DataCommMode.h"

struct CUDA_Context;

struct CUDA_Context* get_CUDA_context(int id);
bool init_CUDA_context(struct CUDA_Context* ctx, int device);
void load_graph_CUDA(struct CUDA_Context* ctx, MarshalGraph& g,
                     unsigned num_hosts);

void reset_CUDA_context(struct CUDA_Context* ctx);

void get_bitset_dist_current_cuda(struct CUDA_Context* ctx,
                                  unsigned long long int* bitset_compute);
void bitset_dist_current_reset_cuda(struct CUDA_Context* ctx);
void bitset_dist_current_reset_cuda(struct CUDA_Context* ctx, size_t begin,
                                    size_t end);
uint32_t get_node_dist_current_cuda(struct CUDA_Context* ctx, unsigned LID);
void set_node_dist_current_cuda(struct CUDA_Context* ctx, unsigned LID,
                                uint32_t v);
void add_node_dist_current_cuda(struct CUDA_Context* ctx, unsigned LID,
                                uint32_t v);
bool min_node_dist_current_cuda(struct CUDA_Context* ctx, unsigned LID,
                                uint32_t v);
void batch_get_node_dist_current_cuda(struct CUDA_Context* ctx,
                                      unsigned from_id, uint8_t* v);
void batch_get_node_dist_current_cuda(struct CUDA_Context* ctx,
                                      unsigned from_id,
                                      unsigned long long int* bitset_comm,
                                      size_t* v_size, DataCommMode* data_mode);
void batch_get_mirror_node_dist_current_cuda(struct CUDA_Context* ctx,
                                             unsigned from_id, uint8_t* v);
void batch_get_mirror_node_dist_current_cuda(
    struct CUDA_Context* ctx, unsigned from_id,
    size_t* v_size, DataCommMode* data_mode);
void batch_get_reset_node_dist_current_cuda(struct CUDA_Context* ctx,
                                            unsigned from_id, uint8_t* v,
                                            uint32_t i);
void batch_get_reset_node_dist_current_cuda(
    struct CUDA_Context* ctx, unsigned from_id,
    size_t* v_size, DataCommMode* data_mode, uint32_t i);
void batch_set_mirror_node_dist_current_cuda(
    struct CUDA_Context* ctx, unsigned from_id,
    DataCommMode data_mode);
void batch_set_node_dist_current_cuda(struct CUDA_Context* ctx,
                                      unsigned from_id,
                                      unsigned long long int* bitset_comm,
                                      DataCommMode data_mode);
void batch_add_node_dist_current_cuda(struct CUDA_Context* ctx,
                                      unsigned from_id,
                                      unsigned long long int* bitset_comm,
                                      DataCommMode data_mode);
void batch_min_node_dist_current_cuda(struct CUDA_Context* ctx,
                                      unsigned from_id,
                                      unsigned long long int* bitset_comm,
                                      DataCommMode data_mode);

void BFSSanityCheck_cuda(unsigned int& sum, unsigned int& max,
                         const uint32_t& local_infinity,
                         struct CUDA_Context* ctx);
void BFS_cuda(unsigned int __begin, unsigned int __end, int& __retval,
              struct CUDA_Context* ctx);
void BFS_all_cuda(int& __retval, struct CUDA_Context* ctx);
void InitializeGraph_cuda(unsigned int __begin, unsigned int __end,
                          const uint32_t& local_infinity,
                          uint64_t local_src_node, struct CUDA_Context* ctx);
void InitializeGraph_all_cuda(const uint32_t& local_infinity,
                              uint64_t local_src_node,
                              struct CUDA_Context* ctx);
