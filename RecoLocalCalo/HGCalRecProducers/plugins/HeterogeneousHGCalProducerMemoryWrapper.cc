#include "HeterogeneousHGCalProducerMemoryWrapper.h"

namespace memory {
  namespace allocation {

    cms::cuda::device::unique_ptr<std::byte[]> uncalibRecHitDevice(const uint32_t& nhits,
                                                                   const uint32_t& stride,
                                                                   HGCUncalibratedRecHitSoA* soa,
                                                                   const cudaStream_t& stream) {
      std::vector<int> sizes = {npointers::float_hgcuncalibrechits_soa * sizeof(float),
                                npointers::uint32_hgcuncalibrechits_soa * sizeof(uint32_t)};
      int size_tot = std::accumulate(sizes.begin(), sizes.end(), 0);
      cms::cuda::device::unique_ptr<std::byte[]> mem =
          cms::cuda::make_device_unique<std::byte[]>(stride * size_tot, stream);

      soa->amplitude_ = reinterpret_cast<float*>(mem.get());
      soa->pedestal_ = soa->amplitude_ + stride;
      soa->jitter_ = soa->pedestal_ + stride;
      soa->chi2_ = soa->jitter_ + stride;
      soa->OOTamplitude_ = soa->chi2_ + stride;
      soa->OOTchi2_ = soa->OOTamplitude_ + stride;
      soa->flags_ = reinterpret_cast<uint32_t*>(soa->OOTchi2_ + stride);
      soa->aux_ = soa->flags_ + stride;
      soa->id_ = soa->aux_ + stride;

      soa->nbytes_ = size_tot;
      soa->nhits_ = nhits;
      soa->stride_ = stride;

      assert(sizes.begin() + npointers::ntypes_hgcuncalibrechits_soa == sizes.end());

      return mem;
    }

    //allocates page-locked (pinned) and non cached (write-combining) memory for UncalibratedRecHits SoAs on the host
    cms::cuda::host::unique_ptr<std::byte[]> uncalibRecHitHost(const uint32_t& nhits,
                                                               const uint32_t& stride,
                                                               HGCUncalibratedRecHitSoA* soa,
                                                               const cudaStream_t& stream) {
      std::vector<int> sizes = {npointers::float_hgcuncalibrechits_soa * sizeof(float),
                                npointers::uint32_hgcuncalibrechits_soa * sizeof(uint32_t)};
      int size_tot = std::accumulate(sizes.begin(), sizes.end(), 0);
      cms::cuda::host::unique_ptr<std::byte[]> mem =
          cms::cuda::make_host_unique<std::byte[]>(stride * size_tot, stream);

      soa->amplitude_ = reinterpret_cast<float*>(mem.get());
      soa->pedestal_ = soa->amplitude_ + stride;
      soa->jitter_ = soa->pedestal_ + stride;
      soa->chi2_ = soa->jitter_ + stride;
      soa->OOTamplitude_ = soa->chi2_ + stride;
      soa->OOTchi2_ = soa->OOTamplitude_ + stride;
      soa->flags_ = reinterpret_cast<uint32_t*>(soa->OOTchi2_ + stride);
      soa->aux_ = soa->flags_ + stride;
      soa->id_ = soa->aux_ + stride;
      soa->aux_ = soa->flags_ + stride;
      soa->id_ = soa->aux_ + stride;

      soa->nbytes_ = size_tot;
      soa->nhits_ = nhits;
      soa->stride_ = stride;

      assert(sizes.begin() + npointers::ntypes_hgcuncalibrechits_soa == sizes.end());

      return mem;
    }

    cms::cuda::host::unique_ptr<std::byte[]> calibRecHitHost(const uint32_t& nhits,
                                                             const uint32_t& stride,
                                                             HGCRecHitSoA* soa,
                                                             const cudaStream_t& stream) {
      std::vector<int> sizes = {npointers::float_hgcrechits_soa * sizeof(float),
                                npointers::uint32_hgcrechits_soa * sizeof(uint32_t),
                                npointers::uint8_hgcrechits_soa * sizeof(uint8_t)};
      int size_tot = std::accumulate(sizes.begin(), sizes.end(), 0);
      cms::cuda::host::unique_ptr<std::byte[]> mem =
          cms::cuda::make_host_unique<std::byte[]>(stride * size_tot, stream);
      soa->energy_ = reinterpret_cast<float*>(mem.get());
      soa->time_ = soa->energy_ + stride;
      soa->timeError_ = soa->time_ + stride;
      soa->id_ = reinterpret_cast<uint32_t*>(soa->timeError_ + stride);
      soa->flagBits_ = soa->id_ + stride;
      soa->son_ = reinterpret_cast<uint8_t*>(soa->flagBits_ + stride);

      soa->nbytes_ = size_tot;
      soa->nhits_ = nhits;
      soa->stride_ = stride;

      assert(sizes.begin() + npointers::ntypes_hgcrechits_soa == sizes.end());

      return mem;
    }

    void calibRecHitDevice(
        const uint32_t& nhits, const uint32_t& stride, const uint32_t& nbytes, HGCRecHitSoA* soa, std::byte* mem) {
      soa->energy_ = reinterpret_cast<float*>(mem);
      soa->time_ = soa->energy_ + stride;
      soa->timeError_ = soa->time_ + stride;
      soa->id_ = reinterpret_cast<uint32_t*>(soa->timeError_ + stride);
      soa->flagBits_ = soa->id_ + stride;
      soa->son_ = reinterpret_cast<uint8_t*>(soa->flagBits_ + stride);

      soa->nbytes_ = nbytes;
      soa->nhits_ = nhits;
      soa->stride_ = stride;
    }
  }  // namespace allocation
}  // namespace memory
