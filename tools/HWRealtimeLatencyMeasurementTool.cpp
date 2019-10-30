#include "HWRealtimeLatencyMeasurementTool.h"

#include <algorithm>
#include <iostream>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <string>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sstream>


extern "C" {
#include <sys/time.h> // needed for getrusage
#include <sys/resource.h> // needed for getrusage
}

#include "hal/Handle/HMFRun.h"
#include "hal/Handle/HICANNHw.h"
#include "hal/backend/HICANNBackend.h"
#include "hal/backend/DNCBackend.h"

namespace po = boost::program_options;


HWRealtimeLatencyMeasurementTool::HWRealtimeLatencyMeasurementTool(HMF::Handle::FPGAHw &f, HMF::Coordinate::DNCOnFPGA const d) :
	f(f),
	dnc(d),
	h(*f.get(dnc, HMF::Coordinate::HICANNOnDNC(geometry::Enum(0)))),
	rc(f.get_realtime_comm()),
	packets(100000)
{
	for (size_t i = 0; i < 64*4; i++) {
		addresses.emplace_back(std::make_pair(HMF::FPGA::SpinnInputAddress_t(i), HMF::FPGA::PulseAddress(
						HMF::Coordinate::DNCOnFPGA(dnc),
						HMF::Coordinate::HICANNOnDNC(h.to_HICANNOnDNC()),
						HMF::Coordinate::GbitLinkOnHICANN(i/64 * 2),
						HMF::HICANN::Neuron::address_t(i % 64))));
	}
	HWRealtimeLatencyMeasurementTool::configureHardware();
	rc.start_sending_thread();
}

void HWRealtimeLatencyMeasurementTool::configureHardware() {

	std::cout << "# configuring hardware ..." << std::endl;

	// DNC & Hicann
	HMF::Handle::HICANN& h(*f.get(dnc, HMF::Coordinate::HICANNOnDNC(geometry::Enum(0))));

	// disable scheriff
	// FIXME => disable scheriff by new method!

	// Reset FPGA and all the other shit
	HMF::FPGA::reset(f);
	HMF::HICANN::init(h, false);

	// Set Loopback mode on Hicann
	HWRealtimeLatencyMeasurementTool::setHicannLoopback(h);

	size_t runtime_in_dnc_clock_cycles = 400; //arbitrary number is arbitrary

	// do experiment before (copied from BV's python-based test)
	HMF::FPGA::write_playback_program(
	    f, HMF::FPGA::PulseEventContainer(), runtime_in_dnc_clock_cycles, 63,
	    true /*enable trace recording*/, true /*drop background events*/);
	HMF::FPGA::prime_experiment(f);
	HMF::FPGA::start_experiment(f);
	HMF::FPGA::read_trace_pulses(f, runtime_in_dnc_clock_cycles);

	// SpiNNaker IF up/downsampler -- we need #up == #down to measure rtt
	size_t up_sample_count = 1;
	size_t down_sample_count = 1;
	HMF::FPGA::set_spinnaker_pulse_upsampler(f, up_sample_count);
	HMF::FPGA::set_spinnaker_pulse_downsampler(f, down_sample_count);

	// routing table
	HMF::FPGA::SpinnRoutingTable routing_table;
	for (auto & p : addresses) {
		routing_table.set(p.first, p.second);
	}
	HMF::FPGA::set_spinnaker_routing_table(f, routing_table);
}

void HWRealtimeLatencyMeasurementTool::setHicannLoopback(HMF::Handle::HICANN &h) {
	/*
	 sets loopback on HICANN 0, such that channel 0->1, 2->3, etc.
	 */

	// configure DNC mergers
	HMF::HICANN::DNCMergerLine mergers;
	HMF::HICANN::DNCMerger mer;
	for (size_t j=0; j<8; j++) {
		if (j%2) {
			mer.config = HMF::HICANN::Merger::RIGHT_ONLY;
		} else {
			mer.config = HMF::HICANN::Merger::LEFT_ONLY;
		}
		mer.slow = false;
		mer.loopback = (j%2)==0; // 0->1, 2->3 etc...
		mergers[HMF::Coordinate::DNCMergerOnHICANN(j)] = mer;
	}

	HMF::HICANN::set_dnc_merger(h, mergers);

	HMF::DNC::GbitReticle gbit;

	HMF::HICANN::GbitLink link;
	for (size_t i=0; i<8; i++) {
		if ((i%2)==1) {
			link.dirs[i] = HMF::HICANN::GbitLink::Direction::TO_DNC;
		} else {
			link.dirs[i] = HMF::HICANN::GbitLink::Direction::TO_HICANN;
		}
	}

	gbit[h.to_HICANNOnDNC()] = link; // only HICANN 0

	HMF::HICANN::set_gbit_link(h, link);
	HMF::DNC::set_hicann_directions(f, dnc, gbit);

	// make sure to switch off dnc loopback
	HMF::DNC::set_loopback(f, dnc, HMF::DNC::Loopback());
}

