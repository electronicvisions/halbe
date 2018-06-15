#include "CommandLineArgs.h"

#include <boost/program_options.hpp>
#include "hal/Coordinate/iter_all.h"

#include "logger.h"

CommandLineArgs g_conn;

CommandLineArgs::CommandLineArgs() :
		h(HMF::Coordinate::HICANNOnDNC(geometry::Enum(0))),
		// vertical setup supports only dnc "1"
		d(HMF::Coordinate::DNCOnFPGA(geometry::Enum(1))),
		f(HMF::Coordinate::FPGAGlobal(geometry::Enum(0))),
		fpga_ip(),
		pmu_ip(),
		highspeed(true),
		arq(true),
		use_scheriff(false)
	{}

CommandLineArgs CommandLineArgs::parse(int argc, char *argv[])
{
	CommandLineArgs conn;

	std::string fpga_ip, pmu_ip, on;
	geometry::Enum h, d, f, w;
	std::vector< geometry::Enum> ah;
	size_t ll;
	std::string lf;
	bool ld, scheriff, kintex;
	std::set< ::HMF::Coordinate::HICANNOnDNC> available_hicanns;
	std::vector<geometry::Enum> default_hicanns;
	for(auto hicann_coord : HMF::Coordinate::iter_all< HMF::Coordinate::HICANNOnDNC>()) {
		default_hicanns.push_back(geometry::Enum(hicann_coord.id()));
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
		("dnc,d",       po::value<geometry::Enum>(&d)->default_value(geometry::Enum(0)),
			"specify HMF::Coordinate::DNCOnFPGA")
		("hicann,h",    po::value<geometry::Enum>(&h)->default_value(geometry::Enum(0)),
			"specify HMF::Coordinate::HICANNOnDNC")
		("available_hicanns,a",    po::value<std::vector<geometry::Enum>>(&ah)->default_value(default_hicanns, "")->multitoken(),
			"on V/CubeSetup specify which HICANNs are available in Jtag chain,\n"
			"e.g. --available_hicanns 0 1 4 5 for 2 double hicanns in first two slots")
		("fpga,f",      po::value<geometry::Enum>(&f)->default_value(geometry::Enum(0)),
			"specify HMF::Coordinate::FPGAOnWafer;\n"
			"used to determine on-wafer position of the FPGA (which is selected by the IP)")
		("wafer,w",     po::value<geometry::Enum>(&w)->default_value(geometry::Enum(0)),
			"specify Wafer (global enum);\n")
		("kintex,k",    po::value<bool>(&kintex)->default_value(false),
			 "Kintex mode (otherwise: Virtex)?")
		("scheriff",    po::value<bool>(&scheriff)->default_value(true),
			"call the scheriff")
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
	conn.fpga_ip = HMF::Coordinate::IPv4::from_string(fpga_ip);
	conn.pmu_ip = HMF::Coordinate::IPv4::from_string(pmu_ip);
	conn.h = HMF::Coordinate::HICANNOnDNC(h);
	conn.d = HMF::Coordinate::DNCOnFPGA(d);
	conn.f = HMF::Coordinate::FPGAGlobal(
		HMF::Coordinate::FPGAOnWafer(f), HMF::Coordinate::Wafer(w));
	if((on.at(0) == 'w' || on.at(0) == 'W') || (w > 0)) {
		//on wafer
		if (kintex)
			conn.setup = HMF::Coordinate::SetupType::BSSWafer;
		else
			conn.setup = HMF::Coordinate::SetupType::FACETSWafer;
	} else {
		if (kintex)
			conn.setup = HMF::Coordinate::SetupType::CubeSetup;
		else
			conn.setup = HMF::Coordinate::SetupType::VSetup;
	}

	for (auto hicann = ah.begin(); hicann != ah.end(); hicann++) {
		available_hicanns.insert(HMF::Coordinate::HICANNOnDNC(*hicann));
	}
	conn.available_hicanns = available_hicanns;
	conn.use_scheriff = scheriff;

	return conn;
}
