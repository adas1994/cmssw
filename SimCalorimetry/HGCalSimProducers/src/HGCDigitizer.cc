#include "DataFormats/ForwardDetId/interface/ForwardSubdetector.h"
#include "DataFormats/ForwardDetId/interface/HFNoseDetId.h"
#include "DataFormats/ForwardDetId/interface/HGCalDetId.h"
#include "SimDataFormats/CaloTest/interface/HGCalTestNumbering.h"
#include "SimCalorimetry/HGCalSimProducers/interface/HGCDigitizer.h"
#include "SimGeneral/MixingModule/interface/PileUpEventPrincipal.h"
#include "SimDataFormats/CaloHit/interface/PCaloHitContainer.h"
#include "SimDataFormats/CaloHit/interface/PCaloHit.h"
#include "SimDataFormats/CrossingFrame/interface/CrossingFrame.h"
#include "SimDataFormats/CrossingFrame/interface/MixCollection.h"
#include "FWCore/Utilities/interface/RandomNumberGenerator.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/HGCalCommonData/interface/HGCalGeometryMode.h"
#include "Geometry/HcalCommonData/interface/HcalHitRelabeller.h"
#include "DataFormats/Math/interface/liblogintpack.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include "FWCore/Utilities/interface/transform.h"

//#define EDM_ML_DEBUG
using namespace std;
using namespace hgc_digi;

typedef std::vector<std::pair<float, float>>::iterator itr;
typedef std::unordered_map<uint32_t, std::vector<std::pair<float, float>>> IdHit_Map;
typedef std::tuple<float, float, float> hit_timeStamp;
typedef std::unordered_map<uint32_t, std::vector<hit_timeStamp>> hitRec_container;
typedef std::vector<hit_timeStamp>::iterator hitRec_itr;
typedef std::vector<hit_timeStamp>::const_iterator hitRec_citr;
namespace {

  constexpr std::array<double, 4> occupancyGuesses = {{0.5, 0.2, 0.2, 0.8}};

  float getPositionDistance(const HGCalGeometry* geom, const DetId& id) { return geom->getPosition(id).mag(); }

  float getPositionDistance(const HcalGeometry* geom, const DetId& id) {
    return geom->getGeometry(id)->getPosition().mag();
  }

  int getCellThickness(const HGCalGeometry* geom, const DetId& detid) {
    const auto& dddConst = geom->topology().dddConstants();
    return (1 + dddConst.waferType(detid));
  }

  int getCellThickness(const HcalGeometry* geom, const DetId& detid) { return 1; }

  void getValidDetIds(const HGCalGeometry* geom, std::unordered_set<DetId>& valid) {
    const std::vector<DetId>& ids = geom->getValidDetIds();
    valid.reserve(ids.size());
    valid.insert(ids.begin(), ids.end());
  }

  void getValidDetIds(const HcalGeometry* geom, std::unordered_set<DetId>& valid) {
    const std::vector<DetId>& ids = geom->getValidDetIds();
    for (const auto& id : ids) {
      if (HcalEndcap == id.subdetId() && DetId::Hcal == id.det())
        valid.emplace(id);
    }
    valid.reserve(valid.size());
  }

  DetId simToReco(const HcalGeometry* geom, unsigned simid) {
    DetId result(0);
    const auto& topo = geom->topology();
    const auto* dddConst = topo.dddConstants();
    HcalDetId id = HcalHitRelabeller::relabel(simid, dddConst);

    if (id.subdet() == int(HcalEndcap)) {
      result = id;
    }

    return result;
  }

  DetId simToReco(const HGCalGeometry* geom, unsigned simId) {
    DetId result(0);
    const auto& topo = geom->topology();
    const auto& dddConst = topo.dddConstants();

    if ((dddConst.geomMode() == HGCalGeometryMode::Hexagon8) ||
        (dddConst.geomMode() == HGCalGeometryMode::Hexagon8Full) ||
        (dddConst.geomMode() == HGCalGeometryMode::Trapezoid)) {
      result = DetId(simId);
    } else {
      int subdet(DetId(simId).subdetId()), layer, cell, sec, subsec, zp;
      HGCalTestNumbering::unpackHexagonIndex(simId, subdet, zp, layer, sec, subsec, cell);
      //sec is wafer and subsec is celltyp                                                          
      //skip this hit if after ganging it is not valid                                              
      auto recoLayerCell = dddConst.simToReco(cell, layer, sec, topo.detectorType());
      cell = recoLayerCell.first;
      layer = recoLayerCell.second;
      if (layer < 0 || cell < 0) {
        return result;
      } else {
        //assign the RECO DetId                                                                     
        result = HGCalDetId((ForwardSubdetector)subdet, zp, layer, subsec, sec, cell);
      }
    }
    return result;
  }


