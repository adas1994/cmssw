#include "RecoLocalCalo/HGCalRecProducers/plugins/HGCalUncalibRecHitWorkerWeights.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

void configureIt(const edm::ParameterSet& conf, HGCalUncalibRecHitRecWeightsAlgo<HGCalDataFrame>& maker) {
  constexpr char isSiFE[] = "isSiFE";
  constexpr char adcNbits[] = "adcNbits";
  constexpr char adcSaturation[] = "adcSaturation";
  constexpr char tdcNbits[] = "tdcNbits";
  constexpr char tdcSaturation[] = "tdcSaturation";
  constexpr char tdcOnset[] = "tdcOnset";
  constexpr char toaLSB_ns[] = "toaLSB_ns";
  constexpr char fCPerMIP[] = "fCPerMIP";
  constexpr char doseMap[]  = "doseMap";
  constexpr char scaleByDoseAlgo[] = "scaleByDoseAlgo";
  constexpr char scaleByDoseFactor[] = "scaleByDoseFactor";
  constexpr char ileakParam[] = "ileakParam";
  constexpr char cceParams[] = "cceParams";

  if (conf.exists(isSiFE)) {
    maker.set_isSiFESim(conf.getParameter<bool>(isSiFE));
  } else {
    maker.set_isSiFESim(false);
  }

  if (conf.exists(adcNbits)) {
    uint32_t nBits = conf.getParameter<uint32_t>(adcNbits);
    double saturation = conf.getParameter<double>(adcSaturation);
    float adcLSB = saturation / pow(2., nBits);
    maker.set_ADCLSB(adcLSB);
  } else {
    maker.set_ADCLSB(-1.);
  }

  if (conf.exists(tdcNbits)) {
    uint32_t nBits = conf.getParameter<uint32_t>(tdcNbits);
    double saturation = conf.getParameter<double>(tdcSaturation);
    double onset = conf.getParameter<double>(tdcOnset);  // in fC
    float tdcLSB = saturation / pow(2., nBits);
    maker.set_TDCLSB(tdcLSB);
    maker.set_tdcOnsetfC(onset);
  } else {
    maker.set_TDCLSB(-1.);
    maker.set_tdcOnsetfC(-1.);
  }

  if (conf.exists(toaLSB_ns)) {
    maker.set_toaLSBToNS(conf.getParameter<double>(toaLSB_ns));
  } else {
    maker.set_toaLSBToNS(-1.);
  }

  if (conf.exists(fCPerMIP)) {
    maker.set_fCPerMIP(conf.getParameter<std::vector<double> >(fCPerMIP));
  } else {
    maker.set_fCPerMIP(std::vector<double>({1.0}));
  }

  if (conf.exists(doseMap) && conf.exists(scaleByDoseAlgo)) {
    const std::string dose_Map = conf.getParameter<std::string>("doseMap");
    const uint32_t algo       = conf.getParameter<uint32_t>("scaleByDoseAlgo");
    maker.set_doseMap(dose_Map, algo);
  } else {
    maker.set_doseMap("SimCalorimetry/HGCalSimProducers/data/doseParams_3000fb_fluka-3.7.20.tx", 0);
  }

  if (conf.exists("scaleByDoseFactor")) {
    const double doseFactor = conf.getParameter<double>("scaleByDoseFactor");
    maker.set_FluenceScaleFactor(doseFactor);
  } else {
    maker.set_FluenceScaleFactor(1.);
  }

  if (conf.exists("ileakParam")) {
    const edm::ParameterSet& ileakParamSet = conf.getParameterSet("ileakParam");
    const std::vector<double> ileak_param = ileakParamSet.getParameter<std::vector<double> >("ileakParam");
    maker.set_IleakParam(ileak_param);
  } else {
    const std::vector<double> ileak_param = {0.993,-42.668};
    maker.set_IleakParam(ileak_param);
  }

  if (conf.exists("cceParams")) {
    const edm::ParameterSet& cceParamsSet  = conf.getParameterSet("cceParams");
    const std::vector<double> cceParamFine  = cceParamsSet.getParameter<std::vector<double> >("cceParamFine");
    const std::vector<double> cceParamThin  = cceParamsSet.getParameter<std::vector<double> >("cceParamThin");
    const std::vector<double> cceParamThick = cceParamsSet.getParameter<std::vector<double> >("cceParamThick");
    maker.set_CceParam(cceParamFine, cceParamThin, cceParamThick);
  } else {
    const std::vector<double> cceParamFine = {1.5e+15, -3.00394e-17, 0.318083};
    const std::vector<double> cceParamThin  = {1.5e+15, -3.09878e-16, 0.211207};
    const std::vector<double> cceParamThick = {6e+14,   -7.96539e-16, 0.251751};
    maker.set_CceParam(cceParamFine, cceParamThin, cceParamThick);

  }
  

}

