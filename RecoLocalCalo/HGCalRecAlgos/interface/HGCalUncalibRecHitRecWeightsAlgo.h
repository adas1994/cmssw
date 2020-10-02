#ifndef RecoLocalCalo_HGCalRecAlgos_HGCalUncalibRecHitRecWeightsAlgo_HH
#define RecoLocalCalo_HGCalRecAlgos_HGCalUncalibRecHitRecWeightsAlgo_HH

/** \class HGalUncalibRecHitRecWeightsAlgo
  *  compute amplitude, pedestal, time jitter, chi2 of a pulse
  *  using a weights method, a la Ecal 
  *
  *  \author Valeri Andreev
  *  
  *
  */

#include "RecoLocalCalo/HGCalRecAlgos/interface/HGCalUncalibRecHitRecAbsAlgo.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"
#include "SimCalorimetry/HGCalSimAlgos/interface/HGCalSiNoiseMap.h"
#include <vector>
#include <cmath>

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"

template <class C>
class HGCalUncalibRecHitRecWeightsAlgo {
public:
  HGCalUncalibRecHitRecWeightsAlgo<C>(){
    rad_map_ = new HGCalSiNoiseMap();
  };
  // destructor
  virtual ~HGCalUncalibRecHitRecWeightsAlgo<C>(){};

  void set_isSiFESim(const bool isSiFE) { isSiFESim_ = isSiFE; }
  bool isSiFESim() const { return isSiFESim_; }

  void set_ADCLSB(const double adclsb) { adcLSB_ = adclsb; }
  void set_TDCLSB(const double tdclsb) { tdcLSB_ = tdclsb; }

  void set_toaLSBToNS(const double lsb2ns) { toaLSBToNS_ = lsb2ns; }

  void set_tdcOnsetfC(const double tdcOnset) { tdcOnsetfC_ = tdcOnset; }

  void set_fCPerMIP(const std::vector<double>& fCPerMIP) {
    if (std::any_of(fCPerMIP.cbegin(), fCPerMIP.cend(), [](double conv) { return conv <= 0.0; })) {
      throw cms::Exception("BadConversionFactor") << "At least one of fCPerMIP is zero!" << std::endl;
    }
    fCPerMIP_ = fCPerMIP;
  }
  
  void set_doseMap(const std::string& doseMap, const uint32_t& scaleByDoseAlgo) {
    rad_map_->setDoseMap(doseMap, scaleByDoseAlgo);
  }

  void set_FluenceScaleFactor(const double scaleByDoseFactor) {
    rad_map_->setFluenceScaleFactor(scaleByDoseFactor);
  }

  void set_IleakParam(const std::vector<double>& ileakParam) {
    rad_map_->setIleakParam(ileakParam);
  }

  void set_CceParam(const std::vector<double> &paramsFine,
		    const std::vector<double> &paramsThin,
		    const std::vector<double> &paramsThick) {
    rad_map_->setCceParam(paramsFine, paramsThin, paramsThick);
  }
  void setGeometry(const HGCalGeometry* geom) {
    if (geom) {
      ddd_ = &(geom->topology().dddConstants());
      rad_map_->setGeometry(geom);
    }
    else {
      ddd_ = nullptr;
    }
  }

  /// Compute HGCUncalibratedRecHit from DataFrame
  virtual HGCUncalibratedRecHit makeRecHit(const C& dataFrame) { //------ makeRecHit starts ------------------
    double amplitude_(-1.), pedestal_(-1.), jitter_(-1.), chi2_(-1.);
    uint32_t flag = 0;
    /// ---- My Addition ---------------------------------
    
    
    HGCalDetId detId = dataFrame.id();
    bool isHGCal = detId.isHGCal();
    bool isForward = detId.isForward();
    int cell = detId.cell();
    int wafer = detId.wafer();
    int waferType = detId.waferType();
    int layer = detId.layer();
    std::cout<<"*** "<<isHGCal<<std::endl;
    
    //std::cout<<"before printing detId"<<std::endl;
    //std::cout<<detId<<std::endl;
    constexpr int iSample = 2;  //only in-time sample
    const auto& sample = dataFrame.sample(iSample);
    bool isTDC = sample.mode();
    bool isBusy = (isTDC);
    HGCalSiNoiseMap::GainRange_t gain = static_cast<HGCalSiNoiseMap::GainRange_t> (sample.gain());
    double rawADC = (double)(sample.data());
    std::cout<<"before siop"<<std::endl;
    HGCalSiNoiseMap::SiCellOpCharacteristics siop = rad_map_->getSiCellOpCharacteristics(detId);
    std::cout<<"after siop"<<std::endl;
    double mipADC = double(siop.mipADC);
    //std::cout<<gain<<"\t"<<sample.gain()<<std::endl;
    double nmips = rawADC/mipADC;
    std::cout<<nmips<<std::endl;
    //number of MIPs
    if(isTDC) {
      double adcLSB( 1./80.);
      if(gain==HGCalSiNoiseMap::q160fC) adcLSB=1./160.;
      if(gain==HGCalSiNoiseMap::q320fC) adcLSB=1./320.;
                 
      double charge( (std::floor(tdcOnsetfC_ / adcLSB) + 1.0) * adcLSB + (rawADC+0.5)*tdcLSB_ );
      nmips = charge/double(mipADC);
    }
    if(isBusy) {
      nmips=0;
    }
    
    if (isSiFESim_) {
      // mode == true: TDC readout was activated and amplitude comes from TimeOverThreshold
      if (sample.mode()) {
        flag = !sample.threshold();  // raise flag if busy cell
        amplitude_ = (std::floor(tdcOnsetfC_ / adcLSB_) + 1.0) * adcLSB_ + (double(sample.data()) + 0.5) * tdcLSB_;

        if (sample.getToAValid()) {
          jitter_ = double(sample.toa()) * toaLSBToNS_;
        }
      } else {
        amplitude_ = double(sample.data()) * adcLSB_;  // why do we not have +0.5 here ?
        if (sample.getToAValid()) {
          jitter_ = double(sample.toa()) * toaLSBToNS_;
        }
      }  //isSiFESim_
    }    //mode()

    
    // trivial digitization, i.e. no signal shape
    else {
      amplitude_ = double(sample.data()) * adcLSB_;
    }
    int thickness = (ddd_ != nullptr) ? ddd_->waferType(dataFrame.id()) : 0;
    amplitude_ = amplitude_ / fCPerMIP_[thickness];
    return HGCUncalibratedRecHit(dataFrame.id(), amplitude_, pedestal_, jitter_, chi2_, flag);
  } //------ makeRecHit ends ---------------------

private:
  HGCalSiNoiseMap *rad_map_;
  double adcLSB_, tdcLSB_, toaLSBToNS_, tdcOnsetfC_;
  bool isSiFESim_;
  std::vector<double> fCPerMIP_;
  const HGCalDDDConstants* ddd_;
};
#endif
