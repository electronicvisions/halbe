#pragma once

#include <array>
#include <bitter/bitter.h>
#include "hal/backend/HICANNBackend.h"

namespace HMF {

struct DenmemConfig
{
	DenmemConfig(size_t neuron, std::bitset<25> const& bits) :
		neuron(neuron)
	{
		spl1        = bit::crop<6>(bits);
		nmem_top    = bit::crop<8>(bits, 6);
		nmem_bot    = bit::crop<8>(bits, 6+8);
		nb_vertical = bit::crop<1>(bits, 6+8+8);
		fireen_top  = bit::crop<1>(bits, 6+8+8+1);
		fireen_bot  = bit::crop<1>(bits, 6+8+8+1+1);
	}

	std::bitset<6> spl1;
	std::bitset<8> nmem_top;
	std::bitset<8> nmem_bot;
	std::bitset<1> nb_vertical;
	std::bitset<1> fireen_top;
	std::bitset<1> fireen_bot;

	size_t const neuron;
};

static std::array<int, 16> const iomap {{
	0, 1, 2, 3, 4, 5, -1, -1, 7, -1, 6, -1, -1, -1, -1, -1
}};


std::array<std::bitset<25>, 4> format(HICANN::NeuronQuad const& quad);

std::bitset<4> getIObits(HMF::Coordinate::Y const& y, HICANN::NeuronQuad const& quad);

HICANN::NeuronQuad randomize_quad();

void verify(HICANN::NeuronQuad const& quad,
			DenmemConfig const& config,
			bool strict = true);

void verify_reader(
	HICANN::NeuronQuad const& quad,
	std::array<std::bitset<25>, 4> const& data);

} // HMF
