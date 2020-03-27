#include "HeterogeneousCore/CUDACore/interface/ESProduct.h"

// Declare the struct for the payload to be transferred. Here the
// example is an array with (potentially) dynamic size. Note that all of
// below becomes simpler if the array has compile-time size.
struct HeterogeneousGeometryESProduct {
  float *payload_array;
  unsigned int payload_var;
};

// Declare the wrapper ESProduct. The corresponding ESProducer should
// produce objects of this type.
class HeterogeneousGeometryESProductWrapper {
 public:
  // Constructor takes the standard CPU ESProduct, and transforms the
  // necessary data to array(s) in pinned host memory
  HeterogeneousGeometryESProductWrapper(HeterogeneousGeometryESProduct const&);
  
  // Deallocates all pinned host memory
  ~HeterogeneousGeometryESProductWrapper();
  
  // Function to return the actual payload on the memory of the current device
  HeterogeneousGeometryESProduct const *getHeterogeneousGeometryESProductAsync(cudaStream_t stream) const;
  
 private:
  // Holds the data in pinned CPU memory
  float *payload_array_;
  unsigned int payload_var_;
  const unsigned int nelements_ = 10;

  // Helper struct to hold all information that has to be allocated and
  // deallocated per device
  struct GPUData {
    // Destructor should free all member pointers
    ~GPUData();
    // internal pointers are on device, struct itself is on CPU
    HeterogeneousGeometryESProduct *host = nullptr;
    // internal pounters and struct are on device
    HeterogeneousGeometryESProduct *device = nullptr;
  };

  // Helper that takes care of complexity of transferring the data to
  // multiple devices
  cms::cuda::ESProduct<GPUData> gpuData_;
};
