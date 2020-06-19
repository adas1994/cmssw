#include "RecoLocalCalo/HGCalESProducers/plugins/HeterogeneousHGCalHEFCellPositionsFiller.h"

HeterogeneousHGCalHEFCellPositionsFiller::HeterogeneousHGCalHEFCellPositionsFiller(const edm::ParameterSet& ps)
{
  //setWhatProduced(this/*, dependsOn(&HeterogeneousHGCalHEFCellPositionsFiller::geometryCallback)*/).setConsumes(geometryToken_);
  geometryToken_ = setWhatProduced(this).consumesFrom<HGCalGeometry, IdealGeometryRecord>(edm::ESInputTag{"", "HGCalHESiliconSensitive"});
  posmap_ = new hgcal_conditions::positions::HGCalPositionsMapping();
}

HeterogeneousHGCalHEFCellPositionsFiller::~HeterogeneousHGCalHEFCellPositionsFiller()
{
  delete posmap_;
}

//the geometry is not required if the layer offset is hardcoded (potential speed-up)
void HeterogeneousHGCalHEFCellPositionsFiller::set_conditions_()
{
  //fill the CPU position structure from the geometry
  posmap_->z_per_layer.clear();
  posmap_->numberCellsHexagon.clear();
  posmap_->detid.clear();

  int upper_estimate_wafer_number  = 2 * ddd_->lastLayer(true) * (ddd_->waferMax() - ddd_->waferMin());
  int upper_estimate_cell_number = upper_estimate_wafer_number * 24 * 24; 
  posmap_->z_per_layer.resize( (ddd_->lastLayer(true) - ddd_->firstLayer() + 1) * 2 );
  posmap_->numberCellsHexagon.reserve(upper_estimate_wafer_number);
  posmap_->detid.reserve(upper_estimate_cell_number);
  //set positons-related variables
  posmap_->waferSize        = static_cast<float>( params_->waferSize_ );
  posmap_->sensorSeparation = static_cast<float>( params_->sensorSeparation_ );
  posmap_->firstLayer       = ddd_->firstLayer();
  assert( posmap_->firstLayer==1 ); //otherwise the loop over the layers has to be changed
  posmap_->lastLayer        = ddd_->lastLayer(true);
  posmap_->waferMin         = ddd_->waferMin();
  posmap_->waferMax         = ddd_->waferMax();

  //store detids following a geometry ordering
  for(int ilayer=1; ilayer<=posmap_->lastLayer; ++ilayer) {
    posmap_->z_per_layer[ilayer - 1] = static_cast<float>( ddd_->waferZ(ilayer, true) ); //originally a double
    posmap_->z_per_layer[ilayer + (ddd_->lastLayer(true) - ddd_->firstLayer())] = static_cast<float>( ddd_->waferZ(ilayer, true) ); //originally a double
    
    for(int iwaferU=posmap_->waferMin; iwaferU<posmap_->waferMax; ++iwaferU) {
      for(int iwaferV=posmap_->waferMin; iwaferV<posmap_->waferMax; ++iwaferV) {
	int type_ = ddd_->waferType(ilayer, iwaferU, iwaferV); //0: fine; 1: coarseThin; 2: coarseThick (as defined in DataFormats/ForwardDetId/interface/HGCSiliconDetId.h)

	int nCellsHex = ddd_->numberCellsHexagon(ilayer, iwaferU, iwaferV, false);
	posmap_->numberCellsHexagon.push_back( nCellsHex );

	//left side of wafer
	for(int cellUmax=nCellsHex, icellV=0; cellUmax<2*nCellsHex && icellV<nCellsHex; ++cellUmax, ++icellV)
	  {
	    for(int icellU=0; icellU<=cellUmax; ++icellU)
	      {
		HGCSiliconDetId detid_(DetId::HGCalHSi, 1, type_, ilayer, iwaferU, iwaferV, icellU, icellV);
		posmap_->detid.push_back( detid_.rawId() );
	      }
	  }
	//right side of wafer
	for(int cellUmin=1, icellV=nCellsHex; cellUmin<=nCellsHex && icellV<2*nCellsHex; ++cellUmin, ++icellV)
	  {
	    for(int icellU=cellUmin; icellU<2*nCellsHex; ++icellU)
	      {
		HGCSiliconDetId detid_(DetId::HGCalHSi, 1, type_, ilayer, iwaferU, iwaferV, icellU, icellV);
		posmap_->detid.push_back( detid_.rawId() );
	      }
	  }
      }
    }
  }
}

std::unique_ptr<HeterogeneousHGCalHEFCellPositionsConditions> HeterogeneousHGCalHEFCellPositionsFiller::produce(const HeterogeneousHGCalHEFCellPositionsConditionsRecord& iRecord)
{
  auto geom = iRecord.getTransientHandle(geometryToken_);
  ddd_ = &( geom->topology().dddConstants() );
  params_ = ddd_->getParameter();

  //const cms::cuda::ScopedContextAcquire ctx{event.streamID(), std::move(w), ctxState_};
  
  set_conditions_();

  HeterogeneousHGCalHEFCellPositionsConditions esproduct(posmap_);
  d_conds = esproduct.getHeterogeneousConditionsESProductAsync( 0 ); //could use ctx.stream()?
  KernelManagerHGCalCellPositions kernel_manager( posmap_->detid.size() );
  kernel_manager.fill_positions(d_conds);
  //std::unique_ptr<HeterogeneousHGCalHEFCellPositionsConditionsESProduct> up(d_conds);

  std::unique_ptr<HeterogeneousHGCalHEFCellPositionsConditions> up = std::make_unique<HeterogeneousHGCalHEFCellPositionsConditions>(posmap_);
  return up;
}

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/typelookup.h"
#include "FWCore/Framework/interface/eventsetuprecord_registration_macro.h"
DEFINE_FWK_EVENTSETUP_MODULE(HeterogeneousHGCalHEFCellPositionsFiller);
