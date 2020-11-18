#ifndef CUDADAtaFormats_HGCal_RecHitGPUProduct_H
#define CUDADAtaFormats_HGCal_RecHitGPUProduct_H

#include <cassert>
#include <numeric>

#include "HeterogeneousCore/CUDAUtilities/interface/device_unique_ptr.h"
#include "CUDADataFormats/HGCal/interface/HGCRecHitSoA.h"
#include "CUDADataFormats/HGCal/interface/HGCUncalibratedRecHitSoA.h"

class RecHitGPUProduct {
public:
  RecHitGPUProduct() = default;
  explicit RecHitGPUProduct(const uint32_t &nhits, const cudaStream_t &stream);
  ~RecHitGPUProduct() = default;

  RecHitGPUProduct(const RecHitGPUProduct &) = delete;
  RecHitGPUProduct &operator=(const RecHitGPUProduct &) = delete;
  RecHitGPUProduct(RecHitGPUProduct &&) = default;
  RecHitGPUProduct &operator=(RecHitGPUProduct &&) = default;

  std::byte *get() const { return ptr_.get(); }

  uint32_t nHits() const { return nhits_; }
  uint32_t stride() const { return stride_; }
  uint32_t nBytes() const { return size_tot_; }

private:
  cms::cuda::device::unique_ptr<std::byte[]> ptr_;
  uint32_t stride_;
  uint32_t nhits_;
  uint32_t size_tot_;
};

#endif  //CUDADAtaFormats_HGCal_RecHitGPUProduct_H
