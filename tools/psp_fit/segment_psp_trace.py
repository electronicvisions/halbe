"""
Tools to segment data of a periodically occuring event according to
the period.
"""

import pylab as p
# from __future__ import division
from scipy.optimize import fmin


def segment(data, dt, interval):
    """
    `reshape with one floating point index`

    reshape the given, one-dimensional array `data` with the given
    `interval`. `interval` can be a floating number; the result of the
    reshape is aligned to the nearest possible integer value of the
    shift.
    """
    segment_len = int(p.ceil(interval / dt))
    n_segments = len(data) // segment_len

    result = p.empty((n_segments, segment_len))

    # "fuzzy reshape"
    for i in xrange(n_segments):
        offset = int(p.around(i * interval / dt))
        result[i, :] = data[offset:offset + segment_len]

    return result


def optimize_segment(data, dt, interval_start, ftol=1e-3):
    """
    Estimate the recurrence time of a periodic signal within data.

    data : 1d numpy.ndarray
        the periodic signal

    dt : float
        sampling interval for data

    interval_start : float
        the interval that is taken as initial guess for
        the optimization (in the same unit as dt)

    ftol : float
        tolerance on the solution (passed to scipy.optimize.fmin)
    """

    i = fmin(
        lambda interval: p.sum(
            p.var(
                segment(data, dt, interval),
                axis=0)),
        interval_start,
        ftol=ftol)
    return i[0]
