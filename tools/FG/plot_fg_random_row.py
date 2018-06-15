import time
import numpy as np
import matplotlib.pyplot as plt
import datetime

def plot(xa, ya, z, ip, dnc, hicann, block, row, save):
	assert(len(xa)==len(ya))
	neu = {}
	for i, x in enumerate(xa):
		try:
			neu[x].append(ya[i])
		except KeyError:
			neu[x] = []
			neu[x].append(ya[i])

	mittelwert = []
	for x in range(0,1024):
		try:
			mittelwert.append(np.mean(neu[x]))
		except KeyError:
			continue
	std = []
	for x in range(0,1024):
		try:
			std.append(np.std(neu[x]))
		except KeyError:
			continue
	digitalwert = []
	for x in xa:
		if x in digitalwert:
			continue
		else:
			digitalwert.append(x)
	digitalwert.sort()
	#print digitalwert
	#print mittelwert
	#print std
	a = np.array(digitalwert) 
	b = a/1023.*1.8
	M = np.mean(std[:800])
	plt.figure(1, figsize=(10, 8), dpi=100, facecolor='w')
	#plt.suptitle('Testplot', fontsize=20)
	plt.subplot(211)
	plt.plot(digitalwert, mittelwert, a, b, 'g--', linewidth=0.5)
	plt.text(100, 1.42, 'Runtime for set_fg_values in [s]:', fontsize=10)
	plt.text(180, 1.22, z, fontsize=10)
	plt.axis([0,1024,0,1.8])
	plt.xlabel('Digitalwert')
	plt.ylabel('Analogwert [V]')
	plt.grid(True)
	plt.subplot(212)
	plt.plot(digitalwert,std,'r-', linewidth=0.5)
	plt.subplot(212).set_yscale('log')
	plt.axis([0,1024,0.0001,0.05])
	plt.axhline(y=M, linewidth=0.5, color='g')
	plt.xlabel('Digitalwert')
	plt.ylabel('Sigma [V]')
	plt.grid(True)
	plt.suptitle('Row %d in Block %d on HC %d on DNC %d on FPGA %s - %s'%(row,block,hicann,dnc,ip,datetime.date.today().isoformat()))
	if save:
		plt.savefig('./results/single_row_random_'+time.strftime("%Y-%m-%d_%H:%M:%S")+'.png',dpi=300)
	else:
		plt.show()
	
	