  void saveSimHitAccumulator_forPreMix(PHGCSimAccumulator& simResult,
                                       const hgc::HGCPUSimHitDataAccumulator& simData,
                                       const std::unordered_set<DetId>& validIds,
                                       const float minCharge,
                                       const float maxCharge,
				       const std::string digiCollection) {
    
    constexpr auto nEnergies = std::tuple_size<decltype(hgc_digi::HGCCellHitInfo().PUhit_info)>::value;
    static_assert(nEnergies <= PHGCSimAccumulator::SimHitCollection::energyMask + 1,
                  "PHGCSimAccumulator bit pattern needs to updated");
    static_assert(hgc_digi::nSamples <= PHGCSimAccumulator::SimHitCollection::sampleMask + 1,
                  "PHGCSimAccumulator bit pattern needs to updated");
    const float minPackChargeLog = minCharge > 0.f ? std::log(minCharge) : -2;
    const float maxPackChargeLog = std::log(maxCharge);
    constexpr uint16_t base = 1 << PHGCSimAccumulator::SimHitCollection::sampleOffset;
    
    simResult.reserve(simData.size());
    int count = 0;
    for (const auto& id : validIds) {
      auto found = simData.find(id);
      if (found == simData.end())
        continue;
      count++;
      const hgc_digi::PUSimHitData& accCharge_across_bxs = found->second.PUhit_info[0];
      const hgc_digi::PUSimHitData& timing_across_bxs = found->second.PUhit_info[1];
      for (size_t iSample = 0; iSample < hgc_digi::nSamples; ++iSample) {
	if(iSample == 9) {
	  const std::vector<float>& accCharge_inthis_bx = accCharge_across_bxs[iSample];
	  const std::vector<float>& timing_inthis_bx = timing_across_bxs[iSample];
	  std::vector<unsigned short> vc, vt;
	  size_t nhits = accCharge_inthis_bx.size();

	  for (size_t ihit = 0; ihit < nhits; ++ihit) {
	    if (accCharge_inthis_bx[ihit] > minCharge) {
	      unsigned short c =
                logintpack::pack16log(accCharge_inthis_bx[ihit], minPackChargeLog, maxPackChargeLog, base);
	      unsigned short t = logintpack::pack16log(timing_inthis_bx[ihit], minPackChargeLog, maxPackChargeLog, base);
	      vc.emplace_back(c);
	      vt.emplace_back(t);
	    }
	  }
	  
	  simResult.emplace_back(id.rawId(), iSample, vc, vt);
	  float accCharge_thisBx = found->second.thisBx_accCharge_after_CellFireOff;
	  if(accCharge_thisBx > minCharge) {
	    unsigned short c = logintpack::pack16log(accCharge_thisBx, minPackChargeLog, maxPackChargeLog, base);
	    simResult.emplace_back(id.rawId(), iSample, c);
	    
	  }
	} else {
	  
	  float accCharge_outOfTime = 0;
	  const std::vector<float>& accCharge_inthis_bx = accCharge_across_bxs[iSample];
	  size_t numHits = accCharge_inthis_bx.size();
	  if(numHits > 0) {
	    for(size_t ihit = 0; ihit < numHits; ++ihit) {
	      float charge = accCharge_inthis_bx[ihit];
	      accCharge_outOfTime += charge;
	    
	    }
	  }
	  if(accCharge_outOfTime > minCharge) {
	    unsigned short c = logintpack::pack16log(accCharge_outOfTime, minPackChargeLog, maxPackChargeLog, base);
	    simResult.emplace_back(id.rawId(), iSample, c);
	    
	  }
	}
      }
    }
    
    
    simResult.shrink_to_fit();
  }

  template <typename GEOM>
  void loadSimHitAccumulator_forPreMix(hgc::HGCSimHitDataAccumulator& simData,
                                       const GEOM* geom,
                                       IdHit_Map& hitRefs_bx0,
                                       const PHGCSimAccumulator& simAccumulator,
                                       const float minCharge,
                                       const float maxCharge,
                                       bool setIfZero,
                                       const std::array<float, 3>& tdcForToAOnset,
                                       const bool minbiasFlag,
                                       std::unordered_map<uint32_t, bool>& hitOrder_monitor,
                                       const unsigned int thisBx) {
    
    const float minPackChargeLog = minCharge > 0.f ? std::log(minCharge) : -2;
    const float maxPackChargeLog = std::log(maxCharge);
    constexpr uint16_t base = 1 << PHGCSimAccumulator::SimHitCollection::sampleOffset;
    for (const auto& detIdIndexHitInfo : simAccumulator) {
      
      
      unsigned int detId = detIdIndexHitInfo.detId();
      
      auto simIt = simData.emplace(detId, HGCCellInfo()).first;
      
      size_t nhits = detIdIndexHitInfo.nhits();
      
      hitOrder_monitor[detId] = false;
      
      if (nhits > 0) {
	
        unsigned short iSample = detIdIndexHitInfo.sampleIndex();
	
	bool is_accumulated = detIdIndexHitInfo.isAccumulated();
        float p_charge, p_time;
        unsigned short unsigned_charge, unsigned_time;
	if(is_accumulated == false && iSample == (unsigned short)thisBx) {
	  const auto& unsigned_charge_array = detIdIndexHitInfo.chargeArray();
	  const auto& unsigned_time_array = detIdIndexHitInfo.timeArray();
	  
	  for (size_t ihit = 0; ihit < nhits; ++ihit) {
	    unsigned_charge = (unsigned_charge_array[ihit] & PHGCSimAccumulator::SimHitCollection::dataMask);
	    unsigned_time = (unsigned_time_array[ihit] & PHGCSimAccumulator::SimHitCollection::dataMask);
	    p_time = logintpack::unpack16log(unsigned_time, minPackChargeLog, maxPackChargeLog, base);
	    p_charge = logintpack::unpack16log(unsigned_charge, minPackChargeLog, maxPackChargeLog, base);

	    (simIt->second).hit_info[0][iSample] += p_charge;
	    
	    if (hitRefs_bx0[detId].empty()) {
	      hitRefs_bx0[detId].emplace_back(p_charge, p_time);
	    } else if(p_time <= hitRefs_bx0[detId].back().second) {
	      
	      itr findPos = std::upper_bound(hitRefs_bx0[detId].begin(),
						hitRefs_bx0[detId].end(),
						std::make_pair(0.f, p_time),
						[](const auto& i, const auto& j) { return i.second <= j.second; });

	      itr insertedPos = findPos;
	      if(findPos->second == p_time)
		findPos->first += p_charge;
	      else {
		insertedPos = hitRefs_bx0[detId].insert(findPos,
							(findPos == hitRefs_bx0[detId].begin())
							? std::pair<float, float>(p_charge, p_time)
							: std::pair<float, float>((findPos - 1)->first + p_charge, p_time));
	      }
	      for(itr step = insertedPos; step != hitRefs_bx0[detId].end(); ++step) {
		if (step != insertedPos)
		  step->first += p_charge;		
	      }
	      hitOrder_monitor[detId] = true;
	    }  
	    else {
	      hitRefs_bx0[detId].emplace_back(hitRefs_bx0[detId].back().first + p_charge, p_time);
	    }
	  }
	}
	else {
	  unsigned_charge = detIdIndexHitInfo.accCharge() & PHGCSimAccumulator::SimHitCollection::dataMask;
	  p_charge = logintpack::unpack16log(unsigned_charge, minPackChargeLog, maxPackChargeLog, base);
	  (simIt->second).hit_info[0][iSample] += p_charge;

	}
      }
    }
    if (minbiasFlag) {
      for (const auto& hitmapIterator : hitRefs_bx0) {
        unsigned int detectorId = hitmapIterator.first;
        auto simIt = simData.emplace(detectorId, HGCCellInfo()).first;
        const bool orderChanged = hitOrder_monitor[detectorId];
        int waferThickness = getCellThickness(geom, detectorId);
        float cell_threshold = tdcForToAOnset[waferThickness - 1];
        const auto& hitRec = hitmapIterator.second;
        float accChargeForToA(0.f), fireTDC(0.f);
        const auto aboveThrPos = std::upper_bound(
            hitRec.begin(), hitRec.end(), std::make_pair(cell_threshold, 0.f), [](const auto& i, const auto& j) {
              return i.first < j.first;
            });

        if (aboveThrPos == hitRec.end()) {
          accChargeForToA = hitRec.back().first;
          fireTDC = 0.f;
        } else if (hitRec.end() - aboveThrPos > 0 || orderChanged) {
          accChargeForToA = aboveThrPos->first;
          fireTDC = aboveThrPos->second;
          if (aboveThrPos - hitRec.begin() >= 1) {
            const auto& belowThrPos = aboveThrPos - 1;
            float chargeBeforeThr = belowThrPos->first;
            float timeBeforeThr = belowThrPos->second;
            float deltaQ = accChargeForToA - chargeBeforeThr;
            float deltaTOF = fireTDC - timeBeforeThr;
            fireTDC = (cell_threshold - chargeBeforeThr) * deltaTOF / deltaQ + timeBeforeThr;
          }
        }
        (simIt->second).hit_info[1][9] = fireTDC;
      }
    }
    //std::cout<<"Exiting Load"<<std::endl;
    /*for(const auto& x : simData){
      const auto xf = x.first;
      //std::cout<<"-------------- "<<xf<<" -----------------"<<std::endl;
      const auto xs = x.second.hit_info[0];
      //for(size_t i =0; i < 15; ++i) {
	//if(xs[i] > 0)
	//std::cout<<i<<"\t"<<xs[i]<<std::endl;
      //}

      }*/
  }
  
}  //namespace

