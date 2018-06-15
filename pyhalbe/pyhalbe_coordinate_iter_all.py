"""
python pendant to hal/Coordinate/iter_all.h
"""

def _iter_all(Type):
    """
    Generator to iterate over a coordinate
    """
    try:
        value_type = Type.enum_type
        make = lambda ii: Type(value_type(ii))
    except AttributeError:
        value_type = Type
        make = lambda ii: Type(ii)

    for value in range(value_type.begin, value_type.end):
        yield make(value)

def iter_all(*Types):
    """
    Iterates over all posible values of one or more coordinates, e.g.:
        - for neuron in iter_all(NeuronOnHICANN):
              print neuron
        - for hicann, neuron in iter_all(HICANNOnWafer, NeuronOnHICANN):
              print hicann, neuron
    """
    if len(Types) == 1:
        return _iter_all(Types[0])
    else:
        from itertools import product
        return product(*(_iter_all(T) for T in Types))
