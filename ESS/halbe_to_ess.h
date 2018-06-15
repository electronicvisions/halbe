#pragma once

#include <map>
#include <string>
#include <array>
#include <vector>
#include <bitset>
#include <memory>
#include <iostream>

//ESS datatypes
#include "systemsim/HAL2ESSContainer.h"
//HALbe datatypes
#include "hal/DNCContainer.h"
#include "hal/FPGAContainer.h"
#include "hal/HMFUtil.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANNContainer.h"
#include "hal/HICANN/FGConfig.h"
#include "hal/HICANN/FGBlock.h"
#include "hal/HICANN/FGErrorResult.h"
#include "hal/HICANN/FGStimulus.h"
#include "hal/HICANN/STDPControl.h"

#include "hal/ADC/Config.h"
#include "hal/ADC/Status.h"
#include "hal/ADC/USBSerial.h"
//Handle
#include "hal/Handle/FPGA.h"
#include "hal/Handle/PMU.h"

// from hicann_system
#include "RealtimeComm.h"

// Logging
#include <log4cxx/logger.h>

//instantiating logger
static log4cxx::LoggerPtr _logger = log4cxx::Logger::getLogger("hal2ess");

//Forward declarations
class HALaccess;
class Stage2VirtualHardware;

namespace PyNNParameters
{
    struct EIF_cond_exp_isfa_ista;
}

namespace HMF
{
    namespace Handle
    {
	    struct HICANN;
	    struct FPGA;
        struct ADC;
    } //end namespace HANDLE
    namespace HICANN
    {
        class L1Address;
        struct SynapseDriver;
        struct FGControl;
        struct MergerTree;
        struct DNCMerger;
        struct GbitLink;
        struct DNCMergerLine;
    } //end namespace HICANN
} //end namespace HMF
//end forward Declarations

// functions with no effect in ESS (technical stuff like power, readout of fg values, repeater locking)
#define ESS_DUMMY() do { LOG4CXX_DEBUG(_logger, static_cast<std::string>(__func__) << " unsupported by ESS, -> function call has no effect" ); } while(false);

// functions not yet implemented in ESS, which might be implemented in the future
#define ESS_NOT_IMPLEMENTED() do { LOG4CXX_WARN(_logger, static_cast<std::string>(__func__) << " not implemented by ESS" ); } while(false);

namespace HMF
{

class HAL2ESS
{
public:
	HAL2ESS(Coordinate::Wafer wafer, std::string filepath = "");
	~HAL2ESS();

//functions controlling the simulation

	//initializes the simulation by calling the corresponding function of mvirtual_hw
	void initialize_sim();

	//setting the global parameters
	void set_speedup(double const speedup);

	//setting the timestep for the numerical simulation of the neuron 
	void set_timestep(double const dt);

	//enabling and setting the weight distortion
	void set_weight_distortion(bool const enable, double const distortion = 0.);

	// choose timed or non-timed merger
	void enable_timed_merger(bool const enable);

	//enable spike_debugging
	void enable_spike_debugging(bool const enable);

	/// set filename for lost event statistics
	void set_pulse_statistics_file(std::string file);

	/// Set path to directory containing calibration data as xml-files
	/// If not specified or path = "", the default calibration is used.
	void set_calib_path(std::string path);

	//runs the simulation by calling the corresponding function of mvirtual_hw
	void run_sim(long duration_in_ns);

//Functions of HICANNBackend
	//Crossbars and Switches
	void set_crossbar_switch_row(Handle::HICANN const& h, Coordinate::HLineOnHICANN y_, geometry::Side s, HICANN::CrossbarRow const & switches);
	HICANN::CrossbarRow get_crossbar_switch_row(Handle::HICANN const& h, Coordinate::HLineOnHICANN y_, geometry::Side s);

	void set_syndriver_switch_row(Handle::HICANN const& h, Coordinate::SynapseSwitchRowOnHICANN const& s, HICANN::SynapseSwitchRow const& switches);
	HICANN::SynapseSwitchRow get_syndriver_switch_row(Handle::HICANN const& h, Coordinate::SynapseSwitchRowOnHICANN const& s);

