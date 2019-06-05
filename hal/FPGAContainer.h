#pragma once

#include "HMFCommon.h"
#include "hal/Coordinate/HMFGeometry.h"

#include <bitset>
#include <vector>

#include <boost/serialization/vector.hpp>
#include "pywrap/compat/rant.hpp"
#include "pywrap/compat/tuple.hpp"
#include "hal/strong_typedef.h"
#include <cassert>

#include "hal/HICANN/L1Address.h"
#include "hal/FPGA/PulseAddress.h"

struct fpga_pulse_packet; // fwd decl
typedef fpga_pulse_packet fpga_pulse_packet_t;

namespace SF {
	class Archive;
}

namespace HMF {
namespace FPGA {

static const unsigned int DNC_frequency_in_MHz = 250;

using ::HMF::HICANN::L1Address;

/// A input address for the Spinnaker IF of the FPGA.
/// i.e. an address, that is received and processed further by the FPGA
class SpinnInputAddress_t :
	public Coordinate::detail::RantWrapper<SpinnInputAddress_t, uint16_t, 1023, 0>
{
public:
	PYPP_CONSTEXPR explicit SpinnInputAddress_t(uint16_t const & val = 0) : rant_t(val) {}
	PYPP_DEFAULT(SpinnInputAddress_t & operator= (SpinnInputAddress_t const &));
};

/// A output address of the Spinnaker IF of the FPGA.
/// i.e. an address, that is sent from the FPGA
/// Note:
///  the Spinnaker IF in the FPGA currently just forwards the FPGAPulseAddress consisting of 14-bit
///  However, there are some modificators/compressors available in the FPGA design not yet supported by the Software.
///  The 24-bit used here comply with the new UDP-PulsePacket standard developed in CapaoCaccia 2013
///  For Details see https://capocaccia.ethz.ch/capo/wiki/2013/immns13#UDPStandard
class SpinnOutputAddress_t :
	public Coordinate::detail::RantWrapper<SpinnOutputAddress_t, uint32_t, (1<<24)-1, 0>
{
public:
	PYPP_CONSTEXPR explicit SpinnOutputAddress_t(uint32_t const & val = 0) : rant_t(val) {}
	PYPP_DEFAULT(SpinnOutputAddress_t & operator= (SpinnOutputAddress_t const &));
};

/// class holding the config of Sending Part of the SpiNNaker IF
/// sets sending behaviour: if active, FPGA sends all pulses from the HICANNs to the given IP via the given Port;
/// else, it waits for incoming packets and sends pulses to the source of the first packet.
class SpinnSenderConfig
{
public:
	bool getActive() const {return _active;}
	void setActive(bool active) {_active=active;}

	Coordinate::IPv4 getTargetIP() const {return _target_ip;}
	void setTargetIP(Coordinate::IPv4 target_ip) {_target_ip=target_ip;}

	uint16_t getTargetPort() const {return _target_port;}
	void setTargetPort(uint16_t target_port) {_target_port=target_port;}

	SpinnSenderConfig(): _active(false) {}
	bool operator==(const SpinnSenderConfig & other) const;

private:
	bool             _active;
	Coordinate::IPv4 _target_ip;
	uint16_t         _target_port;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("active", _active)
		   & make_nvp("target_ip", _target_ip)
		   & make_nvp("target_port", _target_port);
	}
};

/// configure generation of output addresses and handling of input addresses.
/// single_hicann_en: enable single HICANN mode: only send spikes from the requested HICANN, address is truncated to 9bit, i.e. handled as if it was coming from HICANN 0
/// single_hicann_addr: address of HICANN in single HICANN mode
/// addr_offset: offset that is added to all addresses; addresses are truncated to 14bit
/// input_addr_mode: 0-default, 1-reverse byte order before handing address to routing memory
/// output_addr_mode: 0-default, 1-reverse byte order, 2-shift address to upper 16bit, 3-shift and reverse byte order (address in LSBs, but two bytes swapped)
class SpinnAddressConfig
{
public:
	enum AddressMode {
		DEFAULT = 0,
		REVERSE_BYTE_ORDER,
		SHIFT,
		SHIFT_N_REVERSE_BYTE_ORDER
	};
	PYPP_CONSTEXPR SpinnAddressConfig():
		_single_hicann_mode(0)
		,_single_hicann_addr(0)
		,_addr_offset(0)
		,_out_address_mode(DEFAULT)
		,_in_address_mode(DEFAULT)
	{}

