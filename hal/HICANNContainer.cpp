#include "HICANNContainer.h"

#include <map>
#include <ostream>
#include <iterator>
#include <boost/optional/optional_io.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/serialization.hpp>
#include <bitter/bitter.h>

#include "hate/optional.h"

using namespace halco::hicann::v2;
using namespace halco::common;

namespace HMF {
namespace HICANN {

std::bitset<4> SynapseWeight::format() const
{
	return bit::reverse(std::bitset<4>(value()));
}

////////////////////////////////////////////////////////////////////////////////
// BackgroundGenerator

const size_t
	BackgroundGenerator::max_isi_random,
	BackgroundGenerator::max_isi_regular,
	BackgroundGenerator::min_isi_random,
	BackgroundGenerator::min_isi_regular;

void BackgroundGenerator::set_mode(
		bool random,
		uint32_t isi
		)
{
	if ( random )
	{
		if ( isi >  max_isi_random )
			throw std::runtime_error("Specified inter spike interval is too high. Maximum in random mode is 32768");
		else if ( isi <  min_isi_random)
			throw std::runtime_error("Specified inter spike interval is too low. Minimum in random mode is 4");
		mRandom = random;
		// for the RANDOM Case it is a little complicated:
		// at each clk it is compared whether the value of the LFSR register is higher than the set period.
		// If and only if the LFSR was below PERIOD in the clock cycle before, then a spike will be generated.
		// The PERIOD to mean-rate relation has been simulated in https://brainscales-r.kip.uni-heidelberg.de/projects/tns/wiki/PoissonSources
		// PERIOD has to be set in the following way:
		// PERIOD = 65535(1-2/isi)
		// The maximum rate was determined to be 0.25 * (sys_clk_cycle)^-1
		mPeriod = 65535*(1-2./isi);
	}
	else {
		if ( isi >  max_isi_regular)
			throw std::runtime_error("Specified inter spike interval is too high. Maximum in regular mode is 65536");
		else if ( isi <  min_isi_regular)
			throw std::runtime_error("Specified inter spike interval is too low. Minimum in regular mode is 2");
		mRandom = random;
		mPeriod = isi - 1; // spikes are generated every period+1 cycles
	}
}

void BackgroundGenerator::set_mode(BkgRandomISI isi) {
	set_mode(true, isi.value());
}

void BackgroundGenerator::set_mode(BkgRegularISI isi) {
	set_mode(false, isi.value());
}

std::ostream& operator<< (std::ostream& o, BackgroundGenerator const& b) {
	o << "enable = " << b.mEnable
	  << ", random = " << b.mRandom
	  << ", seed = " << b.mSeed
	  << ", period = " << b.mPeriod
	  << ", address= " << b.mAddress;
	return o;
}

////////////////////////////////////////////////////////////////////////////////
// Neuron

Neuron::Neuron() :
	_address(0),
	_activate_firing(false),
	_enable_spl1_output(false),
	_enable_fire_input(false),
	_enable_aout(false),
	_enable_current_input(false)
{}

bool Neuron::operator== (Neuron const& b) const
{
	return _address == b.address()
		&& _activate_firing == b.activate_firing()
		&& _enable_fire_input == b.enable_fire_input()
		&& _enable_aout == b.enable_aout()
		&& _enable_spl1_output == b._enable_spl1_output
		&& _enable_current_input == b.enable_current_input();
}

bool Neuron::operator!= (Neuron const& b) const
{
	return !(*this == b);
}

std::ostream& operator<< (std::ostream& os, Neuron const& n)
{
	os << "Neuron(address=" << n._address
		<< ", activate_firing=" << n._activate_firing
		<< ", enable_fire_input=" << n._enable_fire_input
		<< ", enable_aout=" << n._enable_aout
		<< ", enable_spl1_output=" << n._enable_spl1_output
		<< ", enable_current_input=" << n._enable_current_input << ")";
	return os;
}


////////////////////////////////////////////////////////////////////////////////
// NeuronQuad

NeuronQuad::NeuronQuad() :
	mNeuron(),
	vert_switches(0),
	hor_switches(0)
{}

Neuron& NeuronQuad::operator[](coord_t const& n)
{
	return mNeuron[n.y()][n.x()];
}

Neuron const& NeuronQuad::operator[](coord_t const& n) const
{
	return mNeuron[n.y()][n.x()];
}

void NeuronQuad::setVerticalInterconnect(coord_t::x_type x, bool value)
{
	vert_switches[x] = value;
}

bool NeuronQuad::getVerticalInterconnect(coord_t::x_type x) const
{
	return vert_switches[x];
}

void NeuronQuad::setHorizontalInterconnect(coord_t::y_type y, bool value)
{
	hor_switches[y] = value;
}

bool NeuronQuad::getHorizontalInterconnect(coord_t::y_type y) const
{
	return hor_switches[y];
}

int NeuronQuad::getHWAddress(coord_t const& n)
{
	return neuron_mapping[n.toEnum()];
}

bool NeuronQuad::operator==(NeuronQuad const& b) const
{
	return mNeuron == b.mNeuron &&
		vert_switches == b.vert_switches &&
		hor_switches == b.hor_switches;
}

bool NeuronQuad::operator!=(NeuronQuad const& b) const
{
	return !(*this == b);
}

std::array<int, NeuronQuad::num_cols*NeuronQuad::num_rows> const NeuronQuad::neuron_mapping =
	{{ 1, 3, 0, 2, }};

std::ostream& operator<< (std::ostream& os, NeuronQuad const& q)
{
	os << "NeuronQuad(";
	for (size_t ii=0; ii<4; ++ii)
		os << ii << ": " << q[NeuronOnQuad(Enum(ii))] << ", ";
	os << "vsw: " << q.vert_switches << ", hsw:" << q.hor_switches << ")";
	return os;
}

////////////////////////////////////////////////////////////////////////////////
// NeuronConfig

const size_t NeuronConfig::number_sides;
const size_t
	NeuronConfig::ra_fast1,
	NeuronConfig::gla_fast1,
	NeuronConfig::gl_fast1,
	NeuronConfig::ra_fast0,
	NeuronConfig::gla_fast0,
	NeuronConfig::gl_fast0,
	NeuronConfig::ra_slow1,
	NeuronConfig::gla_slow1,
	NeuronConfig::gl_slow1,
	NeuronConfig::ra_slow0,
	NeuronConfig::gla_slow0,
	NeuronConfig::gl_slow0,
	NeuronConfig::bigcap1,
	NeuronConfig::bigcap0,
	NeuronConfig::spl1reset,
	NeuronConfig::neuronreset1,
	NeuronConfig::neuronreset0;

void NeuronConfig::activate_neuron_reset()
{
	reset_neuron = true;
}


void NeuronConfig::deactivate_neuron_reset()
{
	reset_neuron = false;
}

bool NeuronConfig::get_neuron_reset() const
{
	return reset_neuron;
}

void NeuronConfig::activate_spl1_reset()
{
	reset_spl1 = true;
}

void NeuronConfig::deactivate_spl1_reset()
{
	reset_spl1 = false;
}

bool NeuronConfig::get_spl1_reset() const
{
	return reset_spl1;
}

bool NeuronConfig::operator ==(NeuronConfig const& b) const {
	return (bigcap==b.bigcap && slow_I_radapt==b.slow_I_radapt && fast_I_radapt==b.fast_I_radapt
			&& slow_I_gladapt==b.slow_I_gladapt && fast_I_gladapt==b.fast_I_gladapt
			&& slow_I_gl==b.slow_I_gl && fast_I_gl==b.fast_I_gl);
}

bool NeuronConfig::operator !=(NeuronConfig const& b) const {
	return !(*this == b);
}

////////////////////////////////////////////////////////////////////////////////
// Repeater

bool Repeater::operator==(Repeater const& other) const
{
	return mRen == other.mRen
		&& mLen == other.mLen
		&& mMode == other.mMode
		&& mDirection == other.mDirection;
}

bool Repeater::operator!=(Repeater const& other) const
{
	return (*this==other);
}

std::ostream& operator<<(std::ostream& os, Repeater const& a)
{
	os << "Repeater(L:" << a.mLen << ", R:" << a.mRen;
	switch (a.getMode())
	{
		case Repeater::FORWARDING: os << ", M: FORWARDING"; break;
		case Repeater::IDLE      : os << ", M: IDLE      "; break;
		case Repeater::INPUT     : os << ", M: INPUT     "; break;
		case Repeater::OUTPUT    : os << ", M: OUTPUT    "; break;
		case Repeater::LOOPBACK  : os << ", M: LOOPBACK  "; break;
	}
	return os << ", D: " << a.getDirections() << ")" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// VerticalRepeater

bool VerticalRepeater::operator==(VerticalRepeater const& r) const
{
	return static_cast<Repeater const&>(*this) == static_cast<Repeater const&>(r);
}

bool VerticalRepeater::operator!=(VerticalRepeater const& r) const
{
	return !(*this == r);
}

std::ostream& operator<<(std::ostream& os, VerticalRepeater const& a)
{
	return os << "Vertical" << static_cast<const Repeater &>(a);
}

////////////////////////////////////////////////////////////////////////////////
// HorizontalRepeater

bool HorizontalRepeater::operator==(HorizontalRepeater const& r) const
{
	return static_cast<Repeater const&>(*this) == static_cast<Repeater const&>(r);
}

bool HorizontalRepeater::operator!=(HorizontalRepeater const& r) const
{
	return !(*this == r);
}

std::ostream& operator<<(std::ostream& os, HorizontalRepeater const& a)
{
	return os << "Horizontal" << static_cast<const Repeater &>(a);
}

////////////////////////////////////////////////////////////////////////////////
// Analog

const Analog::value_type
	Analog::fg_right,
	Analog::membrane_bot_odd,
	Analog::membrane_top_odd,
	Analog::membrane_bot_even,
	Analog::membrane_top_even,
	Analog::fg_left,
	Analog::fireline_neuron0,
	Analog::preout,
	Analog::dll_voltage,
	Analog::none;

bool Analog::operator ==(Analog const& rhs) const
{
	return config == rhs.config;
}

bool Analog::operator !=(Analog const& rhs) const
{
	return !(*this == rhs);
}

size_t Analog::mult(halco::hicann::v2::AnalogOnHICANN const s) const
{
	return (s+1)%2;
}

////////////////////////////////////////////////////////////////////////////////
// HICANNStatus

void Status::check(){
	if (getHardwareId()!= 0x14849434) throw std::runtime_error("HICANN ID invalid");
	//~ if (getCRCCount()) throw std::runtime_error("HICANN CRC counter is not zero");
}

////////////////////////////////////////////////////////////////////////////////
// ostreams

#define OSTREAM_IT(NAME) \
std::ostream& operator<<(std::ostream& os, NAME const& o) \
{ \
	auto prev = *std::begin(o); \
	auto count = 0; \
	for (auto val : o) { \
		if (val == prev) { \
			count++; \
		} else { \
			os << count << "x "; \
			os << prev << " "; \
			prev = val; \
			count = 1; \
		} \
	} \
	os << count << "x "; \
	os << prev << std::endl; \
	return os; \
}

OSTREAM_IT(CrossbarRow)
OSTREAM_IT(SynapseSwitchRow)
OSTREAM_IT(WeightRow)
OSTREAM_IT(DecoderRow)
OSTREAM_IT(DecoderDoubleRow)
OSTREAM_IT(BackgroundGeneratorArray)
OSTREAM_IT(FGRow4)
OSTREAM_IT(FGRowOnFGBlock4)

std::ostream& operator<<(std::ostream& os, RepeaterBlock::TestEvent const& o)
{
	os << "TestEvent("
	   << "address=" << o.address
	   << ", time=" << o.time << ")";
	return os;
}
OSTREAM_IT(TestEvent_3)

std::ostream& operator<<(std::ostream& os, Analog const& o)
{
	os << "Analog:" << std::endl;
	os << "  " << o.getConfig() << std::endl;
	return os;
}

std::ostream& operator<<(std::ostream& os, NeuronConfig const& o)
{
	os << "NeuronConfig:" << std::endl;
	os << "  bigcap = " << o.bigcap << std::endl;
	os << "  slow_I_radapt = " << o.slow_I_radapt << std::endl;
	os << "  fast_I_radapt = " << o.fast_I_radapt << std::endl;
	os << "  slow_I_gladapt = " << o.slow_I_gladapt << std::endl;
	os << "  fast_I_gladapt = " << o.fast_I_gladapt << std::endl;
	os << "  slow_I_gl = " << o.slow_I_gl << std::endl;
	os << "  fast_I_gl = " << o.fast_I_gl << std::endl;
	os << "  SRAM timings = " << o.timings << std::endl;
	return os;
}

std::ostream& operator<<(std::ostream& os, RepeaterBlock const& o)
{
	os << "RepeaterBlock:" << std::endl;
	os << "  drvresetb = " << o.drvresetb << std::endl;
	os << "  dllresetb = " << o.dllresetb << std::endl;
	os << "  fextcap   = " << o.fextcap << std::endl;
	os << "  start_tdi = " << o.start_tdi << std::endl;
	os << "  start_tdo = " << o.start_tdo << std::endl;
	os << "  full_flag = " << o.full_flag << std::endl;
	os << "  tdi_data (0) = " << o.tdi_data[halco::hicann::v2::TestPortOnRepeaterBlock(0)];
	os << "  tdi_data (1) = " << o.tdi_data[halco::hicann::v2::TestPortOnRepeaterBlock(1)];
	os << "  tdo_data (0) = " << o.tdo_data[halco::hicann::v2::TestPortOnRepeaterBlock(0)];
	os << "  tdo_data (1) = " << o.tdo_data[halco::hicann::v2::TestPortOnRepeaterBlock(1)];
	os << "  SRAM timings = " << o.timings;
	return os;
}

std::ostream& operator<<(std::ostream& os, Status const& o)
{
	os << "Status:" << std::endl;
	os << "  CRCCount = " << static_cast<int>(o.getCRCCount()) << std::endl;
	os << "  HardwareId = " << o.getHardwareId() << std::endl;
	os << "  StatusReg = " << static_cast<int>(o.getStatusReg()) << std::endl;
	return os;
}

namespace {
	size_t const additional_cycles_syndrv_setup = 2;
	size_t const additional_cycles_syndrv_read = 2;
	size_t const additional_cycles_syndrv_write = 2;
}

size_t SRAMControllerTimings::cycles_read() const
{
	return setup_precharge +
	       additional_cycles_syndrv_setup +
	       read_delay +
	       additional_cycles_syndrv_read;
}

size_t SRAMControllerTimings::cycles_write() const
{
	return setup_precharge +
	       additional_cycles_syndrv_setup +
	       write_delay +
	       additional_cycles_syndrv_write;
}

std::ostream& operator<<(std::ostream& os, SRAMControllerTimings const& o)
{
	os << "read delay: " << o.read_delay.value() << ", "
	   << "setup precharge: " << o.setup_precharge.value() << ", "
	   << "write delay: " << o.write_delay.value() << '\n'
	   << "cycles read: " << o.cycles_read() << '\n'
	   << "cycles write: " << o.cycles_write() << '\n';
	return os;
}

////////////////////////////////////////////////////////////////////////////////
// STDPEvaluationPattern

bool STDPEvaluationPattern::operator==(const STDPEvaluationPattern& p) const
{
	return (aa == p.aa &&
	        ac == p.ac &&
	        ca == p.ca &&
	        cc == p.cc);
}

bool STDPEvaluationPattern::operator!=(const STDPEvaluationPattern& p) const
{
	return !(*this == p);
}

template <typename Archiver>
void STDPEvaluationPattern::serialize(Archiver& ar, unsigned const int)
{
	ar & boost::serialization::make_nvp("aa", aa)
	   & boost::serialization::make_nvp("ac", ca)
	   & boost::serialization::make_nvp("ca", ac)
	   & boost::serialization::make_nvp("cc", cc);
}

std::ostream& operator<<(std::ostream& os, STDPEvaluationPattern const& p)
{
	os << "STDPEvaluationPattern: "
	   << "cc: " << p.cc << ", aa: " << p.aa << ", ac: " << p.ac << ", ca: " << p.ca;
	return os;
}

std::ostream& operator<<(std::ostream& os, SynapseControllerTimings const& o)
{
	os << "write delay: " << o.write_delay.value() << ", "
	   << "output delay: " << o.output_delay.value() << ", "
	   << "setup precharge delay: " << o.setup_precharge.value() << ", "
	   << "enable delay: " << o.enable_delay.value();
	return os;
}

////////////////////////////////////////////////////////////////////////////////
// SynapseControlRegister

bool SynapseControlRegister::operator==(SynapseControlRegister const& reg) const
{
	return (hate::compare_optional_equal(idle, reg.idle, true) &&
	        sca == reg.sca &&
	        scc == reg.scc &&
	        without_reset == reg.without_reset &&
	        sel == reg.sel &&
	        last_row == reg.last_row &&
	        row == reg.row &&
	        newcmd == reg.newcmd &&
	        continuous == reg.continuous &&
	        encr == reg.encr &&
	        cmd == reg.cmd);
}

bool SynapseControlRegister::operator!=(SynapseControlRegister const& reg) const
{
	return !(*this == reg);
}

std::ostream& operator<<(std::ostream& os, SynapseControlRegister const& reg)
{
	os << "idle: " << reg.idle << '\n';
	os << "sca: " << reg.sca << '\n';
	os << "scc: " << reg.scc << '\n';
	os << "without reset: " << reg.without_reset << '\n';
	os << "sel: " << reg.sel << '\n';
	os << "last row: " << reg.last_row << '\n';
	os << "row: " << reg.row << '\n';
	os << "newcmd: " << reg.newcmd << '\n';
	os << "continuous: " << reg.continuous << '\n';
	os << "encr: " << reg.encr << '\n';
	os << "cmd: " << static_cast<int>(reg.cmd) << '\n';
	return os;
}

template <typename Archiver>
void SynapseControlRegister::serialize(Archiver& ar, unsigned const int)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("idle", idle)
	   & make_nvp("sca", sca)
	   & make_nvp("scc", scc)
	   & make_nvp("without_reset", without_reset)
	   & make_nvp("sel", sel)
	   & make_nvp("last_row", last_row)
	   & make_nvp("row", row)
	   & make_nvp("newcmd", newcmd)
	   & make_nvp("continuous", continuous)
	   & make_nvp("encr", encr)
	   & make_nvp("cmd", cmd);
}

////////////////////////////////////////////////////////////////////////////////
// SynapseConfigurationRegister

void SynapseConfigurationRegister::enable_dllreset()
{
	dllresetb = SynapseDllresetb(SynapseDllresetb::min);
}

void SynapseConfigurationRegister::disable_dllreset()
{
	dllresetb = SynapseDllresetb(SynapseDllresetb::max);
}

bool SynapseConfigurationRegister::operator==(SynapseConfigurationRegister const& reg) const
{
	return (synarray_timings == reg.synarray_timings &&
	        dllresetb == reg.dllresetb &&
	        gen == reg.gen &&
	        pattern0 == reg.pattern0 &&
	        pattern1 == reg.pattern1);
}

bool SynapseConfigurationRegister::operator!=(SynapseConfigurationRegister const& reg) const
{
	return !(*this == reg);
}

std::ostream& operator<<(std::ostream& os, SynapseConfigurationRegister const& reg)
{
	os << reg.synarray_timings << '\n';
	os << "dllresetb: " << reg.dllresetb << '\n';
	os << "gen" << reg.gen << '\n';
	os << "pattern0: " << reg.pattern0 << '\n';
	os << "pattern1: " << reg.pattern1 << '\n';
	return os;
}

template <typename Archiver>
void SynapseConfigurationRegister::serialize(Archiver& ar, unsigned const int)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("synarray_timings", synarray_timings)
	   & make_nvp("dllresetb", dllresetb)
	   & make_nvp("gen", gen)
	   & make_nvp("pattern0", pattern0)
	   & make_nvp("pattern1", pattern1);
}

////////////////////////////////////////////////////////////////////////////////
// SynapseStatusRegister

bool SynapseStatusRegister::operator==(SynapseStatusRegister const& reg) const
{
	return (auto_busy == reg.auto_busy &&
	        slice_busy == reg.slice_busy &&
	        syndrv_busy == reg.syndrv_busy);
}

bool SynapseStatusRegister::operator!=(SynapseStatusRegister const& reg) const
{
	return !(*this == reg);
}

std::ostream& operator<<(std::ostream& os, SynapseStatusRegister const& reg)
{
	os << "auto_busy: " << reg.auto_busy << '\n';
	os << "slice_busy: " << reg.slice_busy << '\n';
	os << "syndrv_busy: " << reg.syndrv_busy << '\n';
	return os;
}

template <typename Archiver>
void SynapseStatusRegister::serialize(Archiver& ar, unsigned const int)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("auto_busy", auto_busy)
	   & make_nvp("slice_busy", slice_busy)
	   & make_nvp("syndrv_busy", syndrv_busy);
}

