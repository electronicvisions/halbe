#pragma once

#include "hal/test.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANN/FGRow.h"
#include "pywrap/compat/rant.hpp"

namespace HMF {

// fwd dcl
namespace Handle { class HICANN; }

namespace HICANN {

/// Global Floating Gate parameters for one block on the left for Syndriver,
/// STDP and other global parameters outside AnnCore.
enum shared_parameter {
	//!<              WHERE    INDEX       EXPLANATION
	V_reset,      //!< both      0     voltage, membrane is pulled to during reset pulse (left:even, right:odd denmems) [NEURON]
	int_op_bias,  //!< both      1     internal OP bias
	V_dllres,     //!< both      2     dll ctrl voltage of receivers is pulled to this voltage during reset of pll
	V_bout,       //!< left      3     global biasing of neuron readout
	V_bexp,       //!< right     3     lower exp voltage driver bias
	V_fac,        //!< both      4     voltage V_fac used for short-term plasticity in facilitation mode [SYNDRIVER:STP]
	I_breset,     //!< both      5     current used to pull down membrane to reset potential(left:even, right:odd denmems) [NEURON]
	V_dep,        //!< both      6     voltage V_dep used for short-term plasticity in depression mode [SYNDRIVER:STP]
	I_bstim,      //!< both      7     bias for neuron stimulation circuit [NEURONSTIM]
	V_thigh,      //!< both      8     stdp readout compare voltageB [STDP]
	V_gmax3,      //!< both      9     sets synaptic conductance [SYNDRIVER]
	V_tlow,       //!< both     10     stdp readout compare voltage [STDP]
	V_gmax0,      //!< both     11     sets synaptic conductance [SYNDRIVER]
	V_clra,       //!< left     12     stdp clr voltage (acausal) [STDP]
	V_clrc,       //!< right    12     stdp clr voltage (causal) [STDP]
	V_gmax1,      //!< both     13     sets synaptic conductance [SYNDRIVER]
	V_stdf,       //!< both     14     stdf reset voltage [SYNDRIVER:STP]
	V_gmax2,      //!< both     15     sets synaptic conductance [SYNDRIVER]
	V_m,          //!< both     16     start load voltage of causal stdp capacitor, (ground for acausal) [STDP]
	V_bstdf,      //!< both     17     bias for short-term plasticity [SYNDRIVER:STP]
	V_dtc,        //!< both     19     bias for dtc in short-term plasticity circuit [SYNDRIVER:STP]
	V_br,         //!< both     21     bias for stdp readout circuit [STDP]
	V_ccas,       //!< both     23     biasing of l1 input amplifier + Vcbias
	__last_shared
};

enum neuron_parameter {
	//!< INDEXES:      LEFT   RIGHT        EXPLANATION
	E_l,        //!<     6      16     leakage reversal potential
	E_syni,     //!<    18       2     synapse inhibitory reversal potential
	E_synx,     //!<    16       0     synapse excitatory reversal potential
	I_bexp,     //!<     1      15     biasing for opamp of exponential circuit (technical)
	I_convi,    //!<     3       1     controls maximum inhibitory synaptic conductance/current
	I_convx,    //!<    17       3     controls maximum excitatory synaptic conductance/current
	I_fire,     //!<     7      19     current flowing onto adaptation capacitance at a spike -> Integrated for spikepulselength
	I_gl,       //!<    11      21     proportional to leakage conductance for small signals and maximum leakage current
	I_gladapt,  //!<     9      11     adaptation conductance modeling parameter a in Gerstner model
	I_intbbi,   //!<    21       7     bias for integrating opamp in inhibitory synapse input circuit(technical)
	I_intbbx,   //!<    19       5     bias for integrating opamp in excitatory synapse input circuit(technical)
	I_pl,       //!<    13       9     current for adjustment of refractory periode (integrated)
	I_radapt,   //!<    15      23     conductance for adjustment of adaptation time constant
	I_rexp,     //!<    23      13     exp_slope current - controls delta_T
	I_spikeamp, //!<     5      17     bias current of spike threshold comparator (technical)
	V_exp,      //!<    20       4     exp reference potential
	V_syni,     //!<     8      10     zero voltage of collecting line from synapse array(technical)
	V_syntci,   //!<    10       8     voltage controlling timeconstant of inhibitory synaptic pulse
	V_syntcx,   //!<    14      12     voltage controlling timeconstant of excitatory synaptic pulse
	V_synx,     //!<    22       6     zero voltage of collecting line from synapse array(technical)
	V_t,        //!<    12      14     membrane Voltage needed to detect a spike
// #ifdef HICANNv4
	V_convoffi, //!<     4      18     (HICANNv4) synaptic input bias correction
	V_convoffx, //!<     2      20     (HICANNv4) synaptic input bias correction
// #endif
	__last_neuron
};

extern int const not_connected;

// CK: TODO move to Coordinates?
neuron_parameter getNeuronParameter(Coordinate::FGBlockOnHICANN const& b,
		                            Coordinate::FGRowOnFGBlock const & r);
shared_parameter getSharedParameter(Coordinate::FGBlockOnHICANN const& b,
		                            Coordinate::FGRowOnFGBlock const & r);
Coordinate::FGRowOnFGBlock
getNeuronRow(Coordinate::FGBlockOnHICANN const& b, neuron_parameter p);
Coordinate::FGRowOnFGBlock
getSharedRow(Coordinate::FGBlockOnHICANN const& b, shared_parameter p);

/// Check if the given parameter is a current
bool isCurrentParameter(neuron_parameter p);
/// Check if the given parameter is a current
bool isCurrentParameter(shared_parameter p);
/// Check if the given parameter is a voltage
bool isVoltageParameter(neuron_parameter p);
/// Check if the given parameter is a voltage
bool isVoltageParameter(shared_parameter p);

std::string to_string(neuron_parameter p);
std::string to_string(shared_parameter p);

struct FGBlock
{
	static size_t const fg_blocks = 4;
	static size_t const fg_lines = 24;
	static size_t const fg_columns = 129;

