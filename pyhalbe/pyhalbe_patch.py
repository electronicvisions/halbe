"""
pyhalbe.so will load this module and execute patch() during initialization
"""

import sys
import pyhalco_common
import Coordinate


def patch(module):
    """
    This hook will be executed at the end of the pyhalbe module generation
    """
    for _, submodule in module._submodules:
        # Patch enum comparions operators
        pyhalco_common._patch_enums(submodule.__dict__)

        # Fix object names and module
        # pyhalbe._HICANN_FGBlock -> pyhalbe.HICANN.FGBlock
        for name, obj in submodule.__dict__.iteritems():
            if getattr(obj, '__name__', '').endswith("_" + name):
                try:
                    obj.__module__ = submodule.__name__
                    obj.__name__ = name
                except AttributeError:
                    pass

    # Add Coordinate as pyhalbe.Coordiante to be compatible with older pickles
    sys.modules['pyhalbe.Coordinate'] = Coordinate
    module.Coordinate = Coordinate

import pyhalbe.ADC as ADC
Coordinate.ADC = ADC.USBSerial
