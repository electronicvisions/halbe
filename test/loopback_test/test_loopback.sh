#!/bin/bash

TOP_PATH=.
HALBE_PATH=halbe
TESTS2=$TOP_PATH/bin/tests2
TEST_PLAYBACK=$HALBE_PATH/test/loopback_test/test_playback.py

if [ $# -gt 2 ] ; then 
	IP=$1
	FPGA=$2
	DNC=$3
	PORT=$(expr 1700 + $3)
else
	echo "Usage: $0 <ip> <fpga> <dnc>"
	exit 1
fi

echo $IP
echo $FPGA
echo $DNC
echo $PORT

. $HALBE_PATH/test/loopback_test/sh2ju++.sh
juLogSuiteOpen "test_loopback-f$FPGA-d$DNC"

export LD_LIBRARY_PATH=$TOP_PATH/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$TOP_PATH/lib:$PYTHONPATH

for hicann in 0 1 2 3 4 5 6 7 ; do
	echo "1 x" | juLog -name="l2_bench-test_fpga-f$FPGA-d$DNC-h$hicann" ! $TESTS2 -bje 8 $hicann -ip $IP -jp $PORT -m tm_l2bench
	echo "2 x" | juLog -name="l2_bench-test_hicann-f$FPGA-d$DNC-h$hicann" ! $TESTS2 -bje 8 $hicann -ip $IP -jp $PORT -m tm_l2bench

	echo "1 1 1 100" | juLog -name="l2_pulses-ddr-f$FPGA-d$DNC-h$hicann" -error="ERROR.*Init failed, abort" ! $TESTS2 -bje2fa 8 $hicann -r 1 -ip $IP -jp $PORT -m tm_l2pulses
	#echo "0 1 1" | juLog -name="l2_pulses-pulsegen-f$FPGA-d$DNC-h$hicann" -error="ERROR.*Init failed, abort" ! $TESTS2 -bje2f 8 $hicann -ip $IP -jp $PORT -m tm_l2pulses

	juLog -name="test_playback-f$FPGA-d$DNC-h$hicann" $TEST_PLAYBACK --ip $IP --on w --f $FPGA --d $DNC --h $hicann --loglevel 2 --kill-scheriff
done

juLogSuiteClose
