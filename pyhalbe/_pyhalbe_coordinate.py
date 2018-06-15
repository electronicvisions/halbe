"""
Compatiblity module, that allows us to load old pickle files
"""

import Coordinate

for name in dir(Coordinate):
    if not name.startswith('__'):
        globals()[name] = getattr(Coordinate, name)