	//Synapses and Synapse drivers
	void set_weights_row(Handle::HICANN const& h, Coordinate::SynapseRowOnHICANN const& s, HICANN::WeightRow const& weights);
	HICANN::WeightRow get_weights_row(Handle::HICANN const& h, Coordinate::SynapseRowOnHICANN const& s);

	void set_decoder_double_row(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s, HICANN::DecoderDoubleRow const& data);
	HICANN::DecoderDoubleRow get_decoder_double_row(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s);

	void set_synapse_driver(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s, HICANN::SynapseDriver const& drv_row);
	HICANN::SynapseDriver get_synapse_driver(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s);

	//Setting the Neurons
	void set_denmem_quad(Handle::HICANN const& h, Coordinate::QuadOnHICANN qb, HICANN::NeuronQuad const& nquad);
	HICANN::NeuronQuad get_denmem_quad(Handle::HICANN const& h, Coordinate::QuadOnHICANN qb);

	//sets the configuration of a neuron block. only the capacity is needed for the ess
	void set_neuron_config(Handle::HICANN const& h, HICANN::NeuronConfig const& nblock);
	HICANN::NeuronConfig get_neuron_config(Handle::HICANN const& h);

	//Floating Gates
	HICANN::FGErrorResultRow wait_fg(Handle::HICANN &, Coordinate::FGBlockOnHICANN const &);
	HICANN::FGErrorResultQuadRow wait_fg(Handle::HICANN &);
	void set_fg_values(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& b, HICANN::FGBlock const& fg);
	void set_fg_values(Handle::HICANN const& h, HICANN::FGControl const& fg);
	HICANN::FGErrorResultQuadRow set_fg_row_values(Handle::HICANN & h, Coordinate::FGRowOnFGBlock row, HICANN::FGControl const& fg, bool const, bool const);
	HICANN::FGErrorResultQuadRow set_fg_row_values(
		Handle::HICANN & h,
		HICANN::FGRowOnFGBlock4 rows,
		HICANN::FGRow4 data,
		bool const writeDown,
		bool const blocking = true);
	HICANN::FGErrorResultQuadRow set_fg_row_values(Handle::HICANN & h, Coordinate::FGBlockOnHICANN block, Coordinate::FGRowOnFGBlock row,
	HICANN::FGRow const& fg, bool const writeDown, bool const blocking = true);

	void set_fg_config(Handle::HICANN & h, Coordinate::FGBlockOnHICANN const& block, const HICANN::FGConfig & config);
    HICANN::FGConfig get_fg_config(Handle::HICANN const&, Coordinate::FGBlockOnHICANN const&);

    //not a function of HICANNBackend anymore, but still here for tests
	HICANN::FGBlock get_fg_values(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& addr);
    //these functions read out a fg-cell via the analog output -> not possible in the ESS (use get_fg_values instead!)
	void set_fg_cell(Handle::HICANN const&, Coordinate::NeuronOnHICANN const&,  HICANN::neuron_parameter const&)    {ESS_DUMMY();}
	void set_fg_cell(Handle::HICANN const&, Coordinate::FGBlockOnHICANN const&, HICANN::shared_parameter const&)    {ESS_DUMMY();}
	void set_fg_cell(Handle::HICANN const&, Coordinate::FGBlockOnHICANN const&, Coordinate::FGCellOnFGBlock const&) {ESS_DUMMY();}

	//these functions set and get a current stimulus from a fg-cell
	void set_current_stimulus(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& b, HICANN::FGStimulus const& stim);
	HICANN::FGStimulus get_current_stimulus(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& b);

	//Repeater
	void set_repeater(Handle::HICANN const& h, Coordinate::VRepeaterOnHICANN r, HICANN::VerticalRepeater const& rc);
	HICANN::VerticalRepeater get_repeater(Handle::HICANN const& h, Coordinate::VRepeaterOnHICANN r);