	typedef uint16_t value_type;

	FGBlock(Coordinate::FGBlockOnHICANN const& b);
	PYPP_DEFAULT(FGBlock());
	PYPP_DEFAULT(FGBlock(FGBlock const&));
	PYPP_DEFAULT(FGBlock& operator=(FGBlock const&));

	value_type getShared(Coordinate::FGBlockOnHICANN const& b,
						 shared_parameter param) const;
	void setShared(Coordinate::FGBlockOnHICANN const& b,
				   shared_parameter param,
				   value_type const& val);

	// don't use this function unless you know what you are doing
	value_type getSharedRaw(size_t idx) const;
	void setSharedRaw(size_t idx, value_type const& val);

	/// Set raw values, coordinates as in HICANN::set_fg_cell
	value_type getRaw(size_t row, size_t column) const;
	void setRaw(size_t row, size_t column, value_type val);

	value_type getRaw(Coordinate::FGCellOnFGBlock cell) const;
	void setRaw(Coordinate::FGCellOnFGBlock , value_type val);

	value_type getNeuron(Coordinate::FGBlockOnHICANN const& b,
						 Coordinate::NeuronOnFGBlock const& nrn,
						 neuron_parameter np) const;
	void setNeuron(Coordinate::FGBlockOnHICANN const& b,
				   Coordinate::NeuronOnFGBlock const& nrn,
				   neuron_parameter np,
				   value_type const& val);

	// don't use this function unless you know what you are doing
	value_type getNeuronRaw(size_t col, size_t row) const;
	void setNeuronRaw(size_t col, size_t row, value_type const& val);

	FGRow getFGRow(Coordinate::FGRowOnFGBlock row) const;

	void setDefault(Coordinate::FGBlockOnHICANN const& b);

	static int getSharedHardwareIdx(Coordinate::FGBlockOnHICANN const&, shared_parameter const&);
	static int getNeuronHardwareIdx(Coordinate::FGBlockOnHICANN const&, neuron_parameter const&);

	bool operator== (FGBlock const& rhs) const;
	bool operator!= (FGBlock const& rhs) const;

#ifndef PYPLUSPLUS
private:
	// default FG values
	static std::array<std::pair<shared_parameter, value_type>,
		shared_parameter::__last_shared> const shared_default;
	static std::array<std::pair<neuron_parameter, value_type>,
		neuron_parameter::__last_neuron> const neuron_default;

public:
	// Lookup tables for parameter array positions
	typedef std::array<int, shared_parameter::__last_shared> shared_lut_t;
	typedef std::array<int, neuron_parameter::__last_neuron> neuron_lut_t;

private:
	static shared_lut_t const shared_lut_left;
	static shared_lut_t const shared_lut_right;

	static neuron_lut_t const neuron_lut_left;
	static neuron_lut_t const neuron_lut_right;

public:
	static bool is_left(Coordinate::FGBlockOnHICANN const& b);

	static shared_lut_t const& getSharedLut(Coordinate::FGBlockOnHICANN const& b);
	static neuron_lut_t const& getNeuronLut(Coordinate::FGBlockOnHICANN const& b);

	std::array<std::bitset<20>, 65>
	set_formatter(Coordinate::FGBlockOnHICANN const& b,
			  rant::integral_range<size_t, 23> const& row) const;
#endif // PYPLUSPLUS

	friend std::ostream& operator<< (std::ostream& os, FGBlock const& fgb);

	typedef std::array<rant::integral_range<value_type, 1023>, fg_lines> fg_t;

private:
	fg_t mShared;
	std::array<fg_t, fg_columns-1> mNeuron;

	Coordinate::FGBlockOnHICANN mCoordinate;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver & ar, unsigned int const);

	FRIEND_TEST(FGControl, Defaults);
	FRIEND_TEST(FGBlock, Range);
	FRIEND_TEST(FGBlock, Dimensions);
};


template<typename Archiver>
void FGBlock::serialize(Archiver & ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("shared", mShared)
	   & make_nvp("neuron", mNeuron);
}

} // HICANN
} // HMF
