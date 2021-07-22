"""
This module monkey patches pyhalbe to offer an API check functionality.

Using this module one can do an API check of the pyhalbe test functions without
the neccessity of a hardware connection. To enable this module the environment
variable PYHALBE_API_CHECK must be set to true prior importing this module.

If enabled, all attributes of type ''Boost.Python.function'' in module pyhalbe
(and its submodules) are replaced (monkey patched) by a wrapper function which
catches all errors except the ''Boost.Python.ArgumentError'' which is rethrown.
If the module was enabled can be checked by the property ''enabled''. The
module also offers the type ''BostPythonArgumentError'' to explicitly check for
the above error.

For the API check to work one has to disable the unittests as well. This can
easily be done using the monkeyPatch function.

Intended usage:
    HWTest class setUp function imports this module and checks if it was
    enabled. If so it additionally patches its unittest assertions (all
    functions ''assert*'' in ''self'' and we're good.

    import pyhalbe_apicheck_extension as apicheck
    if apicheck.enabled: apicheck.monkeyPatch(self, "assert")

    Tests can be run using the apicheck.sh which sets the PYHALBE_API_CHECK and
    adds pseudo IP and PORT parameters.

    $ ./apicheck.sh test_HICANNBackendHWTests.py

Author:     Kai Husmann <kai.husmann@kip.uni-heidelberg.de> (KHS)
Creation:   [2013-06-12 08:51:58]
Final:      [2013-06-25 14:07:30]
"""


##### ##### ##### #####

# Public/ reusable attributes:


def loadEnvironBool(env, defaultsTo = False):
    """Loads a boolean value from environment variable ''env''. Accepts ''0, f,
    false, n, no'' and ''1, t, true, y, yes''. The parameter can be given in
    any case. An empty parameter defaults to ''False'' whereas an unparsable
    parameter throws a RuntimeError"""

    from os import getenv
    v = getenv(env)

    if v == None: return defaultsTo

    if ( v.lower() in '0 f false n no'.split(' ') ):
            return False

    #else..
    if ( v.lower() in '1 t true y yes'.split(' ') ):
            return True

    #else..
    raise RuntimeError('%s environment variable could not be parsed:\n\t%s is neither true nor false.' % (env, v))



# intended usage: monkeyPatch(self, 'assert') to disable all assertions
def monkeyPatch(namespace, startswith, replacement=None):
    """Monkeypatches all attributes in ''namespace'' that start with
    ''startswith'' and replaces them with a lambda function returning
    ''replacement'' (defaults to None)"""

    for f in dir(namespace):
        if f.startswith(startswith):
            setattr(namespace, f, lambda *args,**kwargs: replacement)



# attribute description tuple (module, name_of_attribute, attribute)
class AttributeDescriptor:
    """A tuple containing all data that is nececcary for effective work using
    get/setattr to change a modules behaviour"""

    def __init__(self, module, name, attribute):
        # fails: assert attribute.__name__ in dir(module)
        assert getattr(module, name) == attribute

        self.module     = module
        self.name       = name
        self.attribute  = attribute

    def __str__(self):
        return '%s \t%-31s \t%s' % (self.module.__name__, self.name, self.attribute)



##### ##### ##### #####

# Private/ non-reusable attributes:


# Hack to get the type of Boost.Python.ArgumentError
def __getBoostPythonArgumentError():
    """This should be called internally only. Executes
    pyhalbe.Debug.getHalbeGitVersion with definitely wrong parameters to raise
    a Boost.Python.ArgumentError exception. The exception is caught and its
    type is returned"""

    ARGUMENT_ERROR_MODULE       = 'Boost.Python'
    ARGUMENT_ERROR_NAME         = 'ArgumentError'
    ARGUMENT_ERROR_MSG_PART     = 'did not match C++ signature:'

    try:
        # The following call should always throw a Boost.Python.ArgumentError
        p.Debug.getHalbeGitVersion("a stupid and wrong parameter", 1, 2, 3, 4, 5, 6, 7, 8, 9)
    except Exception as e:
        assert type(e).__module__   == ARGUMENT_ERROR_MODULE
        assert type(e).__name__     == ARGUMENT_ERROR_NAME
        if not ( ARGUMENT_ERROR_MSG_PART in e.message.split('\n') ): # foreign
            #locale could break this assertion? In this case it can safely be
            #removed
            logger.warn("ArgumentError.message missing expected: %s" % ARGUMENT_ERROR_MSG_PART)
        return type(e)

    assert False # Unreachable Code! (above try block should always throw, be catched and return the error type


#def __collectBoostPythonFunctions(): # TODO simplify _wrap_pyhalbe...
#    functions   = []
#
#    rtype       = ['module']                        # type thats recursed
#    atype       = ['Boost.Python.function']         # type thats added
#
#    def recmod(module):
#        for child in dir(module):
#            if ( type(child) == 'module' ): recmod(getattr(module, child)
#            elif ( type(child).__module__

def __patch_pyhalbe_hw_handle():
    """ Replace factory functions for hardware handles by new ones returning noop dumping handles"""
    logger.debug("Patch handle factory functions")
    H = p.Handle
    dumper = H.Dump()

    def createFPGAHw(coord, ip, *args):
        return H.FPGADump(dumper, coord)

    def createADCHw(*args):
        return H.ADCDump(dumper, *args)

    def freeFPGAHw(h):
        pass

    def freeADCHw(h):
        pass

    p.Handle.createFPGAHw = createFPGAHw
    p.Handle.createADCHw  = createADCHw
    p.Handle.freeFPGAHw   = freeFPGAHw
    p.Handle.freeADCHw    = freeADCHw


