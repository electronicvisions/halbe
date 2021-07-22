"""
Demonstrates the"reshape with non-integer intevals" and the optimization of
the reshape interval by minimizing the standard deviation of the overlap.
"""

import pylab as p
from .segment_psp_trace import segment, optimize_segment


if __name__ == '__main__':
    data = p.np.load("traces/spiketrace_1.npz")["arr_0"][:2400000]

    dt = 1.
    p.figure()
    m = lambda interval: p.sum(
        p.var(
            segment(data, dt, interval),
            axis=0))

    shift_values = p.arange(3841, 3842, .01)
    p.plot(shift_values, list(map(m, shift_values)), 'x')
    p.show()

    r = optimize_segment(data, 1., 3840)
    print(r)

    p.figure()
    seg = segment(data, 1., r)
    print(len(seg))
    mean = p.mean(seg, axis=0)
    std = p.std(seg, axis=0)
    dt = 1.
    time = p.arange(len(std)) * dt
    p.plot(time, mean, "r-")
    p.fill_between(time, mean - std, mean + std, alpha=.2)
    p.title("r =" + str(r))

    p.figure()
    for rr in p.arange(3841, 3841.5, .05):
        p.plot(p.mean(segment(data, 1., rr), axis=0))
