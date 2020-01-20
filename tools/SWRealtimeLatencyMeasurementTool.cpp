#include "SWRealtimeLatencyMeasurementTool.h"

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

namespace po = boost::program_options;


SWRealtimeLatencyMeasurementTool::SWRealtimeLatencyMeasurementTool(HMF::Handle::FPGAHw &f, bool const master) :
	f(f), rc(f.get_realtime_comm()), master(master)
{
	rc.start_sending_thread();
}

void SWRealtimeLatencyMeasurementTool::measuringLoop() {

	std::cout << "# current time: " << rc.gettime() << std::endl;

	if (!master) {
		while(true) {
			auto sp = HMF::FPGA::spin_and_get_next_realtime_pulse_as_custom(f);
			HMF::FPGA::send_custom_realtime_pulse(f, {sp.timestamp0, rc.gettime(), 0, spike::SYNC});
		}
		return;
	}

	std::vector<uint64_t> rtt(10000);
	std::vector<int64_t> clkdiff(rtt.size());
	std::vector<uint64_t> rawtime(rtt.size());
	std::vector<uint64_t> mytime(rtt.size());
	std::vector<uint64_t> delays(rtt.size());
	std::vector<int64_t> offsets(rtt.size());
	std::vector<uint64_t> remotetime(rtt.size());


	std::cout << "# master" << std::endl;
	uint64_t start_time = rc.gettime();
	for(size_t i = 0; i < rtt.size(); i++)
		rc.gettime();
	uint64_t call_time = (rc.gettime() - start_time)/rtt.size();
	std::cout << "# syscall takes: " << 0.001*call_time << "us." << std::endl;

	struct rusage page_usage_before;
	getrusage(RUSAGE_SELF, &page_usage_before);

	for(size_t i = 0; i < rtt.size();) {
		// send sync spike
		HMF::FPGA::send_custom_realtime_pulse(f, {rc.gettime(), rc.curtime(), 0, spike::SYNC});

		// wait for answer
		// TODO: needs timeout for dropped packets
		auto sp = HMF::FPGA::spin_and_get_next_realtime_pulse_as_custom(f);

		auto tmp = rc.gettime();
		rtt[i] = std::max(tmp, sp.timestamp0) - std::min(tmp, sp.timestamp0);
		rc._delay = rc._delay*0.9 + rtt[i]/2*0.1;
		rc._offset = static_cast<int64_t>(0.99*rc._offset + 0.01*(1.*(sp.timestamp + rc._delay/*rtt[i]/2*/) - rc._curtime));
		//std::cout << "rc._curtime is " << rc._curtime << std::endl;
		//std::cout << "offset is " << static_cast<int64_t>((sp.timestamp + rc._delay) - rc._curtime) << std::endl;
		//std::cout << "offset is " << static_cast<int64_t>(0.01*(1.*(sp.timestamp + rc._delay) - rc._curtime)) << std::endl;
		//std::cout << "offset is " << static_cast<int64_t>(rc._offset) << std::endl;

		clkdiff[i] = rc.curtime() - (sp.timestamp + rc._delay);
		rawtime[i] = rc._curtime;
		remotetime[i] = sp.timestamp;
		mytime[i] = rc.curtime();
		delays[i] = rc._delay;
		offsets[i] = rc._offset;
		//std::cout << "\t(lat was " << rtt[i]/2 << ")" << std::endl;
		//std::cout << "\t(sp.timestamp0 was        " << sp.timestamp0 << ")" << std::endl;
		//std::cout << "\t(sp.timestamp was         " << sp.timestamp << ")" << std::endl;
		//std::cout << "\t(sp.timestamp + delay was " << sp.timestamp + rc._delay << ")" << std::endl;
		//std::cout << "\t(curtime was               " << rc.curtime() << ")" << std::endl;
		//std::cout << "\t(delay was                  " << rc._delay << std::endl;
		//std::cout << std::endl;
		i++;
	}
	struct rusage page_usage_after;
	getrusage(RUSAGE_SELF, &page_usage_after);
	if (  (page_usage_before.ru_majflt != page_usage_after.ru_majflt)
			|| (page_usage_before.ru_minflt != page_usage_after.ru_minflt)) {
		printf("Before: Major-pagefaults:%ld, Minor Pagefaults:%ld\n", page_usage_before.ru_majflt, page_usage_before.ru_minflt);
		printf("After:  Major-pagefaults:%ld, Minor Pagefaults:%ld\n", page_usage_after.ru_majflt, page_usage_after.ru_minflt);
		throw std::runtime_error("page faults during experiment!");
	}

	auto rtt_copy = rtt;
	sort(rtt_copy.begin(), rtt_copy.end());
	uint64_t sum = std::accumulate(rtt_copy.begin(), rtt_copy.end(), 0);
	auto max = std::max_element(rtt_copy.begin(), rtt_copy.end());
	auto min = std::min_element(rtt_copy.begin(), rtt_copy.end());
	std::cout << "# RTT seems to be: " << 0.001*sum/(rtt.size()-1)
		<< " (min/max = "
		<< *min/2000. << "/" << *max/2000. << ")" << std::endl;
	std::cout << "# RTT min/max: " << 0.001*rtt_copy[1] << "/" << 0.001*rtt_copy[rtt_copy.size()-1] << std::endl;
	std::cout << "# RTT 2nd/0.10/0.33/median/0.66/0.80/0.90/next-to-last/last: "
		<< 0.001*rtt_copy[1] << " / "
		<< 0.001*rtt_copy[rtt.size()/10] << " / "
		<< 0.001*rtt_copy[rtt.size()/3] << " / "
		<< 0.001*rtt_copy[rtt.size()/2] << " / "
		<< 0.001*rtt_copy[2*rtt.size()/3] << " / "
		<< 0.001*rtt_copy[4*rtt.size()/5] << " / "
		<< 0.001*rtt_copy[9*rtt.size()/10] << " / "
		<< 0.001*rtt_copy[95*rtt.size()/100] << " / "
		<< 0.001*rtt_copy[rtt.size()-2] << " / "
		<< 0.001*rtt_copy[rtt.size()-1]
		<< std::endl;
	//sort(clkdiff.begin(), clkdiff.end());

	std::cout << "# Absolute time difference:" << std::endl;
	std::cout << "# \tfrst:" << 0.001*clkdiff[0] << std::endl;
	std::cout << "# \tmidd:" << 0.001*clkdiff[clkdiff.size()/2] << std::endl;
	std::cout << "# \t-30%:" << 0.001*clkdiff[clkdiff.size()*0.70] << std::endl;
	std::cout << "# \t-20%:" << 0.001*clkdiff[clkdiff.size()*0.80] << std::endl;
	std::cout << "# \t-15%:" << 0.001*clkdiff[clkdiff.size()*0.85] << std::endl;
	std::cout << "# \t-10%:" << 0.001*clkdiff[clkdiff.size()*0.90] << std::endl;
	std::cout << "# \t -5%:" << 0.001*clkdiff[clkdiff.size()*0.95] << std::endl;
	std::cout << "# \t -5%:" << 0.001*clkdiff[clkdiff.size()*0.96] << std::endl;
	std::cout << "# \t -3%:" << 0.001*clkdiff[clkdiff.size()*0.97] << std::endl;
	std::cout << "# \t -2%:" << 0.001*clkdiff[clkdiff.size()*0.98] << std::endl;
	std::cout << "# \t -1%:" << 0.001*clkdiff[clkdiff.size()*0.99] << std::endl;
	std::cout << "# \tlast:" << 0.001*clkdiff[clkdiff.size()-1] << std::endl;

	{
		double dsum = std::accumulate(rtt.begin(), rtt.end(), 0.0);
		double dmean = dsum / rtt.size();
		double dsq_sum = std::inner_product(rtt.begin(), rtt.end(), rtt.begin(), 0.0);
		double dstdev = std::sqrt(dsq_sum / rtt.size() - dmean * dmean);
		std::cout << "# End2End Latency: " << 0.001*rc._delay << " +/- " << 0.001*dstdev << std::endl;
	}

	{
		const int nelems = clkdiff.size()*0.1;
		auto beforeend = clkdiff.begin();
		std::advance(beforeend, clkdiff.size() - nelems);
		double dsum = std::accumulate(beforeend, clkdiff.end(), 0.0);
		double dmean = dsum / std::distance(beforeend, clkdiff.end());
		double dsq_sum = std::inner_product(beforeend, clkdiff.end(), beforeend, 0.0);
		double dstdev = std::sqrt(dsq_sum / std::distance(beforeend, clkdiff.end()) - dmean * dmean);
		std::cout << "# Clock Offset   : " << 0.001*rc._offset << " +/- " << 0.001*dstdev << std::endl;
	}
	/*
	   sleep(5);

	   for (size_t i = 0; i < rtt.size(); i++) {
	   std::cout << rawtime[i] << " ";
	   std::cout << mytime[i] << " ";
	   std::cout << offsets[i] << " ";
	   std::cout << rtt[i] << " ";
	   std::cout << delays[i] << " ";
	   std::cout << clkdiff[i] << " ";
	   std::cout << remotetime[i] << " ";
	   std::cout << std::endl;
	   }*/

}