	void setInAddressMode(AddressMode mode) {
		assert( mode == DEFAULT || mode == REVERSE_BYTE_ORDER );
		_in_address_mode = mode;
	}
	AddressMode getInAddressMode() const { return _in_address_mode; }

	void setOutAddressMode(AddressMode mode) { _out_address_mode = mode; }
	AddressMode getOutAddressMode() const { return _out_address_mode; }

	void setSingleHicannMode(bool mode) {_single_hicann_mode=mode;}
	bool getSingleHicannMode() const { return _single_hicann_mode;}

	void setSingleHicannAddress(unsigned int address) {_single_hicann_addr=address;}
	unsigned int getSingleHicannAddress() const { return _single_hicann_addr;}

	void setAddressOffset(unsigned int offset) {_addr_offset=offset;}
	unsigned int getAddressOffset() const { return _addr_offset;}

	bool operator==(const SpinnAddressConfig & other) const;
private:
	// output modifiers
	bool _single_hicann_mode;
	unsigned int _single_hicann_addr; // FIXME: include a nice Coordinate here, e.g. HICANNOnFPGA
	unsigned int _addr_offset;
	AddressMode _out_address_mode;
	// input modifiers
	AddressMode _in_address_mode;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("single_hicann_mode", _single_hicann_mode)
		   & make_nvp("single_hicann_addr", _single_hicann_addr)
		   & make_nvp("addr_offset", _addr_offset)
		   & make_nvp("out_address_mode", _out_address_mode)
		   & make_nvp("in_address_mode", _in_address_mode);
	}
};

class Status : public ::HMF::StatusBase
{
public:
	PYPP_CONSTEXPR Status()
		: StatusBase(),
		  m_git_hash(0),
		  m_git_dirty_flag(true),
		  m_trace_pulse_count(0),
		  m_pb_pulse_count(0),
		  m_hicann_arq_downlink_rx_counter(0),
		  m_hicann_arq_downlink_tx_counter(0),
		  m_hicann_arq_uplink_rx_counter(0),
		  m_hicann_arq_uplink_tx_counter(0),
		  m_pb_release_error(true),
		  m_pb2arq_fifo_overflow(true)
	{}

	uint32_t get_git_hash() const;
	void set_git_hash(uint32_t const);

	bool get_git_dirty_flag() const;
	void set_git_dirty_flag(bool const);

	size_t get_trace_pulse_count() const;
	void set_trace_pulse_count(size_t const);

	size_t get_pb_pulse_count() const;
	void set_pb_pulse_count(size_t const);

	size_t get_hicann_arq_downlink_rx_counter() const;
	void set_hicann_arq_downlink_rx_counter(size_t const);

	size_t get_hicann_arq_downlink_tx_counter() const;
	void set_hicann_arq_downlink_tx_counter(size_t const);

	size_t get_hicann_arq_uplink_rx_counter() const;
	void set_hicann_arq_uplink_rx_counter(size_t const);

	size_t get_hicann_arq_uplink_tx_counter() const;
	void set_hicann_arq_uplink_tx_counter(size_t const);

	bool get_pb_release_error() const;
	void set_pb_release_error(bool const);

	bool get_pb2arq_fifo_overflow() const;
	void set_pb2arq_fifo_overflow(bool const);

	void check();