void HWRealtimeLatencyMeasurementTool::measuringLoop() {


	std::vector<uint64_t> rtt(packets);

	struct rusage page_usage_before;
	getrusage(RUSAGE_SELF, &page_usage_before);

	// TODO: add timeout mechanism!

	std::cout << "## measuring latency ... " << std::endl;


	for(size_t i = 0; i < rtt.size(); i++) {

		size_t idx = i % addresses.size();

		uint64_t sendtime = rc.gettime();

		// send spike (sending is done by sending thread)
		HMF::FPGA::send_spinnaker_realtime_pulse(f, {addresses[idx].first.value()});

		// wait for answer
		auto sp = HMF::FPGA::spin_and_get_next_realtime_pulse_as_spinnaker(f);

		auto const tmp_time = rc.gettime();
		rtt[i] = std::max(tmp_time, sendtime) - std::min(tmp_time, sendtime);

		// expected pulse address is configured pulse address with dnc if channel flipped
		HMF::FPGA::PulseAddress exp_pa(addresses[idx].second);
		auto c = exp_pa.getChannel();
		// exp_pa.setChannel(c.flip(0));
		exp_pa.setChannel( HMF::Coordinate::GbitLinkOnHICANN(static_cast<uint8_t>(c/2)*2 + !(c%2)));
		HMF::FPGA::PulseAddress tmp(sp.label);
		if (tmp != exp_pa) {
			std::stringstream ss;
			ss << "wrong spinnaker label received: " << tmp << std::endl;
			ss << "wanted: " << exp_pa << std::endl;
			throw std::runtime_error(ss.str());
		}
	}

	std::cout << "# raw RTT 0/1/2/-3/-2/-1(last) measurement "
		<< 0.001*rtt.at(0) << " / "
		<< 0.001*rtt.at(1) << " / "
		<< 0.001*rtt.at(2) << " / "
		<< 0.001*rtt.at(rtt.size()-3) << " / "
		<< 0.001*rtt.at(rtt.size()-2) << " / "
		<< 0.001*rtt.at(rtt.size()-1) << std::endl;
	auto rtt_copy = rtt;
	sort(rtt_copy.begin(), rtt_copy.end());
	std::cout << "# RTT 2nd/0.10/0.33/median/0.66/0.80/0.90/-3/-2/-1(last): "
		<< 0.001*rtt_copy[1] << " / "
		<< 0.001*rtt_copy[rtt.size()/10] << " / "
		<< 0.001*rtt_copy[rtt.size()/3] << " / "
		<< 0.001*rtt_copy[rtt.size()/2] << " / "
		<< 0.001*rtt_copy[2*rtt.size()/3] << " / "
		<< 0.001*rtt_copy[4*rtt.size()/5] << " / "
		<< 0.001*rtt_copy[9*rtt.size()/10] << " / "
		<< 0.001*rtt_copy[95*rtt.size()/100] << " / "
		<< 0.001*rtt_copy[rtt.size()-3] << " / "
		<< 0.001*rtt_copy[rtt.size()-2] << " / "
		<< 0.001*rtt_copy[rtt.size()-1]
		<< std::endl;

	{
		std::vector<uint64_t> rtt_copy2;
		std::copy_n(rtt_copy.begin(), rtt_copy.size() - 10, std::back_inserter(rtt_copy2));
		double dsum = std::accumulate(rtt_copy2.begin(), rtt_copy2.end(), 0.0);
		double dmean = dsum / rtt_copy2.size();
		double dsq_sum = std::inner_product(rtt_copy2.begin(), rtt_copy2.end(), rtt_copy2.begin(), 0.0);
		double dstdev = std::sqrt((dsq_sum / rtt_copy2.size() - dmean * dmean) / (1.0 - 1.0/rtt_copy2.size()));
		std::cout << "# Mean Latency (0.5*rtt_copy2): " << 0.001*dmean/2.0 << " +/- " << 0.001*dstdev/2.0 << std::endl;
	}

	std::cout << "done!" << std::endl;
}


int main(int argc, char * argv[]) {

	std::string fpga_ip, pmu_ip, on;
	geometry::Enum d, w;

	// options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("fpga_ip",          po::value<std::string>(&fpga_ip)->default_value("0.0.0.0"),
			 "specify FPGA ip")
		("on",          po::value<std::string>(&on)->default_value("vertical"),
			 "specify hardware backend [[w]afer,[v]ertical]")
		("dnc",       po::value<geometry::Enum>(&d)->default_value(geometry::Enum(1)),
			 "specify DNC (FPGA-local enum)")
		("wafer",     po::value<geometry::Enum>(&w)->default_value(geometry::Enum(0)),
			 "specify Wafer number)")
		("pmu_ip",          po::value<std::string>(&pmu_ip)->default_value("0.0.0.0"),
			 "specify PMU ip")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	// option: help
	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	po::notify(vm);
	// option: fpga_ip
	if (vm.count("fpga_ip")) {
		std::cout << "# remote ip set to " << fpga_ip << ".\n";
	} else {
		return EXIT_FAILURE;
	}
	// option: pmu_ip
	if (vm.count("pmu_ip")) {
		std::cout << "# remote ip set to " << pmu_ip << ".\n";
	} else {
		return EXIT_FAILURE;
	}

	bool on_wafer = (on.at(0) == 'w' || on.at(0) == 'W');

	auto gfpga = HMF::Coordinate::FPGAGlobal(
		HMF::Coordinate::FPGAOnWafer(), HMF::Coordinate::Wafer(w));

	// create FPGAHandle
	HMF::Handle::FPGAHw f(gfpga, HMF::Coordinate::IPv4::from_string(fpga_ip), HMF::Coordinate::DNCOnFPGA(d), HMF::Coordinate::IPv4::from_string(pmu_ip), on_wafer);

	// create tool object and measure latency
	HWRealtimeLatencyMeasurementTool t(f, HMF::Coordinate::DNCOnFPGA(d));

	t.HWRealtimeLatencyMeasurementTool::measuringLoop();

	return 0;
}
