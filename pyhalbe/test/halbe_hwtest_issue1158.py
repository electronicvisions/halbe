#!/usr/bin/env python

import unittest
from pyhalbe import HICANN


class ParameterTest(unittest.TestCase):
    def test_parameter_dict(self):
        pdict = {}

        """
        problem: neuron_parameter and shared_parameter in the same dict
        - int(HICANN.neuron_parameter.E_l) == 0
        - int(HICANN.shared_parameter.V_reset) == 0

        using those as keys in dict may overwrite each other
        """

        pdict[HICANN.neuron_parameter.E_l] = 300
        pdict[HICANN.shared_parameter.V_reset] = 200

        self.assertFalse(HICANN.neuron_parameter.E_l is HICANN.shared_parameter.V_reset, "E_l is not V_reset")
        self.assertFalse(HICANN.neuron_parameter.E_l == HICANN.shared_parameter.V_reset, "E_l is not equal to V_reset")
        self.assertEqual(pdict[HICANN.neuron_parameter.E_l], 300, "value should not be overwritten")
        self.assertEqual(pdict[HICANN.shared_parameter.V_reset], 200, "value should not be overwritten")


if __name__ == "__main__":
    unittest.main()
