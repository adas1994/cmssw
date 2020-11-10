import os, sys, glob
import FWCore.ParameterSet.Config as cms
from Configuration.StandardSequences.Eras import eras
from Configuration.ProcessModifiers.gpu_cff import gpu
from RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi import HGCalRecHit

#arguments parsing
from FWCore.ParameterSet.VarParsing import VarParsing
F = VarParsing('analysis')
F.register('withGPU',
           0,
           F.multiplicity.singleton,
           F.varType.bool,
           "Whether to run with GPUs or CPUs.")
#F.register('PU',
#           -1,
#           F.multiplicity.singleton,
#           F.varType.int,
#           "Pileup to consider.")
F.parseArguments()

PU=140

#package loading
process = cms.Process("gpuValidation", gpu) 
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load('Configuration.Geometry.GeometryExtended2026D49Reco_cff')
process.load('HeterogeneousCore.CUDAServices.CUDAService_cfi')
process.load('RecoLocalCalo.HGCalRecProducers.HGCalRecHit_cfi')
process.load('SimCalorimetry.HGCalSimProducers.hgcalDigitizer_cfi')
process.load( "HLTrigger.Timer.FastTimerService_cfi" )

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1000) )

#indir = '/eos/user/b/bfontana/Samples/'
indir = '/home/bfontana/'
#filename_suff = 'step3_ttbar_PU' + str(PU)
filename_suff = 'hadd_out_PU' + str(PU) 
fNames = [ 'file:' + x for x in glob.glob(os.path.join(indir, filename_suff + '*.root')) ]
for _ in range(4):
    fNames.extend(fNames)

keep = 'keep *'
drop1 = 'drop HGCRecHitsSorted_HGCalRecHit_HGC*E*RecHits_*'
drop2 = 'drop CSCDetIdCSCALCTPreTriggerDigiMuonDigiCollection_simCscTriggerPrimitiveDigis__HLT'
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(fNames),
                            inputCommands = cms.untracked.vstring(drop1, drop2, keep),
                            duplicateCheckMode = cms.untracked.string("noDuplicateCheck"))

wantSummaryFlag = True
nThreads = 1
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool( wantSummaryFlag )) #add option for edmStreams

process.ThroughputService = cms.Service( "ThroughputService",
                                         eventRange = cms.untracked.uint32( 300 ),
                                         eventResolution = cms.untracked.uint32( 1 ),
                                         printEventSummary = cms.untracked.bool( wantSummaryFlag ),
                                         enableDQM = cms.untracked.bool( False )
                                         #valid only for enableDQM=True
                                         #dqmPath = cms.untracked.string( "HLT/Throughput" ),
                                         #timeRange = cms.untracked.double( 60000.0 ),
                                         #dqmPathByProcesses = cms.untracked.bool( False ),
                                         #timeResolution = cms.untracked.double( 5.828 )
)

process.FastTimerService.enableDQM = False
process.FastTimerService.writeJSONSummary = True
process.FastTimerService.jsonFileName = 'resources.json'
process.MessageLogger.categories.append('ThroughputService')

HeterogeneousHGCalEERecHits = cms.EDProducer( 'HeterogeneousHGCalEERecHitProducer',
                                              HGCEEUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCEEUncalibRecHits'),
                                              HGCEE_keV2DIGI 	     = HGCalRecHit.__dict__['HGCEE_keV2DIGI'],
                                              minValSiPar    	     = HGCalRecHit.__dict__['minValSiPar'],
                                              maxValSiPar    	     = HGCalRecHit.__dict__['maxValSiPar'],
                                              constSiPar     	     = HGCalRecHit.__dict__['constSiPar'],
                                              noiseSiPar     	     = HGCalRecHit.__dict__['noiseSiPar'],
                                              HGCEE_fCPerMIP 	     = HGCalRecHit.__dict__['HGCEE_fCPerMIP'],
                                              HGCEE_isSiFE   	     = HGCalRecHit.__dict__['HGCEE_isSiFE'],
                                              HGCEE_noise_fC 	     = HGCalRecHit.__dict__['HGCEE_noise_fC'],
                                              HGCEE_cce      	     = HGCalRecHit.__dict__['HGCEE_cce'],
                                              rcorr          	     = cms.vdouble( HGCalRecHit.__dict__['thicknessCorrection'][0:3] ),
                                              weights        	     = HGCalRecHit.__dict__['layerWeights'] )