HGCDigitizer::HGCDigitizer(const edm::ParameterSet& ps, edm::ConsumesCollector& iC)
    : simHitAccumulator_(new HGCSimHitDataAccumulator()),
      pusimHitAccumulator_(new HGCPUSimHitDataAccumulator()),
      myDet_(DetId::Forward),
      mySubDet_(ForwardSubdetector::ForwardEmpty),
      refSpeed_(0.1 * CLHEP::c_light),  //[CLHEP::c_light]=mm/ns convert to cm/ns
      averageOccupancies_(occupancyGuesses),
      nEvents_(1) {
  //configure from cfg

  hitCollection_ = ps.getParameter<std::string>("hitCollection");
  digiCollection_ = ps.getParameter<std::string>("digiCollection");
  maxSimHitsAccTime_ = ps.getParameter<uint32_t>("maxSimHitsAccTime");
  bxTime_ = ps.getParameter<double>("bxTime");
  geometryType_ = ps.getParameter<uint32_t>("geometryType");
  digitizationType_ = ps.getParameter<uint32_t>("digitizationType");
  verbosity_ = ps.getUntrackedParameter<uint32_t>("verbosity", 0);
  tofDelay_ = ps.getParameter<double>("tofDelay");
  premixStage1_ = ps.getParameter<bool>("premixStage1");
  premixStage1MinCharge_ = ps.getParameter<double>("premixStage1MinCharge");
  premixStage1MaxCharge_ = ps.getParameter<double>("premixStage1MaxCharge");
  std::unordered_set<DetId>().swap(validIds_);
  iC.consumes<std::vector<PCaloHit>>(edm::InputTag("g4SimHits", hitCollection_));
  const auto& myCfg_ = ps.getParameter<edm::ParameterSet>("digiCfg");

  if (myCfg_.existsAs<edm::ParameterSet>("chargeCollectionEfficiencies")) {
    cce_.clear();
    const auto& temp = myCfg_.getParameter<edm::ParameterSet>("chargeCollectionEfficiencies")
                           .getParameter<std::vector<double>>("values");
    for (double cce : temp) {
      cce_.emplace_back(cce);
    }
  } else {
    std::vector<float>().swap(cce_);
  }

  if (hitCollection_.find("HitsEE") != std::string::npos) {
    if (geometryType_ == 0) {
      mySubDet_ = ForwardSubdetector::HGCEE;
    } else {
      myDet_ = DetId::HGCalEE;
    }
    theHGCEEDigitizer_ = std::make_unique<HGCEEDigitizer>(ps);
  }
  if (hitCollection_.find("HitsHEfront") != std::string::npos) {
    if (geometryType_ == 0) {
      mySubDet_ = ForwardSubdetector::HGCHEF;
    } else {
      myDet_ = DetId::HGCalHSi;
    }
    theHGCHEfrontDigitizer_ = std::make_unique<HGCHEfrontDigitizer>(ps);
  }
  if (hitCollection_.find("HcalHits") != std::string::npos and geometryType_ == 0) {
    mySubDet_ = ForwardSubdetector::HGCHEB;
    theHGCHEbackDigitizer_ = std::make_unique<HGCHEbackDigitizer>(ps);
  }
  if (hitCollection_.find("HitsHEback") != std::string::npos and geometryType_ == 1) {
    myDet_ = DetId::HGCalHSc;
    theHGCHEbackDigitizer_ = std::make_unique<HGCHEbackDigitizer>(ps);
  }
  if (hitCollection_.find("HFNoseHits") != std::string::npos) {
    mySubDet_ = ForwardSubdetector::HFNose;
    myDet_ = DetId::Forward;
    theHFNoseDigitizer_ = std::make_unique<HFNoseDigitizer>(ps);
  }
}

//
void HGCDigitizer::initializeEvent(edm::Event const& e, edm::EventSetup const& es) {
  // reserve memory for a full detector
  
  unsigned idx = getType();
  simHitAccumulator_->reserve(averageOccupancies_[idx] * validIds_.size());
  pusimHitAccumulator_->reserve(averageOccupancies_[idx] * validIds_.size());
  
}

