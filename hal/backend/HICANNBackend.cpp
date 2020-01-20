#include "hal/backend/HICANNBackend.h"
#include "hal/backend/HICANNBackendHelper.h"

#include <bitter/bitter.h>

#include "hal/backend/FPGABackend.h"
#include "hal/HICANN/FGInstruction.h"
#include "halco/common/iter_all.h"
#include "halco/hicann/v2/format_helper.h"

// handles 
#include "hal/backend/dispatch.h"

// runtime
#include "hal/Handle/HMFRun.h"

// TODO: ugly includes from hicann-system!
#include "reticle_control.h"
#include "fpga_control.h"          //FPGA control class
#include "dnc_control.h"           //DNC control class
#include "hicann_ctrl.h"           //HICANN control class
#include "synapse_control.h"       //synapse control class
#include "l1switch_control.h"      //layer 1 switch control class
#include "repeater_control.h"      //repeater control class
#include "neuron_control.h"        //neuron control class (merger, background genarators)
#include "neuronbuilder_control.h" //neuron builder control class
#include "fg_control.h"            //floating gate control
#include "spl1_control.h"          //spl1 control class
#include "dncif_control.h"         //DNC interface control

using namespace facets;
using namespace bit;
using namespace halco::hicann::v2;
using namespace halco::common;

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("halbe.backend.hicann");

namespace HMF {
namespace HICANN {

/* macro usage:
 * HALBE_SETTER( function name, type1, variable name1, type2, variable name2, ... ) { }
 * HALBE_SETTER_GUARDED( configuration state event, function name, type1, variable name1, ... ) { }
 * HALBE_GETTER( return type, function name, type1, variable name1, type2, variable name2, ... ) { }
 */

HALBE_SETTER_GUARDED(EventSetupL1,
	set_crossbar_switch_row,
	Handle::HICANN & , h,
	HLineOnHICANN const&, y,
	Side const&, s,
	HICANN::CrossbarRow const & , switches)
{
	ReticleControl& reticle = *h.get_reticle();

	// HLine 0 is the upper horizontal lane (according to the left crossbar)
	// the permutation in the center of HICANN has no effect on the numbering inside one chip
	ci_data_t cfg  = 0;    //hardware-friendly data format
	ci_addr_t addr = 63-y; //calculate hardware address, does not depend on the side

	HicannCtrl::L1Switch index; //choose control instance
	index = (s == left) ? HicannCtrl::L1SWITCH_CENTER_LEFT : HicannCtrl::L1SWITCH_CENTER_RIGHT;

	for (size_t i = 0; i < 4; i++) {
		///swap the bits lowest<->highest for the right side because of the vertical lane numbering
		size_t ii = (s == right) ? 3-i : i;
		cfg = bit::set(cfg, ii, switches[i]);
	}

	reticle.hicann[h.jtag_addr()]->getLC(index).write_cfg(addr, cfg);
}


HALBE_GETTER(HICANN::CrossbarRow, get_crossbar_switch_row,
	Handle::HICANN &, h,
	HLineOnHICANN const&, y,
	Side const&, s)
{
	ReticleControl& reticle = *h.get_reticle();

	ci_data_t cfg  = 0;    //hardware-friendly data format
	ci_addr_t addr = 63-y; //calculate hardware address, does not depend on the side

	HicannCtrl::L1Switch index; //choose control instance
	index = (s == left) ? HicannCtrl::L1SWITCH_CENTER_LEFT : HicannCtrl::L1SWITCH_CENTER_RIGHT;

	reticle.hicann[h.jtag_addr()]->getLC(index).read_cfg(addr); //read data from hardware
	reticle.hicann[h.jtag_addr()]->getLC(index).get_read_cfg(addr, cfg);

	HICANN::CrossbarRow returnvalue;
	for (size_t i = 0; i < 4; i++) {
		///swap the bits lowest<->highest for the right side because of the vertical lane numbering
		size_t ii = (s == right) ? 3-i : i;
		returnvalue[i] = bit::test(cfg, ii);
	}
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_syndriver_switch_row,
	Handle::HICANN &, h,
	SynapseSwitchRowOnHICANN const&, s,
	SynapseSwitchRow const&, switches)
{
	ReticleControl& reticle = *h.get_reticle();

	ci_data_t cfg  = 0; //hardware-friendly data format
	ci_addr_t addr = 0; //hardware line address
	HicannCtrl::L1Switch index; //control instance index

	//calculate hardware line address and choose control instance
	if (s.line() < 112) { //top half
		addr = 111-s.line();
		index = (s.toSideHorizontal() == left) ? HicannCtrl::L1SWITCH_TOP_LEFT : HicannCtrl::L1SWITCH_TOP_RIGHT;
	}
	else { //bottom half
		addr = s.line()-112;
		index = (s.toSideHorizontal() == left) ? HicannCtrl::L1SWITCH_BOTTOM_LEFT : HicannCtrl::L1SWITCH_BOTTOM_RIGHT;
	}

	///note that HICANN-documentation is incorrect here: LOWEST bits correspond to HIGHEST vertical lines!
	///hence all swappings have to be done vice-versa: swap bits for left side, not for right
	for (size_t i = 0; i < 16; i++) {
		size_t ii = (s.toSideHorizontal() == left) ? 15-i : i;
		cfg = bit::set(cfg, i, switches[ii]); //build config byte
	}

	reticle.hicann[h.jtag_addr()]->getLC(index).write_cfg(addr, cfg);
}


HALBE_GETTER(SynapseSwitchRow, get_syndriver_switch_row,
	Handle::HICANN &, h,
	SynapseSwitchRowOnHICANN const&, s)
{
	ReticleControl& reticle = *h.get_reticle();

	ci_data_t cfg  = 0; //hardware-friendly data format
	ci_addr_t addr = 0; //hardware line address
	HicannCtrl::L1Switch index; //control instance index

	//calculate hardware line address and choose control instance
	if (s.line() < 112) { //top half
		addr = 111-s.line();
		index = (s.toSideHorizontal() == left) ? HicannCtrl::L1SWITCH_TOP_LEFT : HicannCtrl::L1SWITCH_TOP_RIGHT;
	}
	else { //bottom half
		addr = s.line()-112;
		index = (s.toSideHorizontal() == left) ? HicannCtrl::L1SWITCH_BOTTOM_LEFT : HicannCtrl::L1SWITCH_BOTTOM_RIGHT;
	}

	reticle.hicann[h.jtag_addr()]->getLC(index).read_cfg(addr); //read data from hardware
	reticle.hicann[h.jtag_addr()]->getLC(index).get_read_cfg(addr, cfg);

	SynapseSwitchRow returnvalue;
	for (size_t i = 0; i < 16; i++) {
		//for the left side flip the bits: lowest<->highest because of the double-swapping of the vertical lane numbering
		size_t ii = (s.toSideHorizontal() == left) ? 15-i : i;
		returnvalue[i] = bit::test(cfg, ii);
	}
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupSynapses,
	set_weights_row,
	Handle::HICANN &, h,
	SynapseRowOnHICANN const&, s,
	WeightRow const&, weights)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& hicann = reticle.hicann[h.jtag_addr()];

