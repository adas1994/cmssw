import os, subprocess
import numpy as np
import bokehplot as bkp
from bokeh.models.ranges import FactorRange

eos_folder = '/eos/user/'
user = subprocess.check_output(b'echo $USER', shell=True, encoding='utf-8').split('\n')[0]
path = os.path.join(eos_folder, user[0], user, 'www/GPUWork')

nfigs, npoints = 7, 5
nexperiments = 3
b = bkp.BokehPlot(os.path.join(path, 'measurements.html'), nfigs=nfigs)

x = np.arange(1, npoints+1)

options = {'width': 400, 'height': 350, 
           't.text_font_size': '9pt',
           'x.axis_label': 'Pileup',
           'x.ticker': [1, 2, 3, 4, 5], 
           'x.major_label_overrides': {1: '0', 2: '50', 3: '100', 4: '140', 5: '200'},
           'l.click_policy': 'hide'}
style = 'circle'
title1 = 'RecHit calibrator speed-up'
title2 = 'CPU/GPU speed-up'
title3 = 'RecHit calibrator throughput'
colours = ['green', 'blue', 'red']
threadsdict = {0: 256, 1: 512, 2: 1024}

sGPUy = [ np.array([0.004991, 0.016372, 0.029064, 0.036054, 0.053280]), #256 threads
          np.array([0.005375, 0.015516, 0.025280, 0.031587, 0.039127]), #512 threads
          np.array([0.005474, 0.015060, 0.024048, 0.032301, 0.041301]) ] #1024 threads
sCPUy = np.array([0.054225, 0.083921, 0.098404, 0.115801, 0.128898])
sRATy = [ sCPUy / g for g in sGPUy ]

#speed-up measurements
for ifig in range(nexperiments):
    options.update( {'t.text': title1+'\n{} threads/block'.format(threadsdict[ifig]),
                     'y.axis_label': 'Seconds per event'} )
    b.graph(idx=ifig, data=[x,sCPUy], style=style, color='blue', line=True, legend_label='CPU', fig_kwargs=options)
    b.graph(idx=ifig, data=[x,sGPUy[ifig]], style=style, color='red', line=True, legend_label='GPU')
    options.pop('t.text')
    options.update( {'t.text': title2, 'y.axis_label': 'Speed-up'} )
    b.graph(idx=nexperiments, data=[x,sRATy[ifig]], style=style, color=colours[ifig], line=True, legend_label=str(threadsdict[ifig])+' threads/block', fig_kwargs=options)
    options.pop('t.text')

tGPUy = [ np.array([1, 1, 1, 1, 1]), #256 threads
          np.array([1, 1, 1, 1, 1]), #512 threads
          np.array([1, 1, 1, 1, 1]) ] #1024 threads
#tCPUy = np.array([1, 1, 1, 1, 1])
tRATy = [ sCPUy / g for g in sGPUy ]

#throughput measurements
for ifig in range(nexperiments+1,nfigs):
    data_idx = ifig-nexperiments-1
    options.update( {'t.text': title3+'\n{} threads/block'.format(threadsdict[data_idx]),
                     'y.axis_label': 'Throughput [ev/s]'} )
    #b.graph(idx=ifig, data=[x,tCPUy], style=style, color='blue', line=True, legend_label='CPU')
    b.graph(idx=ifig, data=[x,tGPUy[data_idx]], style=style, color='red', line=True, legend_label='GPU', fig_kwargs=options)
    options.pop('t.text')
    options.update( {'t.text': title2, 'y.axis_label': 'Throughput'} )
    b.graph(idx=nexperiments, data=[x,tRATy[data_idx]], style=style, color=colours[data_idx], line=True, legend_label=str(threadsdict[data_idx])+' threads/block', fig_kwargs=options)
    options.pop('t.text')

b.save_frames(path=path, layout=[[0,1,2],[3],[4,5,6]])
#b.save_figs(path=path, mode='png')
