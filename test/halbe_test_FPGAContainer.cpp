#include <gtest/gtest.h>

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Coordinate/iter_all.h"
#include "hal/FPGA/PulseAddress.h"

#include <iostream>

using namespace HMF::Coordinate;

namespace HMF {
namespace FPGA {

static PulseAddress::label_t swap_channel_test(PulseAddress::label_t v)
{
	return (7-v);
}

namespace {
	static const PulseAddress::label_t default_value = PulseAddress::default_label;
	const std::array<PulseAddress::label_t, 8> link_to_hw{{0, 2, 4, 6, 1, 3, 5, 7}};
	const std::array<PulseAddress::label_t, 8>  link_from_hw{{0, 4, 1, 5, 2, 6, 3, 7}};
}

TEST(PulseAddress, Converter)
{

	for (PulseAddress::label_t ii = 0; ii < 8; ++ii)
	{
		EXPECT_EQ(PulseAddress::swap_channel(ii), swap_channel_test(ii));
		EXPECT_EQ(PulseAddress::link_to_hw(ii), link_to_hw.at(ii));
		EXPECT_EQ(PulseAddress::link_from_hw(ii), link_from_hw.at(ii));
	}
}

TEST(PulseAddress, Constructor)
{
	DNCOnFPGA d;
	HICANNOnDNC h;
	GbitLinkOnHICANN link;
	L1Address neuron;

	{
		PulseAddress addr;
		//std::cout << addr << std::endl;
		ASSERT_EQ(addr.getDncAddress(),    d);
		ASSERT_EQ(addr.getChipAddress(),   h);
		ASSERT_EQ(addr.getChannel(),       link);
		ASSERT_EQ(addr.getNeuronAddress(), neuron);
		ASSERT_EQ(addr.getLabel(), default_value);
	}

	{
		PulseAddress addr(d, h, link, neuron);
		//std::cout << addr << std::endl;
		ASSERT_EQ(addr.getDncAddress(),    d);
		ASSERT_EQ(addr.getChipAddress(),   h);
		ASSERT_EQ(addr.getChannel(),       link);
		ASSERT_EQ(addr.getNeuronAddress(), neuron);
		ASSERT_EQ(addr.getLabel(), default_value);
	}
}

TEST(PulseAddress, DncAddress)
{
	DNCOnFPGA d;
	HICANNOnDNC h;
	GbitLinkOnHICANN link;
	L1Address neuron;

	for(DNCOnFPGA d_test : iter_all<DNCOnFPGA>())
	{
		PulseAddress addr(d, h, link, neuron);
		addr.setDncAddress(d_test);
		//std::cout << addr << std::endl;
		ASSERT_EQ(addr.getDncAddress(),    d_test);
		ASSERT_EQ(addr.getChipAddress(),   h);
		ASSERT_EQ(addr.getChannel(),       link);
		ASSERT_EQ(addr.getNeuronAddress(), neuron);
		const PulseAddress::label_t label_value =
			default_value | (d_test.value() << 12);
		ASSERT_EQ(addr.getLabel(), label_value);
		ASSERT_EQ(addr, PulseAddress(d_test, h, link, neuron));
	}
}

TEST(PulseAddress, ChipAddress)
{
	DNCOnFPGA d;
	HICANNOnDNC h;
	GbitLinkOnHICANN link;
	L1Address neuron;

	for(HICANNOnDNC h_test : iter_all<HICANNOnDNC>())
	{
		PulseAddress addr(d, h, link, neuron);
		addr.setChipAddress(h_test);
		//std::cout << addr << std::endl;
		ASSERT_EQ(addr.getDncAddress(),    d);
		ASSERT_EQ(addr.getChipAddress(),   h_test);
		ASSERT_EQ(addr.getChannel(),       link);
		ASSERT_EQ(addr.getNeuronAddress(), neuron);
		const PulseAddress::label_t label_value =
			default_value | (link_to_hw.at(h_test.id().value()) << 9);
		ASSERT_EQ(addr.getLabel(), label_value);
		ASSERT_EQ(addr, PulseAddress(d, h_test, link, neuron));
	}
}

TEST(PulseAddress, HICANNOnReticle)
{
	// hicann channel numbering as seen from the DNC
	// |0|2|4|6|
	// |1|3|5|7|

	DNCOnFPGA d;
	HICANNOnDNC h;
	GbitLinkOnHICANN link;
	L1Address neuron;

	{
		h = HICANNOnDNC(X(0), Y(0));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 0);
	}
	{
		h = HICANNOnDNC(X(0), Y(1));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 1);
	}
	{
		h = HICANNOnDNC(X(1), Y(0));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 2);
	}
	{
		h = HICANNOnDNC(X(1), Y(1));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 3);
	}
	{
		h = HICANNOnDNC(X(2), Y(0));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 4);
	}
	{
		h = HICANNOnDNC(X(2), Y(1));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 5);
	}
	{
		h = HICANNOnDNC(X(3), Y(0));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 6);
	}
	{
		h = HICANNOnDNC(X(3), Y(1));
		PulseAddress addr(d, h, link, neuron);
		ASSERT_EQ( (addr.getLabel() >> 9) & 0x7, 7);
	}
}