	set_weights_row_impl(s, weights, [&hicann](sc_write_data const& instr) {
			SynapseControl& sc = hicann->getSC(instr.index);
			sc.write_data(instr.addr, instr.data);
			if (instr.type == sc_write_data::WRITEANDWAIT) {
				// wait until controller not busy
				while(sc.arraybusy()) {}
			}
		});
}

HALBE_SETTER_GUARDED(EventSetupSynapses,
	set_weights_row,
	std::vector<boost::shared_ptr<Handle::HICANN> >, handles,
	halco::hicann::v2::SynapseRowOnHICANN const&, s,
	std::vector<WeightRow> const&, data)
{
	const size_t n_hicanns = handles.size();
	if (data.size() != n_hicanns)
		throw std::invalid_argument(
			"set_weights_row: number of handles and data does not match");

	// to buffer sc writes...
	std::vector<sc_write_data_queue_t> per_hicann_queues(n_hicanns);

	auto it_data = data.begin();
	for (auto& queue : per_hicann_queues) {
		set_weights_row_impl(s, *it_data, [&queue](sc_write_data const& instr) {
			queue.push_back(instr);
		});
		++it_data;
	}

	// per-hicann indices of progress within "write queues" (start at 0th entry)
	std::vector<size_t> idxs(n_hicanns, 0);

	// as long as a single synapse controller hasn't finished...
	for (bool all_done = false; !all_done;) {
		all_done = true;
		for (size_t i = 0; i < n_hicanns; ++i)
			if (popexec_sc_write_data_queue(
				    dynamic_cast<HMF::Handle::HICANNHw&>(*handles[i]), idxs[i],
				    per_hicann_queues[i]))
				all_done = false;
	}
}

HALBE_GETTER(WeightRow, get_weights_row,
	Handle::HICANN &, h,
	SynapseRowOnHICANN const&, s)
{
	ReticleControl& reticle = *h.get_reticle();

	//calculate the correct hardware address of the line and choose the synapse block instance
	uint32_t addr = 0;
	HicannCtrl::Synapse index;
	const SynapseDriverOnHICANN drv = s.toSynapseDriverOnHICANN();

	if (drv.line() < 112) { //upper half of ANNCORE
		addr = 223-(drv.line()*2) - (s.toRowOnSynapseDriver() == top ? 0 : 1);
		index = HicannCtrl::SYNAPSE_TOP;
	}
	else {  //lower half of ANNCORE
		addr = (drv.line()-112)*2 + (s.toRowOnSynapseDriver() == top ? 0 : 1); /// top/bottom here is geometrical, not hardware!
		index = HicannCtrl::SYNAPSE_BOTTOM;
	}

	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	//read the data from hardware: columnset-wise
	uint32_t open_row = facets::SynapseControl::sc_cmd_st_rd | //put together an "open row" command
				(1 << facets::SynapseControl::sc_newcmd_p) |
				(addr << facets::SynapseControl::sc_adr_p);
	uint32_t close_row = facets::SynapseControl::sc_cmd_close | //put together a "close row" command
				(1 << facets::SynapseControl::sc_newcmd_p) |
				(addr << facets::SynapseControl::sc_adr_p);

	WeightRow returnvalue;

	sc.write_data(facets::SynapseControl::sc_ctrlreg, open_row); //open row for reading
	while(sc.arraybusy()) {} //wait until controller not busy

	for (size_t colset = 0; colset < 8; colset++){
		uint32_t read_command = facets::SynapseControl::sc_cmd_read | //put together a read command
							(colset << facets::SynapseControl::sc_colset_p) |
							(1 << facets::SynapseControl::sc_newcmd_p) |
							(addr << facets::SynapseControl::sc_adr_p);

		sc.write_data(facets::SynapseControl::sc_ctrlreg, read_command); //issue read command
		while(sc.arraybusy()) {} //wait until not busy

		for (size_t i = 0; i < 4; i++){ //single chunks in the columnset
			std::bitset<32> sd = sc.read_data(facets::SynapseControl::sc_synout+i);
			sd = bit::reverse(sd); // first "copy"
			for (size_t j = 0; j < 8; j++) { //single synapse weights
				returnvalue[64 * i + 8 * colset + j] =
					SynapseWeight::from_bitset(bit::crop<4>(sd, 4 * j));
			}
		}
	}

	sc.write_data(facets::SynapseControl::sc_ctrlreg, close_row); //close row
	while(sc.arraybusy()) {} //wait until not busy
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupSynapses,
	set_decoder_double_row,
	Handle::HICANN &, h,
	SynapseDriverOnHICANN const&, s,
	DecoderDoubleRow const&, data)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& hicann = reticle.hicann[h.jtag_addr()];

	set_decoder_double_row_impl(s, data, [&hicann](sc_write_data const& instr) {
			SynapseControl& sc = hicann->getSC(instr.index);
			sc.write_data(instr.addr, instr.data);
			if (instr.type == sc_write_data::WRITEANDWAIT) {
				// wait until controller not busy
				while(sc.arraybusy()) {}
			}
		});
}

HALBE_SETTER_GUARDED(EventSetupSynapses,
	set_decoder_double_row,
	std::vector<boost::shared_ptr<Handle::HICANN> >, handles,
	halco::hicann::v2::SynapseDriverOnHICANN const&, syndrv,
	std::vector<DecoderDoubleRow> const&, data)
{
	const size_t n_hicanns = handles.size();
	if (data.size() != n_hicanns)
		throw std::invalid_argument(
			"set_decoder_double_row: number of handles and data does not match");

	// to buffer sc writes...
	std::vector<sc_write_data_queue_t> per_hicann_queues(n_hicanns);

	auto it_data = data.begin();
	for (auto& queue : per_hicann_queues) {
		set_decoder_double_row_impl(syndrv, *it_data, [&queue](sc_write_data const& instr) {
			queue.push_back(instr);
		});
		++it_data;
	}

	// per-hicann indices of progress within "write queues" (start at 0th entry)
	std::vector<size_t> idxs(n_hicanns, 0);

	// as long as a single synapse controller hasn't finished...
	for (bool all_done = false; !all_done;) {
		all_done = true;
		for (size_t i = 0; i < n_hicanns; ++i)
			if (popexec_sc_write_data_queue(
				    dynamic_cast<HMF::Handle::HICANNHw&>(*handles[i]), idxs[i],
				    per_hicann_queues[i]))
				all_done = false;
	}
}

HALBE_GETTER(DecoderDoubleRow, get_decoder_double_row,
	Handle::HICANN &, h,
	SynapseDriverOnHICANN const&, s)
{
	ReticleControl& reticle = *h.get_reticle();

	enum : size_t {TOP = 0, BOT = 1}; //HARDWARE-coords!
	uint32_t addr[2], open_row[2], close_row[2];
	size_t SWTOP = TOP, SWBOT = BOT; //temporary variables for switching between hardware and software coords
	HicannCtrl::Synapse index;

	//calculate the correct hardware address of the line and choose the synapse block instance
	if (s.line() < 112){ //upper half of ANNCORE
		addr[BOT] = 222 - (s.line()*2);
		addr[TOP] = 222 - (s.line()*2) + 1;
		index = HicannCtrl::SYNAPSE_TOP;
	}
	else{ //lower half of ANNCORE
		addr[BOT] = (s.line() - 112)*2;
		addr[TOP] = (s.line() - 112)*2 + 1;
		SWTOP = BOT;
		SWBOT = TOP;
		index = HicannCtrl::SYNAPSE_BOTTOM;
	}
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	//put together "open row" and "close row" commands
	for (size_t ROW = TOP; ROW <= BOT; ROW++) {
		open_row[ROW] = facets::SynapseControl::sc_cmd_st_dec |
				(1 << facets::SynapseControl::sc_newcmd_p) |
				(addr[ROW] << facets::SynapseControl::sc_adr_p);
		close_row[ROW] = facets::SynapseControl::sc_cmd_close |
				(1 << facets::SynapseControl::sc_newcmd_p) |
				(addr[ROW] << facets::SynapseControl::sc_adr_p);
	}

	//read the data from hardware
	std::array<std::array<std::bitset<32>, 32>, 2> hwdata; //temporary data read from hardware
	for (size_t ROW = TOP; ROW <= BOT; ROW++){ //loop over top/bottom rows
		sc.write_data(facets::SynapseControl::sc_ctrlreg, open_row[ROW]); //open row for reading
		while(sc.arraybusy()) {} //wait until controller not busy

		for (size_t colset = 0; colset < 8; colset++){
			uint32_t read_command = facets::SynapseControl::sc_cmd_rdec | //put together a read command
								(colset << facets::SynapseControl::sc_colset_p) |
								(1 << facets::SynapseControl::sc_newcmd_p) |
								(addr[ROW] << facets::SynapseControl::sc_adr_p);

			sc.write_data(facets::SynapseControl::sc_ctrlreg, read_command); //issue read command
			while(sc.arraybusy()) {} //wait until not busy

			for (size_t i = 0; i < 4; i++) //save single chunks in the columnset to the temporary container
				hwdata[ROW][8*i + colset] = sc.read_data(facets::SynapseControl::sc_synout+i);
		}

		sc.write_data(facets::SynapseControl::sc_ctrlreg, close_row[ROW]); //close row
		while(sc.arraybusy()) {} //wait until not busy
	}

	//"unwrap" the hardware data by swapping bits in the correct order
	DecoderDoubleRow returnvalue;
	returnvalue[SWTOP] = top_from_decoder(hwdata[TOP], hwdata[BOT]);
	returnvalue[SWBOT] = bot_from_decoder(hwdata[TOP], hwdata[BOT]);
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_synapse_driver,
	Handle::HICANN &, h,
	SynapseDriverOnHICANN const&, s,
	SynapseDriver const&, driver)
{
	ReticleControl& reticle = *h.get_reticle();

	//TOP/BOT here refers to hardware coordinates
	bool TOP = top, BOT = bottom; //init with values where SW coords = HW coords (top half)

	//calculate the correct hardware address of the line and choose the synapse block instance
	uint32_t addr[2];
	HicannCtrl::Synapse index;

	if (s.line() < 112){ //upper half of ANNCORE
		addr[BOT] = 222 - (s.line()*2);
		addr[TOP] = 222 - (s.line()*2) + 1;
		index = HicannCtrl::SYNAPSE_TOP;
	}
	else{ //lower half of ANNCORE, SW coords != HW coords
		BOT = top;
		TOP = bottom;
		addr[BOT] = (s.line() - 112)*2;
		addr[TOP] = (s.line() - 112)*2 + 1;
		index = HicannCtrl::SYNAPSE_BOTTOM;
	}
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	//convert data to hardware format
	const std::bitset<8> zeropad = 0;
	std::array<std::bitset<16>, 2> gmaxfrac;
	std::array<std::bitset<16>, 2> preouts;
	std::array<std::bitset<16>, 2> hwconfig;

	typedef bitset<4> t4;
	for (auto const& tt : { top, bottom }) {
		gmaxfrac[tt] = bit::concat(t4(driver[tt].get_gmax_div(right)),
								   t4(driver[tt].get_gmax_div(left))).to_ulong();
	}

	//hardware coords here as the preout values depend on each other and their hardware number
	typedef bitset<2> t2;

	t2 const p0(driver[RowOnSynapseDriver(BOT)].get_decoder(top));
	t2 const p1(driver[RowOnSynapseDriver(TOP)].get_decoder(top));
	t2 const p2(driver[RowOnSynapseDriver(BOT)].get_decoder(bottom));
	t2 const p3(driver[RowOnSynapseDriver(TOP)].get_decoder(bottom));

	preouts = encode_preouts(p0, p1, p2, p3);

	hwconfig[BOT] = bit::concat(zeropad,
				bit::convert<bool, 1>(driver.stp_enable),
				bit::convert<bool, 1>(driver.enable),
				bit::convert<bool, 1>(driver.locin),
				bit::convert<bool, 1>(driver.connect_neighbor),
				t2(driver[RowOnSynapseDriver(BOT)].get_gmax()),
				bit::convert<bool, 1>(driver[RowOnSynapseDriver(BOT)].get_syn_in(right)),
				bit::convert<bool, 1>(driver[RowOnSynapseDriver(BOT)].get_syn_in(left)));

	hwconfig[TOP] = bit::concat(zeropad,
				bit::convert<bool, 1>(driver.stp_mode),
				driver.stp_cap,
				t2(driver[RowOnSynapseDriver(TOP)].get_gmax()),
				bit::convert<bool, 1>(driver[RowOnSynapseDriver(TOP)].get_syn_in(right)),
				bit::convert<bool, 1>(driver[RowOnSynapseDriver(TOP)].get_syn_in(left)));

	//right drivers have registers shifted by 8 bits
	if (s.toSideHorizontal()==right) {
		for (size_t i = std::min(TOP, BOT); i <= std::max(TOP, BOT); i++) {
			gmaxfrac[i] = gmaxfrac[i] << 8;
			preouts[i] = preouts[i] << 8;
			hwconfig[i] = hwconfig[i] << 8;
		}
	}

	//write gmax divisors
	while(sc.driverbusy()) {}
	sc.write_data(facets::SynapseControl::sc_engmax+addr[BOT], gmaxfrac[BOT].to_ulong());
	while(sc.driverbusy()) {}
	sc.write_data(facets::SynapseControl::sc_engmax+addr[TOP], gmaxfrac[TOP].to_ulong());
	//write preouts
	while(sc.driverbusy()) {}
	sc.write_data(facets::SynapseControl::sc_endrv+addr[BOT], preouts[bottom].to_ulong());
	while(sc.driverbusy()) {}
	sc.write_data(facets::SynapseControl::sc_endrv+addr[TOP], preouts[top].to_ulong());
	//write driver configuration registers
	while(sc.driverbusy()) {}
	sc.write_data(facets::SynapseControl::sc_encfg+addr[BOT], hwconfig[BOT].to_ulong());
	while(sc.driverbusy()) {}
	sc.write_data(facets::SynapseControl::sc_encfg+addr[TOP], hwconfig[TOP].to_ulong());
	while (sc.driverbusy()) {}
}


