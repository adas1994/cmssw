#include "RecoLocalCalo/HGCalRecProducers/plugins/TEST_HeterogeneousESProduct.h"

HeterogeneousGeometryESProductWrapper::HeterogeneousGeometryESProductWrapper(HeterogeneousGeometryESProduct const& cpuGeometry) {
  cudaCheck(cudaMallocHost(&payload_array_, sizeof(float)*nelements_));
  for(unsigned int i=0; i<nelements_; ++i)
    payload_array_[i] = i;
  payload_var_ = 1;
}

HeterogeneousGeometryESProduct const *HeterogeneousGeometryESProductWrapper::getHeterogeneousGeometryESProductAsync(cudaStream_t stream) const {
  // cms::cuda::ESProduct<T> essentially holds an array of GPUData objects,
  // one per device. If the data have already been transferred to the
  // current device (or the transfer has been queued), the helper just
  // returns a reference to that GPUData object. Otherwise, i.e. data are
  // not yet on the current device, the helper calls the lambda to do the
  // necessary memory allocations and to queue the transfers.
  auto const& data = gpuData_.dataForCurrentDeviceAsync(stream,
	  [this](GPUData& data, cudaStream_t stream)
	  {
	    // Allocate memory. Currently this can be with the CUDA API,
	    // sometime we'll migrate to the caching allocator. Assumption is
	    // that IOV changes are rare enough that adding global synchronization
	    // points is not that bad (for now).
	    
	    // Allocate the payload object on pinned host memory.
	    cudaCheck(cudaMallocHost(&data.host, sizeof(HeterogeneousGeometryESProduct)));
	    // Allocate the payload array(s) on device memory.
	    cudaCheck(cudaMalloc(&data.host->payload_array, sizeof(float)*nelements_));

	    // Allocate the payload object on the device memory.
	    cudaCheck(cudaMalloc(&data.device, sizeof(HeterogeneousGeometryESProduct)));
	    
	    // Complete the host-side information on the payload
	    data.host->payload_var = this->payload_var_;
	    
	    // Transfer the payload, first the array(s) ...
	    cudaCheck(cudaMemcpyAsync(data.host->payload_array, this->payload_array_, sizeof(float)*nelements_, cudaMemcpyHostToDevice, stream));
	    // ... and then the payload object
	    cudaCheck(cudaMemcpyAsync(data.device, data.host, sizeof(HeterogeneousGeometryESProduct), cudaMemcpyHostToDevice, stream));
	  }); //gpuData_.dataForCurrentDeviceAsync

  // Returns the payload object on the memory of the current device
  return data.device;
}

// Destructor frees all member pointers
HeterogeneousGeometryESProductWrapper::GPUData::~GPUData() {
  if(host != nullptr) 
    {
      cudaCheck(cudaFree(host->payload_array));
      cudaCheck(cudaFreeHost(host));
    }
  cudaCheck(cudaFree(device));
}
