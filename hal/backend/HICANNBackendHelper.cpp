#include <iostream>
#include <chrono>
#include <cmath>
#include "halco/hicann/v2/format_helper.h"
#include "hal/backend/HICANNBackendHelper.h"
#include "hal/HICANN/FGInstruction.h"
#include "halco/common/iter_all.h"

// TODO: ugly includes from hicann-system!
#include "fpga_control.h"          //FPGA control class
#include "dnc_control.h"           //DNC control class
#include "synapse_control.h"       //synapse control class
#include "l1switch_control.h"      //layer 1 switch control class
#include "neuron_control.h"        //neuron control class (merger, background genarators)
#include "neuronbuilder_control.h" //neuron builder control class
#include "fg_control.h"            //floating gate control
#include "spl1_control.h"          //spl1 control class
#include "dncif_control.h"         //DNC interface control

using namespace facets;
using namespace halco::hicann::v2;
using namespace halco::common;

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("halbe.backend.hicann");

namespace HMF {
namespace HICANN {

void set_decoder_double_row_impl(
    Handle::HICANNHw& h,
    SynapseController const& synapse_controller,
    halco::hicann::v2::SynapseDriverOnHICANN const& s,
    HMF::HICANN::DecoderDoubleRow const& data)
{
	using namespace facets;

	enum : size_t
	{
		TOP = 0,
		BOT = 1
	};                               // HARDWARE-coords!
	size_t SWTOP = TOP, SWBOT = BOT; // temporary variables for switching between hardware and
	                                 // software coords
	uint32_t addr[2];
	HicannCtrl::Synapse index;

	// calculate the correct hardware address of the line and choose the synapse
	// block instance
	if (s.line() < 112) { // upper half of ANNCORE
		addr[BOT] = 222 - (s.line() * 2);
		addr[TOP] = 222 - (s.line() * 2) + 1;
		index = HicannCtrl::SYNAPSE_TOP;
	} else { // lower half of ANNCORE
		addr[BOT] = (s.line() - 112) * 2;
		addr[TOP] = (s.line() - 112) * 2 + 1;
		SWTOP = BOT;
		SWBOT = TOP;
		index = HicannCtrl::SYNAPSE_BOTTOM;
	}

	ReticleControl& reticle = *h.get_reticle();
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	// generate correctly formatted data for the hardware
	std::array<std::array<std::bitset<32>, 32>, 2> hwdata;
	hwdata[TOP] = top_to_decoder(data[SWTOP], data[SWBOT]);
	hwdata[BOT] = bot_to_decoder(data[SWTOP], data[SWBOT]);

	// write the data to hardware
	for (size_t ROW = TOP; ROW <= BOT; ROW++) { // loop over top/bottom rows. top = 0, bottom = 1
		for (size_t colset = 0; colset < 8; colset++) { // loop over columnsets
			for (size_t i = 0; i < 4; i++) {            // loop over single chunks in the columnset
				// write into buffer first
				sc.write_data(
				    static_cast<unsigned int>(facets::SynapseControl::sc_synin + i),
				    static_cast<unsigned int>(hwdata[ROW][8 * i + colset].to_ulong()));
			}

			uint32_t const flush_command =
			    facets::SynapseControl::sc_cmd_wdec | // put together a flush command
			                                          // for the
			                                          // controller
			    (addr[ROW] << facets::SynapseControl::sc_adr_p) |
			    (colset << facets::SynapseControl::sc_colset_p) |
			    (1 << facets::SynapseControl::sc_newcmd_p);

			// flush the buffer
			sc.write_data(SynapseControl::sc_ctrlreg, flush_command);
			wait_by_dummy(
			    h, s.toSynapseArrayOnHICANN(), synapse_controller.cnfg_reg,
			    synapse_controller.cycles_synarray(SynapseControllerCmd::WDEC));
		}
	}

	// restore initial state
	set_syn_ctrl(h, s.toSynapseArrayOnHICANN(), synapse_controller.ctrl_reg);
}

void set_weights_row_impl(
    Handle::HICANNHw& h,
    SynapseController const& synapse_controller,
    halco::hicann::v2::SynapseRowOnHICANN const& s,
    HMF::HICANN::WeightRow const& weights)
{
	using namespace facets;

	// generate correctly formatted data for the hardware
	std::array<std::bitset<32>, 32> hwdata;

	for (size_t i = 0; i < 32; i++)
		hwdata[i] = bit::concat(
		    weights[8 * i + 0].format(), weights[8 * i + 1].format(), weights[8 * i + 2].format(),
		    weights[8 * i + 3].format(), weights[8 * i + 4].format(), weights[8 * i + 5].format(),
		    weights[8 * i + 6].format(), weights[8 * i + 7].format());

	// calculate the correct hardware address of the line and choose the synapse
	// block instance
	uint32_t addr = 0;
	HicannCtrl::Synapse index;
	const halco::hicann::v2::SynapseDriverOnHICANN drv = s.toSynapseDriverOnHICANN();

	if (drv.line() < 112) { // upper half of ANNCORE
		addr = 223 - (drv.line() * 2) - (s.toRowOnSynapseDriver() == halco::common::top ? 0 : 1);
		index = HicannCtrl::SYNAPSE_TOP;
	} else { // lower half of ANNCORE
		addr = (drv.line() - 112) * 2 +
		       (s.toRowOnSynapseDriver() == halco::common::top ? 0 : 1); /// top/bottom here is
		                                                                   /// geometrical, not
		                                                                   /// hardware!
		index = HicannCtrl::SYNAPSE_BOTTOM;
	}

	ReticleControl& reticle = *h.get_reticle();
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	// write the data to hardware: columnset-wise
	for (size_t colset = 0; colset < 8; colset++) {
		for (size_t i = 0; i < 4; i++) // single chunks in the columnset
			sc.write_data(
			    static_cast<unsigned int>(facets::SynapseControl::sc_synin + i),
			    static_cast<unsigned int>(hwdata[8 * i + colset].to_ulong()));

		// put together a flush command for the controller
		uint32_t flush_command = facets::SynapseControl::sc_cmd_write |
		                         (addr << facets::SynapseControl::sc_adr_p) |
		                         (colset << facets::SynapseControl::sc_colset_p) |
		                         (1 << facets::SynapseControl::sc_newcmd_p);
		sc.write_data(SynapseControl::sc_ctrlreg, flush_command);
		wait_by_dummy(
		    h, s.toSynapseArrayOnHICANN(), synapse_controller.cnfg_reg,
		    synapse_controller.cycles_synarray(SynapseControllerCmd::WRITE));
	}

	// restore initial state
	set_syn_ctrl(h, s.toSynapseArrayOnHICANN(), synapse_controller.ctrl_reg);
}

void wait_by_dummy(
	Handle::HICANN& h,
	halco::hicann::v2::SynapseArrayOnHICANN const& synarray,
	HICANN::SynapseConfigurationRegister const& cnfg_reg,
	size_t num_cycles)
{
	/*
	 * At a PLL HICANN frequency of 100 Mhz, the minimum number of HICANN clock cycles
	 * that pass between packets sent back to back from the FPGA to the HICANN is 2.
	 * This is a worst case estimation as the minimum PLL frequency allowed to be
	 * configured by software is 100 Mhz.
	 */
	size_t const min_cycles_per_packet = 2;
	size_t const num_dummys = std::ceil(num_cycles / (float)min_cycles_per_packet);

	LOG4CXX_DEBUG(logger, short_format(h.coordinate()) << " " << synarray
	                      <<": Perform " << num_dummys << " dummy waits");

	for (size_t i = 0; i < num_dummys; ++i) {
		set_syn_cnfg(h, synarray, cnfg_reg);
	}
}

/** builds neuron builder configuration byte */
std::bitset<25> nbdata(
	bool const firet,
	bool const fireb,
	bool const vertical,
	std::bitset<8> const ntop,
	std::bitset<8> const nbot,
	L1Address const& spl1)
{
	typedef std::bitset<25> type;
	type rval(0);
	std::bitset<6> spl1_(spl1.value());
	rval.set(2*n_numd+spl1_numd+2, fireb);
	rval.set(2*n_numd+spl1_numd+1, firet);
	rval.set(2*n_numd+spl1_numd, vertical);
	rval |= bit::convert<type>(nbot) << n_numd+spl1_numd;
	rval |= bit::convert<type>(ntop) << spl1_numd;
	rval |= bit::convert<type>(bit::reverse(spl1_));
	return rval;
}

size_t denmem_iomap(std::bitset<4> const inout)
{
	size_t const num = inout.to_ulong();
	if (num>5 && num != 8 && num != 10)
		throw std::runtime_error("invalid neuron builder I/O configuration");

	switch(inout.to_ulong()) {
		case 8:
			return 7;
		case 10:
			return 6;
		default:
			return inout.to_ulong();
	}
}

/**
 * format neurons bilder bits for a given neuron
 *
 * @note the top right neuron requires special attention
 */
std::bitset<25> denmen_quad_formatter(
	NeuronOnQuad const& neuron,
	NeuronQuad const& nquad)
{
	std::bitset<8> ntop(0), nbot(0);
	if (neuron == NeuronOnQuad(X(1), Y(0))) {
		for (int ii = 0; ii < 2; ++ii)
		{
			std::bitset<8>& n = (ii==0) ? ntop : nbot;

			std::bitset<4> inout(0);
			inout[0] = nquad[NeuronOnQuad(X(1), Y(ii))].enable_aout();
			inout[1] = nquad[NeuronOnQuad(X(0), Y(ii))].enable_aout();
			inout[2] = nquad[NeuronOnQuad(X(1), Y(ii))].enable_current_input();
			inout[3] = nquad[NeuronOnQuad(X(0), Y(ii))].enable_current_input();

			n = denmem_iomap(inout);
			n.set(3, nquad.getHorizontalInterconnect(Y(ii)));
			n.set(4, nquad[NeuronOnQuad(X(0), Y(ii))].activate_firing());
			n.set(5, nquad[NeuronOnQuad(X(0), Y(ii))].enable_fire_input());
			n.set(6, nquad[NeuronOnQuad(X(1), Y(ii))].enable_fire_input());
			n.set(7, nquad[NeuronOnQuad(X(1), Y(ii))].activate_firing());
		}
	}

	bool const fire = nquad[neuron].enable_spl1_output();
	bool const bottom = neuron.y();
	bool const vert_connect =
		(bottom) ?  nquad.getVerticalInterconnect(neuron.x()) : false;

	return nbdata(
		bottom ? false : fire,
		bottom ? fire : false,
		vert_connect,
		ntop, nbot, // 0s for all neurons but top right
		nquad[neuron].address());
}

void denmem_quad_reader(
	std::bitset<25> const& data,
	NeuronOnQuad const& n,
	NeuronQuad& quad)
{
	static std::array<uint8_t, 8> const iomap = {{ 0, 1, 2, 3, 4, 5, 10, 8 }};

	Neuron& neuron = quad[n];
	bool const bottom = n.y();

	// bit formatting
	neuron.address(L1Address(bit::reverse(bit::crop<6>(data)).to_ulong()));
	neuron.enable_spl1_output(data[bottom ? 24 : 23]);

	if (bottom)
		quad.setVerticalInterconnect(n.x(), data[22]);

	// if neuron is top right neuron
	if (n == NeuronOnQuad(X(1), Y(0))) {
		for (size_t jj = 0; jj<2; ++jj)
		{
			std::bitset<8> const nmem = bit::crop<8>(data, 6 + jj*8);

			quad.setHorizontalInterconnect(Y(jj), nmem[3]);

			quad[NeuronOnQuad(X(0), Y(jj))].activate_firing(nmem[4]);
			quad[NeuronOnQuad(X(0), Y(jj))].enable_fire_input(nmem[5]);

			quad[NeuronOnQuad(X(1), Y(jj))].activate_firing(nmem[7]);
			quad[NeuronOnQuad(X(1), Y(jj))].enable_fire_input(nmem[6]);

			std::bitset<4> const inout = iomap.at(bit::crop<3>(nmem).to_ulong());

			quad[NeuronOnQuad(X(0), Y(jj))].enable_aout(inout[1]);
			quad[NeuronOnQuad(X(0), Y(jj))].enable_current_input(inout[3]);

			quad[NeuronOnQuad(X(1), Y(jj))].enable_aout(inout[0]);
			quad[NeuronOnQuad(X(1), Y(jj))].enable_current_input(inout[2]);
		}
	}
}

/** returns a line of top decoders to be written to hardware. already includes reverting the bits */
std::array<std::bitset<32>, 32>
top_to_decoder(
	std::array<SynapseDecoder, 256> const& top,
	std::array<SynapseDecoder, 256> const& bot)
{
	std::array<std::bitset<32>, 32> returnvalue;

	for (size_t i = 0; i < 32; i++){ //single uints to write to HW
		for (size_t j = 0; j < 8; j++){ //single 4-bit chunks
			std::bitset<4> ttop(top[8*i+j]);
			std::bitset<4> tbot(bot[8*i+j]);
			returnvalue[i][31-4*j]     = tbot[3];
			returnvalue[i][31-(4*j+1)] = ttop[2];
			returnvalue[i][31-(4*j+2)] = tbot[2];
			returnvalue[i][31-(4*j+3)] = ttop[3];
		}
	}

	return returnvalue;
}

/** returns a line of bottom decoders to be written to hardware. already includes reverting the bits */
std::array<std::bitset<32>, 32>
bot_to_decoder(
	std::array<SynapseDecoder, 256> const& top,
	std::array<SynapseDecoder, 256> const& bot)
{
	std::array<std::bitset<32>, 32> returnvalue;

	for (size_t i = 0; i < 32; i++){ //single uints to write to HW
		for (size_t j = 0; j < 8; j++){ //single 4-bit chunks
			std::bitset<4> ttop(top[8*i+j]);
			std::bitset<4> tbot(bot[8*i+j]);
			returnvalue[i][31-4*j]     = tbot[1];
			returnvalue[i][31-(4*j+1)] = ttop[0];
			returnvalue[i][31-(4*j+2)] = tbot[0];
			returnvalue[i][31-(4*j+3)] = ttop[1];
		}
	}

	return returnvalue;
}

/** decodes and returns a line of 256 top decoder values read from the hardware */
std::array<SynapseDecoder, 256>
top_from_decoder(
	std::array<std::bitset<32>, 32> const& top,
	std::array<std::bitset<32>, 32> const& bot)
{
	std::array<SynapseDecoder, 256> returnvalue;

	for (size_t i = 0; i < 256; i++){
		std::bitset<4> t;
		t[0] = bot[i/8][31-(4*(i%8)+1)];
		t[1] = bot[i/8][31-(4*(i%8)+3)];
		t[2] = top[i/8][31-(4*(i%8)+1)];
		t[3] = top[i/8][31-(4*(i%8)+3)];
		returnvalue[i] = SynapseDecoder(t.to_ulong());
	}

	return returnvalue;
}

/** decodes and returns a line of 256 bottom decoder values read from the hardware */
std::array<SynapseDecoder, 256>
bot_from_decoder(
	std::array<std::bitset<32>, 32> const& top,
	std::array<std::bitset<32>, 32> const& bot)
{
	std::array<SynapseDecoder, 256> returnvalue;

	for (size_t i = 0; i < 256; i++){
		std::bitset<4> t;
		t[0] = bot[i/8][31-(4*(i%8)+2)];
		t[1] = bot[i/8][31-4*(i%8)];
		t[2] = top[i/8][31-(4*(i%8)+2)];
		t[3] = top[i/8][31-4*(i%8)];
		returnvalue[i] = SynapseDecoder(t.to_ulong());
	}

	return returnvalue;
}

/** encodes 4x2 MSBs of the synapse decoders to be written to hardware */
std::array<std::bitset<16>, 2>
encode_preouts(
   std::bitset<2> const p0 /*bottom left*/,
   std::bitset<2> const p1 /*top left*/,
   std::bitset<2> const p2 /*bottom right*/,
   std::bitset<2> const p3 /*top right*/)
{
	std::array<std::bitset<16>, 2> returnvalue;

	enum { TOP = 0, BOT = 1 }; //HARDWARE-top/bottom

	//px[0] = na[4]
	//px[1] = na[5]

	//returnvalue[TOP][0] = gen[0] // p[1][0]
	//returnvalue[TOP][1] = gen[1] // p[1][1]
	//returnvalue[TOP][2] = gen[1] // p[1][2]
	//returnvalue[TOP][3] = gen[0] // p[1][3]

	//returnvalue[TOP][4] = gen[0]  // p[3][0]
	//returnvalue[TOP][5] = gen[1]; // p[3][1]
	//returnvalue[TOP][6] = gen[1]; // p[3][2]
	//returnvalue[TOP][7] = gen[0]; // p[3][3]

	returnvalue[BOT][0] = p0[0]; // p[0][0]
	returnvalue[BOT][1] = p1[1]; // p[0][1]
	returnvalue[BOT][2] = p0[1]; // p[0][2]
	returnvalue[BOT][3] = p1[0]; // p[0][3]

	returnvalue[BOT][4] = p2[0]; // p[2][0]
	returnvalue[BOT][5] = p3[1]; // p[2][1]
	returnvalue[BOT][6] = p2[1]; // p[2][2]
	returnvalue[BOT][7] = p3[0]; // p[2][3]

	return returnvalue;
}

/** decodes 2x2x2 MSBs of the synapse decoders read from the hardware */
std::array<std::array<std::bitset<2>, 2>, 2>
decode_preouts(
	std::bitset<16> const dbot,
	std::bitset<16> const dtop)
{
	static_cast<void>(dtop); // see issue #1331, SS will fix later

	std::array<std::array<std::bitset<2>, 2>, 2> returnvalue;

	enum { TOP = 0, BOT = 1 }; //HARDWARE-TOP/BOTTOM
	enum { LEFT = 0, RIGHT = 1 };

	returnvalue[BOT][LEFT][0]  = dbot[0]; //bottom left HW
	returnvalue[BOT][LEFT][1]  = dbot[2];
	returnvalue[TOP][LEFT][0]  = dbot[3]; //top left HW
	returnvalue[TOP][LEFT][1]  = dbot[1];

	returnvalue[BOT][RIGHT][0] = dbot[4]; //bottom right HW
	returnvalue[BOT][RIGHT][1] = dbot[6];
	returnvalue[TOP][RIGHT][0] = dbot[7]; //top right HW
	returnvalue[TOP][RIGHT][1] = dbot[5];

	return returnvalue;
}

/** transforms coordinate to the physical address of the repeater */
ci_addr_t to_repaddr(VLineOnHICANN const x)
{
	std::bitset<6> addr; //to avoid overflows

	if (x % 2 && x < 128)        ///top left repeater block: considering line swapping
		addr = (x+1)/2;          //1->1, 3->2, 5->3, ... , 123->62, 125->63, 127->0
	else if (!(x%2) && x >= 128) /// top right repeater block: considering line swapping
		addr = (256-x)/2;        //128->0, 130->63, 132->62, ... , 250->3, 252->2, 254->1
	else if (!(x%2) && x < 128)  ///bottom left repeater block
		addr = x/2;              //0->0, 2->1, 4->2, ... , 122->61, 124->62, 126->63
	else if (x%2 && x >= 128)    ///bottom right repeater block
		addr = (255-x)/2;        //129->63, 131->62, 133->61, ... , 251->2, 253->1, 255->0
	else throw std::runtime_error("to_repaddr: Invalid VlineOnHICANN specified");

	return addr.to_ulong();
}

/** transforms coordinate to the address of a repeater block */
HicannCtrl::Repeater
to_repblock(VLineOnHICANN const x)
{
	HicannCtrl::Repeater index;

	if      (x % 2  && x < 128)  index = HicannCtrl::Repeater::REPEATER_TOP_LEFT;
	else if (!(x%2) && x >= 128) index = HicannCtrl::Repeater::REPEATER_TOP_RIGHT;
	else if (!(x%2) && x < 128)  index = HicannCtrl::Repeater::REPEATER_BOTTOM_LEFT;
	else if (x % 2  && x >= 128) index = HicannCtrl::Repeater::REPEATER_BOTTOM_RIGHT;
	else throw std::runtime_error("to_repblock: Invalid VlineOnHICANN specified");

	return index;
}

/** transforms coordinate to the physical address of the repeater */
ci_addr_t to_repaddr(HLineOnHICANN const y)
{
	std::bitset<5> addr; //to avoid overflows

	if (y % 2){ //odd line => center right block
		///considering the line swap in the middle of the chip and the address-swapping in
		///the hardware, following transformation is required
		addr = (y+1)/2; //1->1, 3->2 ... 59->30, 61->31, 63->0
	}
	else{ //even line => center left block
		addr = (62-y)/2; //0->31, 2->30, ... , 60->1, 62->0
	}

	return addr.to_ulong();
}

/** transforms coordinate to the address of a repeater block */
HicannCtrl::Repeater
to_repblock(HLineOnHICANN const y)
{
	HicannCtrl::Repeater index;

	if (y % 2) index = HicannCtrl::Repeater::REPEATER_CENTER_RIGHT;
	else       index = HicannCtrl::Repeater::REPEATER_CENTER_LEFT;

	return index;
}

/** transforms gray code to binary, needed for repeater test_input readout */
std::bitset<10>
gray_to_binary(std::bitset<10> const gray)
{
	std::bitset<10> binary;

	binary[9] = gray[9];
	for (int i = 8; i >= 0; i--)
		binary[i] = (gray[i] ^ binary[i+1]);

	return binary;
}

ci_data_t fg_read_answer(
	Handle::HICANNHw & h,
	FGBlockOnHICANN const& b)
{
	ReticleControl& reticle = *h.get_reticle();
	facets::FGControl& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	ci_data_t value;
	ci_addr_t addr;

	fc.read_data(facets::FGControl::REG_SLAVE);
	fc.get_read_data(addr, value);

	// sanity check (might be unecessary, but never trust the hardware)
	if (addr != facets::FGControl::REG_SLAVE)
	{
		throw std::runtime_error(
		    "fg_read_answer: fgcontrol address mismatch (REG_SLAVE) on " +
		    short_format(h.coordinate()));
	}

	return value;
}

FGErrorResultRow fg_log_error(
	Handle::HICANNHw & h,
	FGBlockOnHICANN const& b,
	int row,
	ci_data_t value)
{
	FGErrorResultRow controller_result;

	// Worst timing for a single row should be about 1.37s
	using namespace std::chrono;
	auto end_time = system_clock::now() + seconds(10);

	ReticleControl& reticle = *h.get_reticle();
	facets::FGControl& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	static log4cxx::LoggerPtr fglogger = log4cxx::Logger::getLogger("halbe.fgwriter");
	LOG4CXX_DEBUG(fglogger, "read error flags for block" << b);

	std::stringstream log;
	log << "block " << b << ": writing floating gate cells";

	if (row != -1)
		log << " row " << row;
	log << ": ";

	FGErrorResult data;

	// Read at most FGBlock::fg_columns times (cnt) from the controller
	size_t cnt;
	bool seen_last_column = false;
	for (cnt = 0; (cnt < FGBlock::fg_columns) && (!seen_last_column); cnt++) {
		// Special case "first value": The data has been provided by the caller => don't try to get it.
		if (cnt == 0) {
			// Now we acquire the error flags for the current row: The zeroth/first
			// column could be already marked (cf. parameter value).
			data = FGErrorResult{value};
		} else {
			// get next column addresses (loop will continue until error goes away or 129 is reached)
			fc.write_data(facets::FGControl::REG_ADDRINS, FGInstruction::getNextFalse());

			// Retry to get controller status if controller was busy (last read
			// contains invalid data otherwise)
			do {
				data = FGErrorResult{fg_read_answer(h, b)};
				LOG4CXX_TRACE(fglogger, "read value " << std::bitset<32>(data.get_slave_answer_data()));
			} while(data.get_busy_flag());
		}

		// break when seeing an invalid result
		if (!data.check())
			break;

		// break when reading a non-error entry (FG controller exited from error read-out state)
		if (!data.get_error_flag())
			break;

		// mark last loop execution if we get data from the last column
		if (data.get_cell() == FGBlock::fg_columns - 1)
			seen_last_column = true;

		log << data.get_cell() << std::endl;

		// FGErrorResultRow operator[] will check for a valid address (i.e. < FGBlock::fg_columns)
		FGErrorResult& cur = controller_result[data.get_cell()];
		if (cur.get_error_flag()) {
			LOG4CXX_ERROR(fglogger, "FG Cell " << data.get_cell()
			                                   << " has been already reported as errornous, FG "
			                                      "controller state machine seems to be fubar!");
			throw std::runtime_error(
			    "fg_log_error controller error on " + short_format(h.coordinate()));
		}

		// save returned data to return data structure
		cur = data;

		// FG controller read-out timeout
		if (system_clock::now() > end_time)
			throw std::runtime_error("fg_log_error timeout on " + short_format(h.coordinate()));
	}

	// if we had an error, print it
	if (cnt > 0)
		LOG4CXX_DEBUG(fglogger, log.str());

	return controller_result;
}

FGErrorResultRow fg_busy_wait(
	Handle::HICANNHw & h,
	FGBlockOnHICANN const& b,
	int row)
{
	using namespace std::chrono;
	bool busy;
	ci_data_t value;
	// Worst timing for a single row should be about 1.37s
	auto end_time = system_clock::now() + seconds(10);
	do {
		if (system_clock::now() > end_time)
			throw std::runtime_error("fg_busy_wait timeout on " + short_format(h.coordinate()));
		value = fg_read_answer(h, b);

		busy = FGErrorResult{value}.get_busy_flag();
	} while (busy);
	return fg_log_error(h, b, row, value);
}

FGErrorResultQuadRow fg_busy_wait(Handle::HICANNHw & h)
{
	FGErrorResultQuadRow result;
	for (auto const& fgb : iter_all<FGBlockOnHICANN>())
		result[fgb] = fg_busy_wait(h, fgb);
	return result;
}


void set_repeater_direction(
	HLineOnHICANN const x,
	HICANN::HorizontalRepeater const& rc,
	std::bitset<8>& data)
{
	if (x % 2) //odd lines are operated by right repeater block => inwards direction is to the left
		{
			data[4] = (rc.getLeft())  ? true : false;
			data[5] = (rc.getRight()) ? true : false;
		}
	else       //even lines are operated by left repeater block => inwards direction is to the right
		{
			data[4] = (rc.getRight()) ? true : false;
			data[5] = (rc.getLeft())  ? true : false;
		}
}

void set_repeater_direction(
	VLineOnHICANN const x,
	HICANN::VerticalRepeater const& rc,
	std::bitset<8>& data)
{
	if (x < 128) { //left HICANN side
		if (x % 2) //odd lines are operated by top repeater blocks => inwards direction down
			{
				data[4] = (rc.getBottom()) ? true : false;
				data[5] = (rc.getTop())    ? true : false;
			}
		else       //even lines are operated by bottom repeater block => inwards direction is up
			{
				data[4] = (rc.getTop())    ? true : false;
				data[5] = (rc.getBottom()) ? true : false;
			}
	}
	else { //right HICANN side
		if (x % 2) //odd lines are operated by bottom repeater blocks => inwards direction up
			{
				data[4] = (rc.getTop())    ? true : false;
				data[5] = (rc.getBottom()) ? true : false;
			}
		else       //even lines are operated by top repeater block => inwards direction is down
			{
				data[4] = (rc.getBottom()) ? true : false;
				data[5] = (rc.getTop())    ? true : false;
			}
	}
}

halco::common::SideHorizontal get_repeater_direction(
	HLineOnHICANN const x,
	std::bitset<8> const data)
{
	halco::common::SideHorizontal returnvalue;

	if (x % 2) {
		if (data.test(4)) returnvalue = left;  //leftwards
		if (data.test(5)) returnvalue = right; //rightwards
	}
	else {
		if (data.test(4)) returnvalue = right; //rightwards
		if (data.test(5)) returnvalue = left;  //leftwards
	}

	return returnvalue;
}

halco::common::SideVertical get_repeater_direction(
	VLineOnHICANN const x,
	std::bitset<8> const data)
{
	halco::common::SideVertical returnvalue;

	if (x < 128) { //left HICANN side
		if (x % 2) {
			if (data.test(4)) returnvalue = bottom; //downwards
			if (data.test(5)) returnvalue = top;    //upwards
		}
		else {
			if (data.test(4)) returnvalue = top;    //upwards
			if (data.test(5)) returnvalue = bottom; //downwards
		}
	}
	else { //right HICANN side
		if (x % 2) {
			if (data.test(5)) returnvalue = bottom; //downwards
			if (data.test(4)) returnvalue = top;    //upwards
		}
		else {
			if (data.test(5)) returnvalue = top;    //upwards
			if (data.test(4)) returnvalue = bottom; //downwards
		}	}

	return returnvalue;
}


void repeater_config_formater(
	HICANN::RepeaterBlock const& rbc,
	std::bitset<8> & config)
{
	config[0] = rbc.start_tdo[TestPortOnRepeaterBlock(0)];
	config[1] = rbc.start_tdo[TestPortOnRepeaterBlock(1)];
	config[2] = rbc.start_tdi[TestPortOnRepeaterBlock(0)];
	config[3] = rbc.start_tdi[TestPortOnRepeaterBlock(1)];

	config[4] = rbc.dllresetb;
	config[5] = rbc.drvresetb;
	config[6] = rbc.fextcap[0];
	config[7] = rbc.fextcap[1];
}

size_t translate_dnc_merger(size_t const merger)
{
	assert(merger<8);
	return 7-merger;
}

size_t translate_neuron_merger(size_t const merger)
{
	assert(merger<15);
	static std::array<size_t, 15> const _lut = {{
		 7,  6,  5,  4,  3,  2,  1,  0, // background merger
		11, 10,  9,  8, 13, 12, 14      // other neuron merger
	}};
	return _lut[merger];
}

uint8_t SynapseRowOnArray_to_AddrOnHW(halco::hicann::v2::SynapseRowOnArray const& row,
                                      halco::hicann::v2::SynapseArrayOnHICANN const& synarray)
{
	return synarray.isTop() ? halco::hicann::v2::SynapseRowOnArray::max - row : row;
}

halco::hicann::v2::SynapseRowOnArray AddrOnHW_to_SynapseRowOnArray(
    uint8_t const& addr, halco::hicann::v2::SynapseArrayOnHICANN const& synarray)
{
	return halco::hicann::v2::SynapseRowOnArray(synarray.isTop() ?
		halco::hicann::v2::SynapseRowOnArray::max - addr : addr);
}

void synapse_ctrl_formater(HICANN::SynapseControlRegister const& reg,
                           halco::hicann::v2::SynapseArrayOnHICANN const& synarray,
                           std::bitset<32>& returnvalue)
{
	// bit 31: reserverd
	returnvalue[30] = static_cast<bool>(reg.idle) ? *reg.idle : false;

	returnvalue[29] = reg.sca;
	returnvalue[28] = reg.scc;
	returnvalue[27] = reg.without_reset;

	returnvalue[26] = reg.sel.to_bitset<3>()[2];
	returnvalue[25] = reg.sel.to_bitset<3>()[1];
	returnvalue[24] = reg.sel.to_bitset<3>()[0];

	std::bitset<8> const last_addr = SynapseRowOnArray_to_AddrOnHW(reg.last_row, synarray);
	returnvalue[23] = last_addr[7];
	returnvalue[22] = last_addr[6];
	returnvalue[21] = last_addr[5];
	returnvalue[20] = last_addr[4];
	returnvalue[19] = last_addr[3];
	returnvalue[18] = last_addr[2];
	returnvalue[17] = last_addr[1];
	returnvalue[16] = last_addr[0];

	std::bitset<8> const addr = SynapseRowOnArray_to_AddrOnHW(reg.row, synarray);
	returnvalue[15] = addr[7];
	returnvalue[14] = addr[6];
	returnvalue[13] = addr[5];
	returnvalue[12] = addr[4];
	returnvalue[11] = addr[3];
	returnvalue[10] = addr[2];
	returnvalue[9] = addr[1];
	returnvalue[8] = addr[0];

	// bit 7: reserved

	returnvalue[6] = reg.newcmd;
	returnvalue[5] = reg.continuous;
	returnvalue[4] = reg.encr;

	returnvalue[3] = std::bitset<4>(static_cast<int>(reg.cmd))[3];
	returnvalue[2] = std::bitset<4>(static_cast<int>(reg.cmd))[2];
	returnvalue[1] = std::bitset<4>(static_cast<int>(reg.cmd))[1];
	returnvalue[0] = std::bitset<4>(static_cast<int>(reg.cmd))[0];
}

void synapse_cnfg_formater(
	HICANN::SynapseConfigurationRegister const& reg, std::bitset<32>& returnvalue)
{
	// bits 31-28: reserved

	returnvalue[27] = reg.synarray_timings.write_delay.to_bitset<2>()[1];
	returnvalue[26] = reg.synarray_timings.write_delay.to_bitset<2>()[0];

	returnvalue[25] = reg.synarray_timings.output_delay.to_bitset<4>()[3];
	returnvalue[24] = reg.synarray_timings.output_delay.to_bitset<4>()[2];
	returnvalue[23] = reg.synarray_timings.output_delay.to_bitset<4>()[1];
	returnvalue[22] = reg.synarray_timings.output_delay.to_bitset<4>()[0];

	returnvalue[21] = reg.dllresetb.to_bitset<2>()[1];
	returnvalue[20] = reg.dllresetb.to_bitset<2>()[0];

	returnvalue[19] = reg.gen.to_bitset<4>()[3];
	returnvalue[18] = reg.gen.to_bitset<4>()[2];
	returnvalue[17] = reg.gen.to_bitset<4>()[1];
	returnvalue[16] = reg.gen.to_bitset<4>()[0];

	returnvalue[15] = reg.synarray_timings.setup_precharge.to_bitset<4>()[3];
	returnvalue[14] = reg.synarray_timings.setup_precharge.to_bitset<4>()[2];
	returnvalue[13] = reg.synarray_timings.setup_precharge.to_bitset<4>()[1];
	returnvalue[12] = reg.synarray_timings.setup_precharge.to_bitset<4>()[0];

	returnvalue[11] = reg.synarray_timings.enable_delay.to_bitset<4>()[3];
	returnvalue[10] = reg.synarray_timings.enable_delay.to_bitset<4>()[2];
	returnvalue[9] = reg.synarray_timings.enable_delay.to_bitset<4>()[1];
	returnvalue[8] = reg.synarray_timings.enable_delay.to_bitset<4>()[0];

	returnvalue[7] = reg.pattern0.cc;
	returnvalue[6] = reg.pattern1.cc;
	returnvalue[5] = reg.pattern0.ca;
	returnvalue[4] = reg.pattern1.ca;
	returnvalue[3] = reg.pattern0.ac;
	returnvalue[2] = reg.pattern1.ac;
	returnvalue[1] = reg.pattern0.aa;
	returnvalue[0] = reg.pattern1.aa;
}

void synapse_set_lut_low(HICANN::STDPLUT::LUT& lut, std::bitset<32> const& pattern)
{
	lut[SynapseWeight(0)]  =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 7 * 4)).to_ulong());
	lut[SynapseWeight(8)]  =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 6 * 4)).to_ulong());
	lut[SynapseWeight(4)]  =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 5 * 4)).to_ulong());
	lut[SynapseWeight(12)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 4 * 4)).to_ulong());
	lut[SynapseWeight(2)]  =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 3 * 4)).to_ulong());
	lut[SynapseWeight(10)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 2 * 4)).to_ulong());
	lut[SynapseWeight(6)]  =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 1 * 4)).to_ulong());
	lut[SynapseWeight(14)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 0 * 4)).to_ulong());
}


