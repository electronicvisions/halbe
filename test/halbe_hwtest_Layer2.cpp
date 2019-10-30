#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

#include "hwtest.h"

#include "reticle_control.h"       //reticle control class

#include "bitter/integral.h"

#include "hal/Coordinate/iter_all.h"

using namespace std;
using namespace geometry;
using namespace ::HMF::Coordinate;

namespace HMF {

class Layer2Test : public ::HWTest {
protected:
	// compare two pulse lists for being exactly equal.
	void compare_pulse_lists(FPGA::PulseEventContainer & expected, FPGA::PulseEventContainer & actual){
		ASSERT_EQ(expected.size(), actual.size());
		size_t npulses = expected.size();
		for (size_t n = 0; n<npulses; ++n)
			EXPECT_EQ(expected[n], actual[n]);
	}

	/// compare two pulse lists for having the same address
	void compare_pulse_lists_address(const FPGA::PulseEventContainer & expected,
			FPGA::PulseEventContainer & actual,
			bool flip_channel = false){
		ASSERT_EQ(expected.size(), actual.size());
		size_t npulses = expected.size();
		for (size_t n = 0; n<npulses; ++n) {
			auto exp_addr = static_cast<FPGA::PulseAddress>(expected[n]);
			if (flip_channel)
			{
				auto tmp = exp_addr.getChannel().value();
				exp_addr.setChannel(GbitLinkOnHICANN(bit::flip(tmp, 0)));
			}
			EXPECT_EQ(exp_addr, static_cast<FPGA::PulseAddress>(actual[n]));
		}
	}

