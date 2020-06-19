#include "HeterogeneousHGCalHEBRecHitProducer.h"

HeterogeneousHGCalHEBRecHitProducer::HeterogeneousHGCalHEBRecHitProducer(const edm::ParameterSet& ps):
  token_(consumes<HGCUncalibratedRecHitCollection>(ps.getParameter<edm::InputTag>("HGCHEBUncalibRecHitsTok")))
{
  cdata_.keV2DIGI_   = ps.getParameter<double>("HGCHEB_keV2DIGI");
  cdata_.noise_MIP_  = ps.getParameter<edm::ParameterSet>("HGCHEB_noise_MIP").getParameter<double>("noise_MIP");
  vdata_.weights_    = ps.getParameter< std::vector<double> >("weights");
  cdata_.uncalib2GeV_ = 1e-6 / cdata_.keV2DIGI_;
  assert_sizes_constants_(vdata_);

  uncalibSoA_        = new HGCUncalibratedRecHitSoA();
  d_uncalibSoA_      = new HGCUncalibratedRecHitSoA();
  d_intermediateSoA_ = new HGCUncalibratedRecHitSoA();
  d_calibSoA_        = new HGCRecHitSoA();
  calibSoA_          = new HGCRecHitSoA();
  kcdata_            = new KernelConstantData<HGChebUncalibratedRecHitConstantData>(cdata_, vdata_);  
  kmdata_            = new KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>(uncalibSoA_, d_uncalibSoA_, d_intermediateSoA_, d_calibSoA_, calibSoA_);

  tools_ = std::make_unique<hgcal::RecHitTools>();
  produces<HGChebRecHitCollection>(collection_name_);
}

HeterogeneousHGCalHEBRecHitProducer::~HeterogeneousHGCalHEBRecHitProducer()
{
  delete kmdata_;
  delete kcdata_;
  delete uncalibSoA_;
  delete d_uncalibSoA_;
  delete d_intermediateSoA_;
  delete d_calibSoA_;
  delete calibSoA_;
}

std::string HeterogeneousHGCalHEBRecHitProducer::assert_error_message_(std::string var, const size_t& s)
{
  std::string str1 = "The '";
  std::string str2 = "' array must be at least of size ";
  std::string str3 = " to hold the configuration data.";
  return str1 + var + str2 + std::to_string(s) + str3;
}

void HeterogeneousHGCalHEBRecHitProducer::assert_sizes_constants_(const HGCConstantVectorData& vd)
{
  if( vdata_.weights_.size() > maxsizes_constants::heb_weights )
    cms::cuda::LogError("MaxSizeExceeded") << this->assert_error_message_("weights", vdata_.fCPerMIP_.size());
}

void HeterogeneousHGCalHEBRecHitProducer::beginRun(edm::Run const&, edm::EventSetup const& setup)
{
  edm::ESHandle<CaloGeometry> baseGeom;
  setup.get<CaloGeometryRecord>().get(baseGeom);
  tools_->setGeometry(*baseGeom);

  std::string handle_str;
  handle_str = "HGCalHEScintillatorSensitive";
  edm::ESHandle<HGCalGeometry> geom;
  setup.get<IdealGeometryRecord>().get(handle_str, geom);

  ddd_ = &( geom->topology().dddConstants() );
  params_ = ddd_->getParameter();
  cdata_.layerOffset_ = params_->layerOffset_; //=28 (30-07-2020)
}

void HeterogeneousHGCalHEBRecHitProducer::acquire(edm::Event const& event, edm::EventSetup const& setup, edm::WaitingTaskWithArenaHolder w) {
  const cms::cuda::ScopedContextAcquire ctx{event.streamID(), std::move(w), ctxState_};
  
  event.getByToken(token_, handle_heb_);
  const auto &hits_heb = *handle_heb_;

  unsigned nhits = hits_heb.size();
  stride_ = ( (nhits-1)/32 + 1 ) * 32; //align to warp boundary
  rechits_ = std::make_unique<HGCRecHitCollection>();
  
  if(stride_ > 0)
    {
      kmdata_->nhits_ = nhits;
      kmdata_->stride_ = stride_;
      allocate_memory_(ctx.stream());
      convert_constant_data_(kcdata_);
      convert_collection_data_to_soa_(hits_heb, kmdata_);

      KernelManagerHGCalRecHit kernel_manager(kmdata_);
      kernel_manager.run_kernels(kcdata_, ctx.stream());
    }
}

void HeterogeneousHGCalHEBRecHitProducer::produce(edm::Event& event, const edm::EventSetup& setup)
{
  cms::cuda::ScopedContextProduce ctx{ctxState_}; //only for GPU to GPU producers

  convert_soa_data_to_collection_(*rechits_, kmdata_);
  event.put(std::move(rechits_), collection_name_);
}

void HeterogeneousHGCalHEBRecHitProducer::allocate_memory_(const cudaStream_t& stream)
{
  //_allocate memory for hits on the host
  memory::allocation::host(stride_, uncalibSoA_, mem_in_, stream);
  //_allocate memory for hits on the device
  memory::allocation::device(stride_, d_uncalibSoA_, d_intermediateSoA_, d_calibSoA_, d_mem_, stream);
  //_allocate memory for hits on the host
  memory::allocation::host(stride_, calibSoA_, mem_out_, stream);
}
  
void HeterogeneousHGCalHEBRecHitProducer::convert_constant_data_(KernelConstantData<HGChebUncalibratedRecHitConstantData> *kcdata)
{
  for(size_t i=0; i<kcdata->vdata_.weights_.size(); ++i)
    kcdata->data_.weights_[i] = kcdata->vdata_.weights_[i];
}

void HeterogeneousHGCalHEBRecHitProducer::convert_collection_data_to_soa_(const edm::SortedCollection<HGCUncalibratedRecHit>& hits, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>* kmdata)
{
  for(unsigned i=0; i<kmdata->nhits_; ++i)
    {
      kmdata->h_in_->amplitude_[i] = hits[i].amplitude();
      kmdata->h_in_->pedestal_[i] = hits[i].pedestal();
      kmdata->h_in_->jitter_[i] = hits[i].jitter();
      kmdata->h_in_->chi2_[i] = hits[i].chi2();
      kmdata->h_in_->OOTamplitude_[i] = hits[i].outOfTimeEnergy();
      kmdata->h_in_->OOTchi2_[i] = hits[i].outOfTimeChi2();
      kmdata->h_in_->flags_[i] = hits[i].flags();
      kmdata->h_in_->aux_[i] = 0;
      kmdata->h_in_->id_[i] = hits[i].id().rawId();
    }
}

void HeterogeneousHGCalHEBRecHitProducer::convert_soa_data_to_collection_(HGCRecHitCollection& rechits, KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>* kmdata)
{
  rechits.reserve(kmdata->nhits_);
  for(unsigned i=0; i<kmdata->nhits_; ++i)
    {
      DetId id_converted( kmdata->h_out_->id_[i] );
      rechits.emplace_back( HGCRecHit(id_converted, kmdata->h_out_->energy_[i], kmdata->h_out_->time_[i], 0, kmdata->h_out_->flagBits_[i], kmdata->h_out_->son_[i], kmdata->h_out_->timeError_[i]) );
    }
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(HeterogeneousHGCalHEBRecHitProducer);