# Wrap pyhalbe-hardware functions to perform an apicheck only
def __wrap_pyhalbe():
    """This should be called internally only. Does the actual wrapping of the
    pyhalbe module."""

    # helper class to collect specific attributes of a module/ partly reusable
    class AttributeTypeCollection:

        def __init__(self, module, search_type):
            # initialize
            self.members        = []
            self.module         = module
            
            # separate Some.Nodule.type into Some.Module and type
            search_type_split   = search_type.split('.')
            self.search_type    = search_type
            self.type_module    = reduce(lambda x,y: x+'.'+y, search_type_split[0:-1])
            self.type_name      = search_type_split[-1]

            # do the reflection search
            self._recurse_module(self.module)


        def _recurse_module(self, module):
            logger.debug0( 'Recursing %s of type %s' % (module.__name__, type(module) ) )
            for childName in dir(module):
                # get childs meta data
                child       = getattr(module, childName)
                cTypeModule = type(child).__module__
                cTypeName   = type(child).__name__

                logger.log(4, 'Attribute %s has type %s (%s)' % (childName, cTypeName, cTypeModule))
                if ( cTypeName in ['module']): #,'class'] ):
                    #assert cTypeModule == '__builtin__' # or Boost.Python.class
                    self._recurse_module(child)
                    continue
                elif ( (cTypeName == self.type_name) and (cTypeModule == self.type_module) ):
                    a = AttributeDescriptor(module, childName, child)
                    logger.debug0( 'Appending %s' % a )
                    self.members.append( a )
                    continue
                else:
                    continue

                assert (false) # non-reachable


        def list_collection(self):
            print(('Listing all members of %s of type %s' % (self.module.__name__, self.search_type)))
            for a in self.members:
                print(a)
            print()
                

    # A helper class to Boost.Python.functions
    class ApiCheck_TestWrapper():
        """This class offers the wrapper function for pyhalbe hardware
        functions."""

        def __init__(self, attr_desc): # , wrap_before, wrap_after):
            self.func_module    = attr_desc.module
            self.__name__      = attr_desc.name
            self.orig_function  = attr_desc.attribute
            self.func_fqn       = '%s.%s' % (self.func_module.__name__, self.__name__) # fqn=fully-qualified-name
            setattr(self.func_module, self.__name__, self.wrap_function)


        def wrap_function(self, *args, **kwargs):
            logger.debug0("Calling wrapped %s" % self.func_fqn)

            # Pyoneer changes now expect orig_function to throw only on API failures.
            # i.e there is no wrapping needed atm?

            #return self.orig_function(*args, **kwargs)

            try:
                return self.orig_function(*args, **kwargs)
            except RuntimeError as e:
                logger.warn('Runtime error ignored: %s' % self.func_fqn)
                logger.warn(e)
            except BoostPythonArgumentError as e:
                logger.error('Argument/Signature error found!')
                raise(e)
            except Exception as e:
                logger.warn('Unknown error ignored: %s' % e.message)

    # the function (__wrap_pyhalbe)
    ATTRIBUTE_TYPE_TO_WRAP     = 'Boost.Python.function'

    logger.debug("Wrapping Pyhalbe")
    # TODO offer possibility for scripter to access replacement class if neccessary and do some tweaking..

    # load all hardware accessing functions (attribute of type Boost.Python.function in module pyhalbe)
    hardware_functions_collection = AttributeTypeCollection(p, ATTRIBUTE_TYPE_TO_WRAP)
    #hardware_functions_collection.list_collection()

    # and wrap all these functions:
    tf = p.Debug.getHalbeGitVersion # just for an assertion
    logger.debug0('Test Function (preWrap): %s' % tf)
    for f in hardware_functions_collection.members:
        ApiCheck_TestWrapper(f)
    
    logger.debug0('Test Function (pstWrap): %s' % p.Debug.getHalbeGitVersion)
    assert (tf != p.Debug.getHalbeGitVersion) # assert replacement



##############################
# pyhalbe_apicheck_extension #
##############################

#if __name__ == "__main__":
#    raise("The pyhalbe_apicheck_extension module is supposed to be imported by pyhalbe, it cannot run on its own.")

# else: load the module:
# def load_module() wrapper?


# Prepare logging
import logging
from functools import reduce
logging.debug('LOADING module pyhalbe_apicheck_extension') # kh: if this is missing logging does not take place ? why? needed for initialization of logging module?
logging.addLevelName(5, "DEBUG0") # for esp. recursive output
logger = logging.getLogger(__name__)
setattr(logger, "debug0", lambda *args, **kwargs: logger.log(logging.getLevelName("DEBUG0"), *args, **kwargs))

logger.setLevel(logging.INFO)
#logger.setLevel(logging.DEBUG)
#logger.setLevel(5)
#logger.setLevel(4)



# Loading the module:

import pyhalbe as p
__PYHALBE_API_CHECK           = loadEnvironBool('PYHALBE_API_CHECK')

# @property # ECM: seems to be buggy
def enabled(): return __PYHALBE_API_CHECK # for convenience

BoostPythonArgumentError    = __getBoostPythonArgumentError()

if __PYHALBE_API_CHECK:
    logger.info("pyhalbe apicheck enabled -- i.e. hardware will not be accessed (PYHALBE_API_CHECK)")
    __wrap_pyhalbe()
    __patch_pyhalbe_hw_handle()

# Clean up:
del p
del __getBoostPythonArgumentError
del __wrap_pyhalbe
del __patch_pyhalbe_hw_handle

logger.debug("Exports (dir()):\n%s", dir())
logging.debug('LEAVING module pyhalbe_apicheck_extension, status: %s' % __PYHALBE_API_CHECK)