std::bitset<32> synapse_get_lut_low(HICANN::STDPLUT::LUT const& lut)
{
	return bit::concat(
		bit::reverse(std::bitset<4>(lut[SynapseWeight(0)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(8)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(4)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(12)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(2)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(10)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(6)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(14)])));
}

void synapse_set_lut_high(HICANN::STDPLUT::LUT& lut, std::bitset<32> const& pattern)
{
	lut[SynapseWeight(0  + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 7 * 4)).to_ulong());
	lut[SynapseWeight(8  + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 6 * 4)).to_ulong());
	lut[SynapseWeight(4  + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 5 * 4)).to_ulong());
	lut[SynapseWeight(12 + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 4 * 4)).to_ulong());
	lut[SynapseWeight(2  + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 3 * 4)).to_ulong());
	lut[SynapseWeight(10 + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 2 * 4)).to_ulong());
	lut[SynapseWeight(6  + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 1 * 4)).to_ulong());
	lut[SynapseWeight(14 + 1)] =
		SynapseWeight(bit::reverse(bit::crop<4>(pattern, 0 * 4)).to_ulong());
}

std::bitset<32> synapse_get_lut_high(HICANN::STDPLUT::LUT const& lut)
{
	return bit::concat(
		bit::reverse(std::bitset<4>(lut[SynapseWeight(0  + 1)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(8  + 1)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(4  + 1)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(12 + 1)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(2  + 1)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(10 + 1)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(6  + 1)])),
		bit::reverse(std::bitset<4>(lut[SynapseWeight(14 + 1)])));
}

