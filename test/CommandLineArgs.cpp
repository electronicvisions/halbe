#include "CommandLineArgs.h"

#include <boost/program_options.hpp>
#include "halco/common/iter_all.h"

#include "logger.h"

CommandLineArgs g_conn;

CommandLineArgs::CommandLineArgs() :
		h(halco::hicann::v2::HICANNOnDNC(halco::common::Enum(0))),
		// vertical setup supports only dnc "1"
		d(halco::hicann::v2::DNCOnFPGA(halco::common::Enum(0))),
		f(halco::hicann::v2::FPGAGlobal(halco::common::Enum(0))),
		fpga_ip(),
		pmu_ip(),
		highspeed(true),
		arq(true),
		jtag_freq(halco::hicann::v2::JTAGFrequency())
	{}

CommandLineArgs CommandLineArgs::parse(int argc, char *argv[])
{
	CommandLineArgs conn;

	std::string fpga_ip, pmu_ip, on;
	halco::common::Enum h, d, f, w;
	size_t jtag_freq;
	std::vector< halco::common::Enum> ah;
	size_t ll;
	std::string lf;
	bool ld;
	std::set< ::halco::hicann::v2::HICANNOnDNC> available_hicanns;
	std::vector<halco::common::Enum> default_hicanns;
	for(auto hicann_coord : halco::common::iter_all< halco::hicann::v2::HICANNOnDNC>()) {
		default_hicanns.push_back(halco::common::Enum(hicann_coord.toEnum()));
	}
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("on",          po::value<std::string>(&on)->default_value("w"),
			"specify if \"[w]afer\" or \"[v]ertical\"")
		("fpga_ip",          po::value<std::string>(&fpga_ip)->default_value("0.0.0.0"),
			 "specify IP (std::string) of FPGA")
		("pmu_ip",          po::value<std::string>(&pmu_ip)->default_value("0.0.0.0"),
			 "specify IP (std::string) of PMU")
		("jtag_freq",   po::value<size_t>(&jtag_freq)->default_value(10000),
			 "specify Jtag clock frequency in kHz")
		("dnc,d",       po::value<halco::common::Enum>(&d)->default_value(halco::common::Enum(0)),
			"specify halco::hicann::v2::DNCOnFPGA")
		("hicann,h",    po::value<halco::common::Enum>(&h)->default_value(halco::common::Enum(0)),
			"specify halco::hicann::v2::HICANNOnDNC")
		("available_hicanns,a",    po::value<std::vector<halco::common::Enum>>(&ah)->default_value(default_hicanns, "")->multitoken(),
			"on V/CubeSetup specify which HICANNs are available in Jtag chain,\n"
			"e.g. --available_hicanns 0 1 4 5 for 2 double hicanns in first two slots")
		("fpga,f",      po::value<halco::common::Enum>(&f)->default_value(halco::common::Enum(0)),
			"specify halco::hicann::v2::FPGAOnWafer;\n"
			"used to determine on-wafer position of the FPGA (which is selected by the IP)")
		("wafer,w",     po::value<halco::common::Enum>(&w)->default_value(halco::common::Enum(0)),
			"specify Wafer (global enum);\n")
		("loglevel",    po::value<size_t>(&ll)->default_value(1),
			"specify loglevel [0-ERROR, 1-WARNING, 2-INFO, 3..6-DEBUG0..DEBUG3]")
		("logfile",     po::value<std::string>(&lf),
			"specify logfile [empty string == stdout]")
		("logdual",     po::value<bool>(&ld)->default_value(false),
			"enable dual logging to file and stdout")
		;

	po::variables_map vm;
	po::store(po::parse_environment(desc, "TEST_"), vm);
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help"))
		std::cout << std::endl << desc << std::endl;

	po::notify(vm);

	// create logger FIRST
	Logger& log = Logger::instance("HALbe.test", ll, lf, ld);
	static_cast<void>(log);

	// collect connection information into single struct
	conn.fpga_ip = halco::hicann::v2::IPv4::from_string(fpga_ip);
	conn.pmu_ip = halco::hicann::v2::IPv4::from_string(pmu_ip);
	conn.jtag_freq = halco::hicann::v2::JTAGFrequency(jtag_freq * 1e3);
	conn.h = halco::hicann::v2::HICANNOnDNC(h);
	conn.d = halco::hicann::v2::DNCOnFPGA(d);
	conn.f = halco::hicann::v2::FPGAGlobal(
		halco::hicann::v2::FPGAOnWafer(f), halco::hicann::v2::Wafer(w));
	if((on.at(0) == 'w' || on.at(0) == 'W') || (w > 0)) {
		//on wafer
		conn.setup = halco::hicann::v2::SetupType::BSSWafer;
	} else {
		conn.setup = halco::hicann::v2::SetupType::CubeSetup;
	}

	for (auto hicann = ah.begin(); hicann != ah.end(); hicann++) {
		available_hicanns.insert(halco::hicann::v2::HICANNOnDNC(*hicann));
	}
	conn.available_hicanns = available_hicanns;

	return conn;
}