HALBE_GETTER(SynapseDriver, get_synapse_driver,
	Handle::HICANN &, h,
	SynapseDriverOnHICANN const&, s)
{
	ReticleControl& reticle = *h.get_reticle();

	bool TOP = top, BOT = bottom; //init with values where SW coords = HW coords (top half)

	//calculate the correct hardware address of the line and choose the synapse block instance
	uint32_t addr[2];
	HicannCtrl::Synapse index;

	if (s.line() < 112){ //upper half of ANNCORE
		addr[BOT] = 222 - (s.line()*2);
		addr[TOP] = 222 - (s.line()*2) + 1;
		index = HicannCtrl::SYNAPSE_TOP;
	}
	else{ //lower half of ANNCORE, SW coords != HW coords
		BOT = top;
		TOP = bottom;
		addr[BOT] = (s.line() - 112)*2;
		addr[TOP] = (s.line() - 112)*2 + 1;
		index = HicannCtrl::SYNAPSE_BOTTOM;
	}
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	if (sc.busy()) {
		throw std::runtime_error("get_synapse_driver: synapse controller must not be busy");
	}

	//read out the hardware registers
	std::array<std::bitset<16>, 2> cnfg, pdrv, gmax;
	for (size_t i = std::min(TOP, BOT); i <= std::max(TOP, BOT); i++) { //TOP/BOT
		sc.read_data(facets::SynapseControl::sc_encfg+addr[i]);           // trigger read cycle
		cnfg[i] = sc.read_data(facets::SynapseControl::sc_encfg+addr[i]); // retrieve read data
		sc.read_data(facets::SynapseControl::sc_endrv+addr[i]);           // trigger
		pdrv[i] = sc.read_data(facets::SynapseControl::sc_endrv+addr[i]); // retrieve
		sc.read_data(facets::SynapseControl::sc_engmax+addr[i]);          // trigger
		gmax[i] = sc.read_data(facets::SynapseControl::sc_engmax+addr[i]); // retrieve
	}

	//right drivers have registers shifted by 8 bits
	if (s.toSideHorizontal()==right) {
		for (size_t i = 0; i <= 1; i++) {
			cnfg[i] = cnfg[i] >> 8;
			pdrv[i] = pdrv[i] >> 8;
			gmax[i] = gmax[i] >> 8;
		}
	}

	//note the HW/SW-TOP/BOT-switching for the lower ANNCORE-half
	SynapseDriver driver = SynapseDriver();
	driver.stp_cap          = bit::crop<3>(cnfg[TOP], 4);
	driver.enable           = cnfg[BOT][6];
	driver.locin            = cnfg[BOT][5];
	driver.connect_neighbor = cnfg[BOT][4];
	driver.stp_enable       = cnfg[BOT][7];
	driver.stp_mode         = cnfg[TOP][7];

	//format the preout decoders
	std::array<std::array<std::bitset<2>, 2>, 2> preouts = decode_preouts(pdrv[BOT], pdrv[TOP]);
	driver[bottom].set_decoder(top, DriverDecoder::from_bitset(preouts[BOT][left]));
	driver[top].set_decoder(top, DriverDecoder::from_bitset(preouts[TOP][left]));
	driver[bottom].set_decoder(bottom, DriverDecoder::from_bitset(preouts[BOT][right]));
	driver[top].set_decoder(bottom, DriverDecoder::from_bitset(preouts[TOP][right]));

	//HW/SW-TOP/BOT-switching is already done while reading for the following:
	driver[top].set_gmax_div(left, bit::crop<4>(gmax[top], 0).to_ulong());
	driver[top].set_gmax_div(right, bit::crop<4>(gmax[top], 4).to_ulong());
	driver[bottom].set_gmax_div(left, bit::crop<4>(gmax[bottom], 0).to_ulong());
	driver[bottom].set_gmax_div(right, bit::crop<4>(gmax[bottom], 4).to_ulong());
	driver[top].set_gmax(bit::crop<2>(cnfg[top], 2).to_ulong());
	driver[bottom].set_gmax(bit::crop<2>(cnfg[bottom], 2).to_ulong());
	driver[top].set_syn_in(left, cnfg[top][0]);
	driver[top].set_syn_in(right, cnfg[top][1]);
	driver[bottom].set_syn_in(left, cnfg[bottom][0]);
	driver[bottom].set_syn_in(right, cnfg[bottom][1]);
	return driver;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_denmem_quad,
	Handle::HICANN &, h,
	QuadOnHICANN const&, qb,
	NeuronQuad const&, nquad)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& nbc = reticle.hicann[h.jtag_addr()]->getNBC();

	size_t const offset = 4 * qb;
	for (size_t ii = 0; ii < NeuronOnQuad::enum_type::end; ++ii)
	{
		NeuronOnQuad nrn {Enum{ii}};
		nbc.write_data(offset + NeuronQuad::getHWAddress(nrn),
			denmen_quad_formatter(nrn, nquad).to_ulong());
	}
}


