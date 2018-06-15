#include "HICANNContainer.h"

#include <bitter/bitter.h>
#include <map>
#include <ostream>

using namespace HMF::Coordinate;

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
	return neuron_mapping[n.id()];
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
		case Repeater::INPUTONLY : os << ", M: INPUTONLY "; break;
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

size_t Analog::mult(Coordinate::AnalogOnHICANN const s) const
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
	os << "  tdi_data (0) = " << o.tdi_data[HMF::Coordinate::TestPortOnRepeaterBlock(0)];
	os << "  tdi_data (1) = " << o.tdi_data[HMF::Coordinate::TestPortOnRepeaterBlock(1)];
	os << "  tdo_data (0) = " << o.tdo_data[HMF::Coordinate::TestPortOnRepeaterBlock(0)];
	os << "  tdo_data (1) = " << o.tdo_data[HMF::Coordinate::TestPortOnRepeaterBlock(1)];
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

std::ostream& operator<<(std::ostream& os, SRAMControllerTimings const& o)
{
	os << "read delay: " << o.read_delay.value() << ", "
	   << "setup precharge: " << o.setup_precharge.value() << ", "
	   << "write delay: " << o.write_delay.value();
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
} // namespace HICANN
} // namespace HMF

