#include "HeterogeneousHGCalHEFRecHitProducer.h"

HeterogeneousHGCalHEFRecHitProducer::HeterogeneousHGCalHEFRecHitProducer(const edm::ParameterSet& ps):
  token_(consumes<HGCUncalibratedRecHitCollection>(ps.getParameter<edm::InputTag>("HGCHEFUncalibRecHitsTok")))
{
  cdata_.keV2DIGI_          = ps.getParameter<double>("HGCHEF_keV2DIGI");
  cdata_.xmin_              = ps.getParameter<double>("minValSiPar"); //float
  cdata_.xmax_              = ps.getParameter<double>("maxValSiPar"); //float
  cdata_.aterm_             = ps.getParameter<double>("noiseSiPar"); //float
  cdata_.cterm_             = ps.getParameter<double>("constSiPar"); //float
  vdata_.fCPerMIP_          = ps.getParameter< std::vector<double> >("HGCHEF_fCPerMIP");
  vdata_.cce_               = ps.getParameter<edm::ParameterSet>("HGCHEF_cce").getParameter<std::vector<double> >("values");
  vdata_.noise_fC_          = ps.getParameter<edm::ParameterSet>("HGCHEF_noise_fC").getParameter<std::vector<double> >("values");
  vdata_.rcorr_             = ps.getParameter< std::vector<double> >("rcorr");
  vdata_.weights_           = ps.getParameter< std::vector<double> >("weights");
  cdata_.uncalib2GeV_ = 1e-6 / cdata_.keV2DIGI_;
  assert_sizes_constants_(vdata_);

  uncalibSoA_        = new HGCUncalibratedRecHitSoA();
  d_uncalibSoA_      = new HGCUncalibratedRecHitSoA();
  d_intermediateSoA_ = new HGCUncalibratedRecHitSoA();
  d_calibSoA_        = new HGCRecHitSoA();
  calibSoA_          = new HGCRecHitSoA();
  kcdata_            = new KernelConstantData<HGChefUncalibratedRecHitConstantData>(cdata_, vdata_);
  kmdata_            = new KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>(uncalibSoA_, d_uncalibSoA_, d_intermediateSoA_, d_calibSoA_, calibSoA_);

  tools_ = std::make_unique<hgcal::RecHitTools>();
  produces<HGChefRecHitCollection>(collection_name_);

  /*
  x0 = fs->make<TH1F>( "x_type0"  , "x_type0", 300, -120., 120. );
  y0 = fs->make<TH1F>( "y_type0"  , "y_type0", 300, -120., 120. );
  x1 = fs->make<TH1F>( "x_type1"  , "x_type1", 300, -120., 120. );
  y1 = fs->make<TH1F>( "y_type1"  , "y_type1", 300, -120., 120. );
  x2 = fs->make<TH1F>( "x_type2"  , "x_type2", 300, -120., 120. );
  y2 = fs->make<TH1F>( "y_type2"  , "y_type2", 300, -120., 120. );
  */
}

HeterogeneousHGCalHEFRecHitProducer::~HeterogeneousHGCalHEFRecHitProducer()
{
  delete kmdata_;
  delete kcdata_;
  delete uncalibSoA_;
  delete d_uncalibSoA_;
  delete d_intermediateSoA_;
  delete d_calibSoA_;
  delete calibSoA_;
}

std::string HeterogeneousHGCalHEFRecHitProducer::assert_error_message_(std::string var, const size_t& s1, const size_t& s2)
{
  std::string str1 = "The '";
  std::string str2 = "' array must be at least of size ";
  std::string str3 = " to hold the configuration data, but is of size ";
  return str1 + var + str2 + std::to_string(s1) + str3 + std::to_string(s2);
}

void HeterogeneousHGCalHEFRecHitProducer::assert_sizes_constants_(const HGCConstantVectorData& vd)
{
  if( vdata_.fCPerMIP_.size() > maxsizes_constants::hef_fCPerMIP )
    cms::cuda::LogError("WrongSize") << this->assert_error_message_("fCPerMIP", maxsizes_constants::hef_fCPerMIP, vdata_.fCPerMIP_.size());
  else if( vdata_.cce_.size() > maxsizes_constants::hef_cce )
    cms::cuda::LogError("WrongSize") << this->assert_error_message_("cce", maxsizes_constants::hef_cce, vdata_.cce_.size());
  else if( vdata_.noise_fC_.size() > maxsizes_constants::hef_noise_fC )
    cms::cuda::LogError("WrongSize") << this->assert_error_message_("noise_fC", maxsizes_constants::hef_noise_fC, vdata_.noise_fC_.size());
  else if( vdata_.rcorr_.size() > maxsizes_constants::hef_rcorr )
    cms::cuda::LogError("WrongSize") << this->assert_error_message_("rcorr", maxsizes_constants::hef_rcorr, vdata_.rcorr_.size());
  else if( vdata_.weights_.size() > maxsizes_constants::hef_weights ) 
    cms::cuda::LogError("WrongSize") << this->assert_error_message_("weights", maxsizes_constants::hef_weights, vdata_.weights_.size());
}

void HeterogeneousHGCalHEFRecHitProducer::beginRun(edm::Run const&, edm::EventSetup const& setup) {
  edm::ESHandle<CaloGeometry> baseGeom;
  setup.get<CaloGeometryRecord>().get(baseGeom);
  tools_->setGeometry(*baseGeom);
  
  std::string handle_str = "HGCalHESiliconSensitive";
  edm::ESHandle<HGCalGeometry> handle;
  setup.get<IdealGeometryRecord>().get(handle_str, handle);
  
  ddd_ = &( handle->topology().dddConstants() );
  params_ = ddd_->getParameter();
  cdata_.layerOffset_ = params_->layerOffset_; //=28 (6-07-2020)
}

