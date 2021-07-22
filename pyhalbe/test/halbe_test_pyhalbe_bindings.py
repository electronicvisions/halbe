#!/usr/bin/env python

from Test import PyhalbeTest, parametrize


class Test_PyhalbeBindings(PyhalbeTest):
    def test_import(self):
        import pyhalbe
        pyhalbe.DNC
        pyhalbe.FPGA
        pyhalbe.HICANN
        pyhalbe.Debug

    def test_array_operators(self):
        from pyhalbe import HICANN
        from pyhalco_common import Enum
        import pyhalco_hicann_v2 as Coordinate
        quad = HICANN.NeuronQuad()

        neuron = HICANN.Neuron()
        neuron.enable_aout(True)

        quad[Coordinate.NeuronOnQuad(Enum(0))] = neuron
        self.assertTrue( quad[Coordinate.NeuronOnQuad(Enum(0))].enable_aout())
        self.assertFalse(quad[Coordinate.NeuronOnQuad(Enum(1))].enable_aout())
        self.assertFalse(quad[Coordinate.NeuronOnQuad(Enum(2))].enable_aout())
        self.assertFalse(quad[Coordinate.NeuronOnQuad(Enum(3))].enable_aout())


        quad[Coordinate.NeuronOnQuad(Enum(1))].enable_aout(True)
        self.assertTrue( quad[Coordinate.NeuronOnQuad(Enum(0))].enable_aout())
        self.assertTrue( quad[Coordinate.NeuronOnQuad(Enum(1))].enable_aout())
        self.assertFalse(quad[Coordinate.NeuronOnQuad(Enum(2))].enable_aout())
        self.assertFalse(quad[Coordinate.NeuronOnQuad(Enum(3))].enable_aout())

        x = quad[Coordinate.NeuronOnQuad(Enum(2))]
        x.enable_aout(True)
        self.assertTrue( quad[Coordinate.NeuronOnQuad(Enum(0))].enable_aout())
        self.assertTrue( quad[Coordinate.NeuronOnQuad(Enum(1))].enable_aout())
        self.assertTrue( quad[Coordinate.NeuronOnQuad(Enum(2))].enable_aout())
        self.assertFalse(quad[Coordinate.NeuronOnQuad(Enum(3))].enable_aout())

    def test_bitset4(self):
        from pyhalbe import std

        bit = std.Bitset4()
        bit.set(0).set(1).set(2).set(3)
        self.assertEqual(bit.to_ulong(), 15)

        bit = std.Bitset4(4)
        self.assertEqual(bit.to_ulong(), 4)
        self.assertEqual(bit[0], False)
        self.assertEqual(bit[1], False)
        self.assertEqual(bit[2], True)
        self.assertEqual(bit[3], False)

        bit = std.Bitset4()
        bit[0] = True
        bit[1] = True
        bit[2] = True
        bit[3] = True
        self.assertEqual(bit.to_ulong(), 15)

    def test_bitset8(self):
        from pyhalbe import std

        bit = std.Bitset8()
        bitstr = std.Bitset8("11111111")
        bit.set(0).set(1).set(2).set(3).set(4).set(5).set(6).set(7)
        self.assertEqual(bit.to_ulong(), 255)
        self.assertEqual(bit, bitstr)

        bit = std.Bitset8()
        bit[0] = True
        bit[1] = True
        bit[2] = True
        bit[3] = True
        self.assertEqual(bit.to_ulong(), 15)

        bit = std.Bitset8([True] * 4 + [False] * 4)
        self.assertEqual(bit.to_ulong(), 15)

        bit = std.Bitset8(13)
        self.assertEqual(bit.to_ulong(), 13)
        self.assertEqual(bit[0], True)
        self.assertEqual(bit[1], False)
        self.assertEqual(bit[2], True)
        self.assertEqual(bit[3], True)
        self.assertEqual(bit[4], False)
        self.assertEqual(bit[5], False)
        self.assertEqual(bit[6], False)
        self.assertEqual(bit[7], False)

    def test_std_array_bool(self):
        from pyhalbe import std
        x = std.Array_Bool_16([True] * 16)
        x[1] = False
        self.assertEqual(x[0], True)
        self.assertEqual(x[1], False)
        self.assertEqual(x[2], True)
        self.assertEqual(x[8], True)
        self.assertEqual(x[15], True)

    def test_numpy_construtors(self):
        from pyhalbe import HICANN, std

        pattern = [True, False, False, True] * 4
        self.assertEqual( std.Array_Bool_16(pattern)[:], pattern)
        self.assertEqual( HICANN.SynapseSwitchRow(pattern)[:], pattern)

    def test_std_array_bool2(self):
        from pyhalbe.std import Array_Array_Bool_32_224 as Outer
        from pyhalbe.std import Array_Bool_32 as Inner
        olen = len(Outer())
        ilen = len(Inner())
        value_type = bool
        from random import Random
        import numpy as np

        r = Random(2321)

        # Test from list constructor
        a = Inner(list(range(ilen)))
        for ii in range(ilen):
            self.assertEqual(a[ii], value_type(ii))

        # Test from numpy constructor
        a = Inner(np.array(list(range(ilen)), dtype=value_type))
        for ii in range(ilen):
            self.assertEqual(a[ii], value_type(ii))

        a = Inner(np.ones(ilen, dtype=value_type))
        for ii in range(ilen):
            self.assertEqual(a[ii], True)


        # Test constructors
        b = Inner( np.ones(ilen, dtype=value_type) )
        a = Outer( np.ones(shape = (olen, ilen), dtype=value_type) )
        for row in a:
            self.assertIsInstance(row, Inner)
            self.assertEqual(row, b)
            for item in row:
                self.assertEqual(item, 1)

        # Test assignment
        a = Outer( np.ones(shape = (olen, ilen), dtype=value_type) )
        a[olen-1][ilen-1] = False
        self.assertEqual(a[olen-1][ilen-1], False)

        data = [ Inner([r.randint(0, True) for ii in range(ilen) ]) for ii in range(olen) ]
        for ii, d in enumerate(data):
            a[ii] = d
        for ii, d in enumerate(data):
            self.assertEqual(a[ii], d)

    def test_some_conversion_operators(self):
        import pyhalco_common as C
        x0 = C.X(0)
        x1 = C.X(1)
        x2 = C.X(2)
        # conversion from unranged X to XRanged
        C._XRanged_less__1ul_comma__0ul__greater_(x0)
        C._XRanged_less__1ul_comma__0ul__greater_(x1)
        # ok, let's try a number that's too big
        try:
           C._XRanged_less__1ul_comma__0ul__greater_(x2)
        except:
           pass

    def test_enum_compare(self):
        from pyhalbe import HICANN

        sp = [k for k in HICANN.shared_parameter.values.values()]
        np = [k for k in HICANN.neuron_parameter.values.values()]

        for a, b in zip(np + sp, np + sp):
            self.assertEqual(a, b)

        for n in np:
            for s in sp:
                self.assertNotEqual(n, s)

        for a in np:
            for b in np:
                if not a == b:
                    self.assertNotEqual(a, b)

    def test_short_format(self):
        from pyhalco_common import Enum
        import pyhalco_hicann_v2 as C

        self.assertEqual(C.short_format(C.HICANNGlobal(C.HICANNOnWafer(Enum(42)),C.Wafer(12))), "W012H042")
        self.assertEqual(C.short_format(C.FPGAGlobal(C.FPGAOnWafer(Enum(11)),C.Wafer(5))), "W005F011")
        self.assertEqual(C.short_format(C.HICANNOnWafer(Enum(12))), "H012")
        self.assertEqual(C.short_format(C.FPGAOnWafer(Enum(12))), "F012")
        self.assertEqual(C.short_format(C.Wafer(32)), "W032")

    @parametrize(['shared_parameter', 'neuron_parameter'])
    def test_enum_pickling(self, param):
        from pyhalbe import HICANN
        import pickle
        enum = getattr(HICANN, param)

        values = [k for k in enum.values.values()]
        for v in values:
            loaded = pickle.loads(pickle.dumps(v))
            self.assertIs(loaded, v)

    @parametrize(['shared_parameter', 'neuron_parameter'])
    def test_parameter_to_string(self, param):
        from pyhalbe import HICANN
        enum = getattr(HICANN, param)

        for name, parameter in enum.names.items():
            if '__' not in name:
                self.assertEqual(name,
                                 HICANN.to_string(parameter))

if __name__ == '__main__':
    PyhalbeTest.main()
