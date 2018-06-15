#!/bin/bash -eu

ON=wafer
FPGA_IP=192.168.1.1
DNCOnFPGA=0
HICANNOnDNC=0
FPGAOnWAFER=0
WAFER=0
# common args to python-based halbe tests and the test-main test
TEST_ARGS=" --on=$ON --ip=${FPGA_IP} --dnc=${DNCOnFPGA} --hicann=${HICANNOnDNC} --fpga=${FPGAOnWAFER} --wafer=${WAFER}"
TEST_MAIN_FILTER='*HW*:-ADCTest.*:HICANNAnalogTest.*:HICANNBackendTest/1.*'

RESULTDIR=build/test_results

mkdir -p ${RESULTDIR}

export NOSE_WITH_XUNIT="1"

function pytest () {
	calltest python $1 ${TEST_ARGS} --xml-output-dir ${RESULTDIR}
}

# provides LD_LIBRARY_PATH, PYTHONPATH ($PWD/lib) and PATH ($PWD/bin)
module load localdir

cd bin/tests/

function die() {
    MSG=$1
    >&2 echo "$MSG"
    exit 1
}

function calltest() {
    EXECUTABLE=$1
    [ "$#" == "0" ] && die "parameters missing"
    shift
    EXECUTABLE_PATH=$(which "$EXECUTABLE" || true)
    if [ ! -x "$EXECUTABLE_PATH" ]; then
        die "executable \"$EXECUTABLE\" not found/executable"
    fi
    "$EXECUTABLE_PATH" "$@" || true
}

# main halbe test
calltest ./halbe-test ${TEST_ARGS} --gtest_filter="${TEST_MAIN_FILTER}" --gtest_output="xml:${RESULTDIR}"

# python tests follow
pytest test_fpga_status.py
pytest test_playback.py
pytest playback_2_hicanns.py
pytest test_Layer2HWTest.py
pytest test_playback.py
pytest test_HICANNBackendHWTests.py
pytest test_issue1144.py
pytest test_issue1083.py
