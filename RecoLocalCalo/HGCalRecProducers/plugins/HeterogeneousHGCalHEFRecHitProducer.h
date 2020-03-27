#ifndef RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalHEFRecHitProducer_h
#define RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalHEFRecHitProducer_h

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <cuda_runtime.h>

#include "FWCore/Framework/interface/ESProducer.h"
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
#include "Geometry/HGCalCommonData/interface/HGCalWaferIndex.h"

#include "HeterogeneousCore/CUDACore/interface/ScopedContext.h"
#include "HeterogeneousCore/CUDACore/interface/ContextState.h"
#include "HeterogeneousCore/CUDAServices/interface/CUDAService.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"

#include "HeterogeneousHGCalProducerMemoryWrapper.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "CondFormats/HGCalObjects/interface/HeterogeneousHGCalHEFCellPositionsConditions.h"
#include "CondFormats/DataRecord/interface/HeterogeneousHGCalHEFCellPositionsConditionsRecord.h"
#include "RecoLocalCalo/HGCalRecProducers/plugins/KernelManagerHGCalRecHit.h"
#include "RecoLocalCalo/HGCalESProducers/plugins/KernelManagerHGCalCellPositions.h"

class HeterogeneousHGCalHEFRecHitProducer: public edm::stream::EDProducer<edm::ExternalWork> 
{
 public:
  explicit HeterogeneousHGCalHEFRecHitProducer(const edm::ParameterSet& ps);
  ~HeterogeneousHGCalHEFRecHitProducer() override;
  void beginRun(edm::Run const&, edm::EventSetup const&) override;
  
  virtual void acquire(edm::Event const&, edm::EventSetup const&, edm::WaitingTaskWithArenaHolder) override;
  virtual void produce(edm::Event&, const edm::EventSetup&) override;

 private:
  unsigned int nhitsmax_ = 0;
  unsigned int stride_ = 0;
  edm::EDGetTokenT<HGChefUncalibratedRecHitCollection> token_;
  const std::string collection_name_ = "HeterogeneousHGCalHEFRecHits";
  edm::Handle<HGChefUncalibratedRecHitCollection> handle_hef_; 
  size_t handle_size_;
  std::unique_ptr< HGChefRecHitCollection > rechits_;
  cms::cuda::ContextState ctxState_;

  //constants
  HGChefUncalibratedRecHitConstantData cdata_;
  HGCConstantVectorData vdata_;

  //memory
  std::string assert_error_message_(std::string, const size_t&, const size_t&);
  void assert_sizes_constants_(const HGCConstantVectorData&);
  void allocate_memory_(const cudaStream_t&);
  cms::cuda::host::unique_ptr<std::byte[]> mem_in_;
  cms::cuda::device::unique_ptr<std::byte[]> d_mem_;
  cms::cuda::host::unique_ptr<std::byte[]> mem_out_;

  //conditions (geometry, topology, ...)
  std::unique_ptr<hgcal::RecHitTools> tools_;
  /*
  const hgcal_conditions::HeterogeneousHEFConditionsESProduct* d_conds = nullptr;
  hgcal_conditions::positions::HGCalPositionsMapping* posmap_;
  */
  const HGCalDDDConstants* ddd_ = nullptr;
  const HGCalParameters* params_ = nullptr;

  /*
  edm::Service<TFileService> fs;
  TH1F *x0, *x1, *x2, *y0, *y1, *y2;
  */

  //data processing
  void convert_collection_data_to_soa_(const HGChefUncalibratedRecHitCollection&, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>*);
  void convert_soa_data_to_collection_(HGCRecHitCollection&, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>*);
  void convert_constant_data_(KernelConstantData<HGChefUncalibratedRecHitConstantData>*);

  HGCUncalibratedRecHitSoA *uncalibSoA_ = nullptr, *d_uncalibSoA_ = nullptr, *d_intermediateSoA_ = nullptr;
  HGCRecHitSoA *d_calibSoA_ = nullptr, *calibSoA_ = nullptr;
  KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA> *kmdata_;
  KernelConstantData<HGChefUncalibratedRecHitConstantData> *kcdata_;
};

#endif //RecoLocalCalo_HGCalRecProducers_HeterogeneousHGCalHEFRecHitProducer_h