#process.HeterogeneousHGCalHEFCellPositionsFiller = cms.ESProducer("HeterogeneousHGCalHEFCellPositionsFiller")
HeterogeneousHGCalHEFRecHits = cms.EDProducer( 'HeterogeneousHGCalHEFRecHitProducer',
                                               HGCHEFUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCHEFUncalibRecHits'),
                                               HGCHEF_keV2DIGI         = HGCalRecHit.__dict__['HGCHEF_keV2DIGI'],
                                               minValSiPar             = HGCalRecHit.__dict__['minValSiPar'],
                                               maxValSiPar             = HGCalRecHit.__dict__['maxValSiPar'],
                                               constSiPar              = HGCalRecHit.__dict__['constSiPar'],
                                               noiseSiPar              = HGCalRecHit.__dict__['noiseSiPar'],
                                               HGCHEF_fCPerMIP         = HGCalRecHit.__dict__['HGCHEF_fCPerMIP'],
                                               HGCHEF_isSiFE           = HGCalRecHit.__dict__['HGCHEF_isSiFE'],
                                               HGCHEF_noise_fC         = HGCalRecHit.__dict__['HGCHEF_noise_fC'],
                                               HGCHEF_cce              = HGCalRecHit.__dict__['HGCHEF_cce'],
                                               rcorr                   = cms.vdouble( HGCalRecHit.__dict__['thicknessCorrection'][3:6] ),
                                               weights                 = HGCalRecHit.__dict__['layerWeights'] )

HeterogeneousHGCalHEBRecHits = cms.EDProducer( 'HeterogeneousHGCalHEBRecHitProducer',
                                               HGCHEBUncalibRecHitsTok = cms.InputTag('HGCalUncalibRecHit', 'HGCHEBUncalibRecHits'),
                                               HGCHEB_keV2DIGI         = HGCalRecHit.__dict__['HGCHEB_keV2DIGI'],
                                               HGCHEB_noise_MIP        = HGCalRecHit.__dict__['HGCHEB_noise_MIP'],
                                               HGCHEB_isSiFE           = HGCalRecHit.__dict__['HGCHEB_isSiFE'],
                                               weights                 = HGCalRecHit.__dict__['layerWeights'] )

process.HeterogeneousHGCalEERecHits = HeterogeneousHGCalEERecHits
process.HeterogeneousHGCalHEFRecHits = HeterogeneousHGCalHEFRecHits
process.HeterogeneousHGCalHEBRecHits = HeterogeneousHGCalHEBRecHits
process.HGCalRecHits = HGCalRecHit.clone() #CPU version

if F.withGPU:
    process.recHitsTask = cms.Task( process.HeterogeneousHGCalEERecHits, process.HeterogeneousHGCalHEFRecHits, process.HeterogeneousHGCalHEBRecHits )
    outkeeps = ['keep *_*_' + f + '*_*' for f in ['HeterogeneousHGCalEERecHits', 'HeterogeneousHGCalHEFRecHits', 'HeterogeneousHGCalHEBRecHits']]
else:
    process.recHitsTask = cms.Task( process.HGCalRecHits )
    outkeeps = ['keep *_*_' + f + '*_*' for f in ['HGCEERecHits', 'HGCHEFRecHits', 'HGCHEBRecHits'] ]
process.path = cms.Path( process.recHitsTask )

process.out = cms.OutputModule( "PoolOutputModule", 
                                fileName = cms.untracked.string( '/home/bfontana/out_Timing_PU' + str(PU) + '.root'),
                                outputCommands = cms.untracked.vstring(outkeeps[0], outkeeps[1], outkeeps[2]),
)
process.outpath = cms.EndPath(process.out)

print("********************")
print("Input arguments:")
for k,v in F.__dict__["_singletons"].items():
    print("{}: {}".format(k,v))
    print("********************")