void set_PLL_multiplier(
	Handle::HICANNHw & h,
	rant::integral_range<uint32_t, 10, 1> const divider,
	rant::integral_range<uint32_t, 63, 1> const multiplier)
{
	ReticleControl& reticle = *h.get_reticle();

	const double freq = 50.0 * static_cast<double>(multiplier) /
		static_cast<double>(divider);  // Frequency in Mhz
	uint32_t const pdn = 0x1, tst = 0x0;
	// Note (CK): frange=1 means only output between 100-300Mhz, see datasheet
	// http://freelibrary.faraday-tech.com/AIP/FXPLL031HA0A_APGD_Datasheet.pdf
	uint32_t const frange = (freq < 100.0 ? 0x0 : 0x1);

	if (freq < 100.0 || freq > 250.0)
	{
		std::stringstream err;
		err << "Invalid PLL frequency of " << freq << "MHz on " + short_format(h.coordinate());
		throw std::runtime_error(err.str());
	}

	LOG4CXX_DEBUG(logger, "Setting PLL to " << freq << "MHz (50.0 *"
			<< static_cast<double>(multiplier) << "/"
			<< static_cast<double>(divider) << ")");

	reticle.jtag->set_hicann_pos(h.jtag_addr());
	reticle.jtag->HICANN_set_pll_far_ctrl(divider, multiplier, pdn, frange, tst);
	reticle.jtag->set_hicann_pos(0);
}

