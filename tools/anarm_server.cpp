#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>

#include <boost/program_options.hpp>

#include "logger.h"

#include "hal/Handle/ADCHw.h"
#include "hal/Handle/ADCRemoteHw.h"
#include "hal/backend/RemoteADCBackend.h"

namespace po = boost::program_options;


struct ADCBackend_Helper
{
	// opens local ADC handle
	ADCBackend_Helper();
	ADCBackend_Helper(HMF::ADC::USBSerial const);

	// check if boardId() matches local usb device
	void check_helper(HMF::ADC::USBSerial const);


	/* HALbe ADCBackend wrapper functions */
	// FIXME: we could auto-generate this via dispatch mechanism :D
	void config(HMF::ADC::USBSerial const h, HMF::ADC::Config cfg) {
		check_helper(h);
		HMF::ADC::config(adc, cfg);
	}

	double get_sample_rate(HMF::ADC::USBSerial const h) {
		check_helper(h);
		return HMF::ADC::get_sample_rate(adc);
	}

	float get_temperature(HMF::ADC::USBSerial const h) {
		check_helper(h);
		return HMF::ADC::get_temperature(adc);
	}

	void prime(HMF::ADC::USBSerial const h) {
		check_helper(h);
		HMF::ADC::prime(adc);
	}

	void trigger_now(HMF::ADC::USBSerial const h) {
		check_helper(h);
		HMF::ADC::trigger_now(adc);
	}

	HMF::ADC::Status get_status(HMF::ADC::USBSerial const h) {
		check_helper(h);
		return HMF::ADC::get_status(adc);
	}

	HMF::ADC::raw_data_type get_trace(HMF::ADC::USBSerial const h) {
		check_helper(h);
		return HMF::ADC::get_trace(adc);
	}

	HMF::ADC::USBSerial get_board_id(HMF::ADC::USBSerial const h) {
		check_helper(h);
		return HMF::ADC::get_board_id(adc);
	}

private:
	HMF::Handle::ADCHw adc;
};


ADCBackend_Helper::ADCBackend_Helper(HMF::ADC::USBSerial const c) : adc(c)
{
}

void ADCBackend_Helper::check_helper(HMF::ADC::USBSerial const h)
{
	if (adc.boardId() != h)
		throw std::runtime_error("wrong boardId");
}


int main(int argc, const char *argv[]) {
	std::string ip, usb_serial;
	uint16_t port;
	size_t ll;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("ip,i",     po::value<std::string>(&ip)->default_value("0.0.0.0"),
			"specify listening IP")
		("port,p",   po::value<uint16_t>(&port)->required(),
			"specify listening port")
		("anarm,a",  po::value<std::string>(&usb_serial)->required(),
			"specify AnaRM USB serial")
		("loglevel", po::value<size_t>(&ll)->default_value(1),
			"specify loglevel [0-ERROR,1-WARNING,2-INFO,3-DEBUG0,4-DEBUG1,5-DEBUG2,6-DEBUG3]")
		;

	// populate vm variable
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return EXIT_FAILURE;
	}

	po::notify(vm);


	// create logger FIRST
	Logger::instance("Default", ll, "", false);
	Logger::instance("HALbe.anarm_server", ll, "", false);
	Logger::instance("vmodule.usbcom", ll, "", false);

	// create class for handling local AnaRM
	ADCBackend_Helper servant{HMF::ADC::USBSerial(usb_serial)};

	// Fire up RCF...
	RCF::RcfInitDeinit rcfInit;
	//RCF::RcfServer server;
	//RCF::ServerTransport& tcpTransport = server.addEndpoint(RCF::TcpEndpoint(ip, port));

	// we allow a single connection
	//RCF::ThreadPoolPtr tcpThreadPoolPtr( new RCF::ThreadPool(1, 1) );
	//tcpTransport.setThreadPool(tcpThreadPoolPtr);

	RCF::RcfServer server(RCF::TcpEndpoint(ip, port));

	// Set max message length to 512 MiB.
	server.getServerTransport().setMaxMessageLength(512*1024*1024);

	// pass usb_serial to thing...
	server.bind<I_HALbeADC>(servant);
	std::cout << "Starting up..." << std::endl;
	server.start();

	pause();

	return 0;
}
