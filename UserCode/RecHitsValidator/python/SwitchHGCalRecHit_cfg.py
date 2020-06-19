import os, sys, glob
import FWCore.ParameterSet.Config as cms
from Configuration.StandardSequences.Eras import eras
from Configuration.ProcessModifiers.gpu_cff import gpu
from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import HGCalRecHit
from UserCode.RecHitsValidator.HeterogeneousHGCalRecHitsValidator_cfg import HeterogeneousHGCalEERecHits
from UserCode.RecHitsValidator.HeterogeneousHGCalRecHitsValidator_cfg import HeterogeneousHGCalHEFRecHits
from UserCode.RecHitsValidator.HeterogeneousHGCalRecHitsValidator_cfg import HeterogeneousHGCalHEBRecHits

#package loading
process = cms.Process("gpuValidation", gpu) 
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('Configuration.StandardSequences.MagneticField_cff')
#process.load('Configuration.EventContent.EventContent_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load('Configuration.Geometry.GeometryExtended2026D46Reco_cff')
process.load('HeterogeneousCore.CUDAServices.CUDAService_cfi')
process.load('RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi')
process.load('SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi')

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import dEdX_weights_v10
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )

#process.Tracer = cms.Service("Tracer")

#TFileService
fileName = 'switch.root'
process.TFileService = cms.Service("TFileService", 
                                   fileName = cms.string(fileName),
                                   closeFileFast = cms.untracked.bool(True)
                               )

fNames = ['file:/afs/cern.ch/user/b/bfontana/CMSSW_11_1_0_pre6/src/20495.0_CloseByPGun_CE_E_Front_200um+CE_E_Front_200um_2026D41_GenSimHLBeamSpotFull+DigiFullTrigger_2026D41+RecoFullGlobal_2026D41+HARVESTFullGlobal_2026D41/step3.root']
keep = 'keep *'
drop = 'drop CSCDetIdCSCALCTPreTriggerDigiMuonDigiCollection_simCscTriggerPrimitiveDigis__HLT'
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(fNames),
                            inputCommands = cms.untracked.vstring([keep, drop]),
                            duplicateCheckMode = cms.untracked.string("noDuplicateCheck"))

process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool( True ) ) #add option for edmStreams


process.HeterogeneousHGCalEERecHits = HeterogeneousHGCalEERecHits.clone()
process.HeterogeneousHGCalHEFRecHits = HeterogeneousHGCalHEFRecHits.clone()
process.HeterogeneousHGCalHEBRecHits = HeterogeneousHGCalHEBRecHits.clone()
process.task_HeterogeneousHGCalEERecHits = cms.Task( process.HeterogeneousHGCalEERecHits )
process.task_HeterogeneousHGCalHEFRecHits = cms.Task( process.HeterogeneousHGCalHEFRecHits )
process.task_HeterogeneousHGCalHEBRecHits = cms.Task( process.HeterogeneousHGCalHEBRecHits )
process.HGCalRecHits = HGCalRecHit.clone() #CPU version

from HeterogeneousCore.CUDACore.SwitchProducerCUDA import SwitchProducerCUDA
process.switch = SwitchProducerCUDA( cpu = process.HGCalRecHits, # legacy CPU
                                     cuda = cms.EDAlias( HeterogeneousHGCalEERecHitsTask = cms.VPSet( cms.PSet(type = cms.string("HGCEERecHits")) ),
                                                         HeterogeneousHGCalHEFRecHitsTask = cms.VPSet( cms.PSet(type = cms.string("HGCHEFRecHits")) ),
                                                         HeterogeneousHGCalHEBRecHitsTask = cms.VPSet( cms.PSet(type = cms.string("HGCHEBRecHits")) )
                                     ) )

#process.fooCUDA = cms.EDProducer("FooProducerCUDA")
#process.fooTaskCUDA = cms.Task(process.fooCUDA)

process.task = cms.Task( process.switch )
#                         process.fooTaskCUDA )

process.path = cms.Path( process.task )

process.out = cms.OutputModule( "PoolOutputModule", 
                                fileName = cms.untracked.string('out.root') )
process.outpath = cms.EndPath(process.out)