int main(int argc, char * argv[]) {

	// options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("master", po::value<bool>(), "set master mode")
		("ip_remote", po::value<std::string>()->required(), "set FPGA ip")
		("ip_pmu", po::value<std::string>()->required(), "set PMU ip")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	// option: help
	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}
	// option: master
	bool master = false;
	if (vm.count("master")) {
		master = vm["master"].as<bool>();
		std::cout << "# Master mode set to " << master << ".\n";
	} else {
		std::cout << "# Master mode was not set.\n";
	}
	// option: ip_remote
	std::string ip_remote;
	if (vm.count("ip_remote")) {
		ip_remote = vm["ip_remote"].as<std::string>();
		std::cout << "# remote ip set to " << ip_remote << ".\n";
	} else {
		return EXIT_FAILURE;
	}
	// option: ip_pmu
	std::string ip_pmu;
	if (vm.count("ip_pmu")) {
		ip_pmu = vm["ip_pmu"].as<std::string>();
		std::cout << "# pmu ip set to " << ip_pmu << ".\n";
	} else {
		return EXIT_FAILURE;
	}

	// create FPGAHandle
	HMF::Handle::FPGAHw f(halco::hicann::v2::FPGAGlobal(halco::common::Enum(0)), halco::hicann::v2::IPv4::from_string(ip_remote), halco::hicann::v2::DNCOnFPGA(halco::common::Enum(1)), halco::hicann::v2::IPv4::from_string(ip_pmu));

	// create tool object and measure latency
	SWRealtimeLatencyMeasurementTool t(f, master);

	t.SWRealtimeLatencyMeasurementTool::measuringLoop();

	return 0;
}