//
void HGCDigitizer::finalizeEvent(edm::Event& e, edm::EventSetup const& es, CLHEP::HepRandomEngine* hre) {
  
  hitRefs_bx0.clear();
  PhitRefs_bx0.clear();
  hitOrder_monitor.clear();
  //accCharge_after_cellFireOff_bx0.clear();
  const CaloSubdetectorGeometry* theGeom = (nullptr == gHGCal_ ? static_cast<const CaloSubdetectorGeometry*>(gHcal_)
                                                               : static_cast<const CaloSubdetectorGeometry*>(gHGCal_));

  ++nEvents_;

  unsigned idx = getType();
  // release memory for unfilled parts of hash table
  

  if (validIds_.size() * averageOccupancies_[idx] > simHitAccumulator_->size()) {
    simHitAccumulator_->reserve(simHitAccumulator_->size());
    pusimHitAccumulator_->reserve(simHitAccumulator_->size());
  }
  
  //update occupancy guess
  const double thisOcc = simHitAccumulator_->size() / ((double)validIds_.size());
  averageOccupancies_[idx] = (averageOccupancies_[idx] * (nEvents_ - 1) + thisOcc) / nEvents_;
  
  if (premixStage1_) {
    auto simRecord = std::make_unique<PHGCSimAccumulator>();
    
    if (!pusimHitAccumulator_->empty()) {
      saveSimHitAccumulator_forPreMix(
				      *simRecord, *pusimHitAccumulator_, validIds_, 
				      premixStage1MinCharge_, premixStage1MaxCharge_, digiCollection_);
    }
    
    e.put(std::move(simRecord), digiCollection());
    
  } else {
    if (producesEEDigis()) {
      
      auto digiResult = std::make_unique<HGCalDigiCollection>();
      theHGCEEDigitizer_->run(digiResult, *simHitAccumulator_, theGeom, validIds_, digitizationType_, hre);
      
      edm::LogVerbatim("HGCDigitizer") << "HGCDigitizer:: finalize event - produced " << digiResult->size()
                                       << " EE hits";
#ifdef EDM_ML_DEBUG
      checkPosition(&(*digiResult));
#endif
      e.put(std::move(digiResult), digiCollection());
    }
    if (producesHEfrontDigis()) {
      auto digiResult = std::make_unique<HGCalDigiCollection>();
      theHGCHEfrontDigitizer_->run(digiResult, *simHitAccumulator_, theGeom, validIds_, digitizationType_, hre);
      edm::LogVerbatim("HGCDigitizer") << "HGCDigitizer:: finalize event - produced " << digiResult->size()
                                       << " HE silicon hits";
#ifdef EDM_ML_DEBUG
      checkPosition(&(*digiResult));
#endif
      e.put(std::move(digiResult), digiCollection());
    }
    if (producesHEbackDigis()) {
      auto digiResult = std::make_unique<HGCalDigiCollection>();
      theHGCHEbackDigitizer_->run(digiResult, *simHitAccumulator_, theGeom, validIds_, digitizationType_, hre);
      edm::LogVerbatim("HGCDigitizer") << "HGCDigitizer:: finalize event - produced " << digiResult->size()
                                       << " HE Scintillator hits";
#ifdef EDM_ML_DEBUG
      checkPosition(&(*digiResult));
#endif
      e.put(std::move(digiResult), digiCollection());
    }
    if (producesHFNoseDigis()) {
      auto digiResult = std::make_unique<HGCalDigiCollection>();
      theHFNoseDigitizer_->run(digiResult, *simHitAccumulator_, theGeom, validIds_, digitizationType_, hre);
      edm::LogVerbatim("HGCDigitizer") << "HGCDigitizer:: finalize event - produced " << digiResult->size()
                                       << " HFNose hits";
#ifdef EDM_ML_DEBUG
      checkPosition(&(*digiResult));
#endif
      
      e.put(std::move(digiResult), digiCollection());
      
    }
  }
  
  hgc::HGCSimHitDataAccumulator().swap(*simHitAccumulator_);
  hgc::HGCPUSimHitDataAccumulator().swap(*pusimHitAccumulator_);
  
}
void HGCDigitizer::accumulate_forPreMix(edm::Event const& e,
                                        edm::EventSetup const& eventSetup,
                                        CLHEP::HepRandomEngine* hre) {
  //get inputs
  
  edm::Handle<edm::PCaloHitContainer> hits;
  e.getByLabel(edm::InputTag("g4SimHits", hitCollection_), hits);
  if (!hits.isValid()) {
    edm::LogError("HGCDigitizer") << " @ accumulate_minbias : can't find " << hitCollection_
                                  << " collection of g4SimHits";
    return;
  }

  //accumulate in-time the main event
  if (nullptr != gHGCal_) {
    accumulate_forPreMix(hits, 0, gHGCal_, hre);
  } else if (nullptr != gHcal_) {
    accumulate_forPreMix(hits, 0, gHcal_, hre);
  } else {
    throw cms::Exception("BadConfiguration") << "HGCDigitizer is not producing EE, FH, or BH digis!";
  }
}

//
void HGCDigitizer::accumulate(edm::Event const& e, edm::EventSetup const& eventSetup, CLHEP::HepRandomEngine* hre) {
  //get inputs
  edm::Handle<edm::PCaloHitContainer> hits;
  e.getByLabel(edm::InputTag("g4SimHits", hitCollection_), hits);
  if (!hits.isValid()) {
    edm::LogError("HGCDigitizer") << " @ accumulate : can't find " << hitCollection_ << " collection of g4SimHits";
    return;
  }

  //accumulate in-time the main event
  if (nullptr != gHGCal_) {
    accumulate(hits, 0, gHGCal_, hre);
  } else if (nullptr != gHcal_) {
    accumulate(hits, 0, gHcal_, hre);
  } else {
    throw cms::Exception("BadConfiguration") << "HGCDigitizer is not producing EE, FH, or BH digis!";
  }
}

//
void HGCDigitizer::accumulate_forPreMix(PileUpEventPrincipal const& e,
                                        edm::EventSetup const& eventSetup,
                                        CLHEP::HepRandomEngine* hre) {
  
  edm::Handle<edm::PCaloHitContainer> hits;
  e.getByLabel(edm::InputTag("g4SimHits", hitCollection_), hits);

  if (!hits.isValid()) {
    edm::LogError("HGCDigitizer") << " @ accumulate : can't find " << hitCollection_ << " collection of g4SimHits";
    return;
  }

  if (nullptr != gHGCal_) {
    accumulate_forPreMix(hits, e.bunchCrossing(), gHGCal_, hre);
  } else if (nullptr != gHcal_) {
    accumulate_forPreMix(hits, e.bunchCrossing(), gHcal_, hre);
  } else {
    throw cms::Exception("BadConfiguration") << "HGCDigitizer is not producing EE, FH, or BH digis!";
  }
}

