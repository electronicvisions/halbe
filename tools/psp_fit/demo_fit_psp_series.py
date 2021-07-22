"""
Generate a voltage trace with several PSPs and fit individual PSPs,
calculating the parameters and the integral with an error estimate.
"""

from .test_fit import noisy_psp
import pylab as p
from .segment_psp_trace import segment
from .fit import fit
from .psp_shapes import AlphaPSP


dt = .1
times = p.arange(0, 100, dt)


def build_rep_trace(noise=1., seed_val=2931):
    p.seed(seed_val)

    height = 1.
    tau_1 = 10.
    tau_2 = 5.
    start = 30.
    offset = 50.

    repetitions = 100

    result = p.empty(repetitions * len(times))
    for i in range(repetitions):
        v = noisy_psp(height, tau_1, tau_2, start, offset, times, noise)
        result[i * len(times):
               (i + 1) * len(times)] = v

    return result

if __name__ == '__main__':
    noise_est = 0.1
    psps = build_rep_trace(noise_est)
    shapes = segment(psps, dt, 100)

    psp = AlphaPSP()

    i = []
    i_err = []
    res = []
    for shape in shapes:
        r = fit(psp, times, shape, noise_est,
                fail_on_negative_cov=[True, True, True, False, False])
        a, b = psp.integral(times, r[0], r[1])
        print("fit succeeded:", r[-1])
        print("fit result:", r[0])
        print("integral:", a, "+/-", b)
