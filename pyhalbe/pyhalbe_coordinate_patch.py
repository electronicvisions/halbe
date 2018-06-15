"""
Coordinate.so will load this module and execute patch() during initialization

Keep this file for compatibilty with old pickled data!
"""


def __unpickle_boost_enum_helper(enum, value):
    return enum.values[value]


def _patch_enum(enum):
    def __reduce__(self):
        return (__unpickle_boost_enum_helper, (enum, int(self)), None, None, None)
    def __eq__(self, other):
        return isinstance(other, enum) and int(self) == int(other)
    def __ne__(self, other):
        return not isinstance(other, enum) or int(self) != int(other)
    def __lt__(self, other):
        if not isinstance(other, enum):
            raise TypeError("No ordered comparison between different enum types")
        return int(self) <  int(other)
    def __le__(self, other):
        if not isinstance(other, enum):
            raise TypeError("No ordered comparison between different enum types")
        return int(self) <= int(other)
    def __gt__(self, other):
        if not isinstance(other, enum):
            raise TypeError("No ordered comparison between different enum types")
        return int(self) >  int(other)
    def __ge__(self, other):
        if not isinstance(other, enum):
            raise TypeError("No ordered comparison between different enum types")
        return int(self) >= int(other)
    for f in ['__reduce__', '__eq__', '__ne__', '__lt__', '__le__', '__gt__', '__ge__']:
        setattr(enum, f, locals()[f])


def _patch_enums(module_dict):
    for name, enum in module_dict.iteritems():
        if isinstance(enum, type) and issubclass(enum,int) and not (enum is int):
            _patch_enum(enum)

_patch_enums(globals())


def patch(module):
    """
    This hook will be executed at the end of the pyhalbe module generation
    """
    from pyhalbe_coordinate_iter_all import iter_all
    module.iter_all = iter_all
    module._patch_enums = _patch_enums
    module.__unpickle_boost_enum_helper = __unpickle_boost_enum_helper
    _patch_enums(module.__dict__)
