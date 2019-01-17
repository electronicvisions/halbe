#!/usr/bin/env python
from subprocess import check_output, CalledProcessError
from waflib.extras.gtest import summary

import os

def depends(ctx):
    ctx('hicann-system', 'units/hostfpga',
        branch=('master' if ctx.options.with_kintex else 'virtex5'))
    ctx('hicann-system', 'units/hicann_test')
    ctx('hicann-system', 'units/stage2_hal')
    ctx('hicann-system', 'units/communication')
    ctx('halbe', 'scheriff')
    ctx('halco')
    ctx('lib-rcf')
    ctx('ztl')
    ctx('bitter')
    ctx('rant')
    ctx('pywrap')
    ctx('lib-boost-patches')
    ctx('pythonic')
    ctx('logger')
    ctx('vmodule')
    ctx('hate')
    # please specify waf setup --project=XYZ --with-ess to enable ESS-build
    # for old build flow: use configure/build/install --with-ess
    if ctx.options.with_ess:
        ctx('euter')
        ctx('calibtic')
        ctx('systemsim-stage2', branch='halbe_ess')
        ctx('hicann-system', 'units/hicann_cfg')

def options(opt):
    opt.load('compiler_cxx')
    opt.load('boost')
    opt.load('gtest')
    opt.load('documentation')

    hopts = opt.add_option_group('HALbe Options')
    hopts.add_withoption('ess', default=False, # StHALpeitsche!
                         help='Enable/Disable the generation and build of the ess interface')
    hopts.add_withoption('kintex', default=True,
                         help='Enable/Disable Kintex-mode (disabled == old Virtex-mode)')

def configure(cfg):
    # publish option to other contexts
    cfg.env.build_ess = cfg.options.with_ess

    cfg.load('compiler_cxx')
    cfg.load('boost')
    cfg.load('gtest')
    cfg.load('symap2ic_doxygen')
    cfg.load('documentation')

    cfg.check_boost(lib='serialization iostreams filesystem system thread',
            uselib_store='BOOST4HALBE')
    cfg.check_boost(lib='program_options system',
            uselib_store='BOOST4TOOLS')

    cfg.check_cxx(lib='log4cxx', uselib_store='LOG4CXXHALBE', mandatory=1)

    cfg.find_program('git')

    if cfg.env.build_python_bindings:
        cfg.recurse("pyhalbe")