//
void HGCDigitizer::accumulate(PileUpEventPrincipal const& e,
                              edm::EventSetup const& eventSetup,
                              CLHEP::HepRandomEngine* hre) {
  //get inputs
  edm::Handle<edm::PCaloHitContainer> hits;
  e.getByLabel(edm::InputTag("g4SimHits", hitCollection_), hits);

  if (!hits.isValid()) {
    edm::LogError("HGCDigitizer") << " @ accumulate : can't find " << hitCollection_ << " collection of g4SimHits";
    return;
  }

  //accumulate for the simulated bunch crossing
  if (nullptr != gHGCal_) {
    accumulate(hits, e.bunchCrossing(), gHGCal_, hre);
  } else if (nullptr != gHcal_) {
    accumulate(hits, e.bunchCrossing(), gHcal_, hre);
  } else {
    throw cms::Exception("BadConfiguration") << "HGCDigitizer is not producing EE, FH, or BH digis!";
  }
}

//
template <typename GEOM>
void HGCDigitizer::accumulate_forPreMix(edm::Handle<edm::PCaloHitContainer> const& hits,
                                        int bxCrossing,
                                        const GEOM* geom,
                                        CLHEP::HepRandomEngine* hre) {
  if (nullptr == geom)
    return;
  
  //configuration to apply for the computation of time-of-flight
  std::array<float, 3> tdcForToAOnset{{0.f, 0.f, 0.f}};
  float keV2fC(0.f);
  bool weightToAbyEnergy = getWeight(tdcForToAOnset, keV2fC);

  int nchits = (int)hits->size();
  int count_thisbx = 0;
  std::vector<HGCCaloHitTuple_t> hitRefs;
  hitRefs.reserve(nchits);
  for (int i = 0; i < nchits; ++i) {
    const auto& the_hit = hits->at(i);
    DetId id = simToReco(geom, the_hit.id());
    // to be written the verbosity block
    if (id.rawId() != 0) {
      hitRefs.emplace_back(i, id.rawId(), (float)the_hit.time());
    }
  }
  std::sort(hitRefs.begin(), hitRefs.end(), this->orderByDetIdThenTime);
  
  nchits = hitRefs.size();
  for (int i = 0; i < nchits; ++i) {
    const int hitidx = std::get<0>(hitRefs[i]);
    const uint32_t id = std::get<1>(hitRefs[i]);
    if (!validIds_.count(id))
      continue;

    if (id == 0)
      continue;

    const float toa = std::get<2>(hitRefs[i]);
    const PCaloHit& hit = hits->at(hitidx);
    const float charge = hit.energy() * 1e6 * keV2fC;  // * getCCE(geom, id, cce_);

    const float dist2center(getPositionDistance(geom, id));
    const float tof = toa - dist2center / refSpeed_ + tofDelay_;
    const int itime = std::floor(tof / bxTime_) + 9;

    if (itime < 0 || itime > (int)maxBx_)
      continue;

    if (itime >= (int)(maxBx_ + 1))
      continue;

    int waferThickness = getCellThickness(geom, id);
    if (itime == (int)thisBx_) {
      ++count_thisbx;
      if (PhitRefs_bx0[id].empty()) {
        PhitRefs_bx0[id].emplace_back(charge, charge, tof);
      } else if (tof > std::get<2>(PhitRefs_bx0[id].back())) {
        PhitRefs_bx0[id].emplace_back(charge, charge + std::get<1>(PhitRefs_bx0[id].back()), tof);
      } 
        
        
      else {
        //find position to insert new entry preserving time sorting
        hitRec_itr findPos =
	  std::upper_bound(PhitRefs_bx0[id].begin(),
			   PhitRefs_bx0[id].end(),
			   hit_timeStamp(charge, 0.f, tof),
			   [](const auto& i, const auto& j) { return std::get<2>(i) <= std::get<2>(j); });

        hitRec_itr insertedPos = findPos;
	if (std::get<2>(*(findPos)) == tof) {
	  std::get<0>(*(findPos - 1)) += charge;
	  std::get<1>(*(findPos - 1)) += charge;
	}
	else {
          insertedPos = PhitRefs_bx0[id].insert(findPos,
                                                (findPos == PhitRefs_bx0[id].begin())
						? hit_timeStamp(charge, charge, tof)
						: hit_timeStamp(charge, charge + std::get<1>(*(findPos - 1)), tof));
        }
        //cumulate the charge of new entry for all elements that follow in the sorted list
        //and resize list accounting for cases when the inserted element itself crosses the threshold

        for (hitRec_itr step = insertedPos; step != PhitRefs_bx0[id].end(); ++step) {
          if (step != insertedPos)
            std::get<1>(*(step)) += charge;
        }
      }//(PhitRefs_bx0[id].empty())
    } else {
      auto simHitIt = pusimHitAccumulator_->emplace(id, HGCCellHitInfo()).first;
      if(itime >= 0 && (size_t)itime < hgc_digi::nSamples){
        (simHitIt->second).PUhit_info[0][(size_t)itime].push_back(charge);
        //(simHitIt->second).PUhit_info[1][(size_t)itime].push_back(tof);
      }
    }
  }

  for (const auto& hitCollection : PhitRefs_bx0) {
    const uint32_t detectorId = hitCollection.first;
    int waferThickness = getCellThickness(geom, detectorId);
    auto simHitIt = pusimHitAccumulator_->emplace(detectorId, HGCCellHitInfo()).first;
    hitRec_citr threshold_crossPointPtr = 
      std::upper_bound(PhitRefs_bx0[detectorId].begin(),
		       PhitRefs_bx0[detectorId].end(),
		       hit_timeStamp(0.f, tdcForToAOnset[waferThickness - 1], 0.f),
		       [](const auto& i, const auto& j) { return std::get<1>(i) < std::get<1>(j); });
    hitRec_citr endPoint = threshold_crossPointPtr == PhitRefs_bx0[detectorId].end() ? PhitRefs_bx0[detectorId].end()-1 : threshold_crossPointPtr;
    hitRec_citr accumulationPoint = threshold_crossPointPtr == PhitRefs_bx0[detectorId].end() ? PhitRefs_bx0[detectorId].end() : threshold_crossPointPtr+1;
    for(hitRec_citr ix = PhitRefs_bx0[detectorId].begin(); ix !=endPoint+1; ++ix) {
      const hit_timeStamp& hit_timestamp = *(ix);
      (simHitIt->second).PUhit_info[1][thisBx_].push_back(std::get<2>(hit_timestamp));
      (simHitIt->second).PUhit_info[0][thisBx_].push_back(std::get<0>(hit_timestamp));
    }
    if(accumulationPoint != PhitRefs_bx0[detectorId].end()) {
      //auto im = accCharge_after_cellFireOff_bx0.emplace(detectorId, 0.0).first;
      float accDeposition = 0.0;
      for(hitRec_citr iy = accumulationPoint; iy != PhitRefs_bx0[detectorId].end(); ++iy) {
	accDeposition += std::get<0>(*iy);
      }
      //(*im).second = accDeposition;
      (simHitIt->second).thisBx_accCharge_after_CellFireOff = accDeposition;
    }
    //endPoint = nullptr;
    //accumulationPoint = nullptr;
  }

  if (nchits == 0) {
    HGCPUSimHitDataAccumulator::iterator simHitIt = pusimHitAccumulator_->emplace(0, HGCCellHitInfo()).first;
    (simHitIt->second).PUhit_info[1][9].push_back(0.0);
    (simHitIt->second).PUhit_info[0][9].push_back(0.0);
  }
  hitRefs.clear();
  PhitRefs_bx0.clear();
  
  
  //accCharge_after_cellFireOff_bx0.clear();
  
}

