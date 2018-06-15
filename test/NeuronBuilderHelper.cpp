#include <gtest/gtest.h>

#include "hal/backend/HICANNBackendHelper.h"
#include "test/NeuronBuilderHelper.h"


using namespace HMF::HICANN;
using namespace HMF::Coordinate;

#define SET_NEURON(ii, ...) \
	{ \
		NeuronOnQuad id(Enum(ii)); \
		Neuron& neuron = quad[id]; \
		__VA_ARGS__ \
	}

#define SET_NEURON0(...) SET_NEURON(0, __VA_ARGS__)
#define SET_NEURON1(...) SET_NEURON(1, __VA_ARGS__)
#define SET_NEURON2(...) SET_NEURON(2, __VA_ARGS__)
#define SET_NEURON3(...) SET_NEURON(3, __VA_ARGS__)

namespace HMF {

std::array<std::bitset<25>, 4> format(NeuronQuad const& quad)
{
	std::array<std::bitset<25>, 4> data;
	for(size_t ii=0; ii<4; ++ii)
	{
		data[ii] = denmen_quad_formatter(
			Coordinate::NeuronOnQuad(Enum(ii)),
			quad);
	}
	return data;
}

HICANN::L1Address randomL1()
{
	return HICANN::L1Address(rand() % 64);
}

bool randomBit()
{
	return bool(rand() % 2);
}

std::bitset<4> getIObits(Y const& y, NeuronQuad const& quad)
{
	std::bitset<4> c;
	c.set(0, quad[NeuronOnQuad(X(1), y)].enable_aout());
	c.set(1, quad[NeuronOnQuad(X(0), y)].enable_aout());
	c.set(2, quad[NeuronOnQuad(X(1), y)].enable_current_input());
	c.set(3, quad[NeuronOnQuad(X(0), y)].enable_current_input());
	return c;
}

NeuronQuad randomize_quad()
{
	NeuronQuad quad;

	SET_NEURON0(
		neuron.address(randomL1());
		neuron.activate_firing(randomBit());
		neuron.enable_spl1_output(randomBit());
		neuron.enable_fire_input(randomBit());
		neuron.enable_aout(randomBit());
		neuron.enable_current_input(randomBit());
	)

	SET_NEURON1(
		neuron.address(randomL1());
		neuron.activate_firing(randomBit());
		neuron.enable_spl1_output(randomBit());
		neuron.enable_fire_input(randomBit());
		neuron.enable_aout(randomBit());
		neuron.enable_current_input(randomBit());
	)

	SET_NEURON2(
		neuron.address(randomL1());
		neuron.activate_firing(randomBit());
		neuron.enable_spl1_output(randomBit());
		neuron.enable_fire_input(randomBit());
		neuron.enable_aout(randomBit());
		neuron.enable_current_input(randomBit());
	)

	SET_NEURON3(
		neuron.address(randomL1());
		neuron.activate_firing(randomBit());
		neuron.enable_spl1_output(randomBit());
		neuron.enable_fire_input(randomBit());
		neuron.enable_aout(randomBit());
		neuron.enable_current_input(randomBit());
	)

	// set neuron interconnects
	quad.setVerticalInterconnect(X(0), randomBit());
	quad.setVerticalInterconnect(X(1), randomBit());

	quad.setHorizontalInterconnect(Y(0), randomBit());
	quad.setHorizontalInterconnect(Y(1), randomBit());

	return quad;
}

void verify_nmem(NeuronQuad const& quad, std::bitset<8> const& nmem, Y const y)
{
	// for details see HICANN docu chapter "DenmemConfiguration" table
	// concerning 8 nmem bits

	std::bitset<3> t = bit::crop<3>(nmem);
	ASSERT_EQ(iomap.at(getIObits(y, quad).to_ulong()), t.to_ulong());

	// check horizontal interconnect
	ASSERT_EQ(quad.getHorizontalInterconnect(y), nmem.test(3));

	// check activate firing
	ASSERT_EQ(quad[NeuronOnQuad(X(0), y)].activate_firing(), nmem.test(4));
	ASSERT_EQ(quad[NeuronOnQuad(X(1), y)].activate_firing(), nmem.test(7));

	// check fire input of adjacent neuron + membrane interconnect
	ASSERT_EQ(quad[NeuronOnQuad(X(0), y)].enable_fire_input(), nmem.test(5));
	ASSERT_EQ(quad[NeuronOnQuad(X(1), y)].enable_fire_input(), nmem.test(6));
}

void verify(NeuronQuad const& quad, DenmemConfig const& config, bool strict)
{
	NeuronOnQuad id(Enum(config.neuron));
	Neuron const& neuron = quad[id];

	std::bitset<6> addr = bit::reverse(config.spl1);
	ASSERT_EQ(neuron.address(), addr.to_ulong());

	if (config.neuron == 1) {
		verify_nmem(quad, config.nmem_top, Y(0));
		verify_nmem(quad, config.nmem_bot, Y(1));
	} else if (strict) {
		ASSERT_EQ(std::bitset<8>(0), config.nmem_top);
		ASSERT_EQ(std::bitset<8>(0), config.nmem_bot);
	}

	bool const top = (id.y() == 0u);
	if (top) {
		ASSERT_EQ(neuron.enable_spl1_output(), config.fireen_top.to_ulong());
		if (strict) {
			ASSERT_EQ(0, config.fireen_bot.to_ulong());
		}
	} else {
		ASSERT_EQ(neuron.enable_spl1_output(), config.fireen_bot.to_ulong());
		ASSERT_EQ(quad.getVerticalInterconnect(id.x()), config.nb_vertical.to_ulong());
		if (strict) {
			ASSERT_EQ(0, config.fireen_top.to_ulong());
		}
	}
}

void verify_reader(NeuronQuad const& quad,
				   std::array<std::bitset<25>, 4> const& data)
{
	NeuronQuad q;
	for (size_t ii=0; ii<4; ++ii)
	{
		denmem_quad_reader(data[ii], NeuronOnQuad(Enum(ii)), q);
	}
	for (size_t ii=0; ii<4; ++ii)
	{
		NeuronOnQuad nrn{Enum{ii}};
		ASSERT_EQ(quad[nrn], q[nrn]) << nrn;
	}
}
} // HMF
