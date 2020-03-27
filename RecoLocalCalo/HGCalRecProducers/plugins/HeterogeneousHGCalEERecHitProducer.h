#ifndef RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalEERecHitProducer_h
#define RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalEERecHitProducer_h

#include <iostream>
#include <string>
#include <memory>
#include <cuda_runtime.h>

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHit.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHitCollections.h"
#include "DataFormats/ForwardDetId/interface/HGCSiliconDetId.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/EDPutToken.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
#include "Geometry/HGCalCommonData/interface/HGCalDDDConstants.h"

#include "HeterogeneousCore/CUDACore/interface/ScopedContext.h"
#include "HeterogeneousCore/CUDACore/interface/ContextState.h"
#include "HeterogeneousCore/CUDAServices/interface/CUDAService.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"

#include "HeterogeneousHGCalProducerMemoryWrapper.h"
#include "KernelManagerHGCalRecHit.h"

class HeterogeneousHGCalEERecHitProducer: public edm::stream::EDProducer<edm::ExternalWork> 
{
 public:
  explicit HeterogeneousHGCalEERecHitProducer(const edm::ParameterSet& ps);
  ~HeterogeneousHGCalEERecHitProducer() override;

  virtual void acquire(edm::Event const&, edm::EventSetup const&, edm::WaitingTaskWithArenaHolder) override;
  virtual void produce(edm::Event&, const edm::EventSetup&) override;

 private:
  unsigned nhitsmax_ = 0;
  unsigned stride_ = 0;
  edm::EDGetTokenT<HGCeeUncalibratedRecHitCollection> token_;
  const std::string collection_name_ = "HeterogeneousHGCalEERecHits";
  edm::Handle<HGCeeUncalibratedRecHitCollection> handle_ee_; 
  size_t handle_size_;
  std::unique_ptr< HGCeeRecHitCollection > rechits_;
  cms::cuda::ContextState ctxState_;

  //constants
  HGCeeUncalibratedRecHitConstantData cdata_;
  HGCConstantVectorData vdata_;

  //memory
  std::string assert_error_message_(std::string, const size_t&, const size_t&);
  void assert_sizes_constants_(const HGCConstantVectorData&);
  void allocate_memory_(const cudaStream_t&);
  cms::cuda::host::unique_ptr<std::byte[]> mem_in_;
  cms::cuda::device::unique_ptr<std::byte[]> d_mem_;
  cms::cuda::host::unique_ptr<std::byte[]> mem_out_;

  //conditions (geometry, topology, ...)
  //void set_conditions_(const edm::EventSetup&);
  std::unique_ptr<hgcal::RecHitTools> tools_;

  //data processing
  void convert_collection_data_to_soa_(const HGCeeUncalibratedRecHitCollection&, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>*);
  void convert_soa_data_to_collection_(HGCRecHitCollection&, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>*);
  void convert_constant_data_(KernelConstantData<HGCeeUncalibratedRecHitConstantData>*);

  HGCUncalibratedRecHitSoA *uncalibSoA_ = nullptr, *d_uncalibSoA_ = nullptr, *d_intermediateSoA_ = nullptr;
  HGCRecHitSoA *d_calibSoA_ = nullptr, *calibSoA_ = nullptr;
  KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA> *kmdata_;
  KernelConstantData<HGCeeUncalibratedRecHitConstantData> *kcdata_;
  KernelConstantData<HGCeeUncalibratedRecHitConstantData> *d_kcdata_;
  edm::SortedCollection<HGCRecHit> out_data_;
};

#endif //RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalEERecHitProducer_h
