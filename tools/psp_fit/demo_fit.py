"""
Evaluate the fit quality by generating several PSP shapes and fitting
them. The individual fit results and the residuals (using the estimated
errors) are shown in two separate plots.
"""

import pylab as p
from psp_shapes import AlphaPSP
from test_psp_shapes import noisy_psp
from fit import fit


def fit_quality(time, parameters, noise, repetitions):
    """
    Apply the fitting routine a number of times, as given by
    `repetitions`, and return informations about the fit performance.
    """
    results = []
    errors = []

    from numpy.random import seed

    alpha_psp = AlphaPSP()

    for _ in xrange(repetitions):
        seed()

        value = noisy_psp(time=time, noise=noise, **parameters)
        fit_result = fit(alpha_psp, time, value, noise,
                         fail_on_negative_cov=[True, True, True, False, False])
        if fit_result is not None:
            result, error, chi2, success = fit_result
            if chi2 < 1.5 and success:
                print chi2, result
                results.append(result)
                errors.append(error)
        else:
            print "fit failed:",
            print fit_result

    keys = alpha_psp.parameter_names()

    result_dict = dict(((key, []) for key in keys))
    error_dict = dict(((key, []) for key in keys))

    for result in results:
        for r, key in zip(result, keys):
            result_dict[key].append(r)

    for error in errors:
        for r, key in zip(p.diag(error), keys):
            error_dict[key].append(p.sqrt(r))
            if p.isnan(p.sqrt(r)):
                print "+++++++", r

    return ([p.mean(result_dict[key]) for key in keys],
            [p.std(result_dict[key]) for key in keys],
            len(results),
            keys,
            [result_dict[key] for key in keys],
            [error_dict[key] for key in keys])


def evaluate_fit_quality(time, par, noise_values, trials, debug_plot=True):
    """
    present the results obtained by fit_quality(...) in a graphical way
    """
    mean_fit = []
    std_fit = []
    success_count = []
    allvals = []
    errors = []
    for noise in noise_values:
        mean, std, success, keys, a, errs = fit_quality(time, par, noise, trials)
        mean_fit.append(mean)
        std_fit.append(std)
        success_count.append(success)
        allvals.append(a)
        errors.append(errs)
        print p.any(p.isnan(errs))

    mean_fit = p.array(mean_fit)
    std_fit = p.array(std_fit)

    if debug_plot:
        p.figure()

        num_subplots = mean_fit.shape[1] + 1
        plot = None

        p.title("fit quality evaluation")
        for i in range(num_subplots - 1):
            plot = p.subplot(num_subplots, 1, 1 + i, sharex=plot)
            p.axhline(par[keys[i]], c="r")
            p.errorbar(noise_values, mean_fit[:, i], yerr=std_fit[:, i])
            p.semilogx()
            p.xlim(min(noise_values) / 2., max(noise_values) * 2.)
            p.ylabel(keys[i])

            for n, a, e in zip(noise_values, allvals, errors):
                # p.plot([n] * len(a[i]), a[i], "rx", alpha=.4)
                p.errorbar([n] * len(a[i]), a[i],
                           yerr=e[i], fmt="rx", alpha=.4)

        p.subplot(num_subplots, 1, num_subplots, sharex=plot)
        p.plot(noise_values, [trials - x for x in success_count])
        p.ylabel("failure count (of {0} total)".format(trials))
        p.xlabel("noise / [value]")
        p.savefig("plots/example_fit_precision.pdf")

        p.figure()
        for i in range(num_subplots - 1):
            plot = p.subplot(num_subplots - 1, 1, 1 + i, sharex=plot)
            if i == 0:
                p.title("RMS of (fit - param) / estimated_error")
            p.axhline(1)
            p.axhline(0)
            for n, a, e in zip(noise_values, allvals, errors):
                rmsvals = p.sqrt(p.mean(((p.array(a[i]) - par[keys[i]]) / p.array(e[i])) ** 2))
                p.plot([n], rmsvals, "go")
                print "rmsvals for noise={0}, param={1}:".format(n, keys[i]), rmsvals, p.array(e[i])
            p.ylim(0, None)
            p.ylabel(keys[i])

        p.xlabel("noise / [value]")
        p.savefig("plots/example_fit_error_estimate.pdf")


    return mean_fit, std_fit, success_count


if __name__ == '__main__':
    evaluate_fit_quality(p.arange(0, 100, .1),
                         dict(height=1.,
                          tau_1=3.,
                          tau_2=2.,
                          start=40.,
                          offset=5.),
                     [.01, .1, .2, .35, .5, 1.],
                     100)