	void set_repeater(Handle::HICANN const& h, Coordinate::HRepeaterOnHICANN r, HICANN::HorizontalRepeater const& rc);
	HICANN::HorizontalRepeater get_repeater(Handle::HICANN const& h, Coordinate::HRepeaterOnHICANN r);

	//ESS_DUMMY implemented, these functions are necessary for controlling test_events, as far as i know this functionality is not represented in the ESS
	void set_repeater_block(Handle::HICANN const&, Coordinate::RepeaterBlockOnHICANN, HICANN::RepeaterBlock const&){ESS_DUMMY();}
	HICANN::RepeaterBlock get_repeater_block(Handle::HICANN const&, Coordinate::RepeaterBlockOnHICANN){ESS_DUMMY();return HICANN::RepeaterBlock{};}

    void lock_repeater_and_synapse_driver(Handle::HICANN const&, Coordinate::RepeaterBlockOnHICANN const&, HICANN::RepeaterBlock){ESS_DUMMY();}
	//Merger Tree
	void set_merger_tree(Handle::HICANN const& h, HICANN::MergerTree const& m);
	HICANN::MergerTree get_merger_tree(Handle::HICANN const& h);

	void set_dnc_merger(Handle::HICANN const& h, HICANN::DNCMergerLine const& m);
	HICANN::DNCMergerLine get_dnc_merger(Handle::HICANN const& h);

	//this function sets directions for DNC <-> HICANN communication
	void set_gbit_link(Handle::HICANN const& h, HICANN::GbitLink const& link);

	typedef ::HMF::HICANN::Phase phase_t;
	void set_phase(Handle::HICANN const& h, phase_t phase);
	phase_t get_phase(Handle::HICANN const& h);

	//Background Generators
	void set_background_generator(Handle::HICANN const& h, HICANN::BackgroundGeneratorArray const& bg);
	HICANN::BackgroundGeneratorArray get_background_generator(Handle::HICANN const& h);

	//Analog output, configure analog output 
    void set_analog(Handle::HICANN const& h, HICANN::Analog const& a);
	HICANN::Analog get_analog(Handle::HICANN const& h);

	//STDP, not properly supported afaik -> ESS_NOT_IMPLEMENTED implemented
	void start_stdp(Handle::HICANN const&, geometry::SideVertical, HICANN::STDPControl){ESS_NOT_IMPLEMENTED();}
    
    void wait_stdp(Handle::HICANN const&, Coordinate::SideVertical, HICANN::STDPControl){ESS_NOT_IMPLEMENTED();}

	void stop_stdp(Handle::HICANN const&, geometry::SideVertical, HICANN::STDPControl){ESS_NOT_IMPLEMENTED();}

	void stdp_reset_capacitors(Handle::HICANN const&, Coordinate::SynapseRowOnHICANN const&, HICANN::STDPControl::corr_row){ESS_NOT_IMPLEMENTED();}

    HICANN::STDPControl::corr_row stdp_read_correlation(Handle::HICANN const&, Coordinate::SynapseRowOnHICANN const&)
	{
		ESS_NOT_IMPLEMENTED(); return HICANN::STDPControl::corr_row();
	}

	void set_stdp_config(Handle::HICANN const&, geometry::SideVertical, HICANN::STDPControl){ESS_NOT_IMPLEMENTED();}
    HICANN::STDPControl get_stdp_config(Handle::HICANN const&, geometry::SideVertical){ESS_NOT_IMPLEMENTED();return HICANN::STDPControl{};}

	//misc. stuff
	void init(Handle::HICANN const& h, bool zero_synapses = true);

    void flush(Handle::HICANN const&){ESS_DUMMY();}

    void instantiate_hicann(Coordinate::HICANNOnWafer const& h);

	void reset(Handle::HICANN const& h, uint8_t PLL_frequency = 100);

