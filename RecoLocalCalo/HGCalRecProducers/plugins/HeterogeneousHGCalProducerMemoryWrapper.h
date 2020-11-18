#ifndef RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalProducerMemoryWrapper_h
#define RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalProducerMemoryWrapper_h

#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include <type_traits>
#include <numeric>
#include <cuda_runtime.h>

#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHit.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHitCollections.h"
#include "CUDADataFormats/HGCal/interface/HGCRecHitSoA.h"
#include "CUDADataFormats/HGCal/interface/HGCUncalibratedRecHitSoA.h"
#include "CUDADataFormats/HGCal/interface/HGCUncalibratedRecHitsToRecHitsConstants.h"

#include "HeterogeneousCore/CUDACore/interface/ScopedContext.h"
#include "HeterogeneousCore/CUDACore/interface/ContextState.h"
#include "HeterogeneousCore/CUDAServices/interface/CUDAService.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"
#include "HeterogeneousCore/CUDAUtilities/interface/device_unique_ptr.h"
#include "HeterogeneousCore/CUDAUtilities/interface/host_unique_ptr.h"

#include "KernelManagerHGCalRecHit.h"

namespace memory {
  namespace allocation {
    cms::cuda::device::unique_ptr<std::byte[]> uncalibRecHitDevice(const uint32_t&,
                                                                   const uint32_t&,
                                                                   HGCUncalibratedRecHitSoA*,
                                                                   const cudaStream_t&);
    cms::cuda::host::unique_ptr<std::byte[]> uncalibRecHitHost(const uint32_t&,
                                                               const uint32_t&,
                                                               HGCUncalibratedRecHitSoA*,
                                                               const cudaStream_t&);
    cms::cuda::host::unique_ptr<std::byte[]> calibRecHitHost(const uint32_t&,
                                                             const uint32_t&,
                                                             HGCRecHitSoA*,
                                                             const cudaStream_t&);
    void calibRecHitDevice(const uint32_t&, const uint32_t&, const uint32_t&, HGCRecHitSoA*, std::byte*);
  }  // namespace allocation
}  // namespace memory

#endif  //RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalProducerMemoryWrapper_h
