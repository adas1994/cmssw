#include "HeterogeneousHGCalProducerMemoryWrapper.h"

namespace memory {
  namespace npointers {
    //pointers in the soas for the uncalibrated and calibrated hits
    constexpr unsigned int float_hgcuncalibrechits_soa = 6; //number of float pointers in the uncalibrated rechits SoA
    constexpr unsigned int uint32_hgcuncalibrechits_soa = 3; //number of uint32_t pointers in the uncalibrated rechits SoA
    constexpr unsigned int ntypes_hgcuncalibrechits_soa = 2; //number of different pointer types in the uncalibrated rechits SoA
    constexpr unsigned int float_hgcrechits_soa = 3; //number of float pointers in the rechits SoA
    constexpr unsigned int uint32_hgcrechits_soa = 2; //number of uint32_t pointers in the rechits SoA
    constexpr unsigned int uint8_hgcrechits_soa = 1; //number of uint8_t pointers in the rechits SoA
    constexpr unsigned int ntypes_hgcrechits_soa = 3; //number of different pointer types in the rechits SoA
  }
  
  namespace allocation {

    //allocates memory for UncalibratedRecHits SoAs and RecHits SoAs on the device
    void device(const int& nhits, HGCUncalibratedRecHitSoA* soa1, HGCUncalibratedRecHitSoA* soa2, HGCRecHitSoA* soa3, cms::cuda::device::unique_ptr<std::byte[]>& mem, const cudaStream_t& stream)
    {
      std::vector<int> sizes = {npointers::float_hgcuncalibrechits_soa  * sizeof(float),
				npointers::uint32_hgcuncalibrechits_soa * sizeof(uint32_t), //soa1
				npointers::float_hgcuncalibrechits_soa  * sizeof(float),
				npointers::uint32_hgcuncalibrechits_soa * sizeof(uint32_t), //soa2
				npointers::float_hgcrechits_soa         * sizeof(float),
				npointers::uint32_hgcrechits_soa        * sizeof(uint32_t),
				npointers::uint8_hgcrechits_soa         * sizeof(uint8_t)}; //soa3
      int size_tot = std::accumulate( sizes.begin(), sizes.end(), 0);
      mem = cms::cuda::make_device_unique<std::byte[]>(nhits * size_tot, stream);

      soa1->amplitude_     = reinterpret_cast<float*>(mem.get());
      soa1->pedestal_      = soa1->amplitude_    + nhits;
      soa1->jitter_        = soa1->pedestal_     + nhits;
      soa1->chi2_          = soa1->jitter_       + nhits;
      soa1->OOTamplitude_  = soa1->chi2_         + nhits;
      soa1->OOTchi2_       = soa1->OOTamplitude_ + nhits;
      soa1->flags_         = reinterpret_cast<uint32_t*>(soa1->OOTchi2_ + nhits);
      soa1->aux_           = soa1->flags_        + nhits;
      soa1->id_            = soa1->aux_          + nhits;

      soa2->amplitude_     = reinterpret_cast<float*>(soa1->id_ + nhits);
      soa2->pedestal_      = soa2->amplitude_    + nhits;
      soa2->jitter_        = soa2->pedestal_     + nhits;
      soa2->chi2_          = soa2->jitter_       + nhits;
      soa2->OOTamplitude_  = soa2->chi2_         + nhits;
      soa2->OOTchi2_       = soa2->OOTamplitude_ + nhits;
      soa2->flags_         = reinterpret_cast<uint32_t*>(soa2->OOTchi2_ + nhits);
      soa2->aux_           = soa2->flags_        + nhits;
      soa2->id_            = soa2->aux_          + nhits;
  
      soa3->energy_        = reinterpret_cast<float*>(soa2->id_ + nhits);
      soa3->time_          = soa3->energy_       + nhits;
      soa3->timeError_     = soa3->time_         + nhits;
      soa3->id_            = reinterpret_cast<uint32_t*>(soa3->timeError_ + nhits);
      soa3->flagBits_      = soa3->id_           + nhits;
      soa3->son_           = reinterpret_cast<uint8_t*>(soa3->flagBits_ + nhits);

      soa1->nbytes_ = std::accumulate(sizes.begin(), sizes.begin() + npointers::ntypes_hgcuncalibrechits_soa, 0);
      soa2->nbytes_ = std::accumulate(sizes.begin() + npointers::ntypes_hgcuncalibrechits_soa,
				      sizes.begin() + 2*npointers::ntypes_hgcuncalibrechits_soa, 0);
      soa3->nbytes_ = std::accumulate(sizes.begin() + 2*npointers::ntypes_hgcuncalibrechits_soa, sizes.end(), 0);
      assert(sizes.begin()+2*npointers::ntypes_hgcuncalibrechits_soa+npointers::ntypes_hgcrechits_soa == sizes.end());
    }

    //allocates page-locked (pinned) and non cached (write-combining) memory for UncalibratedRecHits SoAs on the host
    void host(const int& nhits, HGCUncalibratedRecHitSoA* soa, cms::cuda::host::unique_ptr<std::byte[]>& mem, const cudaStream_t& stream)
    {
      std::vector<int> sizes = { npointers::float_hgcuncalibrechits_soa  * sizeof(float),
				 npointers::uint32_hgcuncalibrechits_soa * sizeof(uint32_t) };
      int size_tot = std::accumulate(sizes.begin(), sizes.end(), 0);
      mem = cms::cuda::make_host_unique<std::byte[]>(nhits * size_tot, stream);

      soa->amplitude_     = reinterpret_cast<float*>(mem.get());
      soa->pedestal_      = soa->amplitude_    + nhits;
      soa->jitter_        = soa->pedestal_     + nhits;
      soa->chi2_          = soa->jitter_       + nhits;
      soa->OOTamplitude_  = soa->chi2_         + nhits;
      soa->OOTchi2_       = soa->OOTamplitude_ + nhits;
      soa->flags_         = reinterpret_cast<uint32_t*>(soa->OOTchi2_ + nhits);
      soa->aux_           = soa->flags_        + nhits;
      soa->id_            = soa->aux_          + nhits;
      soa->aux_           = soa->flags_        + nhits;
      soa->id_            = soa->aux_          + nhits;
      soa->nbytes_        = size_tot;
    }

    //allocates page-locked (pinned) memory for RecHits SoAs on the host
    void host(const int& nhits, HGCRecHitSoA* soa, cms::cuda::host::unique_ptr<std::byte[]>& mem, const cudaStream_t& stream)
    {
      std::vector<int> sizes = { npointers::float_hgcrechits_soa  * sizeof(float),
				 npointers::uint32_hgcrechits_soa * sizeof(uint32_t),
				 npointers::uint8_hgcrechits_soa  * sizeof(uint8_t) };
      int size_tot = std::accumulate(sizes.begin(), sizes.end(), 0);
      mem = cms::cuda::make_host_unique<std::byte[]>(nhits * size_tot, stream);
      soa->energy_     = reinterpret_cast<float*>(mem.get());
      soa->time_       = soa->energy_     + nhits;
      soa->timeError_  = soa->time_       + nhits;
      soa->id_         = reinterpret_cast<uint32_t*>(soa->timeError_ + nhits);
      soa->flagBits_   = soa->id_         + nhits;
      soa->son_        = reinterpret_cast<uint8_t*>(soa->flagBits_ + nhits);
      soa->nbytes_ = size_tot;
    }
  }
}
