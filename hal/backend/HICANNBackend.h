#pragma once

#include <boost/shared_ptr.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANN.h"
#include "hal/HMFUtil.h"

namespace HMF {

namespace HICANN {

//Crossbars and Switches

/**
 * Sets crossbar switches in a row-wise fashion
 *
 * @param s        SynapseDriver address
 * @param switches Actual switch configuration for a single horizontal line
 *        on one side. switches[0] corresponds to lowest switchable vertical
 *        line number, switches[3] - to the highest switchable line of the row
 */
void set_crossbar_switch_row(
	Handle::HICANN & h,
	Coordinate::HLineOnHICANN const& y,
	Coordinate::Side const& s,
	CrossbarRow const& switches);

CrossbarRow get_crossbar_switch_row(
	Handle::HICANN & h,
	Coordinate::HLineOnHICANN const& y,
	Coordinate::Side const& s);


/**
 * Sets synapse driver switches in a row-wise fashion
 *
 * @param s        SynapseDriver address from 0 to 447
 * @param switches Actual switch configuration for a single horizontal line
 *        on one side. switches[0] corresponds to lowest switchable vertical
 *        line number, switches[15] - to the highest switchable line of the row
 */
void set_syndriver_switch_row(
	Handle::HICANN & h,
	Coordinate::SynapseSwitchRowOnHICANN const& s,
	SynapseSwitchRow const& switches);

SynapseSwitchRow get_syndriver_switch_row(
	Handle::HICANN & h,
	Coordinate::SynapseSwitchRowOnHICANN const& s);



// Synapses and Synapse drivers

/**
 * set weights of a single line (one synapse driver drives 2 lines)
 *
 * @param s       SynapseRow address
 * @param weights Actual 4-bit weight data in a 256-field-array, weight[0]
 *        corresponds to the leftmost synapse, weight[255] - rightmost
 */
void set_weights_row(
	Handle::HICANN & h,
	Coordinate::SynapseRowOnHICANN const& s,
	WeightRow const& weights);

#ifndef PYPLUSPLUS
/**
 * HICANN-parallel "weights row" setter to optimize writing speed.
 *
 * The vector parameters correspond to the scalar function's parameters (cf. above).
 *
 * @notice Performance-optimized function has not been exposed to Python.
 */
void set_weights_row(
	std::vector<boost::shared_ptr<Handle::HICANN> > handles,
	Coordinate::SynapseRowOnHICANN const& s,
	std::vector<WeightRow> const& data);
#endif // !PYPLUSPLUS

WeightRow get_weights_row(
	Handle::HICANN & h,
	Coordinate::SynapseRowOnHICANN const& s);


/**
 * Sets 4-bit decoder values for a double line (both lines driven by the same synapse driver)
 *
 * @param s    SynapseDriver Address
 * @param data Decoder values, data[0] is the top row GEOMETRICALLY (upper and lower halves!)
 */
void set_decoder_double_row(
	Handle::HICANN & h,
	Coordinate::SynapseDriverOnHICANN const& s,
	DecoderDoubleRow const& data);

#ifndef PYPLUSPLUS
/**
 * HICANN-parallel "decoder double row" setter to optimize writing speed.
 *
 * The vector parameters correspond to the scalar function's parameters (cf. above).
 *
 * @notice Performance-optimized function has not been exposed to Python.
 */
void set_decoder_double_row(
	std::vector<boost::shared_ptr<Handle::HICANN> > handles,
	Coordinate::SynapseDriverOnHICANN const& syndrv,
	std::vector<DecoderDoubleRow> const& data);
#endif // !PYPLUSPLUS

DecoderDoubleRow get_decoder_double_row(
	Handle::HICANN & h,
	Coordinate::SynapseDriverOnHICANN const& s);


/**
 * Configures a synapse driver.
 *
 * @param s       SynapseDriver address
 * @param drv_row config data
 *
 * @note  Two rows of synapse configurations are written to the hardware. Note that
 *        these rows are characterized as "top" and "bottom" from the geometrical
 *        point of view, not as it is characterized in hardware description!
 *
 * @note the config data also has a container for the synapses in the associated rows.
 *       However, it is currently not used for synapse configuration in this method.
 *       Instead, function set_weights_row rsp set_decoder_dobule_row is used.
 */
void set_synapse_driver(
	Handle::HICANN & h,
	Coordinate::SynapseDriverOnHICANN const& s,
	SynapseDriver const& drv_row);

SynapseDriver get_synapse_driver(
	Handle::HICANN & h,
	Coordinate::SynapseDriverOnHICANN const& s);



// Setting the Neurons

/**
 * Sets denmem values in blocks of four denmems. One neighboring even and odd
 * neuron plus their opposing counterparts from the other anncore constitute
 * to one quad block
 *
 * @param qb    Address of quadblock relative to HICANN, ranges from 0 to 127
 * @param nquad Denmem configuration data for one quad.
 */
void set_denmem_quad(
	Handle::HICANN & h,
	Coordinate::QuadOnHICANN const& qb,
	NeuronQuad const& nquad);

/**
 * Read back denmem configurations for one NeuronQuad from hardware.
 * Note, that due to a hardware bug, values can't be read reliably, see:
 * https://brainscales-r.kip.uni-heidelberg.de/issues/818 .
 *
 * @param h     HICANN Handle
 * @param qb    Address of quadblock, ranges from 0 (left) to 15 (right)
 */
NeuronQuad get_denmem_quad(
	Handle::HICANN & h,
	Coordinate::QuadOnHICANN const& qb);


/**
 * Sets hardware neuron parameters such as capacity size, leakage etc.
 * Also resets the neurons and the output buffers
 *
 * @param nblock Configuration data for the whole neuron block
 */
void set_neuron_config(Handle::HICANN & h, NeuronConfig const& nblock);
NeuronConfig get_neuron_config(Handle::HICANN & h);



// Floating Gates

/**
 * Block and wait until floating gate block(s) become(s) idle.
 */
HICANN::FGErrorResultRow wait_fg(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const & b);
HICANN::FGErrorResultQuadRow wait_fg(Handle::HICANN & h);

/**
 * Sets both analog floating gate values and FG configuration for the FG blocks.
 *
 * @param fg   Data struct
 *
 * @note As the FGBlockOnHICANN struct has the FGBlockOnHICANN coordinate, it is not neccessary to
 *       give it to the function. Use FGControl::extract_block to get FGBlockOnHICANN right
 */
void set_fg_values(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& b, FGBlock const& fgb);
void set_fg_values(Handle::HICANN & h, FGControl const& fg);
FGBlock get_fg_values(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& b);

/**
 * Writes floating gate values for a row on all blocks in parallel
 *
 * @param row Row of the FGBlocks to configure
 * @param fg   Data struct
 * @param writeDown if true write FG down, otherwise up
 * @param blocking  if true wait for fg controller getting idle
 *
 * @note As the FGBlockOnHICANN struct has the FGBlockOnHICANN coordinate, it is not neccessary to
 *       give it to the function. Use FGControl::extract_block to get FGBlockOnHICANN right.
 * @note When using non-blocking mode make sure to call wait_fg(h) before
 *       continuing with next row. [ECM: This will change when writing via pbmem.]
 *
 * @return FG controller error results for all 4 rows of the 4 blocks (if
 *         blocking == true, else no-error result))
 */
HICANN::FGErrorResultQuadRow set_fg_row_values(Handle::HICANN & h, Coordinate::FGRowOnFGBlock row,
	FGControl const& fg, bool const writeDown, bool const blocking = true);

/**
 * Writes floating gate values for different rows on all blocks in parallel
 *
 * @param rows row of the 4FGBlocks to configure
 * @param data fg_values for the 4 blocks
 * @param writeDown if true write FG down, otherwise up
 * @param blocking  if true wait for fg controller getting idle
 *
 * @note As the FGBlockOnHICANN struct has the FGBlockOnHICANN coordinate, it is not neccessary to
 *       give it to the function. Use FGControl::extract_block to get FGBlockOnHICANN right.
 * @note When using non-blocking mode make sure to call wait_fg(h) before
 *       continuing with next row. [ECM: This will change when writing via pbmem.]
 */
HICANN::FGErrorResultQuadRow set_fg_row_values(
	Handle::HICANN & h,
	FGRowOnFGBlock4 rows,
	FGRow4 const& data,
	bool const writeDown,
	bool const blocking = true);

/**
 * Writes floating gate values for a row on all blocks on a single block
 *
 * @param block FGBlock to configure
 * @param row Row of the FGBlocks to configure
 * @param fg   Data struct
 * @param writeDown if true write FG down, otherwise up
 * @param blocking  if true wait for fg controller getting idle
 *
 * @note As the FGBlockOnHICANN struct has the FGBlockOnHICANN coordinate, it is not neccessary to
 *       give it to the function. Use FGControl::extract_block to get FGBlockOnHICANN right.
 * @note When using non-blocking mode make sure to call wait_fg(h) before
 *       continuing with next row. [ECM: This will change when writing via pbmem.]
 *
 * @return FG controller error results for all 4 rows of the 4 blocks (if
 *         blocking == true, else no-error result))
 */
HICANN::FGErrorResultQuadRow set_fg_row_values(Handle::HICANN & h,
	Coordinate::FGBlockOnHICANN block, Coordinate::FGRowOnFGBlock row,
	FGRow const& fg, bool const writeDown, bool const blocking = true);

// TODO change to something like:
// void set_fg_values(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& addr, FGControll );

/**
 * Sets the config of the floating gate controller
 *
 * @param config Config
 * @param block to configure
 *
 */
void set_fg_config(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& block, const FGConfig & config);
FGConfig get_fg_config(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& b);

// the following commands configure the FGBlock to connect a specific FGCell to
// the input of the corresponding analog out.
void set_fg_cell(Handle::HICANN & h, Coordinate::NeuronOnHICANN const& n, neuron_parameter const&  p);
void set_fg_cell(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& b, shared_parameter const& p);
void set_fg_cell(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& b, Coordinate::FGCellOnFGBlock const& c);

/**
 * Activates neuron stimulation by using current generator of the FGBlockOnHICANN module.
 * Can run once or continuously with 129 current values programmed in RAM.
 * The time resolution is defined within the FGStimulus struct
 *
 * @param stim       129 10-bit current values and pulse length for time resolution
 * @param continuous Run once or continuously
 *
 * @note As the FGStimulus struct has the FGBlockOnHICANN coordinate, it is not neccessary to
 *       give it to the function. Use FGControl::extract_stimulus to get FGStimulus right
 */
void set_current_stimulus(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& b, FGStimulus const& stim);
FGStimulus get_current_stimulus(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& b);



// Repeater

/**
 * Sets one vertical repeater: direction, mode, enable and crosstalk compensation
 *
 * @param r Vertical repeater coordinate
 * @param rc Config data
 */
void set_repeater(
	Handle::HICANN & h,
	Coordinate::VRepeaterOnHICANN const& r,
	VerticalRepeater const& rc);
VerticalRepeater get_repeater(
	Handle::HICANN & h,
	Coordinate::VRepeaterOnHICANN const& r);


/**
 * Sets one horizontal repeater: direction, mode, enable and crosstalk compensation
 *
 * @param r Horizontal repeater coordinate
 * @param rc Config data
 */
void set_repeater(
	Handle::HICANN & h,
	Coordinate::HRepeaterOnHICANN const& r,
	HorizontalRepeater const& rc);
HorizontalRepeater get_repeater(
	Handle::HICANN & h,
	Coordinate::HRepeaterOnHICANN const& r);


/**
 * Configures a repeater block. Controls test output/input functionality and
 * DLL/Synapse Driver reset bits
 *
 * @param addr Block coordinate
 * @param rbc  Configuration data
 */
void set_repeater_block(
	Handle::HICANN & h,
	Coordinate::RepeaterBlockOnHICANN const& addr,
	RepeaterBlock const& rbc);


/**
 * Fetches the results of the test input an the full flags. Returns them
 * in form of a RepeaterBlock. DOES NOT read any other information from the
 * hardware as it is not possible. As a Consequence, there is no way of determining
 * if set_repeater_block() works correctly!
 *
 * @param addr Block coordinate
 *
 * @note Every repeater block has two TDI/TDO registers. They are physically bound to
 * certain horizontal/vertical lines. The registers serve only odd/even repeaters respectively
 * and can be accessed as e.g. tdi_data[RepeaterBlock::EVEN] or tdi_data[RepeaterBlock::ODD]
 * Following constellation is correct:
 * Bottom left repeater block serves  Vwires 0, 4, 8,       ... 124 through EVEN registers
 *                               and  Vwires 2, 6, 10,      ... 126 through ODD registers
 * Top left repeater block serves     Vwires 1, 5, 9,       ... 125 through ODD registers
 *                               and  Vwires 3, 7, 11,      ... 127 through EVEN registers
 * Bottom right repeater block serves Vwires 131, 135, 139, ... 255 through EVEN registers
 *                               and  Vwires 129, 133, 137, ... 253 through ODD registers
 * Top right repeater block serves    Vwires 128, 132, 136, ... 252 through EVEN registers
 *                               and  Vwires 130, 134, 138, ... 254 through ODD registers
 * Center left repeater block serves  Hwires 2, 6, 10,      ... 62 through EVEN registers
 *                               and  Hwires 0, 4, 8,       ... 60 through ODD registers
 * Center right repeater block serves Hwires 1, 5, 9,       ... 61 through ODD registers
 *                               and  Hwires 3, 7, 11,      ... 63 through EVEN registers
 */
RepeaterBlock get_repeater_block(
	Handle::HICANN & h,
	Coordinate::RepeaterBlockOnHICANN const& addr);


/**
 * Pull the DLL and Synapse driver reset in the repeater block
 *
 * @warning use the same data that was put into set_repeater_block
 * @param addr Block coordinate
 * @param rbc  Configuration data
 */
void lock_repeater_and_synapse_driver(
	Handle::HICANN & h,
	Coordinate::RepeaterBlockOnHICANN const& block,
	HICANN::RepeaterBlock rbc);


// Merger Tree

/**
 * Addressing
 * ----------
 *
 * The merger on the background merger level which recieves inputs from the
 * most left output buffer (addr 0) which is also the output buffer with
 * denmems 0-31 is considered to be merger BG0. Merger BG7 is the one
 * receiving input from Output Buffer 7.
 * Merger addresses for the other mergers on other levels in the merger tree
 * hierarchy are incremented from 0 in the same direction as the background
 * merger level. The same is true for DNC Mergers, neuron out register and
 * spl1 repeater addresses.
 * Therefore, the spl1 repeater at address 0 projects onto the horizontal
 * L1 bus with address 62.
 */

/**
 * Configures complete merger tree (excluding DNC mergers)
 *
 * @param m Configuration data
 */
void set_merger_tree(Handle::HICANN & h, MergerTree const& m);
MergerTree get_merger_tree(Handle::HICANN & h);

/**
 * Sets all DNC Mergers
 *
 * @param m Configuration data
 */
void set_dnc_merger(Handle::HICANN & h, DNCMergerLine const& m);
DNCMergerLine get_dnc_merger(Handle::HICANN & h);

/**
 * Sets correct directions for DNC<->HICANN communication in the HICANN.
 * SPL1Control. Not gettable.
 *
 * @param d Configuration: directions for all 8 channels of this HICANN
 * @note: For the DNC side use DNC::set_hicann_directions(..)
 */
void set_gbit_link(Handle::HICANN & h, GbitLink const& d);

/**
 * Set phases of all 8 output buffers. (Data from output buffers can be sampled
 * at rising (phase=0) or falling (phase=1) edge of the clock.) Default value is 0
 * Addressing is derived from global addressing above: phase[0] corresponds to
 * the output buffer 0 etc.
 *
 * @param phase Phase values
 */
void set_phase(Handle::HICANN & h, Phase phase = 0x00);
Phase get_phase(Handle::HICANN & h);



// Background Generators

/**
 * Configures all available 8 background generators at once
 *
 * Addressing
 * ----------
 *
 * The background generator projecting on the same merger as the
 * most left output buffer (addr 0) is supposed to be BG[0] and the bg
 * generator projecting on the same merger as output buffer 7 is supposed to
 * be BG[7]
 *
 * @param bg Configuration data for all 8 available background generators
 */
void set_background_generator(Handle::HICANN & h, BackgroundGeneratorArray const& bg);
BackgroundGeneratorArray get_background_generator(Handle::HICANN & h);



// Analog Outputs

/**
 * Configures both analog outputs
 *
 * @param a Configuration data
 */
void set_analog(Handle::HICANN & h, Analog const& a);
Analog get_analog(Handle::HICANN & h);



// STDP

/**
 * Starts automated processing of STDP for array of synapses
 *
 * @param y Side of HICANN: there is one STDP controller per side
 * @param c Configuration struct containing STDP parameters such as:
 * continuous/single update flag, first and last synapse rows, reset bit
 */
void start_stdp(
	Handle::HICANN & h,
	Coordinate::SideVertical const& y,
	STDPControl const& c);

void wait_stdp(Handle::HICANN & h, Coordinate::SideVertical y, STDPControl c);


/**
 * Stops automated processing of STDP for array of synapses
 *
 * @param y Side of HICANN: there is one STDP controller per side
 * @param c Configuration struct, necessary only for the consistency of
 * the getter functionality
 */
void stop_stdp(
	Handle::HICANN & h,
	Coordinate::SideVertical const& y,
	STDPControl const& c);


/**
 * Resets STDP capacitors of a single synapse line
 *
 * @param y Side of HICANN: there is one STDP controller per side
 * @param s SynapseDriver address
 * @param line Within one driver: either 0 (top) or 1 (bottom) GEOMETRICALLY
 * @param r reset configuration of the synapse row
 */
void stdp_reset_capacitors(
	Handle::HICANN & h,
	Coordinate::SynapseRowOnHICANN const& s,
	STDPControl::corr_row const& r);


/**
 * Reads out the correlation bits of the synapses
 *
 * @param y Side of HICANN: there is one STDP controller per side
 * @param s SynapseDriver address
 * @param line Within one driver: either 0 (top) or 1 (bottom) GEOMETRICALLY
 * @return results of correlation evaluation for the synapse row
 */
STDPControl::corr_row stdp_read_correlation(
	Handle::HICANN & h,
	Coordinate::SynapseRowOnHICANN const& s);


/**
 * Sets digital parameters for STDP: LUT, evaluation bits, reset registers
 * Does not set analog patameters (V_m, V_clr, V_cla, V_thigh, V_tlow, V_br)
 * as they are written to floating gates with all other analog parameters
 *
 * @param y Side of HICANN: there is one STDP controller per side
 * @param c Configuration struct
 */
void set_stdp_config(
	Handle::HICANN & h,
	Coordinate::SideVertical const& y,
	STDPControl const& c);
STDPControl get_stdp_config(
	Handle::HICANN & h,
	Coordinate::SideVertical const& y);


/**
 * Reads out HICANN status register, CRC error register and other status-
 * relevant stuff. Also resets the CRC-counter.
 *
 * @return HICANN status object
 */
Status get_hicann_status(Handle::HICANN & h);


/**
 * Logical chip reset (excluding "zeroing" of the chip SRAM, etc.) -- "cold
 * reset".
 * In the current system this includes FPGA, JTAG, DNC and on-wafer the global
 * wafer reset.
 * @param PLL_frequency: optional setting of system frequency in MHz, default is 100
 * @note This resets ALL hicanns that are connected to the parent FPGA.
 */
void reset(Handle::HICANN & h, uint8_t PLL_frequency = 100);


/**
 * Flush HICANN communication channel
 *
 * This function blocks until HICANN communication channel is idle.
 * It implies something like a FPGA::Flush() (if only 1 HICANN is used), but for
 * multi-HICANN communication this is not the case.
 */
void flush(Handle::HICANN& f);


/**
 * Zero the chip SRAM -- some kind of "warm reset".
 */
void init(Handle::HICANN & h, bool const zero_synapses = true);


/**
 * Prepare HICANN for an experiment.
 * To be called after configuring the HICANN.
 * TODO: implement when needed (i.e. hicann sync, etc)
 */
//void prime(Handle::HICANN & h);


} // namespace HICANN
} // namespace HMF
