#pragma once

#include "hal/HICANN/STDPEval.h"
#include "hal/HICANN/STDPLUT.h"
#include "hal/HICANN/STDPTiming.h"
#include "hal/HICANN/STDPAnalog.h"

#include "hal/Coordinate/HMFGeometry.h"

namespace HMF {
namespace HICANN {

/**
 * STDP control struct: manages weight update Lookup-table, correlation
 * registers, auto-update process and reset behavior of synapses.
 * Each synapse block in HICANN has its own STDP-controller
 */
struct STDPControl
{
public:
	static const size_t
		NUMBER_SLICES = 8,
		SYNAPSES_PER_SLICE = 32,
		SYNAPSES_PER_ROW = 256,
		NUMBER_ROWS = 224;

	enum CorrType { //use to select according registers
		CAUSAL,
		ACAUSAL,
		COMBINED };

	//correlation-register for one synapse row. [0] - causal, [1] - acausal
	typedef std::array<bool, SYNAPSES_PER_ROW> corr_single_row;
	typedef std::array<corr_single_row, 2> corr_row;

	//lookup tables
	STDPLUT lut;

	//correlation evaluation patterns
	STDPEval eval;

	//analog parameters: V_m, V_clr, V_cla, V_thigh, V_tlow, V_br
	STDPAnalog analog;

	//parameters for timing: write del, output delay, precharge delay, shared enable bits
	STDPTiming timing;

	//result of correlation evaluation of the controller, can only be read from HW
	std::array<corr_row, NUMBER_ROWS> correlation_info;

	//reset behavior of synapses in case the according correlation flags are active
	std::array<corr_row, NUMBER_ROWS> reset_info;

	//hardware control register bits: used as start_stdp options
	bool without_reset;
	bool read_causal; //scc bit
	bool read_acausal; //sca bit
	bool continuous_autoupdate;

	STDPControl();

	uint8_t first_row() const { return m_first_row; }
	uint8_t last_row() const { return m_last_row; }

	std::bitset<8> first_row_bits() const { return std::bitset<8>(m_first_row); }
	std::bitset<8> last_row_bits() const { return std::bitset<8>(m_last_row); }

	//set first and last rows with the help of syndriver coordinates
	void set_first_row(Coordinate::SynapseDriverOnHICANN const& s, bool const line);
	void set_last_row(Coordinate::SynapseDriverOnHICANN const& s, bool const line);

	//set first and last rows with uints
	void set_first_row(uint8_t const row);
	void set_last_row(uint8_t const row);

	bool operator ==(STDPControl const& b) const;
	bool operator!=(STDPControl const& other) const { return !(*this == other); }

private:
	uint8_t m_first_row; //first row to start weight update process on, HW coordinates
	uint8_t m_last_row;  //last row for the update process, HW coordinates

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		using namespace boost::serialization;
		ar & make_nvp("lut", lut);
		ar & make_nvp("eval", eval);
		ar & make_nvp("analog", analog);
		ar & make_nvp("timing", timing);
		ar & make_nvp("correlation_info", correlation_info);
		ar & make_nvp("reset_info", reset_info);
		ar & make_nvp("without_reset", without_reset);
		ar & make_nvp("read_causal", read_causal);
		ar & make_nvp("read_acausal", read_acausal);
		ar & make_nvp("continuous_autoupdate", continuous_autoupdate);
		ar & make_nvp("first_row", m_first_row);
		ar & make_nvp("last_row", m_last_row);
	}

	friend std::ostream& operator<< (std::ostream& os, STDPControl const& o);
};

} // end namespace HMF
} // end namespace HICANN