HALBE_GETTER(NeuronQuad, get_denmem_quad,
	Handle::HICANN &, h,
	QuadOnHICANN const&, qb)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& nbc = reticle.hicann[h.jtag_addr()]->getNBC();

	NeuronQuad quad;

	size_t const offset = 4 * qb;
	for (size_t ii = 0; ii < NeuronOnQuad::enum_type::end; ++ii)
	{
		NeuronOnQuad nrn {Enum{ii}};

		ci_addr_t addr;
		ci_data_t data;

		nbc.read_data(offset + quad.getHWAddress(nrn));
		nbc.get_read_data(addr, data);

		if (addr != offset + quad.getHWAddress(nrn))
			throw std::runtime_error("unexpected address");

		denmem_quad_reader(data, nrn, quad);
	}

	return quad;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_neuron_config,
	Handle::HICANN &, h,
	NeuronConfig const&, nblock)
{
	ReticleControl& reticle = *h.get_reticle();

	reticle.hicann[h.jtag_addr()]->getNBC().set_sram_timings(
	    nblock.timings.read_delay, nblock.timings.setup_precharge,
	    nblock.timings.write_delay);

	//reset everything (also enables neuron-reset and SPL1-reset)
	reticle.hicann[h.jtag_addr()]->getNBC().write_data(facets::NeuronBuilderControl::NREGBASE, 0);

	// we should flush ARQ here... otherwise the reset is unset directly after setting (8 clks might be not enough says AG)
	HICANN::flush(h);

	//create configuration
	std::bitset<17> config = 0;
	config[NeuronConfig::gl_slow0]  = nblock.slow_I_gl[top];
	config[NeuronConfig::gla_slow0] = nblock.slow_I_gladapt[top];
	config[NeuronConfig::ra_slow0]  = nblock.slow_I_radapt[top];
	config[NeuronConfig::gl_fast0]  = nblock.fast_I_gl[top];
	config[NeuronConfig::gla_fast0] = nblock.fast_I_gladapt[top];
	config[NeuronConfig::ra_fast0]  = nblock.fast_I_radapt[top];
	config[NeuronConfig::gl_slow1]  = nblock.slow_I_gl[bottom];
	config[NeuronConfig::gla_slow1] = nblock.slow_I_gladapt[bottom];
	config[NeuronConfig::ra_slow1]  = nblock.slow_I_radapt[bottom];
	config[NeuronConfig::gl_fast1]  = nblock.fast_I_gl[bottom];
	config[NeuronConfig::gla_fast1] = nblock.fast_I_gladapt[bottom];
	config[NeuronConfig::ra_fast1]  = nblock.fast_I_radapt[bottom];
	config[NeuronConfig::bigcap0]   = nblock.bigcap[top];
	config[NeuronConfig::bigcap1]   = nblock.bigcap[bottom];

	// the resets are high active!
	config[NeuronConfig::spl1reset]    = !nblock.get_spl1_reset();
	config[NeuronConfig::neuronreset0] = !nblock.get_neuron_reset();
	config[NeuronConfig::neuronreset1] = !nblock.get_neuron_reset();

	//write configuration to hardware
	reticle.hicann[h.jtag_addr()]->getNBC().write_data(facets::NeuronBuilderControl::NREGBASE, config.to_ulong());
}


HALBE_GETTER(NeuronConfig, get_neuron_config,
	Handle::HICANN &, h)
{
	ReticleControl& reticle = *h.get_reticle();

	//read out the hardware registers
	ci_addr_t addr;
	ci_data_t rawdata;
	reticle.hicann[h.jtag_addr()]->getNBC().read_data(NeuronBuilderControl::NREGBASE);
	reticle.hicann[h.jtag_addr()]->getNBC().get_read_data(addr, rawdata);

	//fill the return structure
	std::bitset<17> data = rawdata;
	NeuronConfig returnvalue;

	returnvalue.slow_I_gl[top]         = data[NeuronConfig::gl_slow0];
	returnvalue.slow_I_gladapt[top]    = data[NeuronConfig::gla_slow0];
	returnvalue.slow_I_radapt[top]     = data[NeuronConfig::ra_slow0];
	returnvalue.fast_I_gl[top]         = data[NeuronConfig::gl_fast0];
	returnvalue.fast_I_gladapt[top]    = data[NeuronConfig::gla_fast0];
	returnvalue.fast_I_radapt[top]     = data[NeuronConfig::ra_fast0];
	returnvalue.slow_I_gl[bottom]      = data[NeuronConfig::gl_slow1];
	returnvalue.slow_I_gladapt[bottom] = data[NeuronConfig::gla_slow1];
	returnvalue.slow_I_radapt[bottom]  = data[NeuronConfig::ra_slow1];
	returnvalue.fast_I_gl[bottom]      = data[NeuronConfig::gl_fast1];
	returnvalue.fast_I_gladapt[bottom] = data[NeuronConfig::gla_fast1];
	returnvalue.fast_I_radapt[bottom]  = data[NeuronConfig::ra_fast1];
	returnvalue.bigcap[top]            = data[NeuronConfig::bigcap0];
	returnvalue.bigcap[bottom]         = data[NeuronConfig::bigcap1];
	return returnvalue;
}


HALBE_GETTER_GUARDED(HICANN::FGErrorResultRow,
	EventSetupFG,
	wait_fg,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b)
{
	return fg_busy_wait(h, b);
}


HALBE_GETTER_GUARDED(HICANN::FGErrorResultQuadRow,
	EventSetupFG,
	wait_fg,
	Handle::HICANN &, h)
{
	return fg_busy_wait(h);
}


HALBE_SETTER_GUARDED(EventSetupFG,
	set_fg_values,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b,
	FGBlock const&, fgb)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	// and finally analog FG values
	for (size_t row = 0; row < FGBlock::fg_lines; row++)
	{
		auto data = fgb.set_formatter(b, row);

		size_t cnt = 0;
		for (auto const& val : data)
			fc.write_data(cnt++, val.to_ulong());

		//execute write cycle: first write down, then write up
		fc.write_data(facets::FGControl::REG_ADDRINS,
			FGInstruction::writeDown(row));
		fg_busy_wait(h, b, row);

		fc.write_data(facets::FGControl::REG_ADDRINS,
			FGInstruction::writeUp(row));
		fg_busy_wait(h, b, row);
	}
}


HALBE_SETTER_GUARDED(EventSetupFG,
	set_fg_values,
	Handle::HICANN &, h,
	FGControl const&, fg)
{
	ReticleControl& reticle = *h.get_reticle();
	auto const getFC = [&h,&reticle](size_t ii) {
		return reticle.hicann[h.jtag_addr()]->getFC(ii);
	};

	////setting analog parameters
	std::bitset<20> data;
	for (size_t i = 0; i < FGBlock::fg_lines; i++) {
		for (size_t blk = 0; blk < fg.size(); blk++) {
			FGBlockOnHICANN b {Enum{blk}};
			auto data = fg.getBlock(b).set_formatter(b, i);

			size_t cnt = 0;
			for (auto const& val : data)
				getFC(blk).write_data(cnt++, val.to_ulong());
		}

		//execute write cycle: first write down, then write up
		for (size_t blk = 0; blk < 4; blk++)
			getFC(blk).write_data(facets::FGControl::REG_ADDRINS,
				FGInstruction::writeDown(i));

		// wait for all controllers to finish
		fg_busy_wait(h);

		for (size_t blk = 0; blk < fg.size(); blk++)
			getFC(blk).write_data(facets::FGControl::REG_ADDRINS,
				FGInstruction::writeUp(i));

		// wait for all controllers to finish
		fg_busy_wait(h);
	}
}

HALBE_GETTER(FGBlock, get_fg_values,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b)
{
	(void)h;
	(void)b;
    throw std::runtime_error("Not possible on real hardware :P");
}


HALBE_SETTER_GUARDED_RETURNS(HICANN::FGErrorResultQuadRow,
	EventSetupFG,
	set_fg_row_values,
	Handle::HICANN &, h,
	halco::hicann::v2::FGRowOnFGBlock, row,
	FGControl const&, fg,
	bool const, writeDown,
	bool const, blocking)
{
	ReticleControl& reticle = *h.get_reticle();
	auto const getFC = [&h,&reticle](size_t ii) {
		return reticle.hicann[h.jtag_addr()]->getFC(ii);
	};

	////setting analog parameters
	std::bitset<20> data;
	for (size_t blk = 0; blk < fg.size(); blk++) {
		FGBlockOnHICANN b {Enum{blk}};
		auto data = fg.getBlock(b).set_formatter(b, row);

		size_t cnt = 0;
		for (auto const& val : data)
			// ECM: TODO later (4 pbmem-based cfg) specify delay for async write (see below too)!
			getFC(blk).write_data(cnt++, val.to_ulong());
	}


	////issue command for writing down or up -- calling both required for arbitrary values
	if (writeDown) {
		//execute write cycle: first write down, then write up
		for (size_t blk = 0; blk < 4; blk++)
			getFC(blk).write_data(facets::FGControl::REG_ADDRINS,
				FGInstruction::writeDown(row));
	} else {
		for (size_t blk = 0; blk < fg.size(); blk++)
			getFC(blk).write_data(facets::FGControl::REG_ADDRINS,
				FGInstruction::writeUp(row));
	}

	if (blocking)
		// wait for all controllers to finish
		return fg_busy_wait(h);

	// No errors can be obtained when not waiting for the controller to finish.
	return HICANN::FGErrorResultQuadRow();
}

