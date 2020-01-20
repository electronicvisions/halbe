#!/usr/bin/env python
import sys
import os
import re
import logging
import argparse

from pyplusplus.module_builder import call_policies
from pywrap.wrapper import Wrapper
from pywrap import containers, namespaces, matchers, classes
from pywrap.namespace_util import NamespaceUtil
from pygccxml.declarations import templates
from pyplusplus import decl_wrappers, messages


def add_numpy_construtor_to_strong_typedefs(ns):
    matcher = matchers.match_std_container_t("array")

    def _filter(c):
        return any([matcher(base.related_class) for base in c.bases])

    for c in ns.classes(_filter, allow_empty=True):
        classes.add_numpy_construtor(c)
        for b in c.bases:
            b.related_class.exclude()

# FIXME: fix py++ to include our headers before boost python headers
wrap = Wrapper(license='#include "pyhalbe.h"')
module_name = wrap.module_name()
mb = wrap.mb
ns_util = NamespaceUtil()

# Less noise
ns_util.log.setLevel(logging.WARN)
classes.log.setLevel(logging.WARN)
messages.disable(messages.W1043)  # Ugly names, who cares? ...
messages.disable(messages.W1066)  # Not exposed typedefs

# Collect namespaces
ns_hmf        = mb.namespace("::HMF")
ns_hmf_hicann = mb.namespace("::HMF::HICANN")
ns_realtime   = mb.namespace("::Realtime")
included_ns = [ns_hmf, ns_realtime]

for ns in ['::boost::serialization', '::boost::archive', '::boost::mpi']:
    try:
        mb.namespace(ns).exclude()
    except matchers.declaration_not_found_t:
        pass

def get_fops(name="operator<<"):
    return [tc.decl_string for tc in [op.get_target_class() for op in mb.free_operators() if op.name == name] if tc]

# classes with operator<< defined
cls_w_ostream = get_fops()

def add_print_operator(c):
    """
    class exclusion helper
    because operator<< is missing in most classes

    Args:
        c: class
    Returns:
        True if __str__() should be wrapped
        False otherwise
    """
    allowed_namespace = matchers.namespace_contains_matcher_t(ns_hmf_hicann.name)(c)
    return allowed_namespace and (c.decl_string in cls_w_ostream)

# Special fix up
containers.extend_std_containers(mb)
namespaces.include_default_copy_constructors(mb)

for f in ['get_trace']:
    f = ns_hmf.free_function("get_trace")
    f.call_policies = call_policies.custom_call_policies(
        "::pywrap::ReturnNumpyPolicy", "pywrap/return_numpy_policy.hpp")

#Normally included classes
for ns in included_ns:
    ns.include()
    add_numpy_construtor_to_strong_typedefs(ns)
    namespaces.extend_array_operators(ns)

    for c in ns.classes(allow_empty=True):

        c.include()
        for base in classes.get_all_bases(c):
            base.include()
            base.constructors(lambda c: c.explicit is True, allow_empty=True).allow_implicit_conversion = False

        # propagate "explictness" to python :)
        c.constructors(lambda c: c.explicit is True, allow_empty=True).allow_implicit_conversion = False

        if add_print_operator(c):
            c.add_registration_code('def(bp::self_ns::str(bp::self_ns::self))')

        classes.add_comparison_operators(c)

        # export std::hash to __hash__
        if any(x.member_functions('hash', allow_empty=True).to_list()
                for x in classes.get_all_bases(c) + [c]):
            classes.expose_std_hash(c)

    ns_util.add_namespace(ns, drop_ns_levels=1)  # Don't create a HMF submodule

for c in ['Analog', 'BackgroundGenerator', 'BackgroundGeneratorArray',
          'Crossbar', 'CrossbarRow', 'DNCMerger', 'DNCMergerLine',
          'DecoderDoubleRow', 'DecoderRow', 'DriverDecoder', 'FGBlock',
          'FGConfig', 'FGControl', 'FGInstruction', 'FGStimulus', 'GbitLink',
          'HorizontalRepeater', 'L1Address', 'Merger', 'MergerTree', 'Neuron',
          'NeuronConfig', 'NeuronQuad', 'Repeater', 'RepeaterBlock',
          'RowConfig', 'Status', 'STDPEvaluationPattern', 'STDPLUT',
          'SynapseConfigurationRegister', 'SynapseController','SynapseControlRegister',
          'SynapseDecoder', 'SynapseDllresetb', 'SynapseDriver', 'SynapseGen', 'SynapseSel',
          'SynapseStatusRegister', 'SynapseSwitch', 'SynapseSwitchRow', 'SynapseWeight',
          'TestEvent_3', 'VerticalRepeater', 'WeightRow', 'FGErrorResult',
          'FGErrorResultRow', 'FGErrorResultQuadRow', 'FGRow']:
    cls = ns_hmf.class_('::HMF::HICANN::' + c)
    classes.add_pickle_suite(cls)

c = mb.class_('::HMF::ADC::USBSerial')
c.include()
classes.add_comparison_operators(c)
classes.expose_std_hash(c)
c.add_registration_code('def(bp::self == bp::self)')
c.include_files.append('pywrap/print_helper.hpp')
c.add_registration_code('def(pywrap::PrintNice())')
classes.add_pickle_suite(c)

c = mb.typedef('::HMF::HICANN::STDPLUT::LUT').type.declaration
c.include()

# HMF::Handle stuff & special handling (ECM)
for c in ns_hmf.namespace('Handle').classes(allow_empty=True):
    classes.add_context_manager(c)

# Try to capture the std::container dependcies
std_filter = matchers.and_matcher_t([
    matchers.declaration_matcher_t(decl_type=decl_wrappers.class_t),
    matchers.namespace_contains_matcher_t("std")
])

for ns in included_ns:
    for decl in namespaces.get_deps(ns_hmf, matchers.namespace_contains_matcher_t("std")):
        if decl.indexing_suite or decl.name.startswith("bitset"):
            decl.include()

# std::hash specialisations are only used from within the member hash functions
mb.classes(matchers.match_std_container_t("hash"), allow_empty=True).exclude()

ns_util.add_namespace(mb.namespace("std"))

# include base classes
for cl in mb.classes():
    if cl.ignore:
        continue
    for cl in classes.get_all_bases(cl):
        cl.include()

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'protected')

# exclude names begining with a single underscore or ending with Cpp
namespaces.exclude_by_regex(mb, ['calldefs', 'classes', 'variables'], r'(^_[^_])|(.*Cpp$)|(^impl$)')

ns_util.finish(wrap)

# Add python import callback
mb.add_registration_code(
    'bp::import("pyhalbe_patch").attr("patch")(bp::scope());')

no_classes = len([c for c in mb.classes(allow_empty=True) if not c.ignore])
wrap.set_number_of_files(no_classes/30)

wrap.finish()