TEST(PulseAddress, Channel)
{
	DNCOnFPGA d;
	HICANNOnDNC h;
	GbitLinkOnHICANN link;
	L1Address neuron;

	for(GbitLinkOnHICANN link_test : iter_all<GbitLinkOnHICANN>())
	{
		PulseAddress addr(d, h, link, neuron);
		addr.setChannel(link_test);
		//std::cout << addr << std::endl;
		ASSERT_EQ(addr.getDncAddress(),    d);
		ASSERT_EQ(addr.getChipAddress(),   h);
		ASSERT_EQ(addr.getChannel(),       link_test);
		ASSERT_EQ(addr.getNeuronAddress(), neuron);
		const PulseAddress::label_t label_value =
			(swap_channel_test(link_test.value()) << 6);
		ASSERT_EQ(addr.getLabel(), label_value);
		ASSERT_EQ(addr, PulseAddress(d, h, link_test, neuron));
	}
}

TEST(PulseAddress, NeuronAddress)
{
	DNCOnFPGA d;
	HICANNOnDNC h;
	GbitLinkOnHICANN link;
	L1Address neuron;

	for(L1Address neuron_test : iter_all<L1Address>())
	{
		PulseAddress addr(d, h, link, neuron);
		addr.setNeuronAddress(neuron_test);
		//std::cout << addr << std::endl;
		ASSERT_EQ(addr.getDncAddress(),    d);
		ASSERT_EQ(addr.getChipAddress(),   h);
		ASSERT_EQ(addr.getChannel(),       link);
		ASSERT_EQ(addr.getNeuronAddress(), neuron_test);
		const PulseAddress::label_t label_value =
			default_value | neuron_test.value();
		ASSERT_EQ(addr.getLabel(), label_value);
		ASSERT_EQ(addr, PulseAddress(d, h, link, neuron_test));
	}
}

TEST(PulseAddress, matches)
{
	auto last_dnc = DNCOnFPGA(DNCOnFPGA::max);
	auto last_chip = HICANNOnDNC(Enum(HICANNOnDNC::enum_type::max));
	auto last_channel = GbitLinkOnHICANN(GbitLinkOnHICANN::max);

	for (auto const dnc : iter_all<DNCOnFPGA>()) {
		for (auto const chip : iter_all<HICANNOnDNC>()) {
			for (auto const channel : iter_all<GbitLinkOnHICANN>()) {
				for (auto const neuron : iter_all<L1Address>()) {
					PulseAddress addr(dnc, chip, channel, neuron);
					PulseAddress last_addr(last_dnc, last_chip, last_channel, neuron);
					EXPECT_TRUE(addr.matches(dnc, chip, channel)) << addr;
					EXPECT_FALSE(last_addr.matches(dnc, chip, channel)) << addr << "\n"
					                                                    << last_addr;
				}
				last_dnc = dnc;
				last_chip = chip;
				last_channel = channel;
			}
		}
	}
}

} // end namespace FPGA
} // end namespace HMF
