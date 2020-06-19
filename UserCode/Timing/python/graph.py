import os, subprocess
import numpy as np
import bokehplot as bkp
from bokeh.models.ranges import FactorRange
"""data
PU0, GPU: 0.005740 EE, 0.000117 HEB, 0.001656 HEF
PU0, CPU: 0.061452  HGCalRecHits
PU50, GPU: 0.005318 EE, 0.000125 HEB, 0.001676 HEF

"""
npoints = 5
gpu_x = np.arange(1, npoints+1)
gpu_y = np.array([1.,2.,3.,5.,4.])
cpu_x = gpu_x
cpu_y = gpu_y*2.
ratio_x = cpu_x
ratio_y = cpu_y / gpu_y
b = bkp.BokehPlot('speedup.html', nfigs=1)

options = {'t.text': 'RecHit calibrator: GPU vs CPU performance',
           'x.axis_label': 'Pileup', 'y.axis_label': '',
           'x.ticker': [1, 2, 3, 4, 5], 
           'x.major_label_overrides': {1: '0', 2: '50', 3: '100', 4: '140', 5: '200'} }
b.graph(idx=0, data=[gpu_x,cpu_y], style='circle', color='blue', line=True, legend_label="GPU", fig_kwargs=options)
b.graph(idx=0, data=[gpu_x,gpu_y], style='square', color='red', line=True, legend_label="CPU")
b.graph(idx=0, data=[ratio_x,ratio_y], style='triangle', color='green', line=True, legend_label="speed-up")

home = subprocess.check_output(b'echo $HOME', shell=True, encoding='utf-8').split('\n')[0]
release = 'CMSSW_11_2_0_pre6/src/' 
path = os.path.join(home, release, 'figs/')
b.save_figs(path=path, mode='png')
