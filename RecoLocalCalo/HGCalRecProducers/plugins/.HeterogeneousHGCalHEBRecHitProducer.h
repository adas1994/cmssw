#ifndef RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalHEBRecHitProducer_h
#define RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalHEBRecHitProducer_h

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
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

class HeterogeneousHGCalHEBRecHitProducer: public edm::stream::EDProducer<edm::ExternalWork> 
{
 public:
  explicit HeterogeneousHGCalHEBRecHitProducer(const edm::ParameterSet& ps);
  ~HeterogeneousHGCalHEBRecHitProducer() override;
  void beginRun(edm::Run const&, edm::EventSetup const&) override;

  virtual void acquire(edm::Event const&, edm::EventSetup const&, edm::WaitingTaskWithArenaHolder) override;
  virtual void produce(edm::Event&, const edm::EventSetup&) override;

 private:
  unsigned int nhitsmax_ = 0;
  unsigned int stride_ = 0;
  edm::EDGetTokenT<HGChebUncalibratedRecHitCollection> token_;
  const std::string collection_name_ = "HeterogeneousHGCalHEBRecHits";
  edm::Handle<HGChebUncalibratedRecHitCollection> handle_heb_; 
  size_t handle_size_;
  std::unique_ptr< HGChebRecHitCollection > rechits_;
  cms::cuda::ContextState ctxState_;

  //constants
  HGChebUncalibratedRecHitConstantData cdata_;
  HGCConstantVectorData vdata_;

  //memory
  std::string assert_error_message_(std::string var, const size_t& s);
  void assert_sizes_constants_(const HGCConstantVectorData& vd);
  void allocate_memory_(const cudaStream_t&);
  cms::cuda::host::unique_ptr<std::byte[]> mem_in_;
  cms::cuda::device::unique_ptr<std::byte[]> d_mem_;
  cms::cuda::host::unique_ptr<std::byte[]> mem_out_;

  //conditions
  std::unique_ptr<hgcal::RecHitTools> tools_;
  const HGCalDDDConstants* ddd_ = nullptr;
  const HGCalParameters* params_ = nullptr;

  //data processing
  void convert_collection_data_to_soa_(const HGChebUncalibratedRecHitCollection&, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>*);
  void convert_soa_data_to_collection_(HGCRecHitCollection&, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>*);
  void convert_constant_data_(KernelConstantData<HGChebUncalibratedRecHitConstantData>*);

  HGCUncalibratedRecHitSoA *uncalibSoA_ = nullptr, *d_uncalibSoA_ = nullptr, *d_intermediateSoA_ = nullptr;
  HGCRecHitSoA *d_calibSoA_ = nullptr, *calibSoA_ = nullptr;
  KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA> *kmdata_;
  KernelConstantData<HGChebUncalibratedRecHitConstantData> *kcdata_;
  KernelConstantData<HGChebUncalibratedRecHitConstantData> *d_kcdata_;
  edm::SortedCollection<HGCRecHit> out_data_;
};

#endif //RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalHEBRecHitProducer_h