//
template <typename GEOM>
void HGCDigitizer::accumulate(edm::Handle<edm::PCaloHitContainer> const& hits,
                              int bxCrossing,
                              const GEOM* geom,
                              CLHEP::HepRandomEngine* hre) {
  if (nullptr == geom)
    return;

  //configuration to apply for the computation of time-of-flight
  std::array<float, 3> tdcForToAOnset{{0.f, 0.f, 0.f}};
  float keV2fC(0.f);
  bool weightToAbyEnergy = getWeight(tdcForToAOnset, keV2fC);

  //create list of tuples (pos in container, RECO DetId, time) to be sorted first
  int nchits = (int)hits->size();

  std::vector<HGCCaloHitTuple_t> hitRefs;
  hitRefs.reserve(nchits);
  for (int i = 0; i < nchits; ++i) {
    const auto& the_hit = hits->at(i);
    DetId id = simToReco(geom, the_hit.id());

    if (verbosity_ > 0) {
      if (producesEEDigis())
        edm::LogVerbatim("HGCDigitizer") << "HGCDigitizer::i/p " << std::hex << the_hit.id() << " o/p " << id.rawId()
                                         << std::dec;
      else
        edm::LogVerbatim("HGCDigitizer") << "HGCDigitizer::i/p " << std::hex << the_hit.id() << " o/p " << id.rawId()
                                         << std::dec;
    }

    if (0 != id.rawId()) {
      hitRefs.emplace_back(i, id.rawId(), (float)the_hit.time());
    }
  }

  std::sort(hitRefs.begin(), hitRefs.end(), this->orderByDetIdThenTime);
  //loop over sorted hits
  nchits = hitRefs.size();
  for (int i = 0; i < nchits; ++i) {
    const int hitidx = std::get<0>(hitRefs[i]);
    const uint32_t id = std::get<1>(hitRefs[i]);

    //get the data for this cell, if not available then we skip it

    if (!validIds_.count(id))
      continue;
    HGCSimHitDataAccumulator::iterator simHitIt = simHitAccumulator_->emplace(id, HGCCellInfo()).first;

    if (id == 0)
      continue;  // to be ignored at RECO level

    const float toa = std::get<2>(hitRefs[i]);
    const PCaloHit& hit = hits->at(hitidx);
    const float charge = hit.energy() * 1e6 * keV2fC;

    //distance to the center of the detector
    const float dist2center(getPositionDistance(geom, id));

    //hit time: [time()]=ns  [centerDist]=cm [refSpeed_]=cm/ns + delay by 1ns
    //accumulate in 15 buckets of 25ns (9 pre-samples, 1 in-time, 5 post-samples)
    const float tof = toa - dist2center / refSpeed_ + tofDelay_;
    const int itime = std::floor(tof / bxTime_) + 9;

    //no need to add bx crossing - tof comes already corrected from the mixing module
    //itime += bxCrossing;
    //itime += 9;

    if (itime < 0 || itime > (int)maxBx_)
      continue;

    //check if time index is ok and store energy
    if (itime >= (int)simHitIt->second.hit_info[0].size())
      continue;

    (simHitIt->second).hit_info[0][itime] += charge;

    //for time-of-arrival: save the time-sorted list of timestamps with cumulative charge just above threshold
    //working version with pileup only for in-time hits
    int waferThickness = getCellThickness(geom, id);
    bool orderChanged = false;
    if (itime == (int)thisBx_) {
      //if start empty => just add charge and time
      if (hitRefs_bx0[id].empty()) {
        hitRefs_bx0[id].emplace_back(charge, tof);

      } else if (tof <= hitRefs_bx0[id].back().second) {
        //find position to insert new entry preserving time sorting
        std::vector<std::pair<float, float>>::iterator findPos =
            std::upper_bound(hitRefs_bx0[id].begin(),
                             hitRefs_bx0[id].end(),
                             std::pair<float, float>(0.f, tof),
                             [](const auto& i, const auto& j) { return i.second <= j.second; });

        std::vector<std::pair<float, float>>::iterator insertedPos = findPos;
        if (findPos->second == tof) {
          //just merge timestamps with exact timing
          findPos->first += charge;
        } else {
          //insert new element cumulating the charge
          insertedPos = hitRefs_bx0[id].insert(findPos,
                                               (findPos == hitRefs_bx0[id].begin())
                                                   ? std::pair<float, float>(charge, tof)
                                                   : std::pair<float, float>((findPos - 1)->first + charge, tof));
        }

        //cumulate the charge of new entry for all elements that follow in the sorted list
        //and resize list accounting for cases when the inserted element itself crosses the threshold
        for (std::vector<std::pair<float, float>>::iterator step = insertedPos; step != hitRefs_bx0[id].end(); ++step) {
          if (step != insertedPos)
            step->first += charge;
          // resize the list stopping with the first timeStamp with cumulative charge above threshold
          if (step->first > tdcForToAOnset[waferThickness - 1] && step->second != hitRefs_bx0[id].back().second) {
            hitRefs_bx0[id].resize(std::upper_bound(hitRefs_bx0[id].begin(),
                                                    hitRefs_bx0[id].end(),
                                                    std::pair<float, float>(0.f, step->second),
                                                    [](const auto& i, const auto& j) { return i.second < j.second; }) -
                                   hitRefs_bx0[id].begin());
            for (auto stepEnd = step + 1; stepEnd != hitRefs_bx0[id].end(); ++stepEnd)
              stepEnd->first += charge;
            break;
          }
        }

        orderChanged = true;
      } else {
        //add new entry at the end of the list
        if (hitRefs_bx0[id].back().first <= tdcForToAOnset[waferThickness - 1]) {
          hitRefs_bx0[id].emplace_back(hitRefs_bx0[id].back().first + charge, tof);
        }
      }
    }
    float accChargeForToA = hitRefs_bx0[id].empty() ? 0.f : hitRefs_bx0[id].back().first;
    //now compute the firing ToA through the interpolation of the consecutive time-stamps at threshold
    if (weightToAbyEnergy)
      (simHitIt->second).hit_info[1][itime] += charge * tof;
    else if (accChargeForToA > tdcForToAOnset[waferThickness - 1] &&
             ((simHitIt->second).hit_info[1][itime] == 0 || orderChanged == true)) {
      float fireTDC = hitRefs_bx0[id].back().second;
      if (hitRefs_bx0[id].size() > 1) {
        float chargeBeforeThr = (hitRefs_bx0[id].end() - 2)->first;
        float tofchargeBeforeThr = (hitRefs_bx0[id].end() - 2)->second;

        float deltaQ = accChargeForToA - chargeBeforeThr;
        float deltaTOF = fireTDC - tofchargeBeforeThr;
        fireTDC = (tdcForToAOnset[waferThickness - 1] - chargeBeforeThr) * deltaTOF / deltaQ + tofchargeBeforeThr;
      }
      (simHitIt->second).hit_info[1][itime] = fireTDC;
    }
  }
  hitRefs.clear();
}
void HGCDigitizer::accumulate_forPreMix(const PHGCSimAccumulator& simAccumulator, const bool minbiasFlag) {
  //configuration to apply for the computation of time-of-flight
  std::array<float, 3> tdcForToAOnset{{0.f, 0.f, 0.f}};
  float keV2fC(0.f);
  bool weightToAbyEnergy = getWeight(tdcForToAOnset, keV2fC);

  if (nullptr != gHGCal_) {
    loadSimHitAccumulator_forPreMix(*simHitAccumulator_,
                                    gHGCal_,
                                    hitRefs_bx0,
                                    simAccumulator,
                                    premixStage1MinCharge_,
                                    premixStage1MaxCharge_,
                                    !weightToAbyEnergy,
                                    tdcForToAOnset,
                                    minbiasFlag,
                                    hitOrder_monitor,
                                    thisBx_);
  } else if (nullptr != gHcal_) {
    loadSimHitAccumulator_forPreMix(*simHitAccumulator_,
                                    gHcal_,
                                    hitRefs_bx0,
                                    simAccumulator,
                                    premixStage1MinCharge_,
                                    premixStage1MaxCharge_,
                                    !weightToAbyEnergy,
                                    tdcForToAOnset,
                                    minbiasFlag,
                                    hitOrder_monitor,
                                    thisBx_);
  }
}