HALBE_SETTER_GUARDED_RETURNS(HICANN::FGErrorResultQuadRow,
	EventSetupFG,
	set_fg_row_values,
	Handle::HICANN &, h,
	const FGRowOnFGBlock4, rows,
	const FGRow4&, rowData,
	bool const, writeDown,
	bool const, blocking)
{
	ReticleControl& reticle = *h.get_reticle();

	////setting analog parameters
	std::bitset<20> data;
	for (FGBlockOnHICANN blk : iter_all<FGBlockOnHICANN>()) {
		const FGRowOnFGBlock r = rows.at(blk.toEnum());
		auto fc = reticle.hicann[h.jtag_addr()]->getFC(blk.toEnum());

		size_t cnt = 0;
		for (auto const& val : rowData.at(blk.toEnum()).set_formatter()) {
			// ECM: TODO later (4 pbmem-based cfg) specify delay for async write (see below too)!
			fc.write_data(cnt++, val.to_ulong());
		}
		fc.write_data(
			facets::FGControl::REG_ADDRINS,
			(writeDown ? FGInstruction::writeDown(r) : FGInstruction::writeUp(r)));
	}

	if (blocking)
		return fg_busy_wait(h);

	// No errors can be obtained when not waiting for the controller to finish.
	return HICANN::FGErrorResultQuadRow();
}

HALBE_SETTER_GUARDED_RETURNS(HICANN::FGErrorResultQuadRow,
	EventSetupFG,
	set_fg_row_values,
	Handle::HICANN &, h,
	halco::hicann::v2::FGBlockOnHICANN, block,
	halco::hicann::v2::FGRowOnFGBlock, row,
	FGRow const&, fg,
	bool const, writeDown,
	bool const, blocking)
{
	ReticleControl& reticle = *h.get_reticle();
	auto const getFC = [&h,&reticle](size_t ii) {
		return reticle.hicann[h.jtag_addr()]->getFC(ii);
	};

	////setting analog parameters
	size_t cnt = 0;
	for (auto const& val : fg.set_formatter())
		// ECM: TODO later (4 pbmem-based cfg) specify delay for async write (see below too)!
		getFC(block.toEnum()).write_data(cnt++, val.to_ulong());


	////issue command for writing down or up -- calling both required for arbitrary values
	if (writeDown) {
		//execute write cycle
		getFC(block.toEnum()).write_data(facets::FGControl::REG_ADDRINS,
			FGInstruction::writeDown(row));
	} else {
		getFC(block.toEnum()).write_data(facets::FGControl::REG_ADDRINS,
			FGInstruction::writeUp(row));
	}

	if (blocking)
		return fg_busy_wait(h);

	// No errors can be obtained when not waiting for the controller to finish.
	return HICANN::FGErrorResultQuadRow();
}


HALBE_SETTER_GUARDED(EventSetupFG,
	set_fg_config,
	Handle::HICANN &, h,
	halco::hicann::v2::FGBlockOnHICANN const &, block,
	const FGConfig &, config)
{
	ReticleControl& reticle = *h.get_reticle();
	::facets::FGControl & fc = reticle.hicann[h.jtag_addr()]->getFC(block.toEnum());
	fc.write_data(facets::FGControl::REG_BIAS,
			config.getBias().to_ulong());

	fc.write_data(facets::FGControl::REG_OP,
			config.getOp().to_ulong());
}


HALBE_GETTER(FGConfig, get_fg_config,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	HICANN::FGConfig config;
	ci_addr_t addr;

	ci_data_t bias;
	fc.read_data(facets::FGControl::REG_BIAS);
	fc.get_read_data(addr, bias);
	if (addr != facets::FGControl::REG_BIAS)
	{
		throw std::runtime_error("get_fg_config: fgcontrol address mismatch (REG_BIAS)");
	}

	std::bitset<4> fg_bias  = bit::crop<4>(bias, 0);
	std::bitset<4> fg_biasn = bit::crop<4>(bias, 4);
	// AK: says they are really reversed but only on the reading end.
	config.fg_bias          = bit::reverse(fg_bias);  //reverse bit order
	config.fg_biasn         = bit::reverse(fg_biasn); //reverse bit order

	config.pulselength      = bit::crop<4>(bias, 8);
	config.groundvm         = bit::test(bias, 12);
	config.calib            = bit::test(bias, 13);

	ci_data_t op;
	fc.read_data(facets::FGControl::REG_OP);
	fc.get_read_data(addr, op);
	if (addr != facets::FGControl::REG_OP)
	{
		throw std::runtime_error("get_fg_config: fgcontrol address mismatch (REG_OP)");
	}

	config.maxcycle         = bit::crop<8>(op, 0);
	config.readtime         = bit::crop<6>(op, 8);
	config.acceleratorstep  = bit::crop<6>(op, 14);
	config.voltagewritetime = bit::crop<6>(op, 20);
	config.currentwritetime = bit::crop<6>(op, 26);

	return config;
}

HALBE_SETTER_GUARDED(EventSetupFG,
	set_fg_cell,
	Handle::HICANN &, h,
	NeuronOnHICANN const&, n,
	neuron_parameter const&, p)
{
	FGBlockOnHICANN const& b = n.toNeuronFGBlock();

	ReticleControl& reticle = *h.get_reticle();
	auto& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	uint8_t const column = FGBlock::is_left(b) ? n.toNeuronOnFGBlock()+1 : 128-n.toNeuronOnFGBlock();
	uint8_t const line   = FGBlock::getNeuronLut(b).at(p);
	if (line == not_connected)
		throw std::invalid_argument("no such neuron parameter");

	fc.write_data(facets::FGControl::REG_ADDRINS, FGInstruction::read(line, column));
}


HALBE_SETTER_GUARDED(EventSetupFG,
	set_fg_cell,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b,
	shared_parameter const&, p)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	uint8_t const line   = FGBlock::getSharedLut(b).at(p);
	if (line == not_connected)
		throw std::invalid_argument("no such shared parameter");

	fc.write_data(facets::FGControl::REG_ADDRINS, FGInstruction::read(line, 0));
}


HALBE_SETTER_GUARDED(EventSetupFG,
	set_fg_cell,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b,
	FGCellOnFGBlock const&, c)
{
	ReticleControl& reticle = *h.get_reticle();
	auto& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	fc.write_data(facets::FGControl::REG_ADDRINS, FGInstruction::read(c.y(), c.x()));
}


HALBE_SETTER_GUARDED(EventSetupFG,
	set_current_stimulus,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b,
	FGStimulus const&, stim)
{
	ReticleControl& reticle = *h.get_reticle();
	facets::FGControl& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	//always using RAM bank 0 here... for no reason...
	for (size_t i = 0; i < 128; i+=2)
	{ //write all cells but the last
		std::bitset<20> const data = bit::concat(
			std::bitset<10>(stim[i+1]),
			std::bitset<10>(stim[i  ]));
		fc.write_data(i/2, data.to_ulong());
	}
	fc.write_data(64, stim.at(128)); //last cell is single

	fc.write_data(facets::FGControl::REG_ADDRINS,
				  FGInstruction::stimulateNeurons(stim.getContinuous()));
}


HALBE_GETTER(FGStimulus, get_current_stimulus,
	Handle::HICANN &, h,
	FGBlockOnHICANN const&, b)
{
	ReticleControl& reticle = *h.get_reticle();
	facets::FGControl& fc = reticle.hicann[h.jtag_addr()]->getFC(b.toEnum());

	FGStimulus returnvalue;

	// FIXME: is it possible to readout wheter FGStimulus is continious or not?
	// returnvalue.setContinuous(...);

	ci_addr_t addr;
	ci_data_t data;

	//always using RAM bank 0 here... for no reason...
	for (size_t ii = 0; ii < 65; ii++)
	{
		fc.read_data(ii);
		fc.get_read_data(addr, data);

		returnvalue[2*ii  ] = bit::crop<10>(data);
		if (ii == 64)
			break;
		returnvalue[2*ii+1] = bit::crop<10>(data, 10); //no 130th value
	}
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_repeater,
	Handle::HICANN &, h,
	VRepeaterOnHICANN const&, r,
	HICANN::VerticalRepeater const&, rc)
{
	auto const& x = r.toVLineOnHICANN();
	set_repeater_helper(h, x, rc, to_repblock(x), to_repaddr(x));
}


HALBE_GETTER(HICANN::VerticalRepeater, get_repeater,
	Handle::HICANN &, h,
	VRepeaterOnHICANN const&, r)
{
	auto const& x = r.toVLineOnHICANN();
	VerticalRepeater returnvalue =
		get_repeater_helper<VerticalRepeater>(h, x, to_repblock(x), to_repaddr(x));
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_repeater,
	Handle::HICANN &, h,
	HRepeaterOnHICANN const&, r,
	HICANN::HorizontalRepeater const&, rc)
{
	auto const& y = r.toHLineOnHICANN();
	set_repeater_helper(h, y, rc, to_repblock(y), to_repaddr(y));
}


