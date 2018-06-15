import matplotlib
 
import pylab
import numpy
 
pylab.subplot(211)
voltages = numpy.loadtxt("voltages.csv")
#pylab.xlim((0, voltages.size))
pylab.plot(voltages[:,0], voltages[:,1])
pylab.xlim(voltages[0][0], voltages[-1][0])

pylab.subplot(212)
spikes = numpy.loadtxt("spikes.csv")

#pylab.ylim((-1, 1))

#pylab.yticks([0, ], ['INPUT'])
pylab.scatter(spikes[:,0], spikes[:,1], color='r', s=70)
pylab.xlim(voltages[0][0], voltages[-1][0])

#pylab.plot(spikes)

pylab.show()