	bool operator==(const Status & other) const;
	bool operator!=(const Status & other) const;

	friend std::ostream& operator<< (std::ostream&, Status const&);


private:
	uint32_t m_git_hash;
	bool m_git_dirty_flag;
	size_t m_trace_pulse_count;
	size_t m_pb_pulse_count;
	size_t m_hicann_arq_downlink_rx_counter;
	size_t m_hicann_arq_downlink_tx_counter;
	size_t m_hicann_arq_uplink_rx_counter;
	size_t m_hicann_arq_uplink_tx_counter;
	bool m_pb_release_error;
	bool m_pb2arq_fifo_overflow;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		ar & boost::serialization::make_nvp("git_hash", m_git_hash)
		   & boost::serialization::make_nvp("git_dirty_flag", m_git_dirty_flag)
		   & boost::serialization::make_nvp("trace_pulse_count", m_trace_pulse_count)
		   & boost::serialization::make_nvp("pb_pulse_count", m_pb_pulse_count)
		   & boost::serialization::make_nvp("hicann_arq_downlink_rx_counter", m_hicann_arq_downlink_rx_counter)
		   & boost::serialization::make_nvp("hicann_arq_downlink_tx_counter", m_hicann_arq_downlink_tx_counter)
		   & boost::serialization::make_nvp("hicann_arq_uplink_rx_counter", m_hicann_arq_uplink_rx_counter)
		   & boost::serialization::make_nvp("hicann_arq_uplink_tx_counter", m_hicann_arq_uplink_tx_counter)
		   & boost::serialization::make_nvp("pb_release_error", m_pb_release_error)
		   & boost::serialization::make_nvp("pb2arq_fifo_overflow", m_pb2arq_fifo_overflow);
	}
};

struct Reset {
	Reset();

	/// Sets all FPGA reset options to "value"
	Reset(bool value);

	// real FPGA reset "pins"
	bool core;
	bool fpgadnc;
	bool ddr2onboard;
	bool ddr2sodimm;
	bool arq;

	// HICANN specifics
	uint8_t PLL_frequency;

	// execute tests during reset by default
	bool enable_tests;

	// number of post-hicann-highspeed-init transmission tests
	size_t cnt_hicann_init_tests;

	bool operator==(const Reset & other) const;

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("core", core)
		   & make_nvp("fpgadnc", fpgadnc)
		   & make_nvp("ddr2onboard", ddr2onboard)
		   & make_nvp("ddr2sodimm", ddr2sodimm)
		   & make_nvp("PLL_frequency", PLL_frequency)
		   & make_nvp("arq", arq)
		   & make_nvp("enable_tests", enable_tests)
		   & make_nvp("cnt_hicann_init_tests", cnt_hicann_init_tests);
	}
};

struct BackgroundGenerator
{
public:
	bool enable;
	bool poisson;

	uint32_t seed; //24 bit seed actually
	uint16_t rate; //14 bit period between events in cycles (FPGA cycles!!!)
	Coordinate::HICANNOnDNC hicann_number; //FPGA BEG can serve only one HICANN
	//BEG can randomly generate addresses in the range, defined below
	uint8_t first_address;   //first address of a range
	uint8_t last_address;    //last address of a range
	std::bitset<8> channels; //event generation on a subset of 8 DNC->HICANN channels

	BackgroundGenerator();

	friend bool operator==(const BackgroundGenerator & a, const BackgroundGenerator & b);

	///hardware channels are swapped
	uint8_t get_hw_channels() const;
private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("enable", enable)
		   & make_nvp("poisson", poisson)
		   & make_nvp("seed", seed)
		   & make_nvp("rate", rate)
		   & make_nvp("first_address", first_address)
		   & make_nvp("last_address", last_address)
		   & make_nvp("channels", channels)
		   & make_nvp("hicann_number", hicann_number);
	}
};


