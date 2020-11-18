#ifndef RecoLocalCalo_HGCalRecProducers_HEBRecHitFromSoA_h
#define RecoLocalCalo_HGCalRecProducers_HEBRecHitFromSoA_h

#include <iostream>
#include <string>
#include <memory>

#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/Exception.h"
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
#include "CUDADataFormats/HGCal/interface/HGCRecHitSoA.h"

class HEBRecHitFromSoA : public edm::stream::EDProducer<edm::ExternalWork> {
public:
  explicit HEBRecHitFromSoA(const edm::ParameterSet& ps);
  ~HEBRecHitFromSoA() override;

  void acquire(edm::Event const&, edm::EventSetup const&, edm::WaitingTaskWithArenaHolder) override;
  void produce(edm::Event&, const edm::EventSetup&) override;
  void convert_soa_data_to_collection_(const uint32_t&, HGCRecHitCollection&, HGCRecHitSoA*);

private:
  cms::cuda::ContextState ctxState_;
  std::unique_ptr<HGChefRecHitCollection> rechits_;
  edm::EDGetTokenT<HGCRecHitSoA> recHitSoAToken_;
  edm::EDPutTokenT<HGChefRecHitCollection> recHitCollectionToken_;
  const std::string collectionName_ = "HeterogeneousHGCalHEBRecHits";
};

#endif  //RecoLocalCalo_HGCalRecProducers_HEBRecHitFromSoA_h
