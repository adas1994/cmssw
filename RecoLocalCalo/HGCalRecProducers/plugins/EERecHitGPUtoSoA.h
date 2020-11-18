#ifndef RecoLocalCalo_HGCalRecProducers_EERecHitGPUtoSoA_h
#define RecoLocalCalo_HGCalRecProducers_EERecHitGPUtoSoA_h

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

class EERecHitGPUtoSoA : public edm::stream::EDProducer<edm::ExternalWork> {
public:
  explicit EERecHitGPUtoSoA(const edm::ParameterSet& ps);
  ~EERecHitGPUtoSoA() override;

  void acquire(edm::Event const&, edm::EventSetup const&, edm::WaitingTaskWithArenaHolder) override;
  void produce(edm::Event&, const edm::EventSetup&) override;

private:
  cms::cuda::ContextState ctxState_;
  edm::EDGetTokenT<cms::cuda::Product<RecHitGPUProduct>> recHitGPUToken_;
  edm::EDPutTokenT<HGCRecHitSoA> recHitCPUSoAToken_;

  void allocate_memory_(const uint32_t&, const uint32_t&, const uint32_t&, const cudaStream_t&);

  std::unique_ptr<HGCRecHitSoA> recHitsSoA_;
  HGCRecHitSoA* d_calibSoA_ = nullptr;
  std::byte* prodMem_;
};

#endif  //RecoLocalCalo_HGCalRecProducers_EERecHitGPUtoSoA_h