def build(bld):
    cxxflags=[
        # ECM: some time in the future, we want this :)
        #'-fvisibility=hidden',
        '-fvisibility-inlines-hidden',
    ]

    # always re-check git version, user could have changed stuff at arbitrary times
    try:
        # waf 1.7/1.8 compatibility
        if isinstance(bld.env.GIT, list):
            cmd = bld.env.GIT[0]
        else:
            cmd = bld.env.GIT

        cmd = [cmd, 'describe', '--long', '--dirty' ,'--always', '--abbrev=32']
        git_version = check_output(cmd, cwd=bld.path.abspath()).rstrip()
    except CalledProcessError:
        git_version = "unversioned_build"
    bld(
        rule = 'echo \'char const * HALBE_GIT_VERSION = "%s";\' > ${TGT}' % git_version,
        target = 'halbe_git_version.h',
        export_includes = [bld.path.get_bld()],
       )

    # TODO temporary compat
    bld(
        features        = 'use',
        target          = 'halbe_coordinate_inc',
        use             = ['halco_hicann_v2_inc'],
        export_includes = ['.'],
    )

    # TODO temporary compat
    bld(
        features        = 'use',
        target          = 'halbe_coordinate',
        use             = ['halco_hicann_v2', 'halbe_coordinate_inc'],
    )

    bld(
        target          = 'halbe_container_inc',
        use             = ['halbe_coordinate_inc', 's2hal_inc', 'realtime_inc', 'hostfpga_inc'],
    )

    # ECM (2018-10-19) FIXME: marocco depends on coordinate checks (existance of switches)
    # which are misplaced (i.e. halbe instead of halco)
    bld.objects(
        target          = 'halbe_container_emscripten',
        source          = bld.path.ant_glob('hal/HICANN/Crossbar.cpp') + bld.path.ant_glob('hal/HICANN/SynapseSwitch.cpp'),
        install_path    = '${PREFIX}/lib',
        use             = ['halbe_container_inc', 'halbe_coordinate', 'rcf-boost-only',
                           'bitter', 'pythonic'],
        cxxflags=cxxflags
    )

    bld.shlib(
        target          = 'halbe_container',
        source          = (bld.path.ant_glob('hal/*.cpp') +
            bld.path.ant_glob('hal/FPGA/*.cpp') +
            bld.path.ant_glob('hal/HICANN/*.cpp') +
            bld.path.ant_glob('hal/ADC/*.cpp')),
        install_path    = '${PREFIX}/lib',
        use             = ['halbe_container_inc', 'halbe_coordinate', 'rcf-boost-only',
                           'bitter', 'pythonic'],
        cxxflags=cxxflags
    )

    bld(
        target          = 'halbe_handle_inc',
        use             = ['halbe_coordinate_inc'],
        export_includes = ['.'],
    )

    if bld.env.build_ess:
        bld(
            target          = 'ESS_inc',
            use             = ['halbe_coordinate_inc'],
            export_includes = ['.'],
        )

        uses = [ 'halbe_container', 'halbe_handle_inc', 'hmf_calibration', 'ncsc_model', 'systemsim', 'ESS_inc','logger_obj', 'euter_inc']
        srcs = bld.path.ant_glob('ESS/*.cpp')
        bld.shlib(
            target          = 'ESS',
            features        = 'post_task',
            source          = srcs,
            install_path    = '${PREFIX}/lib',
            post_task       = 'test-ess',
            use             = uses,
            cxxflags=cxxflags
        )

        bld.objects(
            target          = 'halbe_ess_obj',
            source          = bld.path.ant_glob('hal/Handle/**/*Ess.cpp'),
            use             = ['ESS', 'halbe_handle_inc', 'euter_inc'],
            includes        = ['.'],
            export_defines  = ['HAVE_ESS=1'],
            cxxflags=cxxflags + ['-fPIC']
        )


    halbe_handle_uses = [
        'halbe_ess_obj', # might be undefined (--without-ess) :)
        'halbe_handle_inc',
        'scheriff',
        'vmodule_objects',
        's2hal_obj',
        'tud_jtag',
        'rcf-boost-only'
    ]
    bld.shlib(
        target          = 'halbe_handle',
        source          = bld.path.ant_glob('hal/Handle/**/*.cpp',
                                            excl=['hal/Handle/**/*Ess.cpp']
                                           ),
        install_path    = '${PREFIX}/lib',
        use             = halbe_handle_uses,
        cxxflags=cxxflags
    )


    bld(
        target          = 'halbe_backend_inc',
        use             = ['halbe_handle_inc', 'halbe_container_inc'],
    )

    bld.shlib(
        target          = 'halbe_backend',
        source          = bld.path.ant_glob('hal/backend/**/*.cpp'),
        install_path    = '${PREFIX}/lib',
        use             = ['halbe_handle', 'halbe_container',
                           'vmodule_objects', 's2hal_obj', 'bitter', 'rcf-boost-only',
                           'BOOST4HALBE', 'halbe_git_version.h',
                           'git_version.h', 'RT', 'scheriff_obj', 'hate_inc'],
        cxxflags=cxxflags
    )

    bld.shlib(
        features        = 'cxx cxxshlib',
        target          = 'halbe',
        source          = [],
        use             = ['halbe_backend'],
    )

    bld(
        target       = 'hwtest_obj',
        features     = 'gtest cxx',
        source       = 'test/hwtest.cpp test/CommandLineArgs.cpp',
        use          = ['halbe', 'GTEST', 'BOOST4TOOLS'],
        defines      = ['GTEST_CASE', 'GTEST_USE_OWN_TR1_TUPLE=1'],
        export_defines = ['GTEST_USE_OWN_TR1_TUPLE=1'],
        cxxflags     = cxxflags
    )

    sw_test_srcs =  bld.path.ant_glob('test/**/halbe_test_*.cpp')
    sw_test_srcs += bld.path.ant_glob('test/halbe_test_*/*.cpp')
    sw_test_srcs += bld.path.ant_glob('test/*Helper.cpp')

    hw_test_srcs = bld.path.ant_glob('test/**/halbe_hwtest_*.cpp')
    hw_test_srcs += bld.path.ant_glob('test/*Helper.cpp')

    bld(
        target       = 'halbe_tests',
        features     = 'gtest cxx cxxprogram',
        source       = sw_test_srcs,
        use          = ['hwtest_obj', 'pythonic'],
        install_path = '${PREFIX}/bin',
        cxxflags     = cxxflags
    )

    bld(
        target       = 'halbe_hwtests',
        features     = 'gtest cxx cxxprogram',
        source       = hw_test_srcs,
        test_main    = 'test/gtest-main.cpp',
        use          = ['hwtest_obj', 'pythonic'],
        install_path = '${PREFIX}/bin',
        skip_run     = True,
        cxxflags     = cxxflags
    )

    if bld.env.build_ess:
        bld(
            target       = 'test-ess',
            features     = 'gtest cxx cxxprogram',
            test_main    = 'ESS/test/ess-test-main.cpp',
            source       =  bld.path.ant_glob('ESS/test/test-*.cpp') +
                ['ESS/test/ess-test-util.cpp'],
            use          =  ['ESS', 'halbe'],
            install_path = '${PREFIX}/bin/tests',
            test_timeout = 90, # ECM: empirical data :p
            cxxflags=cxxflags
        )

    datadir = os.path.abspath(os.path.join(bld.options.prefix, 'share'))
    datadirsrc = bld.path.find_dir('test/share')
    bld.install_files(
        datadir,
        files=datadirsrc.ant_glob('**/*'),
        cwd=datadirsrc,
        relative_trick=True,
    )

    bld.add_post_fun(summary)

    if bld.env.build_python_bindings:
        bld.recurse("pyhalbe")

    bld.recurse('tools')


# add doc command
def doc(dcx):
    '''build documentation (doxygen)'''

    dcx(
            features    = 'doxygen',                # the feature to use
            #name        = "HMF HAL Backend Manual", # overrides doxy-par: PROJECT_NAME

            doxyfile    = 'doc/doxyfile',           # a doxyfile, use doxygen -g to generate a template
            pars        = {
                'STRIP_FROM_PATH'   : dcx.path.get_src().abspath(),
            },                                      # a dict of doxy-pars: overrides doxyfile pars

            #doxyinput   = "hal",                    # overrides doxy-par: INPUT (list of paths)
            #doxyoutput  = "doc",                    # overrides doxy-par: OUTPUT_DIRECTORY, (a path)
            pdffile     = 'HMF_HALbe-manual.pdf',   # a pdf file to generate, relative to OUTPUT_DIRECTORY
            doxydebug   = True,                     # generate debug output (the final pars to OUTPUT_DIRECTORY/doxypars.debug)
            #quiet       = True                      # suppress all make and doxygen output (if it does not fail)
    )