void HeterogeneousHGCalHEFRecHitProducer::acquire(edm::Event const& event, edm::EventSetup const& setup, edm::WaitingTaskWithArenaHolder w) {
  const cms::cuda::ScopedContextAcquire ctx{event.streamID(), std::move(w), ctxState_};

  //
  edm::ESHandle<HeterogeneousHGCalHEFCellPositionsConditions> celpos_handle;
  setup.get<HeterogeneousHGCalHEFCellPositionsConditionsRecord>().get(celpos_handle);
  hgcal_conditions::HeterogeneousHEFCellPositionsConditionsESProduct const * celpos = celpos_handle->getHeterogeneousConditionsESProductAsync( 0 );
  //
    
  /*
  HeterogeneousHGCalHEFConditionsWrapper esproduct(params_, posmap_);
  d_conds = esproduct.getHeterogeneousConditionsESProductAsync(ctx.stream());
  */

  event.getByToken(token_, handle_hef_);
  const auto &hits_hef = *handle_hef_;

  unsigned int nhits = hits_hef.size();
  stride_ = ( (nhits-1)/32 + 1 ) * 32; //align to warp boundary
  rechits_ = std::make_unique<HGCRecHitCollection>();

  if(stride_ > 0)
    {
      kmdata_->nhits_  = nhits;
      kmdata_->stride_ = stride_;
      allocate_memory_(ctx.stream());
      convert_constant_data_(kcdata_);
      convert_collection_data_to_soa_(hits_hef, kmdata_);

      KernelManagerHGCalRecHit kernel_manager(kmdata_);
      kernel_manager.run_kernels(kcdata_, ctx.stream());

      //KernelManagerHGCalCellPositions kernel_manager_celpos( 1 ); //test with one single item (one block of one thread)
      //kernel_manager_celpos.test_cell_positions( celpos );
    }
}

void HeterogeneousHGCalHEFRecHitProducer::produce(edm::Event& event, const edm::EventSetup& setup)
{
  cms::cuda::ScopedContextProduce ctx{ctxState_}; //only for GPU to GPU producers

  convert_soa_data_to_collection_(*rechits_, kmdata_);
  event.put(std::move(rechits_), collection_name_);
}

void HeterogeneousHGCalHEFRecHitProducer::allocate_memory_(const cudaStream_t& stream)
{
  //_allocate memory for hits on the host
  memory::allocation::host(stride_, uncalibSoA_, mem_in_, stream);
  //_allocate memory for hits on the device
  memory::allocation::device(stride_, d_uncalibSoA_, d_intermediateSoA_, d_calibSoA_, d_mem_, stream);
  //_allocate memory for hits on the host
  memory::allocation::host(stride_, calibSoA_, mem_out_, stream);
}

void HeterogeneousHGCalHEFRecHitProducer::convert_constant_data_(KernelConstantData<HGChefUncalibratedRecHitConstantData> *kcdata)
{
  for(size_t i=0; i<kcdata->vdata_.fCPerMIP_.size(); ++i)
    kcdata->data_.fCPerMIP_[i] = kcdata->vdata_.fCPerMIP_[i];
  for(size_t i=0; i<kcdata->vdata_.cce_.size(); ++i)
    kcdata->data_.cce_[i] = kcdata->vdata_.cce_[i];
  for(size_t i=0; i<kcdata->vdata_.noise_fC_.size(); ++i)
    kcdata->data_.noise_fC_[i] = kcdata->vdata_.noise_fC_[i];
  for(size_t i=0; i<kcdata->vdata_.rcorr_.size(); ++i)
    kcdata->data_.rcorr_[i] = kcdata->vdata_.rcorr_[i];
  for(size_t i=0; i<kcdata->vdata_.weights_.size(); ++i)
    kcdata->data_.weights_[i] = kcdata->vdata_.weights_[i];
}

void HeterogeneousHGCalHEFRecHitProducer::convert_collection_data_to_soa_(const HGChefUncalibratedRecHitCollection& hits, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>* kmdata)
{
  for(unsigned int i=0; i<kmdata->nhits_; ++i)
    {
      kmdata->h_in_->amplitude_[i]    = hits[i].amplitude();
      kmdata->h_in_->pedestal_[i]     = hits[i].pedestal();
      kmdata->h_in_->jitter_[i]       = hits[i].jitter();
      kmdata->h_in_->chi2_[i]         = hits[i].chi2();
      kmdata->h_in_->OOTamplitude_[i] = hits[i].outOfTimeEnergy();
      kmdata->h_in_->OOTchi2_[i]      = hits[i].outOfTimeChi2();
      kmdata->h_in_->flags_[i]        = hits[i].flags();
      kmdata->h_in_->aux_[i]          = 0;
      kmdata->h_in_->id_[i]           = hits[i].id().rawId();
    }
}

void HeterogeneousHGCalHEFRecHitProducer::convert_soa_data_to_collection_(HGCRecHitCollection& rechits, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>* kmdata)
{
  rechits.reserve(kmdata->nhits_);
  for(uint i=0; i<kmdata->nhits_; ++i)
    {
      DetId id_converted( kmdata->h_out_->id_[i] );
      rechits.emplace_back( HGCRecHit(id_converted, kmdata->h_out_->energy_[i], kmdata->h_out_->time_[i], 0, kmdata->h_out_->flagBits_[i], kmdata->h_out_->son_[i], kmdata->h_out_->timeError_[i]) );
    }
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(HeterogeneousHGCalHEFRecHitProducer);
