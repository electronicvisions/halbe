#pragma once

#include <functional>

#include <bitter/integral.h>
#include <bitter/util.h>
#include <bitter/bitset.h>

#include "hal/macro_HALbe.h"

// handles and runtime
#include "hal/Handle/HICANNHw.h"
#include "hal/Handle/HMFRun.h"

#include "hal/backend/HICANNBackend.h"
#include "hal/HICANN/FGErrorResult.h"

#include "reticle_control.h"
#include "repeater_control.h"      //repeater control class
#include "hicann_ctrl.h"           //HICANN control class
#include "dnc_control.h"

namespace HMF {
namespace HICANN {

struct sc_write_data
{
	unsigned int index;
	enum access_type
	{
		WRITE,
		WRITEANDWAIT
	} type;
	unsigned int addr, data;
};

typedef std::vector<sc_write_data> sc_write_data_queue_t;

void set_decoder_double_row_impl(
	HMF::Coordinate::SynapseDriverOnHICANN const& s, HMF::HICANN::DecoderDoubleRow const& data,
	std::function<void(sc_write_data const&)> callback);

void set_weights_row_impl(
	HMF::Coordinate::SynapseRowOnHICANN const& s, HMF::HICANN::WeightRow const& weights,
	std::function<void(sc_write_data const&)> callback);

bool popexec_sc_write_data_queue(
    HMF::Handle::HICANNHw& h, size_t& idx, sc_write_data_queue_t const& data);

/** builds neuron builder configuration byte */
std::bitset<25> nbdata(
	bool const firet,
	bool const fireb,
	bool const vertical,
	std::bitset<8> const ntop,
	std::bitset<8> const nbot,
	L1Address const& spl1);

size_t denmem_iomap(std::bitset<4> const inout);

/**
 * format neurons builder bits for a given neuron
 *
 * @note the top right neuron requires special attention
 */
std::bitset<25> denmen_quad_formatter(
	Coordinate::NeuronOnQuad const& neuron,
	NeuronQuad const& nquad);

void denmem_quad_reader(
	std::bitset<25> const& data,
	Coordinate::NeuronOnQuad const& n,
	NeuronQuad& quad);

/** returns a line of top decoders to be written to hardware. already includes reverting the bits */
std::array<std::bitset<32>, 32>
top_to_decoder(
	std::array<SynapseDecoder, 256> const& top,
	std::array<SynapseDecoder, 256> const& bot);

/** returns a line of bottom decoders to be written to hardware. already includes reverting the bits */
std::array<std::bitset<32>, 32>
bot_to_decoder(
	std::array<SynapseDecoder, 256> const& top,
	std::array<SynapseDecoder, 256> const& bot);

/** decodes and returns a line of 256 top decoder values read from the hardware */
std::array<SynapseDecoder, 256>
top_from_decoder(
	std::array<std::bitset<32>, 32> const& top,
	std::array<std::bitset<32>, 32> const& bot);

/** decodes and returns a line of 256 bottom decoder values read from the hardware */
std::array<SynapseDecoder, 256>
bot_from_decoder(
	std::array<std::bitset<32>, 32> const& top,
	std::array<std::bitset<32>, 32> const& bot);

/** encodes 4x2 MSBs of the synapse decoders to be written to hardware */
std::array<std::bitset<16>, 2>
encode_preouts(
	std::bitset<2> const p0,
	std::bitset<2> const p1,
	std::bitset<2> const p2,
	std::bitset<2> const p3);

/** decodes 2x2x2 MSBs of the synapse decoders read from the hardware */
std::array<std::array<std::bitset<2>, 2>, 2>
decode_preouts(
	std::bitset<16> const bot,
	std::bitset<16> const top);

/** transforms coordinate to the physical address of the repeater */
facets::ci_addr_t to_repaddr(Coordinate::VLineOnHICANN const x);

/** transforms coordinate to the physical address of the repeater */
facets::ci_addr_t to_repaddr(Coordinate::HLineOnHICANN const y);

/** transforms coordinate to the address of a repeater block */
facets::HicannCtrl::Repeater
to_repblock(Coordinate::VLineOnHICANN const x);

/** transforms coordinate to the address of a repeater block */
facets::HicannCtrl::Repeater
to_repblock(Coordinate::HLineOnHICANN const y);

/** transforms gray code to binary, needed for repeater test_input readout */
std::bitset<10> gray_to_binary(std::bitset<10> const gray);

facets::ci_data_t fg_read_answer(
	Handle::HICANNHw & h,
	Coordinate::FGBlockOnHICANN const& b);

FGErrorResultRow fg_log_error(
	Handle::HICANNHw & h,
	Coordinate::FGBlockOnHICANN const& b,
	int row,
	facets::ci_data_t value);


/**
 * Blocks until floating gate block is no longer busy
 *
 * @param h HICANN Handle
 * @param b FGBlock coordinate inside HICANN
 *
 * @returns error messages that were read out from
 *     the floating gate controller.
 */
FGErrorResultRow fg_busy_wait(
	Handle::HICANNHw & h,
	Coordinate::FGBlockOnHICANN const& b,
	int row = -1);

/**
 * Blocks until all floating gate blocks are no longer busy
 *
 * @param h HICANN Handle
 *
 * @returns error messages for each block that were read
 *     out from the floating gate controller.
 */
FGErrorResultQuadRow fg_busy_wait(Handle::HICANNHw & h);


/** builds up an instruction byte to be written to hardware */
uint32_t fg_instruction(
	FG_pkg::ControlInstruction instr,
	bool bank,
	uint8_t column,
	uint8_t line);

void set_repeater_direction(
	Coordinate::HLineOnHICANN const x,
	HICANN::HorizontalRepeater const& rc,
	std::bitset<8>& data);

void set_repeater_direction(
	Coordinate::VLineOnHICANN const x,
	HICANN::VerticalRepeater const& rc,
	std::bitset<8>& data);

template<typename Repeater, typename Wire>
void set_repeater_helper(
	Handle::HICANNHw const& h,
	Wire const x,
	Repeater const& rc,
	facets::HicannCtrl::Repeater const index,
	facets::ci_addr_t const addr);

geometry::SideHorizontal get_repeater_direction(
	Coordinate::HLineOnHICANN const x,
	std::bitset<8> const data);

geometry::SideVertical get_repeater_direction(
	Coordinate::VLineOnHICANN const x,
	std::bitset<8> const data);

void repeater_config_formater(
	HICANN::RepeaterBlock const& rbc,
	std::bitset<8> & data);

template<typename Repeater, typename Wire>
Repeater get_repeater_helper(
	Handle::HICANNHw const& h,
	Wire const x,
	facets::HicannCtrl::Repeater const index,
	facets::ci_addr_t const addr);

size_t translate_dnc_merger(size_t const merger);

size_t translate_neuron_merger(size_t const merger);

} // HICANN
} // HMF