struct PulseEvent
	: public PulseAddress
	, public boost::equality_comparable<PulseEvent>
	, public boost::less_than_comparable<PulseEvent>
{
	// Low-level pulse data formatting is defined in spec-host2fpga repository
	// as (cf. spec-host2fpga.git:listings):
	/*
	 *int const MTU = 1500;
	 *struct fpga_pulse_packet_t {
	 *    uint16_t pid; // = 0x0ca5
	 *    uint16_t N;
	 *    union {
	 *        struct {
	 *            uint16_t type : 1;
	 *            uint16_t t : 15;
	 *            uint16_t pad : 2;
	 *            uint16_t l : 14;
	 *        } __attribute__((packed)) pulse;
	 *        struct {
	 *            uint16_t type : 1;
	 *            uint32_t N : 31;
	 *        } __attribute__((packed)) overflow;
	 *    } entry[(MTU -4)/4];
	 *} __attribute__((packed));
	 */

    // Spike time is shared (splitting into timestamps & overflow indicators has
    // to happen in lower layers due to packet handling) and in hardware units
    // (clock cycles?). Note that 32 bits is too small (e.g. 1d*100MHz >> 2^32).
    typedef uint64_t spiketime_t;

public:
	// TODO: SJ: Does a public default constructor make any sense in this
	// context?
	PYPP_CONSTEXPR PulseEvent() :
		PulseAddress(),
		_time(0)
	{}

	explicit PYPP_CONSTEXPR PulseEvent(
		PulseAddress const & pulse_address,
		spiketime_t const t) :
			PulseAddress(pulse_address),
			_time(t)
	{}

	explicit PulseEvent(
		dnc_address_t const dnc,
		chip_address_t const chip,
		channel_t const chan,
		L1Address const& neuron,
		spiketime_t const t) :
			PulseAddress( dnc, chip, chan, neuron),
			_time(t)
	{}

	spiketime_t getTime() const { return _time; }
	void setTime(const spiketime_t & time) { _time = time; }

	friend bool operator<(PulseEvent const& a, const PulseEvent& b)
	{
		if (a.getTime() == b.getTime())
			return static_cast<const PulseAddress&>(a) < static_cast<const PulseAddress&>(b);
		return a.getTime() < b.getTime();
	}

	friend bool operator==(const PulseEvent& a, const PulseEvent& b)
	{
		return (
			static_cast<const PulseAddress&>(a) == static_cast<const PulseAddress&>(b) &&
			a.getTime() == b.getTime());
	}

private:
	spiketime_t      _time;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		using boost::serialization::make_nvp;
		using boost::serialization::base_object;
		ar & make_nvp("pulse_address", base_object<PulseAddress>(*this))
		   & make_nvp("time", _time);
	}
};

std::ostream& operator<< (std::ostream& o, PulseEvent const & p);

/**
 * @brief Stream of almost-sorted pulse events as read out by #read_trace_pulses().
 * @note This should only be used as the return type of #read_trace_pulses().
 * @see PulseEventContainer
 */
struct AlmostSortedPulseEvents {
	typedef std::vector<PulseEvent> container_type;

	AlmostSortedPulseEvents();
	AlmostSortedPulseEvents(container_type const& events_, size_t dropped_events_ = 0);
#ifndef PYPLUSPLUS
	AlmostSortedPulseEvents(container_type&& events_, size_t dropped_events_ = 0);
#endif // !PYPLUSPLUS

	container_type events;
	size_t dropped_events;

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		// clang-format off
		using namespace boost::serialization;
		ar & make_nvp("events", events)
		   & make_nvp("dropped_events", dropped_events);
		// clang-format on
	}
}; // AlmostSortedPulseEvents

/**
 * @brief Contains pulse events (e.g. for 8 HICANNs of the same reticle), sorted by time.
 */
struct PulseEventContainer
{
public:
	typedef std::vector<PulseEvent> container_type;