HALBE_GETTER(HICANN::HorizontalRepeater, get_repeater,
	Handle::HICANN &, h,
	HRepeaterOnHICANN const&, r)
{
	auto const& y = r.toHLineOnHICANN();
	HorizontalRepeater returnvalue =
		get_repeater_helper<HorizontalRepeater>(
				h, y, to_repblock(y), to_repaddr(y));
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_repeater_block,
	Handle::HICANN &, h,
	RepeaterBlockOnHICANN const&, block,
	HICANN::RepeaterBlock const&, rbc)
{
	ReticleControl& reticle = *h.get_reticle();

	size_t address = block.toEnum();
	HicannCtrl::Repeater index = static_cast<HicannCtrl::Repeater>(address);
	facets::RepeaterControl& rc = reticle.hicann[h.jtag_addr()]->getRC(index);

	rc.set_sram_timings(rbc.timings.read_delay, rbc.timings.setup_precharge,
	                    rbc.timings.write_delay);

	//write test_output registers
	for (auto tp : iter_all<TestPortOnRepeaterBlock>()) {
		for (size_t j = 0; j < 3; j++) { //loop over single entries
			std::bitset<6> neuron(rbc.tdo_data[tp][j].address);
			std::bitset<10> time = rbc.tdo_data[tp][j].time;

			//put neuron number and time bits in the right order
			std::bitset<2> tm = bit::crop<2>(time, 0);
			std::bitset<8> hb = bit::concat(tm, neuron);
			std::bitset<8> lb = bit::crop<8>(time, 2);

			if (tp == 0){
				rc.write_data(facets::RepeaterControl::rc_td0 + 2*j, hb.to_ulong());
				rc.write_data(facets::RepeaterControl::rc_td0 + 2*j + 1, lb.to_ulong());
			}
			else {
				rc.write_data(facets::RepeaterControl::rc_td1 + 2*j, hb.to_ulong());
				rc.write_data(facets::RepeaterControl::rc_td1 + 2*j + 1, lb.to_ulong());
			}
		}
	}

	std::bitset<8> config;
	repeater_config_formater(rbc, config);

	//write config register
	rc.write_data(facets::RepeaterControl::rc_config, config.to_ulong());
}

HALBE_GETTER(HICANN::RepeaterBlock, get_repeater_block,
	Handle::HICANN &, h,
	RepeaterBlockOnHICANN const&, block)
{
	ReticleControl& reticle = *h.get_reticle();

	size_t address = block.toEnum();
	HicannCtrl::Repeater index = static_cast<HicannCtrl::Repeater>(address);
	facets::RepeaterControl& rc = reticle.hicann[h.jtag_addr()]->getRC(index);

	RepeaterBlock returnvalue;

	//read out the status register
	std::bitset<8> data = rc.read_data(facets::RepeaterControl::rc_status);

	returnvalue.full_flag[0] = data[0];
	returnvalue.full_flag[1] = data[1];

	//read out test_input registers
	for (auto tp : iter_all<TestPortOnRepeaterBlock>()) {
		for (size_t j = 0; j < 3; j++) { //loop over single entries
			std::bitset<8> hb, lb;

			if (tp == 0) {
				hb = rc.read_data(facets::RepeaterControl::rc_td0 + 2*j);
				lb = rc.read_data(facets::RepeaterControl::rc_td0 + 2*j + 1);
			}
			else {
				hb = rc.read_data(facets::RepeaterControl::rc_td1 + 2*j);
				lb = rc.read_data(facets::RepeaterControl::rc_td1 + 2*j + 1);
			}

			//put neuron number and time bits in the right order
			std::bitset<2> temp = crop<2>(hb, 6);
			L1Address neuron(bit::crop<6>(hb, 0).to_ulong());
			std::bitset<10> time = gray_to_binary(bit::concat(lb, temp));

			returnvalue.tdi_data[tp][j] = RepeaterBlock::TestEvent(neuron, time.to_ulong());
		}
	}
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_merger_tree,
	Handle::HICANN &, h,
	HICANN::MergerTree const&, m)
{
	ReticleControl& reticle = *h.get_reticle();

	std::bitset<MergerTree::num_merger> enable = 0, select = 0, slow = 0; //hardware data chunks

	//swap the numbers of mergers according to new coordinates
	for (size_t mer = 0; mer < MergerTree::num_merger; mer++) {
		select[translate_neuron_merger(mer)] = m.getMergerRaw(mer).config[Merger::select_bit];
		enable[translate_neuron_merger(mer)] = m.getMergerRaw(mer).config[Merger::enable_bit];
		slow[translate_neuron_merger(mer)]   = m.getMergerRaw(mer).slow;
	}

	reticle.hicann[h.jtag_addr()]->getNC().write_data(NeuronControl::nc_enable, enable.to_ulong());
	reticle.hicann[h.jtag_addr()]->getNC().write_data(NeuronControl::nc_select, select.to_ulong());
	reticle.hicann[h.jtag_addr()]->getNC().write_data(NeuronControl::nc_slow, slow.to_ulong());
}


HALBE_GETTER(MergerTree, get_merger_tree,
	Handle::HICANN &, h)
{
	ReticleControl& reticle = *h.get_reticle();

	MergerTree returnvalue;

	ci_addr_t addr;
	ci_data_t rawenable, rawselect, rawslow;
	std::bitset<MergerTree::num_merger> enable, select, slow;

	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_enable);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, rawenable);
	enable = rawenable;

	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_select);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, rawselect);
	select = rawselect;

	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_slow);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, rawslow);
	slow = rawslow;

	//reverse-swap the numbers of mergers according to new coordinates
	for (size_t mer = 0; mer < MergerTree::num_merger; mer++) {
		returnvalue.getMergerRaw(mer).config[Merger::select_bit] = select[translate_neuron_merger(mer)];
		returnvalue.getMergerRaw(mer).config[Merger::enable_bit] = enable[translate_neuron_merger(mer)];
		returnvalue.getMergerRaw(mer).slow = slow[translate_neuron_merger(mer)];
	}
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_dnc_merger,
	Handle::HICANN &, h,
	HICANN::DNCMergerLine const&, m)
{
	ReticleControl& reticle = *h.get_reticle();
	static const size_t num_merger = halco::hicann::v2::DNCMergerOnHICANN::size;

	std::bitset<num_merger> enable = 0, select = 0, slow = 0, loopback = 0;

	//swap the numbers of mergers according to new coordinates
	for (uint8_t ii = 0; ii < num_merger; ii++) {
		halco::hicann::v2::DNCMergerOnHICANN mer{ii};
		select[translate_dnc_merger(mer)] = m[mer].config[Merger::select_bit];
		enable[translate_dnc_merger(mer)] = m[mer].config[Merger::enable_bit];
		slow[translate_dnc_merger(mer)]   = m[mer].slow;

		//this is some f**d up sh** right here... super-mega-ultra-permutation...
		if (m[mer].loopback) {
			if (m[DNCMergerLine::loopback_target(mer)].loopback)
				throw std::logic_error("DNC loopback can only be enabled in one direction");
			loopback[translate_dnc_merger(DNCMergerLine::loopback_target(mer))] = true; //hidden swapping!!!
		}
	}

	std::bitset<16> slowenable = bit::concat(slow, enable);
	std::bitset<16> selloop = bit::concat(select, loopback);
	reticle.hicann[h.jtag_addr()]->getNC().write_data(NeuronControl::nc_dncmerger, slowenable.to_ulong());
	reticle.hicann[h.jtag_addr()]->getNC().write_data(NeuronControl::nc_dncloopb, selloop.to_ulong());
}


HALBE_GETTER(DNCMergerLine, get_dnc_merger,
	Handle::HICANN &, h)
{
	ReticleControl& reticle = *h.get_reticle();
	static const size_t num_merger = halco::hicann::v2::DNCMergerOnHICANN::size;

	DNCMergerLine returnvalue;

	ci_addr_t addr;
	ci_data_t slowenable, selectloopback;
	std::bitset<num_merger> enable, select, slow, loopback;

	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_dncmerger);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, slowenable);
	enable = bit::crop<8>(slowenable);
	slow = bit::crop<8>(slowenable, 8);

	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_dncloopb);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, selectloopback);
	loopback = bit::crop<8>(selectloopback);
	select = bit::crop<8>(selectloopback, 8);

	//reverse-swap the numbers of mergers according to new coordinates
	for (uint8_t ii = 0; ii < num_merger; ii++) {
		halco::hicann::v2::DNCMergerOnHICANN mer{ii};
		returnvalue[mer].config[Merger::select_bit] = select[translate_dnc_merger(mer)];
		returnvalue[mer].config[Merger::enable_bit] = enable[translate_dnc_merger(mer)];
		returnvalue[mer].slow = slow[translate_dnc_merger(mer)];

		if (loopback[translate_dnc_merger(mer)]) {
			if (loopback[translate_dnc_merger(DNCMergerLine::loopback_target(mer))])
				throw std::logic_error("DNC loopback can only be enabled in one direction");
			returnvalue[DNCMergerLine::loopback_target(mer)].loopback = true;
		}
		else
			returnvalue[DNCMergerLine::loopback_target(mer)].loopback = false;
	}
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_gbit_link,
	Handle::HICANN &, h,
	GbitLink const&, link)
{
	ReticleControl& reticle = *h.get_reticle();

	std::bitset<17> spl1cfg = 0; //configuration of HICANN-side

	//configuration of the HICANN-side is simple and is made once per HICANN
	spl1cfg[16] = link.timestamp_enable; //set timestamp flag
	for (size_t i = 0; i < 8; i++) { //looping over channels
		///swap the direction bits according to new coordinates
		if(link.dirs[i] != GbitLink::Direction::OFF) {
			spl1cfg[7-i] = true;
			spl1cfg[8+(7-i)] = (link.dirs[i] == GbitLink::Direction::TO_HICANN) ? true : false;
		}
		else spl1cfg[7-i]=false; //switch off the channel
	}

	reticle.hicann[h.jtag_addr()]->getSPL1Control().write_cfg(spl1cfg.to_ulong());
}


