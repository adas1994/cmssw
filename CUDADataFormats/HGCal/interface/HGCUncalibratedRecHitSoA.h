#ifndef CUDADataFormats_HGCal_HGCUncalibratedRecHitSoA_h
#define CUDADataFormats_HGCal_HGCUncalibratedRecHitSoA_h

class HGCUncalibratedRecHitSoA {
public:
  float *amplitude_; //uncalib rechit amplitude, i.e., the average number of MIPs
  float *pedestal_; //reconstructed pedestal
  float *jitter_; //reconstructed time jitter
  float *chi2_; //chi2 of the pulse
  float *OOTamplitude_; //out-of-time reconstructed amplitude
  float *OOTchi2_; //out-of-time chi2
  uint32_t *flags_; //uncalibrechit flags describing its status (DataFormats/HGCRecHit/interface/HGCUncalibratedRecHit.h); to be propagated to the rechits
  uint32_t *aux_; //aux word; first 8 bits contain time (jitter) error
  uint32_t *id_; //uncalibrechit detector id
  int nbytes_; //number of bytes of the SoA
};

#endif
