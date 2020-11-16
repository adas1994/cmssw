This is a partial summary update of the work that has been done towards Deconvoluting Radiation Effects and gain setting at ROC level.
A background information is povided [in this talk](https://indico.cern.ch/event/933714/contributions/3924245/). 
**Configuring The radiation map tool for Silicon Subdetector**
In the [RecoLocalCalo/HGCalRecAlgos/interface/HGCalUncalibRecHitRecWeightsAlgo.h](https://github.com/adas1994/cmssw/blob/GainProject/RecoLocalCalo/HGCalRecAlgos/interface/HGCalUncalibRecHitRecWeightsAlgo.h) we introduced a new private class member to the `HGCalUncalibRecHitRecWeightsAlgo` class - `HGCalSiNoiseMap *rad_map_;`.
The following new class member methods have been introduced to configure `rad_map` - `void set_doseMap(const std::string& doseMap, uint32_t& scaleByDoseAlgo)`, `void set_FluenceScaleFactor(const double scaleByDoseFactor)`, `void set_IleakParam(const std::vector<double>& ileakParam)`, `void set_CceParam(const std::vector<double> &paramsFine,
                    const std::vector<double> &paramsThin,
                    const std::vector<double> &paramsThick)`. We have also modified the `virtual HGCUncalibratedRecHit HGCalUncalibRecHitRecWeightsAlgo::makeRecHit(const C& dataFrame)` where C is the class which the *HGCalUncalibRecHitRecWeightsAlgo* is templated upon. The following new lines have been introduced to the *makeRecHit* function -
                    
`HGCalDetId detId = dataFrame.id();
 HGCalSiNoiseMap::GainRange_t sample_gain = static_cast<HGCalSiNoiseMap::GainRange_t> (sample.gain());
 double cce;
 int thickness = (ddd_ != nullptr) ? ddd_->waferType(dataFrame.id()) : 0;
 if(detId.det() != DetId::HGCalHSc && sample_gain > 0) {
      HGCalSiNoiseMap::SiCellOpCharacteristics siop = rad_map_->getSiCellOpCharacteristics(detId);
      HGCalSiNoiseMap::GainRange_t gain = static_cast<HGCalSiNoiseMap::GainRange_t>(siop.core.gain);
      double adcLSB = rad_map_->getLSBPerGain()[gain];
      cce = siop.mipfC ; //siop.core.cce;                                                                                                 
      double noise = siop.core.noise;
      double mipADC = double(siop.mipADC);
      bool isBusy = (isTDC);
      double nmips = rawADC/mipADC;

    }
    else {
      cce = fCPerMIP_[thickness];
    }
  `
  
So, it has added the code for deconvoluting gain for SiNoiseMap. But, the case for SciNoiseMap is not yet solved.
To make these modifications compatible, we also had to modify this plugin module - [RecoLocalCalo/HGCalRecProducers/plugins/HGCalUncalibRecHitWorkerWeights.cc](https://github.com/adas1994/cmssw/blob/GainProject/RecoLocalCalo/HGCalRecProducers/plugins/HGCalUncalibRecHitWorkerWeights.cc#L62-L96).
  