HALBE_SETTER_GUARDED(EventSetupL1,
	set_phase,
	Handle::HICANN &, h,
	Phase const, phase)
{
	ReticleControl& reticle = *h.get_reticle();

	//swap the data bits according to new coordinates
	Phase _phase = phase;
	Phase temp = phase;
	for (size_t i = 0; i < phase.size(); i++)
		_phase[translate_dnc_merger(i)] = temp[i];

	reticle.hicann[h.jtag_addr()]->getNC().write_data(
		NeuronControl::nc_phase, _phase.to_ulong());
}


HALBE_GETTER(Phase, get_phase,
	Handle::HICANN &, h)
{
	ReticleControl& reticle = *h.get_reticle();

	ci_addr_t addr;
	ci_data_t data;
	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_phase);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, data);

	//reverse-swap the data bits according to new coordinates
	Phase returnvalue;
	for (size_t i = 0; i < returnvalue.size(); i++)
		returnvalue[i] = bit::test(data, translate_dnc_merger(i));
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupL1BG,
	set_background_generator,
	Handle::HICANN &, h,
	BackgroundGeneratorArray const&, bg)
{
	ReticleControl& reticle = *h.get_reticle();
	facets::NeuronControl& nc = reticle.hicann[h.jtag_addr()]->getNC();

	std::bitset<8> enable = 0, poisson = 0; //temporary data
	std::bitset<16> randomreset;            //stores enable and poisson bits

	// Disable all generator
	nc.write_data(NeuronControl::nc_randomreset, randomreset.to_ulong());

	// Write L1 addresses
	for (size_t i = 0; i < 8; i+=2) { //sort the neuron numbers in correct order
		std::bitset<8> addr0 = static_cast<uint8_t>(bg[i].address());
		std::bitset<8> addr1 = static_cast<uint8_t>(bg[i+1].address());
		auto addr = bit::concat(addr0, addr1);
		nc.write_data(NeuronControl::nc_nnumber + translate_neuron_merger(i)/2, addr.to_ulong());
	}

	// Write period
	for (size_t i = 0; i < bg.size(); i++) {
		nc.write_data(NeuronControl::nc_period +  translate_neuron_merger(i), bg[i].period());
	}

	// Enable bg's on aftera nother to, because there is only one register for seeds
	// which is read when enalbing the bg
	for (size_t i = 0; i < bg.size(); i++) {
		// If seed == 0 for random == 1 and enable == 1, the LFSR will be stuck in its death state
		if (bg[i].enable() && bg[i].random() && !bg[i].seed())
			LOG4CXX_WARN(
			    logger,
			    short_format(h.coordinate())
			        << ": Background Generator enabled in random mode with seed 0 will "
			           "have no effect (Linear Feedback Shift Register would be "
			           "initialized in stuck state).");

		if (bg[i].enable() && bg[i].random() && bg[i].address() != L1Address(0))
			LOG4CXX_WARN(logger, short_format(h.coordinate())
			                         << ": Background Generator in random mode will "
			                            "always send spikes with L1Address(0)");

		enable[translate_neuron_merger(i)]  = bg[i].enable();
		poisson[translate_neuron_merger(i)] = bg[i].random();
		randomreset = bit::concat(poisson, enable);

		//set seed, and start
		nc.write_data(NeuronControl::nc_seed, bg[i].seed());
		nc.write_data(NeuronControl::nc_randomreset, randomreset.to_ulong());
	}
}


HALBE_GETTER(BackgroundGeneratorArray, get_background_generator,
	Handle::HICANN &, h)
{
	ReticleControl& reticle = *h.get_reticle();

	ci_addr_t addr;
	ci_data_t temp;
	std::bitset<8> enable = 0, poisson = 0;
	std::array<uint16_t, 8> period;
	std::array<uint8_t, 8> nnumber;
	uint16_t seed;

	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_seed);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, temp);
	seed = bit::crop<16>(temp);

	reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_randomreset);
	reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, temp);
	enable = bit::crop<8>(temp);
	poisson = bit::crop<8>(temp, 8);

	for (size_t i = 0; i < period.size(); i++){
		reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_period+i);
		reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, temp);
		period[i] = bit::crop<16>(temp);
	}

	for (size_t i = 0; i < nnumber.size()/2; i++){
		reticle.hicann[h.jtag_addr()]->getNC().read_data(NeuronControl::nc_nnumber+i);
		reticle.hicann[h.jtag_addr()]->getNC().get_read_data(addr, temp);
		nnumber[i*2] = bit::crop<6>(temp);
		nnumber[i*2+1] = bit::crop<6>(temp, 8);
	}

	BackgroundGeneratorArray returnvalue; //put read data into the container
	for (size_t i = 0; i < returnvalue.size(); i++){
		returnvalue[i] = BackgroundGenerator(); //reset
		returnvalue[i].enable(enable[translate_neuron_merger(i)]);
		returnvalue[i].random(poisson[translate_neuron_merger(i)]);
		returnvalue[i].period(period[translate_neuron_merger(i)]);
		returnvalue[i].address(L1Address(nnumber[translate_neuron_merger(i)]));
		returnvalue[i].seed(seed);
		//WARNING: seed here is the last seed written, not necessary of this particular BG!!!
	}
	return returnvalue;
}


HALBE_SETTER_GUARDED(EventSetupAnalogReadout,
	set_analog,
	Handle::HICANN &, h,
	Analog const&, a)
{
	if (a.get_fireline_neuron0(AnalogOnHICANN(1))) {
		throw std::runtime_error("fireline neuron0 not available on bottom analog readout");
	}

	ReticleControl& reticle = *h.get_reticle();
	reticle.hicann[h.jtag_addr()]->getNBC().write_data(
	NeuronBuilderControl::OUTREGBASE, a.getConfig().to_ulong());
}


HALBE_GETTER(Analog, get_analog,
	Handle::HICANN &, h)
{
	ReticleControl& reticle = *h.get_reticle();

	Analog returnvalue;

	ci_addr_t addr;
	ci_data_t data;
	//size_t flag0, flag1;
	std::bitset<10> analog0, analog1;

	//read data from the hardware
	reticle.hicann[h.jtag_addr()]->getNBC().read_data(NeuronBuilderControl::OUTREGBASE);
	reticle.hicann[h.jtag_addr()]->getNBC().get_read_data(addr, data);

	returnvalue.getConfig() = data;
	return returnvalue;
}

HALBE_SETTER_GUARDED(EventSetupSynapses, set_stdp_lut,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray,
	HICANN::STDPLUT const&, lut)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	/// write lookup-table
	// registers 0 and 1: acausal
	sc.write_data(facets::SynapseControl::sc_lut, synapse_get_lut_low(lut.acausal).to_ulong());
	sc.write_data(facets::SynapseControl::sc_lut + 1, synapse_get_lut_high(lut.acausal).to_ulong());
	// registers 2 and 3: causal
	sc.write_data(facets::SynapseControl::sc_lut + 2, synapse_get_lut_low(lut.causal).to_ulong());
	sc.write_data(facets::SynapseControl::sc_lut + 3, synapse_get_lut_high(lut.causal).to_ulong());
	// registers 4 and 5: combined
	sc.write_data(facets::SynapseControl::sc_lut + 4, synapse_get_lut_low(lut.combined).to_ulong());
	sc.write_data(
	    facets::SynapseControl::sc_lut + 5, synapse_get_lut_high(lut.combined).to_ulong());
}

HALBE_GETTER(STDPLUT, get_stdp_lut,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	STDPLUT returnvalue;

	//read acausal LUT
	std::bitset<32> data = sc.read_data(facets::SynapseControl::sc_lut);
	synapse_set_lut_low(returnvalue.acausal, data);
	data = sc.read_data(facets::SynapseControl::sc_lut + 1);
	synapse_set_lut_high(returnvalue.acausal, data);
	//read causal LUT
	data = sc.read_data(facets::SynapseControl::sc_lut + 2);
	synapse_set_lut_low(returnvalue.causal, data);
	data = sc.read_data(facets::SynapseControl::sc_lut + 3);
	synapse_set_lut_high(returnvalue.causal, data);
	//read combined LUT
	data = sc.read_data(facets::SynapseControl::sc_lut + 4);
	synapse_set_lut_low(returnvalue.combined, data);
	data = sc.read_data(facets::SynapseControl::sc_lut + 5);
	synapse_set_lut_high(returnvalue.combined, data);

	return returnvalue;
}