//
void HGCDigitizer::beginRun(const edm::EventSetup& es) {
  //get geometry
  edm::ESHandle<CaloGeometry> geom;
  es.get<CaloGeometryRecord>().get(geom);

  gHGCal_ = nullptr;
  gHcal_ = nullptr;

  if (producesEEDigis())
    gHGCal_ = dynamic_cast<const HGCalGeometry*>(geom->getSubdetectorGeometry(myDet_, mySubDet_));
  if (producesHEfrontDigis())
    gHGCal_ = dynamic_cast<const HGCalGeometry*>(geom->getSubdetectorGeometry(myDet_, mySubDet_));
  if (producesHFNoseDigis())
    gHGCal_ = dynamic_cast<const HGCalGeometry*>(geom->getSubdetectorGeometry(myDet_, mySubDet_));

  if (producesHEbackDigis()) {
    if (geometryType_ == 0) {
      gHcal_ = dynamic_cast<const HcalGeometry*>(geom->getSubdetectorGeometry(DetId::Hcal, HcalEndcap));
    } else {
      gHGCal_ = dynamic_cast<const HGCalGeometry*>(geom->getSubdetectorGeometry(myDet_, mySubDet_));
    }
  }

  int nadded(0);
  //valid ID lists
  if (nullptr != gHGCal_) {
    getValidDetIds(gHGCal_, validIds_);
  } else if (nullptr != gHcal_) {
    getValidDetIds(gHcal_, validIds_);
  } else {
    throw cms::Exception("BadConfiguration") << "HGCDigitizer is not producing EE, FH, or BH digis!";
  }

  if (verbosity_ > 0)
    edm::LogInfo("HGCDigitizer") << "Added " << nadded << ":" << validIds_.size() << " detIds without "
                                 << hitCollection_ << " in first event processed" << std::endl;
}

//
void HGCDigitizer::endRun() { std::unordered_set<DetId>().swap(validIds_); }

//
void HGCDigitizer::resetSimHitDataAccumulator() {
  for (HGCSimHitDataAccumulator::iterator it = simHitAccumulator_->begin(); it != simHitAccumulator_->end(); it++) {
    it->second.hit_info[0].fill(0.);
    it->second.hit_info[1].fill(0.);
  }
}

uint32_t HGCDigitizer::getType() const {
  uint32_t idx = std::numeric_limits<unsigned>::max();
  if (geometryType_ == 0) {
    switch (mySubDet_) {
      case ForwardSubdetector::HGCEE:
        idx = 0;
        break;
      case ForwardSubdetector::HGCHEF:
        idx = 1;
        break;
      case ForwardSubdetector::HGCHEB:
        idx = 2;
        break;
      case ForwardSubdetector::HFNose:
        idx = 3;
        break;
      default:
        break;
    }
  } else {
    switch (myDet_) {
      case DetId::HGCalEE:
        idx = 0;
        break;
      case DetId::HGCalHSi:
        idx = 1;
        break;
      case DetId::HGCalHSc:
        idx = 2;
        break;
      case DetId::Forward:
        idx = 3;
        break;
      default:
        break;
    }
  }
  return idx;
}

