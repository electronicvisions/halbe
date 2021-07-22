import pylab as p
from numpy import exp
from scipy import optimize

parameter_names = "start A tau_1 tau_2 offset".split()

def psp(time, start, A, tau_1, tau_2, offset, noise):
    t = p.maximum(time - start, 0)
    return A * (p.exp(-t / tau_2) - p.exp(-t / tau_1)) \
        + p.random(t.shape) * noise + offset


def ideal_psp(x, time):
    start, A, tau_1, tau_2, offset = x
    t = p.maximum(time - start, 0)
    return A * (p.exp(-t / tau_2) - p.exp(-t / tau_1)) + offset


def ideal_psp_fixmem(x, tau_m, time):
    start, A, tau, offset = x
    t = p.maximum(time - start, 0)
    return A * (p.exp(-t / tau) - p.exp(-t / tau_m)) + offset


def fit_exp(time, value):
    value = p.array(value)
    time = p.array(time)
    x0 = p.array([value[0], 1., value[-1]])

    result = optimize.leastsq(
        lambda x: x[0] * exp(-time / x[1]) + x[2] - value, x0)

    if result[1] not in [1, 2, 3, 4]:
        return None
    return result[0]


def psp_parameter_estimate(time, value):
    smoothing_kernel = 10
    smoothed_value = p.convolve(
        value,
        p.ones(smoothing_kernel) / float(smoothing_kernel),
        "same")

    mean_est_part = int(len(value) * .1)
    mean_estimate = p.mean(smoothed_value[-mean_est_part:])
    noise_estimate = p.std(value[-mean_est_part:])

    integral = p.sum(smoothed_value - mean_estimate) * (time[1] - time[0])

    f = 1.

    A_estimate = (max(smoothed_value) - mean_estimate) / (1. / 4.)

    min_A = noise_estimate

    if A_estimate < min_A:
        A_estimate = min_A

    t1_est = integral / A_estimate * f
    t2_est = 2 * t1_est

    tmax_est = time[p.argmax(smoothed_value)] + p.log(t2_est / t1_est) * (t1_est * t2_est) / (t1_est - t2_est)

    return p.array([
        tmax_est,
        A_estimate,
        t1_est,
        t2_est,
        mean_estimate])


def psp_parameter_estimate_fixmem(time, value):
    smoothing_kernel = 10
    smoothed_value = p.convolve(
        value,
        p.ones(smoothing_kernel) / float(smoothing_kernel),
        "same")

    mean_est_part = int(len(value) * .1)
    mean_estimate = p.mean(smoothed_value[-mean_est_part:])
    noise_estimate = p.std(value[-mean_est_part:])

    integral = p.sum(smoothed_value - mean_estimate) * (time[1] - time[0])

    f = 1.

    A_estimate = (max(smoothed_value) - mean_estimate) / (1. / 4.)

    min_A = noise_estimate

    if A_estimate < min_A:
        A_estimate = min_A

    t1_est = integral / A_estimate * f
    t2_est = 2 * t1_est

    tmax_est = time[p.argmax(smoothed_value)] + p.log(t2_est / t1_est) * (t1_est * t2_est) / (t1_est - t2_est)

    return p.array([
        tmax_est,
        A_estimate,
        t1_est,
        mean_estimate])


def fit_psp_simple(time, value):
    x0 = psp_parameter_estimate(time, value)

    result = optimize.leastsq (
        lambda x: ideal_psp(x, time) - value,
        x0,
        full_output=1)

    debug_plot = False
    if debug_plot:
        print(result[-2])
        p.figure()
        p.title(result[-2])
        p.plot(time, value)
        p.plot(time, ideal_psp(result[0], time))
        p.show()

    std_estimate = p.std(value[-int(.1 * len(value)):])
    reduced_std_threshold = 3.0
    
    red_std = p.std(ideal_psp(result[0], time) - value) / std_estimate
    #print "fit deviation estimate:", red_std
    #if red_std > reduced_std_threshold:
    #    return None

    return result[0]
    # return ideal_psp(x0, time)


def fit_psp(time, value, tau_m_estimate):
    x0 = p.array([5, .1, .2, p.mean(value[-int(.1 * len(value)):])])
    
    result = optimize.leastsq (
        lambda x: ideal_psp_fixmem(x, tau_m_estimate, time) - value,
        x0,
        full_output=1)

    debug_plot = False
    if debug_plot:
        print(result[-2])
        p.figure()
        p.title(result[-2])
        p.plot(time, value)
        p.plot(time, ideal_psp(result[0], time))
        p.show()

    std_estimate = p.std(value[-int(.1 * len(value)):])
    reduced_std_threshold = 3.0
    
    red_std = p.std(ideal_psp_fixmem(result[0], time, tau_m_estimate) - value) / std_estimate
    #print "fit deviation estimate:", red_std
    if red_std > reduced_std_threshold:
		pass

    return result[0]


def parpsp_e(height, t):
    A = height * p.exp(1)

    v = A * p.exp(-t) * t
    return v


def parpsp(height, tau_frac, t):
    A = height / (tau_frac ** (- 1 / (tau_frac - 1))
                  - tau_frac ** (-tau_frac / (tau_frac - 1)))
    v = A * (p.exp(-t / tau_frac) - p.exp(-t))
    return v


alpha_psp_parameter_names = "height t1 t2 start offset".split()


def alpha_psp(height, t1, t2, start, offset, time):
    t1 = p.maximum(t1, 0.)
    t2 = p.maximum(t2, 0.)

    tau_frac = t2 / p.float64(t1)
    t = p.maximum((time - start) / p.float64(t1), 0)
    epsilon = 1E-8
    if 1. - epsilon < tau_frac < 1. + epsilon:
        return parpsp_e(height, t) + offset
    else:
        return parpsp(height, tau_frac, t) + offset


def alpha_psp_parameter_estimate(time, value, smoothing_samples=10):
    t1_est_min = time[1] - time[0]

    mean_est_part = len(value) * .1
    mean_estimate = p.mean(value[-mean_est_part:])
    noise_estimate = p.std(value[-mean_est_part:])

    smoothed_value = p.convolve(
        value - mean_estimate,
        p.ones(smoothing_samples) / float(smoothing_samples),
        "same") + mean_estimate

    integral = p.sum(smoothed_value - mean_estimate) * (time[1] - time[0])

    f = 1.

    height_estimate = (max(smoothed_value) - mean_estimate)

    min_height = noise_estimate

    if height_estimate < min_height:
        height_estimate = min_height

    t1_est = integral / height_estimate * f
    if t1_est < t1_est_min:
        t1_est = t1_est_min
    t2_est = 2 * t1_est

    tmax_est = time[p.argmax(smoothed_value)]
    tstart_est = tmax_est + p.log(t2_est / t1_est) \
        * (t1_est * t2_est) / (t1_est - t2_est)

    return p.array([
        height_estimate,
        t1_est,
        t2_est,
        tstart_est,
        mean_estimate])


if __name__ == '__main__':
    ttime = p.arange(0, 100, .1)

    v = psp(ttime, 10, 1, 1, 2, 10, .2)
    fit = fit_psp(ttime, v)

    p.plot(ttime, v)
    p.plot(ttime, fit)
    p.show()
