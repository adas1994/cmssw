import os, sys, glob
import FWCore.ParameterSet.Config as cms
from Configuration.StandardSequences.Eras import eras
from Configuration.ProcessModifiers.gpu_cff import gpu
from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import HGCalRecHit

#package loading
process = cms.Process("gpuValidation", gpu) 
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

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(5) )

#TFileService
dirName = '/eos/user/b/bfontana/Samples/'
fileName = 'validation.root'
process.TFileService = cms.Service("TFileService", 
                                   fileName = cms.string( os.path.join(dirName,fileName) ),
                                   closeFileFast = cms.untracked.bool(True)
                               )


fNames = ['file:/eos/user/b/bfontana/Samples/step3_ttbar_PU0.root']

keep = 'keep *'
drop1 = 'drop CSCDetIdCSCALCTPreTriggerDigiMuonDigiCollection_simCscTriggerPrimitiveDigis__HLT'
drop2 = 'drop *_HGCalRecHit_*_*'
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(fNames),
                            inputCommands = cms.untracked.vstring([keep, drop1, drop2]),
                            duplicateCheckMode = cms.untracked.string("noDuplicateCheck"))

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool( False )) #add option for edmStreams
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

process.HeterogeneousHGCalHEFRecHits = cms.EDProducer( 'HeterogeneousHGCalHEFRecHitProducer',
                                                       HGCHEFUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCHEFUncalibRecHits'),
                                                       HGCHEF_keV2DIGI = HGCalRecHit.__dict__['HGCHEF_keV2DIGI'],
                                                       minValSiPar     = HGCalRecHit.__dict__['minValSiPar'],
                                                       maxValSiPar     = HGCalRecHit.__dict__['maxValSiPar'],
                                                       constSiPar      = HGCalRecHit.__dict__['constSiPar'],
                                                       noiseSiPar      = HGCalRecHit.__dict__['noiseSiPar'],
                                                       HGCHEF_fCPerMIP = HGCalRecHit.__dict__['HGCHEF_fCPerMIP'],
                                                       HGCHEF_isSiFE   = HGCalRecHit.__dict__['HGCHEF_isSiFE'],
                                                       HGCHEF_noise_fC = HGCalRecHit.__dict__['HGCHEF_noise_fC'],
                                                       HGCHEF_cce      = HGCalRecHit.__dict__['HGCHEF_cce'],
                                                       rangeMatch      = HGCalRecHit.__dict__['rangeMatch'],
                                                       rangeMask       = HGCalRecHit.__dict__['rangeMask'],
                                                       rcorr           = cms.vdouble( HGCalRecHit.__dict__['thicknessCorrection'][3:6] ),
                                                       weights         = HGCalRecHit.__dict__['layerWeights'] 
)

process.HeterogeneousHGCalHEBRecHits = cms.EDProducer('HeterogeneousHGCalHEBRecHitProducer',
                                                      HGCHEBUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCHEBUncalibRecHits'),
                                                      HGCHEB_keV2DIGI  = HGCalRecHit.__dict__['HGCHEB_keV2DIGI'],
                                                      HGCHEB_noise_MIP = HGCalRecHit.__dict__['HGCHEB_noise_MIP'],
                                                      weights          = HGCalRecHit.__dict__['layerWeights'] 
)
process.HGCalRecHits = HGCalRecHit.clone()

"""
dic1 = process.HeterogeneousHGCalHEFRecHits.__dict__
for k,v in dic1.items():
    print(k, v)
print('')
print(' ===================================== ')
print(' ===================================== ')
print(' ===================================== ')
print('')
dic2 = process.HGCalRecHits.__dict__
for k,v in dic2.items():
    print(k, v)
print( HGCalRecHit.__dict__['thicknessCorrection'][3:6] )
print( HGCalRecHit.__dict__['HGCHEF_cce'] )
quit()
"""

process.valid = cms.EDAnalyzer( "HeterogeneousHGCalRecHitsValidator",
                                cpuRecHitsEEToken = cms.InputTag('HGCalRecHits', 'HGCEERecHits'),
                                gpuRecHitsEEToken = cms.InputTag('HeterogeneousHGCalEERecHits','HeterogeneousHGCalEERecHits'),
                                cpuRecHitsHSiToken = cms.InputTag('HGCalRecHits', 'HGCHEFRecHits'),
                                gpuRecHitsHSiToken = cms.InputTag('HeterogeneousHGCalHEFRecHits','HeterogeneousHGCalHEFRecHits'),
                                cpuRecHitsHSciToken = cms.InputTag('HGCalRecHits', 'HGCHEFRecHits'),
                                gpuRecHitsHSciToken = cms.InputTag('HeterogeneousHGCalHEFRecHits','HeterogeneousHGCalHEFRecHits')
)

process.recHitsTask = cms.Task( process.HGCalRecHits, process.HeterogeneousHGCalEERecHits, process.HeterogeneousHGCalHEFRecHits, process.HeterogeneousHGCalHEBRecHits )
#process.recHitsTask = cms.Task( process.HGCalRecHits, process.HeterogeneousHGCalHEFRecHits )
process.path = cms.Path( process.valid, process.recHitsTask )

process.out = cms.OutputModule( "PoolOutputModule", 
                                fileName = cms.untracked.string( os.path.join(dirName, 'out.root') ) )
process.outpath = cms.EndPath(process.out)