bool HGCDigitizer::getWeight(std::array<float, 3>& tdcForToAOnset, float& keV2fC) const {
  bool weightToAbyEnergy(false);
  if (geometryType_ == 0) {
    switch (mySubDet_) {
      case ForwardSubdetector::HGCEE:
        weightToAbyEnergy = theHGCEEDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHGCEEDigitizer_->tdcForToAOnset();
        keV2fC = theHGCEEDigitizer_->keV2fC();
        break;
      case ForwardSubdetector::HGCHEF:
        weightToAbyEnergy = theHGCHEfrontDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHGCHEfrontDigitizer_->tdcForToAOnset();
        keV2fC = theHGCHEfrontDigitizer_->keV2fC();
        break;
      case ForwardSubdetector::HGCHEB:
        weightToAbyEnergy = theHGCHEbackDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHGCHEbackDigitizer_->tdcForToAOnset();
        keV2fC = theHGCHEbackDigitizer_->keV2fC();
        break;
      case ForwardSubdetector::HFNose:
        weightToAbyEnergy = theHFNoseDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHFNoseDigitizer_->tdcForToAOnset();
        keV2fC = theHFNoseDigitizer_->keV2fC();
        break;
      default:
        break;
    }
  } else {
    switch (myDet_) {
      case DetId::HGCalEE:
        weightToAbyEnergy = theHGCEEDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHGCEEDigitizer_->tdcForToAOnset();
        keV2fC = theHGCEEDigitizer_->keV2fC();
        break;
      case DetId::HGCalHSi:
        weightToAbyEnergy = theHGCHEfrontDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHGCHEfrontDigitizer_->tdcForToAOnset();
        keV2fC = theHGCHEfrontDigitizer_->keV2fC();
        break;
      case DetId::HGCalHSc:
        weightToAbyEnergy = theHGCHEbackDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHGCHEbackDigitizer_->tdcForToAOnset();
        keV2fC = theHGCHEbackDigitizer_->keV2fC();
        break;
      case DetId::Forward:
        weightToAbyEnergy = theHFNoseDigitizer_->toaModeByEnergy();
        tdcForToAOnset = theHFNoseDigitizer_->tdcForToAOnset();
        keV2fC = theHFNoseDigitizer_->keV2fC();
        break;
      default:
        break;
    }
  }
  return weightToAbyEnergy;
}

void HGCDigitizer::checkPosition(const HGCalDigiCollection* digis) const {
  const double tol(0.5);
  if (geometryType_ != 0 && nullptr != gHGCal_) {
    for (const auto& digi : *(digis)) {
      const DetId& id = digi.id();
      const GlobalPoint& global = gHGCal_->getPosition(id);
      double r = global.perp();
      double z = std::abs(global.z());
      std::pair<double, double> zrange = gHGCal_->topology().dddConstants().rangeZ(true);
      std::pair<double, double> rrange = gHGCal_->topology().dddConstants().rangeR(z, true);
      bool ok = ((r >= rrange.first) && (r <= rrange.second) && (z >= zrange.first) && (z <= zrange.second));
      std::string ck = (((r < rrange.first - tol) || (r > rrange.second + tol) || (z < zrange.first - tol) ||
                         (z > zrange.second + tol))
                            ? "***** ERROR *****"
                            : "");
      bool val = gHGCal_->topology().valid(id);
      if ((!ok) || (!val)) {
        if (id.det() == DetId::HGCalEE || id.det() == DetId::HGCalHSi) {
          edm::LogVerbatim("HGCDigitizer") << "Check " << HGCSiliconDetId(id) << " " << global << " R " << r << ":"
                                           << rrange.first << ":" << rrange.second << " Z " << z << ":" << zrange.first
                                           << ":" << zrange.second << " Flag " << ok << ":" << val << " " << ck;
        } else if (id.det() == DetId::HGCalHSc) {
          edm::LogVerbatim("HGCDigitizer") << "Check " << HGCScintillatorDetId(id) << " " << global << " R " << r << ":"
                                           << rrange.first << ":" << rrange.second << " Z " << z << ":" << zrange.first
                                           << ":" << zrange.second << " Flag " << ok << ":" << val << " " << ck;
        } else if ((id.det() == DetId::Forward) && (id.subdetId() == static_cast<int>(HFNose))) {
          edm::LogVerbatim("HGCDigitizer") << "Check " << HFNoseDetId(id) << " " << global << " R " << r << ":"
                                           << rrange.first << ":" << rrange.second << " Z " << z << ":" << zrange.first
                                           << ":" << zrange.second << " Flag " << ok << ":" << val << " " << ck;
        } else {
          edm::LogVerbatim("HGCDigitizer")
              << "Check " << std::hex << id.rawId() << std::dec << " " << id.det() << ":" << id.subdetId() << " "
              << global << " R " << r << ":" << rrange.first << ":" << rrange.second << " Z " << z << ":"
              << zrange.first << ":" << zrange.second << " Flag " << ok << ":" << val << " " << ck;
        }
      }
    }
  }
}

template void HGCDigitizer::accumulate_forPreMix<HcalGeometry>(edm::Handle<edm::PCaloHitContainer> const& hits,
                                                               int bxCrossing,
                                                               const HcalGeometry* geom,
                                                               CLHEP::HepRandomEngine* hre);

template void HGCDigitizer::accumulate_forPreMix<HGCalGeometry>(edm::Handle<edm::PCaloHitContainer> const& hits,
                                                                int bxCrossing,
                                                                const HGCalGeometry* geom,
                                                                CLHEP::HepRandomEngine* hre);

template void HGCDigitizer::accumulate<HcalGeometry>(edm::Handle<edm::PCaloHitContainer> const& hits,
                                                     int bxCrossing,
                                                     const HcalGeometry* geom,
                                                     CLHEP::HepRandomEngine* hre);
template void HGCDigitizer::accumulate<HGCalGeometry>(edm::Handle<edm::PCaloHitContainer> const& hits,
                                                      int bxCrossing,
                                                      const HGCalGeometry* geom,
                                                      CLHEP::HepRandomEngine* hre);
