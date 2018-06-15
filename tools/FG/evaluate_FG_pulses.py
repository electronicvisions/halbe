# -*- coding: utf-8 -*-
# <nbformat>3.0</nbformat>

import sys
import pylab
import datetime,time

pylab.rcParams['figure.figsize'] = 16, 12

import matplotlib.pyplot as plt
import numpy as np

import pickle
input = open(sys.argv[1])
cells = pickle.load(input)
input.close()

num_runs=cells['config']['num_runs']
num_bins=200

ups = [[] for i in range(num_bins)]
downs = [[] for i in range(num_bins)]

bins = np.linspace(0,1.8,num_bins)

for cell_runs in cells['data']:
    last_value = -1
    for ii,run in enumerate(cell_runs):
        if last_value == -1:
            last_value = run
        else:
            if ii < num_runs:
                hist = np.digitize([last_value],bins)
                downs[hist[0]].append(run-last_value)
                #try:
                #    assert(run<last_value)
                #except:
                #    print run,last_value
            else:
                hist = np.digitize([last_value],bins)
                ups[hist[0]].append(run-last_value)
                #assert(run>last_value)
            
            if ii == num_runs-1:
                last_value = -1
            else:
                last_value = run


down_means = []
down_std = []
for i in range(num_bins):
    down_means.append(np.mean(downs[i]))
    down_std.append(np.std(downs[i]))

up_means = []
up_std = []
for i in range(num_bins):
    up_means.append(np.mean(ups[i]))
    up_std.append(np.std(ups[i]))


fig = plt.figure(figsize=(8,12))
fig.subplots_adjust(hspace=.5)
plt.suptitle(str(cells['config']['ip'])+':'+str(cells['config']['dnc'])+':'+str(cells['config']['hicann'])+':'+str(cells['config']['block'])+':'+cells['config']['row'])

plt.subplot(411)
plt.title('down')
plt.xlabel('FG output voltage before pulse [V]')
plt.ylabel('pulse impace [V]')
plt.errorbar(bins,down_means,down_std)

means = []
stddevs = []

for run in range(num_runs):
    means.append(np.mean(cells['data'][:,run]))
    stddevs.append(np.std(cells['data'][:,run]))

plt.subplot(412)
plt.xlabel('number of applied pulses')
plt.ylabel('FG output voltage [V]')
plt.errorbar(np.arange(num_runs),means,stddevs)
plt.text(170,0.6, str(cells['config']['down']))

plt.subplot(413)
plt.title('up')
plt.xlabel('FG output voltage before pulse [V]')
plt.ylabel('pulse impace [V]')
plt.errorbar(bins,up_means,up_std)

means = []
stddevs = []

for run in range(num_runs,2*num_runs):
    means.append(np.mean(cells['data'][:,run]))
    stddevs.append(np.std(cells['data'][:,run]))

plt.subplot(414)
plt.xlabel('number of applied pulses')
plt.ylabel('FG output voltage [V]')
plt.errorbar(np.arange(num_runs),means,stddevs)
plt.text(150,0.05, str(cells['config']['up']))
plt.savefig('./results/pulse_characterization_'+time.strftime("%Y-%m-%d_%H:%M:%S")+'.png',dpi=300)

