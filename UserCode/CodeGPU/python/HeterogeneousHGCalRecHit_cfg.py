import FWCore.ParameterSet.Config as cms
import os, glob

enableGPU = True
from Configuration.ProcessModifiers.gpu_cff import gpu

from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import HGCalRecHit
from SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi import HGCAL_noise_fC, HGCAL_chargeCollectionEfficiencies

process = cms.Process("TESTgpu", gpu) if enableGPU else cms.Process("TESTnongpu")
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('Configuration.StandardSequences.MagneticField_cff')
#process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load('Configuration.Geometry.GeometryExtended2026D49Reco_cff')
process.load('HeterogeneousCore.CUDAServices.CUDAService_cfi')
process.load('RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi')
process.load('SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi')

process.TFileService = cms.Service("TFileService", 
                                   fileName = cms.string("histo.root"),
                                   closeFileFast = cms.untracked.bool(True)
                               )

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32( 1 ))

indir = '/afs/cern.ch/user/b/bfontana/CMSSW_11_2_0_pre5/src/23234.0_TTbar_14TeV+2026D49+TTbar_14TeV_TuneCP5_GenSimHLBeamSpot14+DigiTrigger+RecoGlobal+HARVESTGlobal'
file_wildcard = 'step3.root'
glob = glob.glob( os.path.join(indir, file_wildcard) )
fNames = ['file:' + it for it in glob][:]

keep = 'keep *'
drop = 'drop CSCDetIdCSCALCTPreTriggerDigiMuonDigiCollection_simCscTriggerPrimitiveDigis__HLT'
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(fNames),
                            inputCommands = cms.untracked.vstring([keep, drop]),
                            duplicateCheckMode = cms.untracked.string("noDuplicateCheck"))

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool( False )) #add option for edmStreams

"""
process.HeterogeneousHGCalEERecHits = cms.EDProducer('HeterogeneousHGCalEERecHitProducer',
                                                     HGCEEUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCEEUncalibRecHits'),
                                                     HGCEE_keV2DIGI = HGCalRecHit.__dict__['HGCEE_keV2DIGI'],
                                                     minValSiPar    = HGCalRecHit.__dict__['minValSiPar'],
                                                     maxValSiPar    = HGCalRecHit.__dict__['maxValSiPar'],
                                                     constSiPar     = HGCalRecHit.__dict__['constSiPar'],
                                                     noiseSiPar     = HGCalRecHit.__dict__['noiseSiPar'],
                                                     HGCEE_fCPerMIP = HGCalRecHit.__dict__['HGCEE_fCPerMIP'],
                                                     HGCEE_isSiFE   = HGCalRecHit.__dict__['HGCEE_isSiFE'],
                                                     HGCEE_noise_fC = HGCalRecHit.__dict__['HGCEE_noise_fC'],
                                                     HGCEE_cce      = HGCalRecHit.__dict__['HGCEE_cce'],
                                                     rcorr          = cms.vdouble( HGCalRecHit.__dict__['thicknessCorrection'][0:3] ),
                                                     weights        = HGCalRecHit.__dict__['layerWeights']
)
"""

process.HeterogeneousHGCalHEFCellPositionsFiller = cms.ESProducer("HeterogeneousHGCalHEFCellPositionsFiller")
process.HeterogeneousHGCalHEFRecHits = cms.EDProducer('HeterogeneousHGCalHEFRecHitProducer',
                                                      HGCHEFUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit','HGCHEFUncalibRecHits'),
                                                      HGCHEF_keV2DIGI  = HGCalRecHit.__dict__['HGCHEF_keV2DIGI'],
                                                      minValSiPar     = HGCalRecHit.__dict__['minValSiPar'],
                                                      maxValSiPar     = HGCalRecHit.__dict__['maxValSiPar'],
                                                      constSiPar      = HGCalRecHit.__dict__['constSiPar'],
                                                      noiseSiPar      = HGCalRecHit.__dict__['noiseSiPar'],
                                                      HGCHEF_fCPerMIP = HGCalRecHit.__dict__['HGCHEF_fCPerMIP'],
                                                      HGCHEF_isSiFE   = HGCalRecHit.__dict__['HGCHEF_isSiFE'],
                                                      HGCHEF_noise_fC = HGCalRecHit.__dict__['HGCHEF_noise_fC'],
                                                      HGCHEF_cce      = HGCalRecHit.__dict__['HGCHEF_cce'],
                                                      rcorr           = cms.vdouble( HGCalRecHit.__dict__['thicknessCorrection'][3:6] ),
                                                      weights         = HGCalRecHit.__dict__['layerWeights'] )

"""
process.HeterogeneousHGCalHEBRecHits = cms.EDProducer('HeterogeneousHGCalHEBRecHitProducer',
                                                      HGCHEBUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCHEBUncalibRecHits'),
                                                      HGCHEB_keV2DIGI  = HGCalRecHit.__dict__['HGCHEB_keV2DIGI'],
                                                      HGCHEB_noise_MIP = HGCalRecHit.__dict__['HGCHEB_noise_MIP'],
                                                      weights          = HGCalRecHit.__dict__['layerWeights'] )
"""
process.HGCalRecHits = HGCalRecHit.clone()

fNameOut = 'out'
process.task = cms.Task( process.HeterogeneousHGCalHEFCellPositionsFiller, process.HeterogeneousHGCalHEFRecHits )
#process.task = cms.Task( process.HGCalRecHits, process.HeterogeneousHGCalHEFRecHits )
process.path = cms.Path( process.task )

process.out = cms.OutputModule("PoolOutputModule",
                               fileName = cms.untracked.string(fNameOut+".root"))
process.outpath = cms.EndPath(process.out)