// template implementations

namespace HMF {
namespace HICANN {

template<typename Repeater, typename Wire>
void set_repeater_helper(
	Handle::HICANNHw & h,
	Wire const x,
	Repeater const& rc,
	facets::HicannCtrl::Repeater const index,
	facets::ci_addr_t const addr)
{
	facets::ReticleControl& reticle = *h.get_reticle();

	std::bitset<8> data = 0; //configuration byte to be written to hardware

	set_repeater_direction(x, rc, data);

	//set the mode bits
	if (rc.getMode() == Repeater::Mode::LOOPBACK && rc.getActiveTransmitters() == 0) { data[7]=true; data[6] = true; }
	else if (rc.getMode() == Repeater::Mode::INPUT && rc.getActiveTransmitters() == 1) data[7]=true;  //enable TOUTNEN (!)
	else if (rc.getMode() == Repeater::Mode::INPUTONLY && rc.getActiveTransmitters() == 0) data[7]=true;  //enable TOUTNEN (!)
	else if (rc.getMode() == Repeater::Mode::OUTPUT && rc.getActiveTransmitters() != 0) data[6]=true; //enable TINTEN (!)
	else if (rc.getMode() == Repeater::Mode::FORWARDING && rc.getActiveTransmitters() == 1) ; //do nothing, receiver always on!
	else if (rc.getMode() == Repeater::Mode::IDLE && rc.getActiveTransmitters() == 0) ;       //do nothing
	else throw std::domain_error("Wrong mode and direction combination for this repeater");

	//set the crosstalk-cancellation bits
	data[3]=rc.getRen()[1];
	data[2]=rc.getRen()[0];
	data[1]=rc.getLen()[1];
	data[0]=rc.getLen()[0];

	reticle.hicann[h.jtag_addr()]->getRC(index).write_data(addr, data.to_ulong());
}


template<typename Repeater, typename Wire>
Repeater get_repeater_helper(
	Handle::HICANNHw & h,
	Wire const x,
	facets::HicannCtrl::Repeater const index,
	facets::ci_addr_t const addr)
{
	facets::ReticleControl& reticle = *h.get_reticle();

	//read data from hardware
	std::bitset<8> data = reticle.hicann[h.jtag_addr()]->getRC(index).read_data(addr);

	//fill the return structure
	Repeater returnvalue;
	VerticalRepeater vert; //compatibility reasons
	HorizontalRepeater hor;

	returnvalue.setLen(bit::crop<2>(data, 0));
	returnvalue.setRen(bit::crop<2>(data, 2));

	//decode mode bits
	if      (data[7] && data[6] && !data[5] && !data[4]) returnvalue.setLoopback();
	else if (data[7] && !data[4] && !data[5]) returnvalue.setInput();
	else if (data[7] && (data[5] xor data[4])) returnvalue.setInput(get_repeater_direction(x, data));
	else if (data[6] && (data[5] xor data[4])) returnvalue.setOutput(get_repeater_direction(x, data));
	else if (data[6] && data[5] && data[4]) { //set output in both directions here
		returnvalue.setOutput(get_repeater_direction(x, std::bitset<8>(0).set(5)));
		returnvalue.setOutput(get_repeater_direction(x, std::bitset<8>(0).set(4)));
	}
	else if (data[5] xor data[4]) returnvalue.setForwarding(get_repeater_direction(x, data));
	else if (!data[7] && !data[6] && !data[5] && !data[4]) returnvalue.setIdle();
	else throw std::domain_error("Wrong mode and direction combination read from the hardware");

	return returnvalue;
}


/**
 * Configures the multiplier of the HICANN PLL. The Resulting frequency will be
 * a multiple of 100MHz up to 250MHz. This controls the clock of all synchronous
 * components within the HICANN e.g. repeaters and floating gate controllers.
 * Note, that only L1 components will run at the native PLL clock speed. All
 * other components will use FREQ/4.
 *
 * @param divider Actually 6bit, but ust be smaller than 10, otherwise FREFX is
 *                to small. Should beas small as possible to reduce jitter
 * @param multiplier 6bit, Should be as small as possible to reduce jitter
 */
void set_PLL_multiplier(
	Handle::HICANNHw & h,
	rant::integral_range<uint32_t, 10, 1> const divider,
	rant::integral_range<uint32_t, 63, 1> const multiplier);

void hicann_init(facets::HicannCtrl& hc, facets::DNCControl& dc, bool const isKintex,
                 bool const zero_synapses);

} // HICANN
} // HMF
