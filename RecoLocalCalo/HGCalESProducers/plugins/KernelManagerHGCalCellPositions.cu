#include <cuda.h>
#include <cuda_runtime.h>
#include <inttypes.h>
#include "RecoLocalCalo/HGCalESProducers/plugins/KernelManagerHGCalCellPositions.h"
#include "RecoLocalCalo/HGCalESProducers/plugins/HGCalCellPositionsKernelImpl.cuh"

KernelManagerHGCalCellPositions::KernelManagerHGCalCellPositions(const size_t& nelems)
{
  ::nb_celpos_ = (nelems + ::nt_celpos_.x - 1) / ::nt_celpos_.x;
}

void KernelManagerHGCalCellPositions::fill_positions(const hgcal_conditions::HeterogeneousHEFCellPositionsConditionsESProduct* d_conds)
{
  fill_positions_from_detids<<<::nb_celpos_,::nt_celpos_>>>(d_conds);
  cudaCheck( cudaGetLastError() );
}

void KernelManagerHGCalCellPositions::test_cell_positions(const hgcal_conditions::HeterogeneousHEFCellPositionsConditionsESProduct* d_conds)
{
  test<<<::nb_celpos_,::nt_celpos_>>>(d_conds);
  cudaCheck( cudaGetLastError() );
}
