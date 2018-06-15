"""
demo:

estimate the fit parameter variance from the covariance matrix of a
chi^2 fit. (ideal for linear optimization)
"""

from scipy.optimize import leastsq
import pylab as p
import numpy

# (slope, intercept) fit
f = lambda x, theta: theta[1] * x + theta[0]
theta_0 = p.array((32., 99.))
theta_start = p.array((1., 1.))

# # calculate sample mean
# f = lambda x, theta: p.ones(len(x)) * theta
# theta_0 = p.array(32.)
# theta_start = p.array(1.)

rep = 100
noise = 10.
x = p.arange(0, 500, 1)


def estimate():
    y = f(x, theta_0) + numpy.random.normal(scale=noise, size=len(x))
    r = leastsq(
        lambda theta: (f(x, theta) - y),
        theta_start,
        full_output=True)

    theta = r[0]
    cov_x = r[1]
    return theta, cov_x * (noise ** 2)
    # print cov_x


def plott():
    y = f(x, theta_0) + numpy.random.normal(scale=noise)
    chi2 = lambda theta: sum((f(x, theta) - y) ** 2) / noise ** 2
    p.figure()
    tval = p.arange(25, 35, 1)
    p.plot(tval, [chi2(t) for t in tval], '-')

print "original parameters:         ", theta_0
print "mean fit values:             ", p.mean([estimate()[0] for _ in xrange(rep)], axis=0)
print
print "mean fit parameter deviation:", p.std([estimate()[0] for _ in xrange(rep)], axis=0)
print
print "mean deviation estimate:     ", p.mean([p.sqrt(p.diag(estimate()[1])) for _ in xrange(rep)], axis=0)
print
print "fit parameter covariances:"
print p.cov([estimate()[0] for _ in xrange(rep)], rowvar=0)
print
print "mean covariance matrix:      "
print p.mean([estimate()[1] for _ in xrange(rep)], axis=0)