	//gets the CRC-Error register of the HICANN, afaik not supported by the ESS
	HICANN::Status get_hicann_status(Handle::HICANN const&) {ESS_DUMMY();return HICANN::Status{};}
//end namespace HICANN

//functions of DNCBackend, ESS_DUMMY implemented
    void reset(Handle::FPGA &, Coordinate::DNCOnFPGA const&){ESS_DUMMY()};
	DNC::Status get_dnc_status(Handle::FPGA &, Coordinate::DNCOnFPGA const&, Coordinate::HICANNOnDNC const&){ESS_DUMMY();return DNC::Status{};}
	void set_hicann_directions(Handle::FPGA &, Coordinate::DNCOnFPGA const&, DNC::GbitReticle const&);
	void set_loopback(Handle::FPGA &, Coordinate::DNCOnFPGA const&, DNC::Loopback const&){ESS_NOT_IMPLEMENTED();}

//functions of FPGABackend
    void reset(Handle::FPGA const&);
    void set_fpga_background_generator(Handle::FPGA const& f, Coordinate::DNCOnFPGA const d, FPGA::BackgroundGenerator const& bg);
    void write_playback_pulses(Handle::FPGA const& f, FPGA::PulseEventContainer const& st, FPGA::PulseEvent::spiketime_t runtime, uint16_t fpga_hicann_delay);
	bool get_pbmem_buffering_completed(Handle::FPGA & f);
	FPGA::AlmostSortedPulseEvents read_trace_pulses(Handle::FPGA const& f, FPGA::PulseEvent::spiketime_t runtime, bool drop_background_events = false);
    //dummy implementetions, not needed for ESS afaik
    void reset(Handle::FPGA const&, FPGA::Reset const&){ESS_DUMMY();}
    void reset_pbmem(Handle::FPGA const&){ESS_DUMMY();}
	void init(Handle::FPGA&, bool const = true){ ESS_DUMMY();}
	FPGA::Status get_fpga_status(Coordinate::FPGAGlobal const&){ESS_DUMMY();return FPGA::Status{};}
	void fill_pulse_fifo(Handle::FPGA &, Coordinate::DNCOnFPGA const&, FPGA::PulseEventContainer const&){ESS_NOT_IMPLEMENTED();}
	FPGA::PulseEventContainer read_trace_fifo(Handle::FPGA &, Coordinate::DNCOnFPGA const&){ESS_NOT_IMPLEMENTED();return FPGA::PulseEventContainer{};}
	void prime_systime_counter(Handle::FPGA const&){ESS_DUMMY();}
	void disable_global(Handle::FPGA const&){ESS_DUMMY();}
	void start_systime_counter(Handle::FPGA const&){ESS_DUMMY();}
	void prime_experiment(Handle::FPGA const&){ESS_DUMMY();}
	void start_experiment(Handle::FPGA const& f);
    void start_playback_and_trace_fifo(Handle::FPGA &, Coordinate::DNCOnFPGA const&, bool){ESS_NOT_IMPLEMENTED();}
    void start_trace_fifo(Handle::FPGA &, Coordinate::DNCOnFPGA const&){ESS_NOT_IMPLEMENTED();}
    void stop_playback_and_trace_fifo(Handle::FPGA &, Coordinate::DNCOnFPGA const& ){ESS_NOT_IMPLEMENTED();}
    void set_spinnaker_receive_port(Handle::FPGA const&, uint16_t){ESS_NOT_IMPLEMENTED();}
    void set_spinnaker_routing_table(Handle::FPGA const&, FPGA::SpinnRoutingTable const&){ESS_NOT_IMPLEMENTED();}
    void set_spinnaker_pulse_upsampler(Handle::FPGA const&, size_t){ESS_NOT_IMPLEMENTED();}
    void set_spinnaker_pulse_downsampler(Handle::FPGA const&, size_t){ESS_NOT_IMPLEMENTED();}
    void add_spinnaker_pulse(Handle::FPGA const&, FPGA::SpinnInputAddress_t const&){ESS_NOT_IMPLEMENTED();}
    void send_spinnaker_pulses(Handle::FPGA const &){ESS_NOT_IMPLEMENTED();}
	void send(Handle::FPGA &, Coordinate::DNCOnFPGA const&, FPGA::PulseEventContainer const&, bool, uint64_t){ESS_NOT_IMPLEMENTED();}
	void send(Handle::FPGA const&, FPGA::SpinnakerEventContainer const&){ESS_NOT_IMPLEMENTED();}
	FPGA::PulseEventContainer receive(Handle::FPGA &, Coordinate::DNCOnFPGA const&, uint64_t){ESS_NOT_IMPLEMENTED();return FPGA::PulseEventContainer{};}
	FPGA::PulseEventContainer send_and_receive(Handle::FPGA &, Coordinate::DNCOnFPGA const&, FPGA::PulseEventContainer const&, bool, uint64_t)
	{ESS_NOT_IMPLEMENTED();return FPGA::PulseEventContainer{};}
	void set_fpga_background_generator(Coordinate::FPGAGlobal const&, FPGA::BackgroundGenerator const&){ESS_NOT_IMPLEMENTED();}
    FPGA::SpinnOutputAddress_t get_received_spinnaker_pulse(Handle::FPGA const &){ESS_NOT_IMPLEMENTED();return FPGA::SpinnOutputAddress_t{};}
    void set_spinnaker_sender_config(Handle::FPGA const&, FPGA::SpinnSenderConfig const&){ESS_NOT_IMPLEMENTED();}
    void set_spinnaker_address_config(Handle::FPGA const&, FPGA::SpinnAddressConfig const&){ESS_NOT_IMPLEMENTED();}
    void send_realtime_pulse(Handle::FPGA const &, FPGA::SpinnInputAddress_t){ESS_NOT_IMPLEMENTED();}
    void send_spinnaker_realtime_pulse(Handle::FPGA const &, Realtime::spike_h){ESS_NOT_IMPLEMENTED();}
    void send_custom_realtime_pulse(Handle::FPGA const &, Realtime::spike){ESS_NOT_IMPLEMENTED();}
    std::vector<FPGA::SpinnOutputAddress_t> get_received_realtime_pulses(Handle::FPGA const &){ESS_NOT_IMPLEMENTED();return std::vector<FPGA::SpinnOutputAddress_t>{};}
    FPGA::SpinnOutputAddress_t spin_and_get_next_realtime_pulse(Handle::FPGA const &){ESS_NOT_IMPLEMENTED();return FPGA::SpinnOutputAddress_t{};}
    Realtime::spike spin_and_get_next_realtime_pulse_as_custom(Handle::FPGA const &){ESS_NOT_IMPLEMENTED();return Realtime::spike{0u, 0u, 0u, 0u};}
    Realtime::spike_h spin_and_get_next_realtime_pulse_as_spinnaker(Handle::FPGA const &){ESS_NOT_IMPLEMENTED();return Realtime::spike_h{0u};}
	void queue_spinnaker_realtime_pulse(Handle::FPGA &, Realtime::spike_h){ESS_NOT_IMPLEMENTED();};

//Functions of ADCBackend
	typedef uint16_t raw_type;
	typedef std::vector<raw_type> raw_data_type;