void hicann_init(facets::HicannCtrl& hc, bool const zero_synapses)
{
	// SRAM controller timing (used for all controllers)
	size_t const read_delay      = 64;
	size_t const setup_precharge = 8;
	size_t const write_delay     = 8;

	hc.getSPL1Control().write_reset();

	// reset all important RAMs to zero
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_TOP_LEFT).reset();
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_TOP_RIGHT).reset();
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_CENTER_LEFT).reset();
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_CENTER_RIGHT).reset();
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_BOTTOM_LEFT).reset();
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_BOTTOM_RIGHT).reset();

	// get repeater controls
	RepeaterControl& rc_tl = hc.getRC(HicannCtrl::Repeater::REPEATER_TOP_LEFT);
	RepeaterControl& rc_tr = hc.getRC(HicannCtrl::Repeater::REPEATER_TOP_RIGHT);
	RepeaterControl& rc_cl = hc.getRC(HicannCtrl::Repeater::REPEATER_CENTER_LEFT);
	RepeaterControl& rc_cr = hc.getRC(HicannCtrl::Repeater::REPEATER_CENTER_RIGHT);
	RepeaterControl& rc_bl = hc.getRC(HicannCtrl::Repeater::REPEATER_BOTTOM_LEFT);
	RepeaterControl& rc_br = hc.getRC(HicannCtrl::Repeater::REPEATER_BOTTOM_RIGHT);

	// set sram timings
	rc_tl.set_sram_timings(read_delay, setup_precharge, write_delay);
	rc_tr.set_sram_timings(read_delay, setup_precharge, write_delay);
	rc_cl.set_sram_timings(read_delay, setup_precharge, write_delay);
	rc_cr.set_sram_timings(read_delay, setup_precharge, write_delay);
	rc_bl.set_sram_timings(read_delay, setup_precharge, write_delay);
	rc_br.set_sram_timings(read_delay, setup_precharge, write_delay);

	// after design reset: needed for L1 locking! otherwise: L1 drops spikes
	rc_tl.reset();
	rc_tr.reset();
	rc_cl.reset();
	rc_cr.reset();
	rc_bl.reset();
	rc_br.reset();

	// set sram timings
	hc.getNBC().set_sram_timings(read_delay, setup_precharge, write_delay);
	hc.getNBC().reset();   // zeroes all neuron builder srams

	hc.getNC().nc_reset(); // zeroes all neuron control srams

	// get synapse controls
	SynapseControl& sc_t = hc.getSC(HicannCtrl::Synapse::SYNAPSE_TOP);
	SynapseControl& sc_b = hc.getSC(HicannCtrl::Synapse::SYNAPSE_BOTTOM);

	// set sram timings
	sc_t.set_sram_timings(read_delay, setup_precharge, write_delay);
	sc_b.set_sram_timings(read_delay, setup_precharge, write_delay);

	// reset
	sc_t.reset_drivers();
	sc_b.reset_drivers();

	if (zero_synapses) {
		sc_t.reset_weights();
		sc_b.reset_weights();
		sc_t.reset_decoders();
		sc_b.reset_decoders();
	}

	// No FG controller resets needed:
	// cf. https://brainscales-r.kip.uni-heidelberg.de:6443/visions/pl/tn8ej8kibjbf8n4b8ypfu7sn5e

}

} // HICANN
} // HMF