////////////////////////////////////////////////////////////////////////////////
// SynapseController

size_t SynapseController::cycles_synarray(SynapseControllerCmd const& cmd) const
{
	size_t const additional_cycles_synarray_rowopen = 4;
	size_t const additional_cycles_synarray_read = 3;
	size_t const additional_cycles_synarray_write = 4;
	size_t const additional_cycles_synarray_rowclose = 3;
	size_t const additional_cycles_synarray_rst_corr = 6;
	switch(cmd)
	{
		case SynapseControllerCmd::IDLE:
		case SynapseControllerCmd::AUTO:
			return 0;
		case SynapseControllerCmd::START_READ:
		case SynapseControllerCmd::START_RDEC:
			return cnfg_reg.synarray_timings.setup_precharge +
			       cnfg_reg.synarray_timings.enable_delay +
			       additional_cycles_synarray_rowopen;
		case SynapseControllerCmd::READ:
		case SynapseControllerCmd::RDEC:
			return cnfg_reg.synarray_timings.output_delay +
			       additional_cycles_synarray_read;
		case SynapseControllerCmd::WRITE:
		case SynapseControllerCmd::WDEC:
			return cnfg_reg.synarray_timings.write_delay +
			       additional_cycles_synarray_write;
		case SynapseControllerCmd::CLOSE_ROW:
			return additional_cycles_synarray_rowclose;
		case SynapseControllerCmd::RST_CORR:
			return cnfg_reg.synarray_timings.output_delay +
			       additional_cycles_synarray_rst_corr;
		default:
			throw std::runtime_error("Unknown SynapseControllerCmd");
	}
}