    ADC::Status get_status(Handle::ADC & h);	
    void config(Handle::ADC & h, ADC::Config const& cfg);
    void prime(Handle::ADC & h);
    double get_sample_rate(Handle::ADC &);
    raw_data_type get_trace(Handle::ADC & h);
    ADC::USBSerial get_board_id(Handle::ADC & h);
    void trigger_now(Handle::ADC &){ESS_DUMMY();}
    float get_temperature(Handle::ADC &);

//Functions of SupportBackend
    void set_hicann_reset(Coordinate::IPv4 const&, Coordinate::HICANNGlobal const&, bool){ESS_DUMMY();}
	void set_reticle_power(Coordinate::IPv4 const&, Coordinate::DNCGlobal const&, bool){ESS_DUMMY();}
    HMF::Support::Power::ReticleStatus get_reticle_status(HMF::Coordinate::IPv4 const&, HMF::Coordinate::DNCGlobal const&)
	{ESS_DUMMY();return HMF::Support::Power::ReticleStatus();}
    HMF::Support::Power::PsbVoltages get_psb_voltages(HMF::Coordinate::IPv4 const&)
    {ESS_DUMMY();return HMF::Support::Power::PsbVoltages();}
    void voh_up(Handle::PMU &){ESS_DUMMY();}
	void voh_down(Handle::PMU &){ESS_DUMMY();}
	void vol_up(Handle::PMU &){ESS_DUMMY();}
	void vol_down(Handle::PMU &){ESS_DUMMY();}
	void resetVoltages(Handle::PMU &){ESS_DUMMY();}
	void set_L1_voltages(Handle::HICANN &, float, float){ESS_DUMMY();}
	HMF::Support::Power::SystemTemperatures get_system_temperatures(HMF::Coordinate::IPv4 const&)
    {ESS_DUMMY();return HMF::Support::Power::SystemTemperatures();}

// Public non-halbe funcions
    // retunrs the adex-model-parameter of nrn
    PyNNParameters::EIF_cond_exp_isfa_ista get_bio_parameter(Handle::HICANN const& h, Coordinate::NeuronOnHICANN const& nrn ) const;
    PyNNParameters::EIF_cond_exp_isfa_ista get_technical_parameter(Handle::HICANN const& h, Coordinate::NeuronOnHICANN const& nrn ) const;

private:
//functions that get data from the ESS
	unsigned int get_hic_id_ESS(Handle::HICANN const& h) const;