HALBE_SETTER_GUARDED(EventSetupSynapses, set_syn_rst,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray,
	HICANN::SynapseController::syn_rst_t const&, syn_rst)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	sc.write_data(facets::SynapseControl::sc_synrst, syn_rst.to_ulong());
}

HALBE_GETTER(HICANN::SynapseController::syn_rst_t, get_syn_rst,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	HICANN::SynapseController::syn_rst_t const returnvalue =
		sc.read_data(facets::SynapseControl::sc_synrst);

	return returnvalue;
}

HALBE_SETTER_GUARDED(EventSetupSynapses, set_syn_ctrl,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray,
	HICANN::SynapseControlRegister const&, ctrl_reg)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	std::bitset<32> ctrl_bitset;
	synapse_ctrl_formater(ctrl_reg, synarray, ctrl_bitset);
	sc.write_data(facets::SynapseControl::sc_ctrlreg, ctrl_bitset.to_ulong());
}

HALBE_GETTER(HICANN::SynapseControlRegister, get_syn_ctrl,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	HICANN::SynapseControlRegister returnvalue;

	std::bitset<32> const data = sc.read_data(facets::SynapseControl::sc_ctrlreg);
	returnvalue.idle = data[30];
	returnvalue.sca = data[29];
	returnvalue.scc = data[28];
	returnvalue.without_reset = data[27];
	returnvalue.sel = SynapseSel(bit::crop<3>(data, 24).to_ulong());

	uint8_t last_addr = bit::crop<8>(data, 16).to_ulong();
	returnvalue.last_row = AddrOnHW_to_SynapseRowOnArray(last_addr, synarray);
	uint8_t addr = bit::crop<8>(data, 8).to_ulong();
	returnvalue.row = AddrOnHW_to_SynapseRowOnArray(addr, synarray);

	returnvalue.newcmd = data[6];
	returnvalue.continuous = data[5];
	returnvalue.encr = data[4];

	returnvalue.cmd = SynapseControllerCmd(bit::crop<4>(data, 0).to_ulong());
	return returnvalue;
}

HALBE_SETTER_GUARDED(EventSetupSynapses, set_syn_cnfg,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray,
	HICANN::SynapseConfigurationRegister const&, cnfg_reg)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	std::bitset<32> cnfg_bitset;
	synapse_cnfg_formater(cnfg_reg, cnfg_bitset);
	sc.write_data(facets::SynapseControl::sc_cnfgreg, cnfg_bitset.to_ulong());
}

HALBE_GETTER(HICANN::SynapseConfigurationRegister, get_syn_cnfg,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	HICANN::SynapseConfigurationRegister returnvalue;

	// bit 31-28: reserved
	std::bitset<28> const data = sc.read_data(facets::SynapseControl::sc_cnfgreg);

	returnvalue.synarray_timings.write_delay = SynapseWriteDelay(bit::crop<2>(data, 26).to_ulong());
	returnvalue.synarray_timings.output_delay =
		SynapseOutputDelay(bit::crop<4>(data, 22).to_ulong());
	returnvalue.dllresetb = SynapseDllresetb(bit::crop<2>(data, 20).to_ulong());
	returnvalue.gen = SynapseGen(bit::crop<4>(data, 16).to_ulong());
	returnvalue.synarray_timings.setup_precharge =
		SynapseSetupPrecharge(bit::crop<4>(data, 12).to_ulong());
	returnvalue.synarray_timings.enable_delay =
		SynapseEnableDelay(bit::crop<4>(data, 8).to_ulong());

	returnvalue.pattern0.cc = data[7];
	returnvalue.pattern1.cc = data[6];

	returnvalue.pattern0.ca = data[5];
	returnvalue.pattern1.ca = data[4];

	returnvalue.pattern0.ac = data[3];
	returnvalue.pattern1.ac = data[2];

	returnvalue.pattern0.aa = data[1];
	returnvalue.pattern1.aa = data[0];

	return returnvalue;
}

HALBE_GETTER(HICANN::SynapseStatusRegister, get_syn_status,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	HICANN::SynapseStatusRegister returnvalue;

	std::bitset<3> const data = sc.read_data(facets::SynapseControl::sc_status);

	returnvalue.auto_busy = data[2];
	returnvalue.slice_busy = data[1];
	returnvalue.syndrv_busy = data[0];

	return returnvalue;
}

HALBE_SETTER_GUARDED(EventSetupL1, set_synapse_controller,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray,
	HICANN::SynapseController const&, synapse_controller)
{
	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	sc.set_sram_timings(synapse_controller.syndrv_timings.read_delay,
	                    synapse_controller.syndrv_timings.setup_precharge,
	                    synapse_controller.syndrv_timings.write_delay);

	set_syn_cnfg(h, synarray, synapse_controller.cnfg_reg);
	set_stdp_lut(h, synarray, synapse_controller.lut);
	set_syn_rst(h, synarray, synapse_controller.syn_rst);
	set_syn_ctrl(h, synarray, synapse_controller.ctrl_reg);
}

HALBE_GETTER(SynapseController, get_synapse_controller,
	Handle::HICANN&, h,
	SynapseArrayOnHICANN const&, synarray)
{
	HICANN::SynapseController returnvalue;

	ReticleControl const& reticle = *h.get_reticle();
	HicannCtrl::Synapse const index =
		synarray.isTop() ? HicannCtrl::SYNAPSE_TOP : HicannCtrl::SYNAPSE_BOTTOM;
	SynapseControl& sc = reticle.hicann[h.jtag_addr()]->getSC(index);

	std::array<uint, 3> data;
	sc.get_sram_timings(data[0], data[1], data[2]);
	returnvalue.syndrv_timings = HICANN::SRAMControllerTimings(
		SRAMReadDelay(data[0]), SRAMSetupPrecharge(data[1]), SRAMWriteDelay(data[2]));

	returnvalue.ctrl_reg = get_syn_ctrl(h, synarray);
	returnvalue.cnfg_reg = get_syn_cnfg(h, synarray);
	returnvalue.status_reg = get_syn_status(h, synarray);
	returnvalue.lut = get_stdp_lut(h, synarray);
	// TODO: Read SYN_CORR register
	returnvalue.syn_rst = get_syn_rst(h, synarray);

	return returnvalue;
}

HALBE_SETTER_GUARDED(EventResetCold,
	reset,
	Handle::HICANN &, hi,
	uint8_t const, PLL_frequency
) {
	static_cast<void>(hi);
	static_cast<void>(PLL_frequency);
	throw std::runtime_error("HICANN reset not implemented in this version, please use FPGA::reset for now.");
}


HALBE_SETTER(flush, Handle::HICANN&, h)
{
	// to flush all buffers, we have to do blocking reads from both HICANN tags
	ReticleControl& reticle = *h.get_reticle();
	HicannCtrl& hc = *reticle.hicann[h.jtag_addr()];

	// Blocking read from HICANN-ARQ tag 1
	if (hc.getSC(HicannCtrl::Synapse::SYNAPSE_TOP).driverbusy())
		// cannot happen... but let's check
		throw std::runtime_error("Wooot, the synapse driver is busy??? WTFASDFASDFASDF@#%$#TG$F");

	// Blocking read from HICANN-ARQ tag 0
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_TOP_LEFT).read_cfg(0);
	facets::ci_addr_t tmp0;
	unsigned int tmp1;
	hc.getLC(HicannCtrl::L1Switch::L1SWITCH_TOP_LEFT).get_read_cfg(tmp0, tmp1);
}


HALBE_SETTER_GUARDED(EventResetWarm,
	init,
	Handle::HICANN &, h,
	bool const, zero_synapses)
{
	ReticleControl& reticle = *h.get_reticle();
	HicannCtrl& hc = *reticle.hicann[h.jtag_addr()];

	hicann_init(hc, zero_synapses);
}


HALBE_GETTER(Status, get_hicann_status,
	Handle::HICANN &, h)
{
	ReticleControl& reticle = *h.get_reticle();

	Status returnvalue;

	uint8_t crc = 0;
	uint64_t id = 0, status = 0;

	reticle.jtag->set_hicann_pos(h.jtag_addr());
	reticle.jtag->read_id(id, reticle.jtag->pos_hicann);

	reticle.jtag->HICANN_read_crc_count(crc);
	reticle.jtag->HICANN_reset_crc_count();

	reticle.jtag->HICANN_read_status(status);

	returnvalue.setCRCCount(crc);
	//8 bit actually, but the function returns uint64... whatever...
	returnvalue.setStatusReg(static_cast<uint8_t>(status));
	returnvalue.setHardwareId(id);
	return returnvalue;
}

} // namespace HICANN
} //namespace HMF
