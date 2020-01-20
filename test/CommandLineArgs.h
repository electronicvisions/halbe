#pragma once

#include "halco/hicann/v2/fwd.h"
#include "hal/Coordinate/HMFGrid.h"
#include "hal/Handle/FPGAHw.h"

struct CommandLineArgs
{
	CommandLineArgs();

	halco::hicann::v2::HICANNOnDNC  h;
	halco::hicann::v2::DNCOnFPGA    d;
	halco::hicann::v2::FPGAGlobal   f;
	halco::hicann::v2::IPv4         fpga_ip;
	halco::hicann::v2::IPv4         pmu_ip;
	std::set<halco::hicann::v2::HICANNOnDNC> available_hicanns;

	bool highspeed;
	bool arq;
	::halco::hicann::v2::SetupType setup;
	halco::hicann::v2::JTAGFrequency jtag_freq; // Hz

	static CommandLineArgs parse(int argc, char *argv[]);
};

extern CommandLineArgs g_conn;
