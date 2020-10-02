import FWCore.ParameterSet.Config as cms

from SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi import hgceeDigitizer, hgchefrontDigitizer, hgchebackDigitizer, hfnoseDigitizer
from SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi import HGCal_setEndOfLifeNoise, HGCAL_ileakParam_toUse, HGCAL_cceParams_toUse

fCPerMIP_v10 = cms.vdouble(2.06,3.43,5.15) #120um, 200um, 300um
integLumi = 3000.
# HGCAL producer of rechits starting from digis
HGCalUncalibRecHit = cms.EDProducer(
    "HGCalUncalibRecHitProducer",
    HGCEEdigiCollection = cms.InputTag('hgcalDigis:EE'),
    HGCEEhitCollection = cms.string('HGCEEUncalibRecHits'),
    HGCHEFdigiCollection = cms.InputTag('hgcalDigis:HEfront'),
    HGCHEFhitCollection = cms.string('HGCHEFUncalibRecHits'),
    HGCHEBdigiCollection = cms.InputTag('hgcalDigis:HEback'),
    HGCHEBhitCollection = cms.string('HGCHEBUncalibRecHits'),
    HGCHFNosedigiCollection = cms.InputTag('hfnoseDigis:HFNose'),
    HGCHFNosehitCollection = cms.string('HGCHFNoseUncalibRecHits'),
    
    HGCEEConfig = cms.PSet(
        isSiFE = cms.bool(True),
        # adc information
        adcNbits      = hgceeDigitizer.digiCfg.feCfg.adcNbits,
        adcSaturation = hgceeDigitizer.digiCfg.feCfg.adcSaturation_fC,
        #tdc information
        tdcNbits      = hgceeDigitizer.digiCfg.feCfg.tdcNbits,
        tdcSaturation = hgceeDigitizer.digiCfg.feCfg.tdcSaturation_fC,
        tdcOnset      = hgceeDigitizer.digiCfg.feCfg.tdcOnset_fC,
        toaLSB_ns     = hgceeDigitizer.digiCfg.feCfg.toaLSB_ns,
        fCPerMIP      = cms.vdouble(1.25,2.57,3.88), #100um, 200um, 300um 
        doseMap           = cms.string('SimCalorimetry/HGCalSimProducers/data/doseParams_3000fb_fluka-3.7.20.txt'),
        scaleByDoseAlgo   = cms.uint32(0),
        scaleByDoseFactor = cms.double(integLumi/3000.),
        ileakParam        = HGCAL_ileakParam_toUse,
        cceParams         = HGCAL_cceParams_toUse,
        ),
    
    HGCHEFConfig = cms.PSet(
        isSiFE = cms.bool(True),
        # adc information
        adcNbits      = hgchefrontDigitizer.digiCfg.feCfg.adcNbits,
        adcSaturation = hgchefrontDigitizer.digiCfg.feCfg.adcSaturation_fC,
        #tdc information
        tdcNbits      = hgchefrontDigitizer.digiCfg.feCfg.tdcNbits,
        tdcSaturation = hgchefrontDigitizer.digiCfg.feCfg.tdcSaturation_fC,
        tdcOnset      = hgchefrontDigitizer.digiCfg.feCfg.tdcOnset_fC,
        toaLSB_ns     = hgchefrontDigitizer.digiCfg.feCfg.toaLSB_ns,
        fCPerMIP      = cms.vdouble(1.25,2.57,3.88), #100um, 200um, 300um
        doseMap           = cms.string('SimCalorimetry/HGCalSimProducers/data/doseParams_3000fb_fluka-3.7.20.txt'),
        scaleByDoseAlgo   = cms.uint32(0),
        scaleByDoseFactor = cms.double(integLumi/3000.),
        ileakParam        = HGCAL_ileakParam_toUse,
        cceParams         = HGCAL_cceParams_toUse,
        ),

    HGCHEBConfig = cms.PSet(
        isSiFE  = cms.bool(True),
        # adc information
        adcNbits      = hgchebackDigitizer.digiCfg.feCfg.adcNbits,
        adcSaturation = hgchebackDigitizer.digiCfg.feCfg.adcSaturation_fC,
        #tdc information
        tdcNbits      = hgchebackDigitizer.digiCfg.feCfg.tdcNbits,
        tdcSaturation = hgchebackDigitizer.digiCfg.feCfg.tdcSaturation_fC,
        tdcOnset      = hgchebackDigitizer.digiCfg.feCfg.tdcOnset_fC,
        toaLSB_ns     = hgchebackDigitizer.digiCfg.feCfg.toaLSB_ns,
        fCPerMIP      = cms.vdouble(1.0,1.0,1.0), #dummy values, it's scintillator
        doseMap           = cms.string('SimCalorimetry/HGCalSimProducers/data/doseParams_3000fb_fluka-3.7.20.txt'),
        scaleByDoseAlgo   = cms.uint32(0),
        scaleByDoseFactor = cms.double(integLumi/3000.),
        ileakParam        = HGCAL_ileakParam_toUse,
        cceParams         = HGCAL_cceParams_toUse,
        ),

    HGCHFNoseConfig = cms.PSet(
        isSiFE = cms.bool(False),
        # adc information
        adcNbits      = hfnoseDigitizer.digiCfg.feCfg.adcNbits,
        adcSaturation = hfnoseDigitizer.digiCfg.feCfg.adcSaturation_fC,
        #tdc information
        tdcNbits      = hfnoseDigitizer.digiCfg.feCfg.tdcNbits,
        tdcSaturation = hfnoseDigitizer.digiCfg.feCfg.tdcSaturation_fC,
        tdcOnset      = hfnoseDigitizer.digiCfg.feCfg.tdcOnset_fC,
        toaLSB_ns     = hfnoseDigitizer.digiCfg.feCfg.toaLSB_ns,
        fCPerMIP      = cms.vdouble(1.25,2.57,3.88), #100um, 200um, 300um
        doseMap           = cms.string('SimCalorimetry/HGCalSimProducers/data/doseParams_3000fb_fluka-3.7.20.txt'),
        scaleByDoseAlgo   = cms.uint32(0),
        scaleByDoseFactor = cms.double(integLumi/3000.),
        ileakParam        = HGCAL_ileakParam_toUse,
        cceParams         = HGCAL_cceParams_toUse,
        ),
    algo = cms.string("HGCalUncalibRecHitWorkerWeights")
)

from Configuration.Eras.Modifier_phase2_hgcalV10_cff import phase2_hgcalV10
phase2_hgcalV10.toModify( HGCalUncalibRecHit.HGCEEConfig , fCPerMIP = fCPerMIP_v10 ) 
phase2_hgcalV10.toModify( HGCalUncalibRecHit.HGCHEFConfig , fCPerMIP = fCPerMIP_v10 )

from Configuration.Eras.Modifier_phase2_hfnose_cff import phase2_hfnose
phase2_hfnose.toModify( HGCalUncalibRecHit.HGCHFNoseConfig ,
          isSiFE = True ,
          fCPerMIP = fCPerMIP_v10
)
