#include <cuda.h>
#include <cuda_runtime.h>
#include <inttypes.h>
#include "DataFormats/ForwardDetId/interface/HGCalDetId.h"
#include "RecoLocalCalo/HGCalESProducers/plugins/HGCalCellPositionsKernelImpl.cuh"

__global__ 
void fill_positions_from_detids(const hgcal_conditions::HeterogeneousHEFCellPositionsConditionsESProduct* conds)
{
  unsigned int tid = blockDim.x * blockIdx.x + threadIdx.x;

  for (unsigned int i = tid; i < conds->nelems_posmap; i += blockDim.x * gridDim.x)
    {
      HeterogeneousHGCSiliconDetId did(conds->posmap.detid[i]);
      const float cU     = static_cast<float>( did.cellU()  );
      const float cV     = static_cast<float>( did.cellV()  );
      const float wU     = static_cast<float>( did.waferU() );
      const float wV     = static_cast<float>( did.waferV() );
      const float ncells = static_cast<float>( did.nCells() );
      const int32_t layer = did.layer();
      
      //based on `std::pair<float, float> HGCalDDDConstants::locateCell(const HGCSiliconDetId&, bool)
      const float r_x2 = conds->posmap.waferSize + conds->posmap.sensorSeparation;
      const float r = 0.5f * r_x2;
      const float sqrt3 = __fsqrt_rn(3.f);
      const float rsqrt3 = __frsqrt_rn(3.f); //rsqrt: 1 / sqrt
      const float R = r_x2 * rsqrt3;
      const float n2 = ncells / 2.f;
      const float yoff_abs = rsqrt3 * r_x2;
      const float yoff = (layer%2==1) ? yoff_abs : -1.f * yoff_abs; //CHANGE according to Sunanda's reply
      float xpos = (-2.f * wU + wV) * r;
      float ypos = yoff + (1.5f * wV * R);
      const float R1 = __fdividef( conds->posmap.waferSize, 3.f * ncells );
      const float r1_x2 = R1 * sqrt3;
      xpos += (1.5f * (cV - ncells) + 1.f) * R1;
      ypos += (cU - 0.5f * cV - n2) * r1_x2;

      conds->posmap.x[i] = xpos; //* side; multiply by -1 if one wants to obtain the position from the opposite endcap. CAREFUL WITH LATER DETECTOR ALIGNMENT!!!
      conds->posmap.y[i] = ypos;
    }
}

__global__
void print_positions_from_detids(const hgcal_conditions::HeterogeneousHEFCellPositionsConditionsESProduct* conds)
{
  unsigned int tid = blockDim.x * blockIdx.x + threadIdx.x;

  for (unsigned int i = tid; i < conds->nelems_posmap; i += blockDim.x * gridDim.x)
    {
      HeterogeneousHGCSiliconDetId did(conds->posmap.detid[i]);
      const int32_t layer = did.layer();
      float posz = conds->posmap.z_per_layer[ layer-1 ];
      printf("PosX: %lf\t PosY: %lf\t Posz: %lf\n", conds->posmap.x[i], conds->posmap.y[i], posz);
    } 
}

__global__
void test(const hgcal_conditions::HeterogeneousHEFCellPositionsConditionsESProduct* conds)
{
  unsigned int tid = blockDim.x * blockIdx.x + threadIdx.x;

  if(tid == 0)
    {
      printf("Nelems: %u\n", static_cast<unsigned>(conds->nelems_posmap));
      for(unsigned i=0; i<10; ++i)
	{
	  printf("%lf ", conds->posmap.z_per_layer[i]);
	  printf("%lf ", conds->posmap.x[i]);
	  printf("\n");
	}
    }
}