	std::string print_spike_stats(const FPGA::PulseEventContainer & pulses)
	{
		std::array<size_t, 8> chip_cnt;
		std::array<size_t, 8> channel_cnt;
		std::fill(chip_cnt.begin(), chip_cnt.end(), 0);
		std::fill(channel_cnt.begin(), channel_cnt.end(), 0);

		size_t npulses = pulses.size();
		for (size_t n = 0; n < npulses; ++n) {
			++channel_cnt[pulses[n].getChannel()];
			++chip_cnt[pulses[n].getChipAddress().id()];
		}
		std::stringstream out;
		for (auto chip : iter_all<HICANNOnDNC>())
		{
			if (chip_cnt[chip.id()] > 0)
			{
				out	<< chip_cnt[chip.id()] << " spikes on "
					<< chip << "\n";
			}
		}
		for (auto channel : iter_all<GbitLinkOnHICANN>())
		{
			if (channel_cnt[channel] > 0)
			{
				out << channel_cnt[channel] << " spikes on "
					<< channel << "\n";
			}
		}
		return out.str();
	}
};

TEST_F(Layer2Test, HICANNLoopbackHWTest) {
	HICANN::init(h, false);

	//configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for(int j=0; j<8; j++){
		if (j%2) mer.config = HICANN::Merger::RIGHT_ONLY;
		else mer.config = HICANN::Merger::LEFT_ONLY;
		mer.slow = false;
		mer.loopback = !(j%2);
		mergers[Coordinate::DNCMergerOnHICANN(j)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

	//configure DNC<->HICANN links
	DNC::GbitReticle gbit = DNC::GbitReticle();
	HICANN::GbitLink link = HICANN::GbitLink();
	for (int i = 0; i < 8; i++){
		if (i%2) link.dirs[i] = HICANN::GbitLink::Direction::TO_DNC;
		else link.dirs[i] = HICANN::GbitLink::Direction::TO_HICANN;
	}
	gbit[hc] = link; //only HICANN0
	HICANN::set_gbit_link(h, link);
	DNC::set_hicann_directions(f, d, gbit);

	// switch off dnc loopback
	DNC::set_loopback(f, d, DNC::Loopback() );

	for (size_t n_chan = 0; n_chan<8; n_chan+=2) {
		GbitLinkOnHICANN link_c(n_chan);
		FPGA::PulseEventContainer::container_type pulse_events;
		FPGA::PulseAddress pulse_address(d, hc, link_c, HICANN::Neuron::address_t(63));
		for (size_t np = 0; np<500; ++np)
			pulse_events.push_back(
				FPGA::PulseEvent(pulse_address, 1ULL * 1000 * np + 1500)); // every 500 cycles

		FPGA::PulseEventContainer pc(std::move(pulse_events));

		FPGA::PulseEvent::spiketime_t const runtime_in_dnc_cycles = pc[pc.size()-1].getTime() + 25e3;

		Logger& log = Logger::instance();
		log(Logger::INFO) << "Start Playback writing";
		FPGA::write_playback_program(
		    f, pc, runtime_in_dnc_cycles, /* fpga_hicann_delay */ 63,
		    true /*enable trace recording*/,
		    false /*drop bg events*/); // 63 fpga_clk cycles are 504 nano seconds
		log(Logger::INFO) << "Start Playback writing done";
		usleep(1000);
		FPGA::prime_experiment(f);
		FPGA::start_experiment(f);
		log(Logger::INFO) << "Started Playback and Trace";
		FPGA::PulseEventContainer received_data = FPGA::read_trace_pulses(f, runtime_in_dnc_cycles);
		log(Logger::INFO) << "read trace_pulses" << Logger::flush;

		std::cout << "Sent:\n"
				  << print_spike_stats(pc) << std::endl;
		std::cout << "Received:\n"
				  << print_spike_stats(received_data) << std::endl;
		EXPECT_EQ(pc.size(), received_data.size()) << "Hicann " << hc << ", Channel " << n_chan;
		compare_pulse_lists_address(pc,received_data, /*flip_channel*/ true);
	}
}

/// Tests the general working of the Playback and Trace Memory
/// This test is equal to the DNCLoopbackTest, but uses the HICANN given on
///  the command line and of course the DDR2 Playback and trace memory
TEST_F(Layer2Test, PlaybackTraceSimpleHWTest) {
	// initialize HICANN LVDS Link
	HICANN::init(h, false);

	//configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for(int j=0; j<8; j++){
		if (j%2) mer.config = HICANN::Merger::RIGHT_ONLY;
		else mer.config = HICANN::Merger::LEFT_ONLY;
		mer.slow = false;
		mer.loopback = !(j%2);
		mergers[Coordinate::DNCMergerOnHICANN(j)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

	//configure DNC<->HICANN links
	DNC::GbitReticle gbit = DNC::GbitReticle();
	HICANN::GbitLink link = HICANN::GbitLink();
	for (int i = 0; i < 8; i++){
		if (i%2) link.dirs[i] = HICANN::GbitLink::Direction::TO_DNC;
		else link.dirs[i] = HICANN::GbitLink::Direction::TO_HICANN;
	}
	gbit[hc] = link; //only HICANN0
	HICANN::set_gbit_link(h, link);
	DNC::set_hicann_directions(f, d, gbit);

	// switch off dnc loopback
	DNC::set_loopback(f, d, DNC::Loopback() );

	FPGA::PulseEventContainer::container_type pulse_events;
	//  500 Pulses with ISI = 500 clks (2000 ns)
	size_t isi = 500; //every 500 cycles
	size_t num_pulses = 5000;
	for (size_t np = 0; np<num_pulses; ++np) {
		FPGA::PulseAddress pulse_address(
				d,
				hc,
				GbitLinkOnHICANN(0),
				HICANN::Neuron::address_t(np%64));
		pulse_events.push_back(FPGA::PulseEvent(pulse_address, 1ULL * isi * np + 500 + 126));
	}
	FPGA::PulseEventContainer pc(std::move(pulse_events));
	FPGA::PulseEvent::spiketime_t const runtime_in_dnc_cycles = pc[pc.size()-1].getTime() + 25e3;

	Logger& log = Logger::instance();
	log(Logger::INFO) << "Start Playback writing";
	FPGA::write_playback_program(
	    f, pc, runtime_in_dnc_cycles, /* fpga_hicann_delay */ 63, true /*enable space recording*/,
	    false /*drop bg events*/); // 63 fpga_clk cycles are 504 nano seconds
	log(Logger::INFO) << "Start Playback writing done";
	FPGA::prime_experiment(f);
	FPGA::start_experiment(f);
	log(Logger::INFO) << "Started Playback and Trace";
	FPGA::PulseEventContainer received_data = FPGA::read_trace_pulses(f, runtime_in_dnc_cycles);
	log(Logger::INFO) << "read trace_pulses" << Logger::flush;
	//
	////////////////////////////////////////////////////

	std::cout << "Sended:\n"
	          << print_spike_stats(pc) << std::endl;
	std::cout << "Received:\n"
	          << print_spike_stats(received_data) << std::endl;
	// sent and received pulses should be exactly equal - including timestamps!
	compare_pulse_lists_address(pc,received_data, /*flip_channel*/ true);

	// switch off loopbak to not interfere other tests
	DNC::set_loopback(f, d, DNC::Loopback());
}

/// Tests the general working of the Playback and Trace Memory
/// This test is equal to the DNCLoopbackTest, but uses always HICANN 0
/// and of course the DDR2 Playback and trace memory
TEST_F(Layer2Test, PlaybackTraceTwiceHWTest) {
	size_t hicann_channel = 0;
	auto hicann_on_dnc = Coordinate::HICANNOnDNC(Enum(hicann_channel));
	// initialize HICANN LVDS Link
	HICANN::init(*f.get(d, hicann_on_dnc), false);


	//configure DNC mergers
	HICANN::DNCMergerLine mergers;
	HICANN::DNCMerger mer;
	for(int j=0; j<8; j++){
		if (j%2) mer.config = HICANN::Merger::RIGHT_ONLY;
		else mer.config = HICANN::Merger::LEFT_ONLY;
		mer.slow = false;
		mer.loopback = !(j%2);
		mergers[Coordinate::DNCMergerOnHICANN(j)] = mer;
	}
	HICANN::set_dnc_merger(h, mergers);

	//configure DNC<->HICANN links
	DNC::GbitReticle gbit = DNC::GbitReticle();
	HICANN::GbitLink link = HICANN::GbitLink();
	for (int i = 0; i < 8; i++){
		if (i%2) link.dirs[i] = HICANN::GbitLink::Direction::TO_DNC;
		else link.dirs[i] = HICANN::GbitLink::Direction::TO_HICANN;
	}
	gbit[hc] = link; //only HICANN0
	HICANN::set_gbit_link(h, link);
	DNC::set_hicann_directions(f, d, gbit);

	// switch off dnc loopback
	DNC::set_loopback(f, d, DNC::Loopback() );

	FPGA::PulseEventContainer::container_type pulse_events;
	FPGA::PulseAddress pulse_address(
			d,
			hicann_on_dnc,
			GbitLinkOnHICANN(0),
			HICANN::Neuron::address_t(63));
	//  500 Pulses with ISI = 500 clks (2000 ns)
	size_t isi = 500; //every 500 cycles
	size_t num_pulses = 5000;
	for (size_t np = 0; np<num_pulses; ++np)
		pulse_events.push_back(FPGA::PulseEvent(pulse_address, 1ULL * isi * (np + 1)));
	FPGA::PulseEventContainer pc(std::move(pulse_events));
	FPGA::PulseEvent::spiketime_t const runtime_in_dnc_cycles = pc[pc.size()-1].getTime() + 25e3;

	for (size_t i=0; i < 5; i++) {
		////////////////////////////////////////////////////
		// typical calling sequence for playback experiment!
		//
		Logger& log = Logger::instance();
		log(Logger::INFO) << "Run " << i;
		log(Logger::INFO) << "Start Playback writing";
		FPGA::write_playback_program(
		    f, pc, runtime_in_dnc_cycles, /* fpga_hicann_delay */ 63,
		    true /*enable trace recording*/,
		    false /*drop bg events*/); // 63 fpga_clk cycles are 504 nano seconds
		log(Logger::INFO) << "Start Playback writing done";
		FPGA::prime_experiment(f);
		FPGA::start_experiment(f);
		log(Logger::INFO) << "Started Playback and Trace";
		FPGA::PulseEventContainer received_data = FPGA::read_trace_pulses(f, runtime_in_dnc_cycles);
		log(Logger::INFO) << "read trace_pulses" << Logger::flush;
		//
		////////////////////////////////////////////////////

		ASSERT_EQ(pc.size(), received_data.size()) << "Run " << i;

		compare_pulse_lists_address(pc,received_data, /*flip_channel*/ true);
	}

	// switch off loopbak to not interfere other tests
	DNC::set_loopback(f, d, DNC::Loopback());
}

} // namespace HMF


