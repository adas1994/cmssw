#ifndef RecoLocalCalo_HGCalRecProducers_EERecHitGPU_h
#define RecoLocalCalo_HGCalRecProducers_EERecHitGPU_h

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

#include "RecoLocalCalo/HGCalRecProducers/plugins/KernelManagerHGCalRecHit.h"

#include "CUDADataFormats/HGCal/interface/RecHitGPUProduct.h"

class EERecHitGPU : public edm::stream::EDProducer<edm::ExternalWork> {
public:
  explicit EERecHitGPU(const edm::ParameterSet &ps);
  ~EERecHitGPU() override;
  void beginRun(edm::Run const &, edm::EventSetup const &) override;

  void acquire(edm::Event const &, edm::EventSetup const &, edm::WaitingTaskWithArenaHolder) override;
  void produce(edm::Event &, const edm::EventSetup &) override;

private:
  edm::EDGetTokenT<HGCeeUncalibratedRecHitCollection> uncalibRecHitCPUToken_;
  edm::EDPutTokenT<cms::cuda::Product<RecHitGPUProduct>> recHitGPUToken_;

  edm::Handle<HGCeeUncalibratedRecHitCollection> handle_;
  std::unique_ptr<HGCeeRecHitCollection> rechits_;
  cms::cuda::ContextState ctxState_;

  //constants
  HGCeeUncalibratedRecHitConstantData cdata_;
  HGCConstantVectorData vdata_;

  //memory
  std::string assert_error_message_(std::string, const size_t &, const size_t &);
  void assert_sizes_constants_(const HGCConstantVectorData &);
  void allocate_memory_(const cudaStream_t &);

  //conditions (geometry, topology, ...)
  std::unique_ptr<hgcal::RecHitTools> tools_;

  //data processing
  void convert_collection_data_to_soa_(const uint32_t &,
                                       const HGCeeUncalibratedRecHitCollection &,
                                       HGCUncalibratedRecHitSoA *);
  void convert_constant_data_(KernelConstantData<HGCeeUncalibratedRecHitConstantData> *);

  RecHitGPUProduct prod_;
  HGCUncalibratedRecHitSoA *h_uncalibSoA_ = nullptr;
  HGCUncalibratedRecHitSoA *d_uncalibSoA_ = nullptr;
  HGCRecHitSoA *d_calibSoA_ = nullptr;

  KernelConstantData<HGCeeUncalibratedRecHitConstantData> *kcdata_;
};

#endif  //RecoLocalCalo_HGCalRecProducers_EERecHitGPU_h