HGCalUncalibRecHitWorkerWeights::HGCalUncalibRecHitWorkerWeights(const edm::ParameterSet& ps)
    : HGCalUncalibRecHitWorkerBaseClass(ps) {
  const edm::ParameterSet& ee_cfg  = ps.getParameterSet("HGCEEConfig");
  const edm::ParameterSet& hef_cfg = ps.getParameterSet("HGCHEFConfig");
  const edm::ParameterSet& heb_cfg = ps.getParameterSet("HGCHEBConfig");
  const edm::ParameterSet& hfnose_cfg = ps.getParameterSet("HGCHFNoseConfig");
  configureIt(ee_cfg, uncalibMaker_ee_);
  configureIt(hef_cfg, uncalibMaker_hef_);
  configureIt(heb_cfg, uncalibMaker_heb_);
  configureIt(hfnose_cfg, uncalibMaker_hfnose_);
  
}

void HGCalUncalibRecHitWorkerWeights::set(const edm::EventSetup& es) {
  if (uncalibMaker_ee_.isSiFESim()) {
    edm::ESHandle<HGCalGeometry> hgceeGeoHandle;
    es.get<IdealGeometryRecord>().get("HGCalEESensitive", hgceeGeoHandle);
    uncalibMaker_ee_.setGeometry(hgceeGeoHandle.product());
  }
  if (uncalibMaker_hef_.isSiFESim()) {
    edm::ESHandle<HGCalGeometry> hgchefGeoHandle;
    es.get<IdealGeometryRecord>().get("HGCalHESiliconSensitive", hgchefGeoHandle);
    uncalibMaker_hef_.setGeometry(hgchefGeoHandle.product());
  }
  uncalibMaker_heb_.setGeometry(nullptr);
  if (uncalibMaker_hfnose_.isSiFESim()) {
    edm::ESHandle<HGCalGeometry> hgchfnoseGeoHandle;
    es.get<IdealGeometryRecord>().get("HGCalHFNoseSensitive", hgchfnoseGeoHandle);
    uncalibMaker_hfnose_.setGeometry(hgchfnoseGeoHandle.product());
  }
}

bool HGCalUncalibRecHitWorkerWeights::runHGCEE(const HGCalDigiCollection::const_iterator& itdg,
                                               HGCeeUncalibratedRecHitCollection& result) {
  result.push_back(uncalibMaker_ee_.makeRecHit(*itdg));
  return true;
}

bool HGCalUncalibRecHitWorkerWeights::runHGCHEsil(const HGCalDigiCollection::const_iterator& itdg,
                                                  HGChefUncalibratedRecHitCollection& result) {
  result.push_back(uncalibMaker_hef_.makeRecHit(*itdg));
  return true;
}

bool HGCalUncalibRecHitWorkerWeights::runHGCHEscint(const HGCalDigiCollection::const_iterator& itdg,
                                                    HGChebUncalibratedRecHitCollection& result) {
  result.push_back(uncalibMaker_heb_.makeRecHit(*itdg));
  return true;
}

bool HGCalUncalibRecHitWorkerWeights::runHGCHFNose(const HGCalDigiCollection::const_iterator& itdg,
                                                   HGChfnoseUncalibratedRecHitCollection& result) {
  result.push_back(uncalibMaker_hfnose_.makeRecHit(*itdg));
  return true;
}

#include "FWCore/Framework/interface/MakerMacros.h"
#include "RecoLocalCalo/HGCalRecProducers/interface/HGCalUncalibRecHitWorkerFactory.h"
DEFINE_EDM_PLUGIN(HGCalUncalibRecHitWorkerFactory, HGCalUncalibRecHitWorkerWeights, "HGCalUncalibRecHitWorkerWeights");
