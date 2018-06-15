"""
Compatibility module for old python code.

We need to pull in all symbols also hidden (starting with "_"), this also
ensures that pickled coordinates stay compatible
"""

# Easier for linters ?
from pyhalco_common import *
from pyhalco_hicann_v2 import *

def _load_all():
    """
    """
    import pyhalco_common
    import pyhalco_hicann_v2

    for m in [pyhalco_common, pyhalco_hicann_v2]:
        for name in dir(m):
            obj = getattr(m, name)
            globals()[name] = obj

_load_all()
del _load_all