bool SynapseController::operator==(SynapseController const& s) const
{
	return (hate::compare_optional_equal(syn_in, s.syn_in, true) &&
	        hate::compare_optional_equal(syn_out, s.syn_out, true) &&
	        syn_rst == s.syn_rst &&
	        hate::compare_optional_equal(syn_corr, s.syn_corr, true) &&
	        ctrl_reg == s.ctrl_reg &&
	        cnfg_reg == s.cnfg_reg &&
	        hate::compare_optional_equal(status_reg, s.status_reg, true) &&
	        lut == s.lut &&
	        syndrv_timings == s.syndrv_timings);
}

bool SynapseController::operator!=(SynapseController const& s) const
{
	return !(*this == s);
}

template <typename Archiver>
void SynapseController::serialize(Archiver& ar, unsigned const int)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("syn_in", syn_in)
	   & make_nvp("syn_out", syn_out)
	   & make_nvp("syn_rst", syn_rst)
	   & make_nvp("syn_corr", syn_corr)
	   & make_nvp("ctrl_reg", ctrl_reg)
	   & make_nvp("cnfg_reg", cnfg_reg)
	   & make_nvp("status_reg", status_reg)
	   & make_nvp("lut", lut)
	   & make_nvp("syndrv_timings", syndrv_timings);
}

