#!/bin/bash

# This script sets the environment variable PYHALBE_API_CHECK to "True" which
# has the following effect on pyhalbe if and only if pyhalbe.apicheck
# is also imported (as HWTest does):

#  * pyoneer is called to set use_hardware and use_mongo to false (@see halbe)
#  * pyhalbe functions which access hardware (type: Boost.Python.function) are wrapped such that,
#    * they throw only if a Boost.Python.ArgumentError is thrown (i.e. API error)
#    * any other errors are catched and ignored
#  * HWTest/Unittest assertions are monkey patched -> they never fail.
#
# A python script can catch the corresponding error using pyhalbe.apicheck.BoostPythonArgumentError

# In other words:
# For running test scripts for a basic API test, without hardware available, one can run:
#
# ./apicheck.sh <test_MyScript.py>
#
# and all hardware accessing functions will be wrapped throwing only on API
# errors and use_hardware=false.

# Author:	Kai Husmann <kai.husmann@kip.uni-heidelberg.de>
# Creation:	[2013-06-12 14:41:32]

if [ $# -lt 1 ]; then
    echo "Usage: $0 <Python script based on HWTest>"
    exit 1
fi

function die() {
    test -n "$1" && echo $1
    exit 1
}

python $1 --help 2>/dev/null | grep -q -- "^HWTest:" || die "$1 is not a HWTest-based test"

ARGUMENTS="--ip 192.168.1.1 --on=vertical"

PYHALBE_API_CHECK=True python $* ${ARGUMENTS}
