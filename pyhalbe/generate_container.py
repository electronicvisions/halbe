#!/usr/bin/env python
import sys,os, re, logging, argparse

from pyplusplus.module_builder import call_policies
from pywrap.wrapper import Wrapper
from pywrap import containers, namespaces, matchers, classes
from pywrap.namespace_util import NamespaceUtil
from pygccxml.declarations import templates
from pyplusplus import decl_wrappers, messages

def add_numpy_construtor_to_strong_typedefs(ns):
    matcher = matchers.match_std_container_t("array")
    def _filter(c):
        return any([ matcher(base.related_class) for base in c.bases])

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
messages.disable(messages.W1043) # Ugly names, who cares? ...
messages.disable(messages.W1066) # Not exposed typedefs

# Collect namespaces
ns_hmf        = mb.namespace("::HMF")
included_ns = [ns_hmf]

for ns in ['::boost::serialization', '::boost::archive', '::boost::mpi']:
    try:
        mb.namespace(ns).exclude()
    except matchers.declaration_not_found_t: pass

# Special fix up
containers.extend_std_containers(mb)
namespaces.include_default_copy_constructors(mb)

c = mb.class_('::boost::asio::ip::address_v4')
c.include()
c.rename('IPv4')
classes.add_comparison_operators(c)
classes.expose_std_hash(c)
c.add_registration_code('def(bp::self == bp::self)')
c.include_files.append('pywrap/print_helper.hpp')
c.add_registration_code('def(pywrap::PrintNice())')
classes.add_pickle_suite(c)

for f in ['get_trace']:
    f = ns_hmf.free_function("get_trace")
    f.call_policies = call_policies.custom_call_policies(
    "::pywrap::ReturnNumpyPolicy", "pywrap/return_numpy_policy.hpp")

f = ns_hmf.free_function("get_pyoneer")
f.call_policies = call_policies.custom_call_policies("bp::return_value_policy<bp::reference_existing_object>")
f.add_declaration_code('#include "pyoneer/PyOneer.h"')

#Normally included classes
for ns in included_ns:
    ns.include()
    add_numpy_construtor_to_strong_typedefs(ns)
    namespaces.extend_array_operators(ns)

    for c in ns.classes(allow_empty=True):

        c.include()
        # propagate "explictness" to python :)
        c.constructors(lambda c: c.explicit == True, allow_empty=True).allow_implicit_conversion = False

        classes.add_comparison_operators(c)

        # export std::hash to __hash__
        if any(x.member_functions('hash', allow_empty=True).to_list()
                for x in classes.get_all_bases(c) + [c]):
            classes.expose_std_hash(c)

    ns_util.add_namespace(ns)

for c in ['Analog', 'BackgroundGenerator', 'BackgroundGeneratorArray', 'Crossbar', 'CrossbarRow',
            'DNCMerger', 'DNCMergerLine', 'DecoderDoubleRow', 'DecoderRow', 'DriverDecoder',
            'FGBlock', 'FGConfig', 'FGControl', 'FGInstruction', 'FGStimulus', 'GbitLink',
            'HorizontalRepeater', 'L1Address', 'Merger', 'MergerTree', 'Neuron', 'NeuronConfig',
            'NeuronQuad', 'Repeater', 'RepeaterBlock', 'RowConfig', 'Status',
            'STDPEvaluationPattern', 'STDPLUT', 'SynapseCmd', 'SynapseConfigurationRegister',
            'SynapseController','SynapseControlRegister', 'SynapseDecoder', 'SynapseDllresetb',
            'SynapseDriver', 'SynapseGen', 'SynapseSel', 'SynapseStatusRegister', 'SynapseSwitch',
            'SynapseSwitchRow', 'SynapseWeight', 'TestEvent_3', 'VerticalRepeater', 'WeightRow']:
    cls = ns_hmf.class_('::HMF::HICANN::' + c)
    classes.add_pickle_suite(cls)

# HMF::Handle stuff & special handling (ECM)
for c in ns_hmf.namespace('Handle').classes(allow_empty=True):
    classes.add_context_manager(c)
    if c.name.startswith('FPGA'):
        for f in c.mem_funs('get', allow_empty=True):
            f.call_policies = call_policies.return_internal_reference()


# Try to capture the std::container dependcies
std_filter = matchers.and_matcher_t([
    matchers.declaration_matcher_t(decl_type=decl_wrappers.class_t),
    matchers.namespace_contains_matcher_t("std")
])

for ns in included_ns:
    for decl in namespaces.get_deps(ns_hmf, matchers.namespace_contains_matcher_t("std")):
        if decl.indexing_suite or decl.name.startswith("bitset"):
            decl.include()

ns_util.add_namespace(mb.namespace("std"))

# expose only public interfaces
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'private')
namespaces.exclude_by_access_type(mb, ['variables', 'calldefs', 'classes'], 'protected')

# exclude names begining with a single underscore or ending with Cpp
namespaces.exclude_by_regex(mb, ['calldefs', 'classes', 'variables'], r'(^_[^_])|(.*Cpp$)|(^impl$)')

ns_util.write_symbols(mb)

no_classes = len([c for c in mb.classes(allow_empty=True) if not c.ignore])
wrap.set_number_of_files(no_classes/10)

wrap.finish()
