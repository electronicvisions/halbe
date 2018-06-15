import matplotlib.pyplot as plt
from matplotlib.pylab import pcolor, colorbar
from matplotlib.colors import LogNorm
import numpy as np
import matplotlib.cm as cm
import datetime

#m = pickle.load( open( "FGBlock.pkl", "rb" ) )
def plot_fg_block(m,target_value,ip,dnc,hicann,block,reprogram,save):
	title_string = "FG Blk %d on HC %d on DNC %d on FPGA %s %s %s"%(block,hicann,dnc,ip,('(rp)' if reprogram else '(no rp)'),datetime.date.today().isoformat())

	Std = []
	Avg = []
	for a in range(0,24):
		std_list = []
		avg_list = []
		for b in range(0,129):
			std_list.append(np.std([x[1] for x in m[a][b] ]))
			avg_list.append(np.mean([x[1] for x in m[a][b] ]))
		Std.append(std_list)
		Avg.append(avg_list)
		
	matrix = np.array(Std)
	plt.figure()
	plt.pcolor(matrix, cmap=cm.binary, norm=LogNorm(),vmin=1e-5,vmax=1e-2)
	cbar = plt.colorbar()
	cbar.set_ticks(list(np.arange(10)*1e-2)+list(np.arange(10)*1e-3)+list(np.arange(10)*1e-4)+list(np.arange(10)*1e-5))
	cbar.update_ticks()
	plt.xlim(0,129)
	plt.ylim(0,24)
	cbar.set_label('STD in [V]')
	plt.xticks( (0,32,64,96,128), ('0', '32','64','96','128'))
	plt.yticks((0,6,12,18,24), ('0','6','12','18','24'))
	plt.xlabel("Spalte")
	plt.ylabel("Zeile")
	plt.title(title_string)

	if save:
		plt.savefig("./results/FG%d_HC%d_DNC%d_FPGA%s_%d"%(block,hicann,dnc,ip,target_value)+("" if reprogram else '_norp')+"_std.png",dpi=300)

	plt.figure()
	matrix = np.array(Avg)
	plt.pcolor(matrix, cmap=cm.binary)
	cbar = plt.colorbar()
	plt.xlabel("Spalte")
	plt.ylabel("Zeile")
	cbar.set_label('mean in [V]')

	if save:
		plt.savefig("./results/FG%d_HC%d_DNC%d_FPGA%s_%d"%(block,hicann,dnc,ip,target_value)+("" if reprogram else '_norp')+"_mean.png",dpi=300)
	else:
		plt.show()