std::ostream& operator<<(std::ostream& os, SynapseController const& s)
{
	os << "Synapse Controller:\n";
	os << s.ctrl_reg << '\n';
	os << s.cnfg_reg << '\n';

	if (static_cast<bool>(s.status_reg)) {
		os << s.status_reg << '\n';
	} else {
		os << "Status Register: --" << '\n';
	}

	os << "SYN_CORR:\n";
	if ( static_cast<bool>(s.syn_corr) ) {
		os << '\t' << (*s.syn_corr)[0] << '\n';
		os << '\t' << (*s.syn_corr)[1] << '\n';
	} else {
		os << "--\n";
	}

	os << "SYN_IN:\n";
	if ( static_cast<bool>(s.syn_in) ) {
		for(auto& single_reg:(*s.syn_in)) {
			os << '\t';
			copy(single_reg.cbegin(),
			     single_reg.cend(),
			     std::ostream_iterator<std::bitset<4>>(os, " "));
			os << '\n';
		}
	} else {
		os << "--\n";
	}

	os << "SYN_OUT:\n";
	if ( static_cast<bool>(s.syn_out) ) {
		for(auto& single_reg:(*s.syn_out)) {
			os << '\t';
			copy(single_reg.cbegin(),
			     single_reg.cend(),
			     std::ostream_iterator<std::bitset<4>>(os, " "));
			os << '\n';
		}
	} else {
		os << "--\n";
	}

	os << "SYN_RST:\n\t" << s.syn_rst << '\n';
	os << s.lut << '\n';
	os << s.syndrv_timings << '\n';
	os << "cycles synarray open row: "
	   << s.cycles_synarray(SynapseControllerCmd::START_READ) << '\n';
	os << "cycles synarray read: "
	   << s.cycles_synarray(SynapseControllerCmd::READ) << '\n';
	os << "cycles synarray write: "
	   << s.cycles_synarray(SynapseControllerCmd::WRITE) << '\n';
	os << "cycles synarray close row: "
	   << s.cycles_synarray(SynapseControllerCmd::CLOSE_ROW) << '\n';
	return os;
}

