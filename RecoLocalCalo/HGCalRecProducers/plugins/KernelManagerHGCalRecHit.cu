#include <cuda.h>
#include <cuda_runtime.h>
#include <inttypes.h>
#include "RecoLocalCalo/HGCalRecProducers/plugins/KernelManagerHGCalRecHit.h"
#include "HGCalRecHitKernelImpl.cuh"

KernelManagerHGCalRecHit::KernelManagerHGCalRecHit()
{
  ::nblocks_ = (data_->nhits_ + ::nthreads_.x - 1) / ::nthreads_.x;
}

KernelManagerHGCalRecHit::KernelManagerHGCalRecHit(KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA> *data):
  data_(data)
{
  ::nblocks_ = (data_->nhits_ + ::nthreads_.x - 1) / ::nthreads_.x;
  nbytes_host_ = (data_->h_out_)->nbytes_ * data_->stride_;
  nbytes_device_ = (data_->d_1_)->nbytes_ * data_->stride_;
}

KernelManagerHGCalRecHit::~KernelManagerHGCalRecHit()
{
}

void KernelManagerHGCalRecHit::transfer_soas_to_device_()
{
  cudaCheck( cudaMemcpyAsync((data_->d_1_)->amplitude_, (data_->h_in_)->amplitude_, nbytes_device_, cudaMemcpyHostToDevice) );
  cudaCheck( cudaGetLastError() );
}

void KernelManagerHGCalRecHit::transfer_soa_to_host_and_synchronize_()
{
  cudaCheck( cudaMemcpyAsync((data_->h_out_)->energy_, (data_->d_out_)->energy_, nbytes_host_, cudaMemcpyDeviceToHost) );
  cudaCheck( cudaGetLastError() );
}

void KernelManagerHGCalRecHit::reuse_device_pointers_()
{
  std::swap(data_->d_1_, data_->d_2_); 
}

void KernelManagerHGCalRecHit::run_kernels(const KernelConstantData<HGCeeUncalibratedRecHitConstantData> *kcdata)
{
  transfer_soas_to_device_();
  /*
  ee_step1<<<::nblocks_, ::nthreads_>>>( *(data_->d_2_), *(data_->d_1_), kcdata->data_, data_->nhits_ );
  after_();
  reuse_device_pointers_();
  */

  ee_to_rechit<<<::nblocks_, ::nthreads_>>>( *(data_->d_out_), *(data_->d_1_), kcdata->data_, data_->nhits_ );
  cudaCheck( cudaGetLastError() );

  transfer_soa_to_host_and_synchronize_();
  cudaCheck( cudaGetLastError() );
  cudaCheck( cudaDeviceSynchronize() );
}

void KernelManagerHGCalRecHit::run_kernels(const KernelConstantData<HGChefUncalibratedRecHitConstantData> *kcdata, const hgcal_conditions::HeterogeneousHEFConditionsESProduct* d_conds)
{
  transfer_soas_to_device_();
  cudaCheck( cudaGetLastError() );

  /*
  hef_step1<<<::nblocks_,::nthreads_>>>( *(data_->d_2), *(data_->d_1_), d_kcdata->data, data_->nhits_);
  cudaCheck( cudaGetLastError() );
  reuse_device_pointers_();
  */

  hef_to_rechit<<<::nblocks_,::nthreads_>>>( *(data_->d_out_), *(data_->d_1_), kcdata->data_, d_conds, data_->nhits_ );
  cudaCheck( cudaGetLastError() );

  transfer_soa_to_host_and_synchronize_();
  cudaCheck( cudaGetLastError() );
  cudaCheck( cudaDeviceSynchronize() );
}

void KernelManagerHGCalRecHit::run_kernels(const KernelConstantData<HGChebUncalibratedRecHitConstantData> *kcdata)
{
  transfer_soas_to_device_();
  cudaCheck( cudaGetLastError() );

  /*
  heb_step1<<<::nblocks_, ::nthreads_>>>( *(data_->d_2_), *(data_->d_1_), d_kcdata->data_, data_->nhits_);
  cudaCheck( cudaGetLastError() );
  reuse_device_pointers_();
  */

  heb_to_rechit<<<::nblocks_, ::nthreads_>>>( *(data_->d_out_), *(data_->d_1_), kcdata->data_, data_->nhits_ );
  cudaCheck( cudaGetLastError() );

  transfer_soa_to_host_and_synchronize_();
  cudaCheck( cudaGetLastError() );
  cudaCheck( cudaDeviceSynchronize() );
}

void KernelManagerHGCalRecHit::fill_positions(const hgcal_conditions::HeterogeneousHEFCellPositionsConditionsESProduct* d_conds)
{
  fill_positions_from_detids<<<::nblocks_,::nthreads_>>>(d_conds);
  cudaCheck( cudaGetLastError() );
  
  //print_positions_from_detids<<<::nblocks_,::nthreads_>>>(d_conds);
}

HGCRecHitSoA* KernelManagerHGCalRecHit::get_output()
{
  return data_->h_out_;
}