	PulseEventContainer();
	PulseEventContainer(container_type const& data, bool is_almost_sorted = false);
	PulseEventContainer(AlmostSortedPulseEvents const& data);

#ifndef PYPLUSPLUS
	PulseEventContainer(container_type&& data, bool is_almost_sorted = false);
	PulseEventContainer(AlmostSortedPulseEvents&& data);
#endif // !PYPLUSPLUS

	void clear();

	/**
	 * @brief Insert pulse event to container, preserving container invariant.
	 */
	void insert_sorted(PulseEvent const& event);

	/**
	 * @brief Append pulse event to container.
	 * @throw std::invalid_argument If container invariant would not be preserved.
	 */
	void append(PulseEvent const& event);

	size_t size() const;

	PulseEvent const& operator[](size_t ii) const { return m_events[ii]; }

	container_type const& data() const { return m_events; }

	bool operator==(const PulseEventContainer& other) const;

	bool operator!=(const PulseEventContainer& other) const;
private:
	void sort(bool is_almost_sorted);

	container_type m_events;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		using namespace boost::serialization;
		// clang-format off
		ar & make_nvp("events", m_events);
		// clang-format on
	}
};

struct SpinnakerEventContainer {
	// TODO
	bool operator==(const SpinnakerEventContainer & other) const;
private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& /*ar*/, const unsigned int)
	{
		// TODO
	   	}
};

/// Routing Table for Incoming Pulses in the Spinnaker Interface.
/// The table holds 1024 address:
/// for each Input Address in the range 0-1023 a PulseAddress can be defined,
/// which is then generated and sent downwards.
/// @note per default, all entries hold the default PulseAddress
class SpinnRoutingTable
{
public:
	static const size_t num_entries = 1024;

	void set(const SpinnInputAddress_t& spinn_addr, const PulseAddress& pulse_addr) {_entries[spinn_addr] = pulse_addr;}
	PulseAddress get(const SpinnInputAddress_t& spinn_addr) const {return _entries[spinn_addr];}

	SpinnRoutingTable():_entries(){}
	bool operator==(const SpinnRoutingTable& other) const;
private:
	std::array< PulseAddress, num_entries > _entries;
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("entries", _entries);
	}
};

} // namespace FPGA
} // namespace HMF

namespace std {

HALBE_GEOMETRY_HASH_CLASS(HMF::FPGA::SpinnInputAddress_t)
HALBE_GEOMETRY_HASH_CLASS(HMF::FPGA::SpinnOutputAddress_t)

} // namespace std

#if !defined(PYPLUSPLUS) && !defined(NO_REALTIME)
#include "RealtimeSpike.h"

namespace boost { namespace serialization {
	template<class Archive>
	void save(Archive &ar, Realtime::spike const &s, const unsigned int) {
		ar << boost::serialization::make_nvp("timestamp0", s.timestamp0);
		ar << boost::serialization::make_nvp("timestamp", s.timestamp);
		ar << boost::serialization::make_nvp("label", s.label);
		ar << boost::serialization::make_nvp("packet_type", s.packet_type);
	}
	template<class Archive>
	void load(Archive &ar, Realtime::spike &s, const unsigned int) {
		ar >> s.timestamp0;
		ar >> s.timestamp;
		ar >> s.label;
		ar >> s.packet_type;
	}
	template<class Archive>
	void serialize(Archive &ar, Realtime::spike &s, const unsigned int v) {
		split_free(ar, s, v);
	}

	template<class Archive>
	void save(Archive &ar, Realtime::spike_h const &s, const unsigned int) {
		ar << boost::serialization::make_nvp("label", s.label);
	}
	template<class Archive>
	void load(Archive &ar, Realtime::spike_h &s, const unsigned int) {
		ar >> s.label;
	}
	template<class Archive>
	void serialize(Archive &ar, Realtime::spike_h &s, const unsigned int v) {
		split_free(ar, s, v);
	}
}} // namespace::serialization
#endif
