import inspect
from scipy import optimize
import pylab as p
from pprint import pprint


def fit(psp_shape, time, voltage, error_estimate, maxcall=1000,
        maximal_red_chi2=2.0, fail_on_negative_cov=None):
    """
    psp_shape : object
        PSPShape instance

    time : numpy.ndarray of floats
        numpy array of data acquisition times

    voltage : numpy.ndarray
        numpy array of voltage values

    error_estimate : float
        estimate for the standard deviation of an individual data point.

    maxcall : int
        maximal number of calls to the fit routine

    fail_on_negative_cov : list of int

    returns : tuple
        (fit_results
         error_estimates
         chi2_per_dof
         success)
    """
    assert len(time) == len(voltage)

    initial_values = psp_shape.initial_fit_values(time, voltage)

    result = optimize.leastsq(
        lambda param: (psp_shape(time, *param) - voltage),
        [initial_values[key] for key in psp_shape.parameter_names()],
        full_output=1,
        maxfev=maxcall)

    resultparams, cov_x, _, _, ier = result

    ndof = len(time) - len(psp_shape.parameter_names())
    fit_voltage = psp_shape(time, *result[0])
    red_chi2 = sum(((fit_voltage - voltage)) ** 2) \
               / (error_estimate ** 2 * ndof)

    fail_neg = p.any(p.diag(cov_x) < 0)
    if fail_on_negative_cov is not None:
        fail_neg = p.any(p.logical_and(p.diag(cov_x) < 0,
                           fail_on_negative_cov))

    cov_x *= error_estimate ** 2

    success = ((not fail_neg)
               and (ier in [1, 2, 3, 4])
               and (red_chi2 <= maximal_red_chi2))

    processed, processed_cov = psp_shape.process_fit_results(
        resultparams,
        cov_x)

    return processed, processed_cov, red_chi2, success
