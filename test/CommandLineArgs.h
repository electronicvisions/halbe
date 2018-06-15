#pragma once

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Coordinate/HMFGrid.h"
#include "hal/Handle/FPGAHw.h"

struct CommandLineArgs
{
	CommandLineArgs();

	HMF::Coordinate::HICANNOnDNC  h;
	HMF::Coordinate::DNCOnFPGA    d;
	HMF::Coordinate::FPGAGlobal   f;
	HMF::Coordinate::IPv4         fpga_ip;
	HMF::Coordinate::IPv4         pmu_ip;
	std::set<HMF::Coordinate::HICANNOnDNC> available_hicanns;

	bool highspeed;
	bool arq;
	::HMF::Coordinate::SetupType setup;
	bool use_scheriff;

	static CommandLineArgs parse(int argc, char *argv[]);
};

extern CommandLineArgs g_conn;
