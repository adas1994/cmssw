This is a partial summary update of the work that has been done towards Deconvoluting Radiation Effects and gain setting at ROC level.
A background information is povided [in this talk](https://indico.cern.ch/event/933714/contributions/3924245/). 
**Configuring The radiation map tool for Silicon Subdetector**
In the RecoLocalCalo/HGCalRecAlgos/interface/HGCalUncalibRecHitRecWeightsAlgo.h we introduced a new private class member to the `HGCalUncalibRecHitRecWeightsAlgo` class - `HGCalSiNoiseMap *rad_map_;`.
The following new class member methods have been introduced to configure `rad_map` - `void set_doseMap(const std::string& doseMap, uint32_t& scaleByDoseAlgo)`, `void set_FluenceScaleFactor(const double scaleByDoseFactor)`, `void set_IleakParam(const std::vector<double>& ileakParam)`, `void set_CceParam(const std::vector<double> &paramsFine,
                    const std::vector<double> &paramsThin,
                    const std::vector<double> &paramsThick)` and `void setGeometry(const HGCalGeometry* geom)`

