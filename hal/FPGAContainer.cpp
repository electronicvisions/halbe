#include "FPGAContainer.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <random>

#include "macro_HALbe.h"

#include "types-fpga.h" // struct fpga_pulse_packet_t

namespace {

template <typename ForwardIt>
void insertion_sort(ForwardIt beg, ForwardIt end)
{
	for (auto it = beg; it != end; ++it) {
		auto const insertion = std::upper_bound(beg, it, *it);
		std::rotate(insertion, it, std::next(it));
	}
}

} // namespace

namespace HMF {
namespace FPGA {

bool SpinnSenderConfig::operator==(const SpinnSenderConfig & other) const {
	return COMPARE_EQUAL(other, _active, _target_ip, _target_port);
}

bool SpinnAddressConfig::operator==(const SpinnAddressConfig & other) const {
	return COMPARE_EQUAL(other, _single_hicann_mode, _single_hicann_addr, _addr_offset, _out_address_mode, _in_address_mode);
}

uint32_t Status::get_git_hash() const
{
	return m_git_hash;
}

void Status::set_git_hash(uint32_t const hash)
{
	m_git_hash = hash;
}

bool Status::get_git_dirty_flag() const
{
	return m_git_dirty_flag;
}

void Status::set_git_dirty_flag(bool const flag)
{
	m_git_dirty_flag = flag;
}

size_t Status::get_trace_pulse_count() const
{
	return m_trace_pulse_count;
}

void Status::set_trace_pulse_count(size_t const count)
{
	m_trace_pulse_count = count;
}

size_t Status::get_pb_pulse_count() const
{
	return m_pb_pulse_count;
}

void Status::set_pb_pulse_count(size_t const count)
{
	m_pb_pulse_count = count;
}

size_t Status::get_hicann_arq_downlink_rx_counter() const
{
	return m_hicann_arq_downlink_rx_counter;
}

void Status::set_hicann_arq_downlink_rx_counter(size_t const count)
{
	m_hicann_arq_downlink_rx_counter = count;
}

size_t Status::get_hicann_arq_downlink_tx_counter() const
{
	return m_hicann_arq_downlink_tx_counter;
}

void Status::set_hicann_arq_downlink_tx_counter(size_t const count)
{
	m_hicann_arq_downlink_tx_counter = count;
}

size_t Status::get_hicann_arq_uplink_rx_counter() const
{
	return m_hicann_arq_uplink_rx_counter;
}

void Status::set_hicann_arq_uplink_rx_counter(size_t const count)
{
	m_hicann_arq_uplink_rx_counter = count;
}

size_t Status::get_hicann_arq_uplink_tx_counter() const
{
	return m_hicann_arq_uplink_tx_counter;
}

void Status::set_hicann_arq_uplink_tx_counter(size_t const count)
{
	m_hicann_arq_uplink_tx_counter = count;
}

bool Status::get_pb_release_error() const
{
	return m_pb_release_error;
}

void Status::set_pb_release_error(bool const flag)
{
	m_pb_release_error = flag;
}

bool Status::get_pb2arq_fifo_overflow() const
{
	return m_pb2arq_fifo_overflow;
}

void Status::set_pb2arq_fifo_overflow(bool const flag)
{
	m_pb2arq_fifo_overflow = flag;
}


void Status::check(){
	if (getHardwareId() != 0x1c56c007)
		throw std::runtime_error("FPGA ID invalid");
}

bool Status::operator==(const Status &other) const
{
	return m_git_hash == other.m_git_hash
	    && m_git_dirty_flag == other.m_git_dirty_flag
	    && m_trace_pulse_count == other.m_trace_pulse_count
	    && m_pb_pulse_count == other.m_pb_pulse_count
	    && m_hicann_arq_downlink_rx_counter == other.m_hicann_arq_downlink_rx_counter
	    && m_hicann_arq_downlink_tx_counter == other.m_hicann_arq_downlink_tx_counter
	    && m_hicann_arq_uplink_rx_counter == other.m_hicann_arq_uplink_rx_counter
	    && m_hicann_arq_uplink_tx_counter == other.m_hicann_arq_uplink_tx_counter
	    && m_pb_release_error == other.m_pb_release_error
	    && m_pb2arq_fifo_overflow == other.m_pb2arq_fifo_overflow;
}

bool Status::operator!=(const Status &other) const
{
	return !(*this == other);
}

std::ostream& operator<< (std::ostream& o, Status const& a)
{
	o << "--- FPGA Stats ---\n"
	  << "Git hash: 0x" << std::hex << a.m_git_hash << std::dec << " | Dirty flag: " << a.m_git_dirty_flag
	  << "\nTrace Pulse Count: " << a.m_trace_pulse_count << " | PB pulse count: " << a.m_pb_pulse_count
	  << "\nUplink packet counter reads: " << a.m_hicann_arq_uplink_rx_counter << " | writes: " << a.m_hicann_arq_uplink_tx_counter
	  << "\nNetwork debug register writes: " << a.m_hicann_arq_downlink_rx_counter << " | writes: " << a.m_hicann_arq_downlink_tx_counter
	  << "\nPb release error: " << a.m_pb_release_error << " | fifo overflow: " << a.m_pb2arq_fifo_overflow << std::endl;
	return o;
}


// set all to some value
Reset::Reset(bool value) :
	core(value),
	fpgadnc(value),
	ddr2onboard(value),
	ddr2sodimm(value),
	arq(value),
	PLL_frequency(100),
	enable_tests(false),
	cnt_hicann_init_tests(10)
{}

// g++-4.6-style...
Reset::Reset() :
	core(true),
	fpgadnc(true),
	ddr2onboard(true),
	ddr2sodimm(true),
	arq(true),
	PLL_frequency(100),
	enable_tests(false),
	cnt_hicann_init_tests(10)
{}

bool Reset::operator==(const Reset & other) const
{
	return COMPARE_EQUAL(other, core, fpgadnc, ddr2onboard, ddr2sodimm,
	                     arq, PLL_frequency, enable_tests, cnt_hicann_init_tests);
}


std::ostream& operator<< (std::ostream& o, PulseEvent const & p) {
	o << static_cast<PulseAddress>(p)
		<< ", Time: " << p.getTime();
	return o;
}
////////////////////////////////////////////////////////////////////////////////
// PulseEventStream

AlmostSortedPulseEvents::AlmostSortedPulseEvents() : events(), dropped_events()
{
}

AlmostSortedPulseEvents::AlmostSortedPulseEvents(
	container_type const& events_, size_t const dropped_events_)
	: events(events_), dropped_events(dropped_events_)
{
}

AlmostSortedPulseEvents::AlmostSortedPulseEvents(container_type&& events_, size_t dropped_events_)
	: events(std::move(events_)), dropped_events(dropped_events_)
{
}

////////////////////////////////////////////////////////////////////////////////
// PulseEventContainer

PulseEventContainer::PulseEventContainer() : m_events()
{
}

PulseEventContainer::PulseEventContainer(container_type const& data, bool const is_almost_sorted)
	: m_events(data)
{
	sort(is_almost_sorted);
}

PulseEventContainer::PulseEventContainer(container_type&& data, bool const is_almost_sorted)
	: m_events(std::move(data))
{
	sort(is_almost_sorted);
}

PulseEventContainer::PulseEventContainer(AlmostSortedPulseEvents const& data)
	: m_events(data.events)
{
	sort(true);
}

PulseEventContainer::PulseEventContainer(AlmostSortedPulseEvents&& data)
	: m_events(std::move(data.events))
{
	sort(true);
}

void PulseEventContainer::clear()
{
	m_events.clear();
}

void PulseEventContainer::insert_sorted(PulseEvent const& event)
{
	m_events.insert(std::upper_bound(m_events.begin(), m_events.end(), event), event);
}

void PulseEventContainer::append(PulseEvent const& event)
{
	if (m_events.empty() || m_events.back().getTime() <= event.getTime()) {
		m_events.push_back(event);
		return;
	}
	throw std::invalid_argument("can not append earlier event to sorted pulse event container");
}

size_t PulseEventContainer::size() const
{
	return m_events.size();
}

void PulseEventContainer::sort(bool const is_almost_sorted)
{
	if (is_almost_sorted) {
		insertion_sort(m_events.begin(), m_events.end());
	} else {
		std::sort(m_events.begin(), m_events.end());
	}
}

////////////////////////////////////////////////////////////////////////////////
// SpinnakerEventContainer

bool SpinnakerEventContainer::operator==(SpinnakerEventContainer const & ) const
{
	throw std::runtime_error("not implemented");
}

const size_t SpinnRoutingTable::num_entries;
bool SpinnRoutingTable::operator==(const SpinnRoutingTable & other) const {
	return COMPARE_EQUAL(other, _entries);
}

uint8_t BackgroundGenerator::get_hw_channels() const {
	std::bitset<8> temp;
	for (int i = 0; i < 8; i++) temp[i] = channels[7-i];
	return temp.to_ulong();
}

BackgroundGenerator::BackgroundGenerator() :
	enable(false),
	poisson(false),
	seed(1),
	rate(0),
	hicann_number(geometry::Enum(0)),
	first_address(0),
	last_address(0),
	channels(0)
{}

bool operator==(const BackgroundGenerator & a, const BackgroundGenerator & b)
{
	return
		a.enable        == b.enable &&
		a.poisson       == b.poisson &&
		a.seed          == b.seed &&
		a.rate          == b.rate &&
		a.hicann_number == b.hicann_number &&
		a.first_address == b.first_address &&
		a.last_address  == b.last_address &&
		a.channels      == b.channels;
}

} // namespace FPGA
} // namespace HMF

// SF Serialization
#ifndef RCF_USE_BOOST_SERIALIZATION
#include <SF/Archive.hpp>
void HMF::PulsePacket::serialize(SF::Archive &ar) {
	// Optimizations:
	// - serialized object is always owner
	// - read state won't be transmitted
	assert(isOwner);
	ar //& isOwner
	   & timeOffset
	   //& readTime
	   //& readIdx
	   & insertTime
	   & insertIdx
	   & data;
}
#endif
