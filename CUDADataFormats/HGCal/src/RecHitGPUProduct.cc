#include "CUDADataFormats/HGCal/interface/RecHitGPUProduct.h"

RecHitGPUProduct::RecHitGPUProduct(const uint32_t& nhits, const cudaStream_t& stream) : nhits_(nhits) {
  stride_ = ((nhits - 1) / 32 + 1) * 32;  //align to warp boundary

  std::vector<int> sizes = {memory::npointers::float_hgcrechits_soa * sizeof(float),
                            memory::npointers::uint32_hgcrechits_soa * sizeof(uint32_t),
                            memory::npointers::uint8_hgcrechits_soa * sizeof(uint8_t)};
  size_tot_ = std::accumulate(sizes.begin(), sizes.end(), 0);
  ptr_ = cms::cuda::make_device_unique<std::byte[]>(stride_ * size_tot_, stream);
  assert(sizes.begin() + memory::npointers::ntypes_hgcrechits_soa == sizes.end());
}
