import unittest
import pylab as p
from .psp_shapes import AlphaPSP, jacobian
import scipy
from numpy.testing import assert_array_almost_equal


def noisy_psp(height, tau_1, tau_2, start, offset, time, noise):
    alpha_psp = AlphaPSP()
    return alpha_psp(time, height, tau_1, tau_2, start, offset) \
        + (scipy.random.normal(0., noise, size=time.shape))


class TestJacobian(unittest.TestCase):
    def test_jacobian(self):
        def f(x):
            return p.sum(x * x)

        n = 10
        size = 10.

        repetitions = 100

        for i in range(repetitions):
            x = p.random(n) * size
            assert_array_almost_equal(2 * x, jacobian(f, x), decimal=4)


class TestAlphaPSP(unittest.TestCase):
    def test_symmetry(self):
        time = p.arange(0., 100., .01)

        std_thresh = 1e-13

        alpha_psp = AlphaPSP()

        start = 50
        for height in [1, 10, 100]:
            for t1 in [.1, 1., 10., 23., 100]:
                for t2 in [.1, 1., 10., 23., 100]:
                    v1 = alpha_psp(time, height, t1, t2, start, 5.)
                    v2 = alpha_psp(time, height, t2, t1, start, 5.)

                    std = p.std(v1 - v2)

                    self.assertLess(std, std_thresh)

    def test_process_fit_results(self):
        r = p.arange(5)
        e = p.outer(p.arange(5), p.arange(5))

        alpha_psp = AlphaPSP()
        pr, pe = alpha_psp.process_fit_results(r, e)

        self.assertTrue(p.all(pr == p.array([0, 2, 1, 3, 4])))
        self.assertEqual(pe[1, 1], 4)
        self.assertEqual(pe[2, 2], 1)

        self.assertLess(pr[2], pr[1])
        self.assertLess(pe[2, 2], pe[1, 1])

        # test again with permuted values
        pr, pe = alpha_psp.process_fit_results(pr, pe)

        self.assertTrue(p.all(pr == p.array([0, 2, 1, 3, 4])))
        self.assertEqual(pe[1, 1], 4)
        self.assertEqual(pe[2, 2], 1)

        self.assertLess(pr[2], pr[1])
        self.assertLess(pe[2, 2], pe[1, 1])


if __name__ == '__main__':
    unittest.main()
