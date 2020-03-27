#ifndef CUDADataFormats_HGCal_HGCUncalibratedRecHitsToRecHitsConstants_h
#define CUDADataFormats_HGCal_HGCUncalibratedRecHitsToRecHitsConstants_h

#include <vector>

//maximum sizes for SoA's arrays holding configuration data ("constants")
namespace maxsizes_constants {
  //EE
  constexpr size_t ee_fCPerMIP = 3; //number of elements pointed by hgcEE_fCPerMIP_
  constexpr size_t ee_cce = 3; //number of elements pointed by hgcEE_cce_
  constexpr size_t ee_noise_fC = 3; //number of elements pointed by hgcEE_noise_fC_
  constexpr size_t ee_rcorr = 3; //number of elements pointed by rcorr_
  constexpr size_t ee_weights = 51; //number of elements posize_ted by weights_
  //HEF
  constexpr size_t hef_fCPerMIP = 3; //number of elements pointed by hgcEE_fCPerMIP_
  constexpr size_t hef_cce = 3; //number of elements pointed by hgcEE_cce_
  constexpr size_t hef_noise_fC = 3; //number of elements pointed by hgcEE_noise_fC_
  constexpr size_t hef_rcorr = 3; //number of elements pointed by rcorr_
  constexpr size_t hef_weights = 51; //number of elements pointed by weights_
  //HEB
  constexpr size_t heb_weights = 51; //number of elements pointed by weights_
}

class HGCConstantVectorData {
 public:
  std::vector<double> fCPerMIP_;
  std::vector<double> cce_;
  std::vector<double> noise_fC_;
  std::vector<double> rcorr_;
  std::vector<double> weights_;
};

class HGCeeUncalibratedRecHitConstantData {
 public:
  double fCPerMIP_[maxsizes_constants::ee_fCPerMIP]; //femto coloumb to MIP conversion; one value per sensor thickness
  double cce_[maxsizes_constants::ee_cce];           //charge collection efficiency, one value per sensor thickness
  double noise_fC_[maxsizes_constants::ee_noise_fC]; //noise, one value per sensor thickness
  double rcorr_[maxsizes_constants::ee_rcorr];              //thickness correction
  double weights_[maxsizes_constants::ee_weights];          //energy weights to recover rechit energy deposited in the absorber

  double keV2DIGI_;  //energy to femto coloumb conversion: 1000 eV/3.62 (eV per e) / 6.24150934e3 (e per fC)
  double uncalib2GeV_; //sets the ADC; obtained by dividing 1e-6 by hgcEE_keV2DIGI_
  float xmin_;              //used for computing the time resolution error
  float xmax_; //used for computing the time resolution error
  float aterm_; //used for computing the time resolution error
  float cterm_; //used for computing the time resolution error
};

class HGChefUncalibratedRecHitConstantData {
 public:
  double fCPerMIP_[maxsizes_constants::hef_fCPerMIP]; //femto coloumb to MIP conversion; one value per sensor thickness
  double cce_[maxsizes_constants::hef_cce];           //charge collection efficiency, one value per sensor thickness
  double noise_fC_[maxsizes_constants::hef_noise_fC]; //noise, one value per sensor thickness
  double rcorr_[maxsizes_constants::hef_rcorr];              //thickness correction
  double weights_[maxsizes_constants::hef_weights];          //energy weights to recover rechit energy deposited in the absorber

  double keV2DIGI_;    //energy to femto coloumb conversion: 1000 eV/3.62 (eV per e) / 6.24150934e3 (e per fC)
  double uncalib2GeV_; //sets the ADC; obtained by dividing 1e-6 by hgcHEF_keV2DIGI_
  float xmin_;         //used for computing the time resolution error
  float xmax_;         //used for computing the time resolution error
  float aterm_;        //used for computing the time resolution error
  float cterm_;        //used for computing the time resolution error
  uint32_t layerOffset_;  //layer offset relative to layer#1 of the EE subsetector
};

class HGChebUncalibratedRecHitConstantData {
 public:
  double weights_[maxsizes_constants::heb_weights]; //energy weights to recover rechit energy deposited in the absorber

  double keV2DIGI_;   //energy to femto coloumb conversion: 1000 eV/3.62 (eV per e) / 6.24150934e3 (e per fC)
  double uncalib2GeV_; //sets the ADC; obtained by dividing 1e-6 by hgcHEB_keV2DIGI_
  double noise_MIP_;  //noise
  uint32_t layerOffset_;  //layer offset relative to layer#1 of the EE subsetector
};

#endif
