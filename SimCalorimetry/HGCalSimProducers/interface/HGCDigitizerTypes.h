#ifndef __SimCalorimetry_HGCCalSimProducers_HGCDigitizerTypes_h__
#define __SimCalorimetry_HGCCalSimProducers_HGCDigitizerTypes_h__

#include <unordered_map>
#include <array>
#include <functional>

#include "DataFormats/DetId/interface/DetId.h"

namespace hgc_digi {

  //15 time samples: 9 pre-samples, 1 in-time, 5 post-samples
  constexpr size_t nSamples = 15;
  constexpr size_t npreHits = 30;
  typedef float HGCSimData_t;

  typedef std::array<HGCSimData_t, nSamples> HGCSimHitData;

  struct HGCCellInfo {
    //1st array=energy, 2nd array=energy weighted time-of-flight
    std::array<HGCSimHitData, 2> hit_info;
    int thickness;
    double size;
  };

  typedef std::unordered_map<uint32_t, HGCCellInfo> HGCSimHitDataAccumulator;
  
  //---------------- Data Structure for HGC-Premixing ------------------------
  typedef std::vector<HGCSimData_t> HGCSimDataCollection;
  typedef std::array<HGCSimDataCollection, nSamples> PUSimHitData;

  struct HGCCellHitInfo {
    std::array<PUSimHitData, 2> PUhit_info;
    int thickness;
    double size;
    HGCSimData_t thisBx_accCharge_after_CellFireOff;
  };
  
  typedef std::unordered_map<uint32_t, HGCCellHitInfo> HGCPUSimHitDataAccumulator;
}  // namespace hgc_digi
#endif