////////////////////////////////////////////////////////////////////////////////
// STDPLUT

STDPLUT::STDPLUT()
{
	// use "additive" learning rule as default
	for (int i = SynapseWeight::min; i < SynapseWeight::end; i++) {
		causal[SynapseWeight(i)] = SynapseWeight(std::min(i + 1, 15));
		acausal[SynapseWeight(i)] = SynapseWeight(std::max(0, i - 1));

		combined[SynapseWeight(i)] = SynapseWeight(i);
	}
}

bool STDPLUT::operator==(STDPLUT const& b) const
{
	return (b.causal == causal &&
	        b.acausal == acausal &&
	        b.combined == combined
	);
}

bool STDPLUT::operator!=(STDPLUT const& other) const
{
	return !(*this == other);
}

template <typename Archiver>
void STDPLUT::serialize(Archiver& ar, unsigned int const)
{
	ar & boost::serialization::make_nvp("causal", causal)
	   & boost::serialization::make_nvp("acausal", acausal)
	   & boost::serialization::make_nvp("combined", combined);
}

std::ostream& operator<<(std::ostream& os, STDPLUT::LUT const& lut)
{
	os << "LUT( ";
	for (auto val : lut) {
		os << static_cast<size_t>(val) << " ";
	}
	os << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, STDPLUT const& o)
{
	os << "STDPLUT(";
	os << "causal = " << o.causal << ", ";
	os << "acausal = " << o.acausal << ", ";
	os << "combined = " << o.combined << ")";
	return os;
}


} // namespace HICANN
} // namespace HMF

BOOST_CLASS_EXPORT_IMPLEMENT(HMF::HICANN::STDPEvaluationPattern)
BOOST_CLASS_EXPORT_IMPLEMENT(HMF::HICANN::SynapseControlRegister)
BOOST_CLASS_EXPORT_IMPLEMENT(HMF::HICANN::SynapseConfigurationRegister)
BOOST_CLASS_EXPORT_IMPLEMENT(HMF::HICANN::SynapseStatusRegister)
BOOST_CLASS_EXPORT_IMPLEMENT(HMF::HICANN::STDPLUT)
BOOST_CLASS_EXPORT_IMPLEMENT(HMF::HICANN::STDPLUT::LUT)
BOOST_CLASS_EXPORT_IMPLEMENT(HMF::HICANN::SynapseController)

#include "boost/serialization/serialization_helper.tcc"

EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(HMF::HICANN::STDPEvaluationPattern)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(HMF::HICANN::STDPLUT)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE_FREE(HMF::HICANN::STDPLUT::LUT)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(HMF::HICANN::SynapseControlRegister)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(HMF::HICANN::SynapseConfigurationRegister)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(HMF::HICANN::SynapseStatusRegister)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(HMF::HICANN::SynapseController)