	unsigned int get_hic_x_ESS(Handle::HICANN const& h) const;

	unsigned int get_hic_y_ESS(Handle::HICANN const& h) const;

    HICANN::L1Address get_L1Address_ESS(Handle::HICANN const& h, Coordinate::NeuronOnHICANN const& neuron) const;

	HICANN::CrossbarRow get_crossbar_switch_row_ESS(Handle::HICANN const& h, Coordinate::HLineOnHICANN y_, geometry::Side s) const;

    HICANN::BackgroundGeneratorArray get_background_generator_ESS(Handle::HICANN const& h) const;

    HICANN::HorizontalRepeater get_repeater_ESS(Handle::HICANN const& h, Coordinate::HRepeaterOnHICANN r) const;

    HICANN::VerticalRepeater get_repeater_ESS(Handle::HICANN const& h, Coordinate::VRepeaterOnHICANN r) const;
    
    HICANN::SynapseSwitchRow get_syndriver_switch_row_ESS(Handle::HICANN const& h, Coordinate::SynapseSwitchRowOnHICANN const& s) const;

    HICANN::SynapseDriver get_synapse_driver_ESS(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s) const;

    HICANN::DecoderDoubleRow get_decoder_double_row_ESS(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s) const;

    HICANN::WeightRow get_weights_row_ESS(Handle::HICANN const& h, Coordinate::SynapseRowOnHICANN const& s) const;

    HICANN::FGBlock get_fg_values_ESS(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& addr) const;

//constants
	static const int num_wafer = 1;		//this must be 1 because the ess does not support more than 1 wafer yet
//Member

	Coordinate::Wafer mWafer;     ///< wafer id
	const unsigned int mNumFPGAs; ///< number of FPGAs: 12 for Virtex, 48 for Kintex
	bool                                    mrun;           //flag to indicate whether ESS was executed yet
	std::string                             mfilepath;  ///< directory where all temporary files during simulation and debug goes to.
    std::unique_ptr<HALaccess>				mhal_access;
	std::unique_ptr<Stage2VirtualHardware>	mvirtual_hw;
	std::string                             mpulse_statistics_file;  // file to which summary of lost event logger shall be written
	// The number of FPGAs is variable: 12 for Virtex and 48 for Kintex systems
	// PulseContainer for recording FPGAEvents
	std::vector<FPGA::PulseEventContainer> mFPGAdata;
	// Configdata for the FPGAs
	std::vector<ESS::fpga_config> mFPGAConfig;
	// Configdata for the DNCs
	std::array<ESS::dnc_config,48> mDNCConfig;
//functions
    HALaccess const& getAccess() const
	{return *mhal_access;}
//friends
	friend class ESSTest;
};

}//end namespace HMF

#undef ESS_DUMMYf135G
