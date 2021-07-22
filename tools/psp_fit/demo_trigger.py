"""
Demo script to investigate the influence of the trigger algorithm on
the resulting mean psp shape
"""

import pylab as p
from .test_psp_shapes import noisy_psp


if __name__ == '__main__':
    height = 2.
    tau_1 = 2.
    tau_2 = 20.
    start = 10.
    offset = 30.

    repetitions = 100

    dt = .1
    time = p.arange(0, 140, dt)
    noise = .2
    sin_amplitude = .1
    sin_omega = .5
    sliding_average_len = 10

    psp_voltage = []

    for i in range(repetitions):
        phase = p.random() * p.pi * 2
        voltage = noisy_psp(height, tau_1, tau_2, start, offset, time, noise) \
            + sin_amplitude * p.sin(sin_omega * time + phase)
        psp_voltage.append(voltage)

    psp_voltage = p.array(psp_voltage)

    def insert_params():
        s = """
        height = {height}
        tau_1 = {tau_1}
        tau_2 = {tau_2}
        start = {start}
        offset = {offset}
        repetitions = {repetitions}
        noise = {noise}
        sin_amplitude = {sin_amplitude}
        sin_omega = {sin_omega}
        dt = {dt}
        sliding_average_len = {sliding_average_len}
        """.format(**globals())
        p.text(.8, .6, s, transform=p.gca().transAxes)

    p.figure()
    p.title("example generated psp")
    p.xlabel("time / AU")
    p.ylabel("voltage / AU")
    p.plot(time, psp_voltage[0])
    insert_params()

    mean = p.mean(psp_voltage, axis=0)
    std = p.std(psp_voltage, axis=0)
    p.figure()
    p.title("mean and standard deviation, ideal trigger")
    p.plot(time, mean, 'r-')
    p.fill_between(time, mean - std, mean + std, alpha=.3)
    p.xlabel("time / AU")
    p.ylabel("voltage / AU")
    insert_params()

    kernel = p.ones(sliding_average_len) / float(sliding_average_len)
    mean_max_index = p.argmax(mean)
    for i in range(len(psp_voltage)):
        smoothed = p.convolve(psp_voltage[i], kernel, "same")
        shift = mean_max_index - p.argmax(smoothed)
        psp_voltage[i] = p.roll(smoothed, shift)

    p.figure()
    p.title("mean and standard deviation, max trigger")
    mean_shifted = p.mean(psp_voltage, axis=0)
    std = p.std(psp_voltage, axis=0)
    p.plot(time, mean_shifted, 'r-')
    p.plot(time, mean, 'k--', alpha=.7)
    p.ylim(offset - height * .2, None)
    p.fill_between(time, mean - std, mean + std, alpha=.3)
    p.xlabel("time / AU")
    p.ylabel("voltage / AU")
    insert_params()

    p.show()
