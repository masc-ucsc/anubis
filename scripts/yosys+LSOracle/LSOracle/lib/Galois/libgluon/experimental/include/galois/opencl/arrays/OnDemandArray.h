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

#ifndef GALOISGPU_OCL_ONDEMANDARRAY_H_
#define GALOISGPU_OCL_ONDEMANDARRAY_H_

namespace galois {
namespace opencl {
/*******************************************************************************
 *
 ********************************************************************************/

template <typename T>
struct OnDemandArray {
  typedef cl_mem DevicePtrType;
  typedef T* HostPtrType;
  cl_mem_flags allocation_flags;
  CL_Device* device;

  explicit OnDemandArray(unsigned long sz, CL_Device* d)
      : device(d), device_data(0), num_elements(sz), err(0) {
    DeviceMemoryType MemType = DISCRETE;
    host_data                = nullptr;
    host_data                = new T[num_elements];
    switch (MemType) {
    case HOST_CACHED:
      allocation_flags = CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR;
      break;
    case PINNED:
      allocation_flags = CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR;
      break;
    case CONSTANT:
      allocation_flags = CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR;
      break;
    case DISCRETE:
    default:
      allocation_flags = CL_MEM_COPY_HOST_PTR; // CL_MEM_USE_HOST_PTR;
      break;
    }
  }
  void allocate_on_device() {
    device_data = clCreateBuffer(device->context(), allocation_flags,
                                 sizeof(T) * num_elements, host_data, &err);
    ReportDataAllocation(device, sizeof(T) * num_elements, err);
  }
  ////////////////////////////////////////////////
  void copy_to_device() {
    CHECK_CL_ERROR(
        err = clEnqueueWriteBuffer(device->command_queue(), device_data,
                                   CL_TRUE, 0, sizeof(T) * num_elements,
                                   (void*)(host_data), 0, NULL, NULL),
        " Copying to device ");
    ReportCopyToDevice(device, sizeof(T) * num_elements, err);
  }
  ////////////////////////////////////////////////
  void allocate_and_copy_to_device() {
    allocate_on_device();
    CHECK_CL_ERROR(
        err = clEnqueueWriteBuffer(device->command_queue(), device_data,
                                   CL_TRUE, 0, sizeof(T) * num_elements,
                                   (void*)(host_data), 0, NULL, NULL),
        " Copying to device ");
    ReportCopyToDevice(device, sizeof(T) * num_elements, err);
  }
  ////////////////////////////////////////////////
  void copy_to_device(cl_event* event) {
    CHECK_CL_ERROR(
        err = clEnqueueWriteBuffer(device->command_queue(), device_data,
                                   CL_FALSE, 0, sizeof(T) * num_elements,
                                   (void*)(host_data), 0, NULL, event),
        " Copying async. to device ");
    ReportCopyToDevice(device, sizeof(T) * num_elements, err);
  }
  void allocate_and_copy_to_device(cl_event* event) {
    allocate_on_device();
    CHECK_CL_ERROR(
        err = clEnqueueWriteBuffer(device->command_queue(), device_data,
                                   CL_FALSE, 0, sizeof(T) * num_elements,
                                   (void*)(host_data), 0, NULL, event),
        " Copying async. to device ");
    ReportCopyToDevice(device, sizeof(T) * num_elements, err);
  }
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  void copy_to_device(void* aux) {
    CHECK_CL_ERROR(err = clEnqueueWriteBuffer(
                       device->command_queue(), device_data, CL_TRUE, 0,
                       sizeof(T) * num_elements, (void*)(aux), 0, NULL, NULL),
                   " Copying aux to device ");
    ReportCopyToDevice(device, sizeof(T) * num_elements, err);
  }
  ////////////////////////////////////////////////
  void copy_to_host() {
    CHECK_CL_ERROR(err =
                       clEnqueueReadBuffer(device->command_queue(), device_data,
                                           CL_TRUE, 0, sizeof(T) * num_elements,
                                           (void*)(host_data), 0, NULL, NULL),
                   "Copying to host ");
    ReportCopyToHost(device, sizeof(T) * num_elements, err);
  }
  void copy_to_host(void* ptr) {
    CHECK_CL_ERROR(err = clEnqueueReadBuffer(
                       device->command_queue(), device_data, CL_TRUE, 0,
                       sizeof(T) * num_elements, (void*)(ptr), 0, NULL, NULL),
                   "Copying to host ");
    ReportCopyToHost(device, sizeof(T) * num_elements, err);
  }
  ////////////////////////////////////////////////
  void copy_to_host_and_deallocate() {
    CHECK_CL_ERROR(err =
                       clEnqueueReadBuffer(device->command_queue(), device_data,
                                           CL_TRUE, 0, sizeof(T) * num_elements,
                                           (void*)(host_data), 0, NULL, NULL),
                   "Copying to host ");
    ReportCopyToHost(device, sizeof(T) * num_elements, err);
    deallocate();
  }
  ////////////////////////////////////////////////
  size_t size() { return num_elements; }
  ////////////////////////////////////////////////
  operator T*() { return host_data; }
  T& operator[](size_t idx) { return host_data[idx]; }
  DevicePtrType& device_ptr(void) { return device_data; }
  HostPtrType& host_ptr(void) { return host_data; }
  Array<T>* get_array_ptr(void) { return this; }
  ~OnDemandArray<T>() {
#ifdef _GOPT_DEBUG
    std::cout << "Deleting array host:: " << host_data
              << " , device :: " << device_data << "\n";
#endif
    if (host_data)
      delete[] host_data;
    if (device_data)
      deallocate();
  }
  void deallocate() {
    clReleaseMemObject(device_data);
    ReportDataAllocation(device, -sizeof(T) * num_elements, err);
    device_data = 0;
  }
  HostPtrType host_data;
  DevicePtrType device_data;
  size_t num_elements;
  int err;

protected:
};
} // end namespace opencl
} // end namespace galois

#endif /* GALOISGPU_OCL_ONDEMANDARRAY_H_ */
