#pragma once

#include <boost/operators.hpp>

#include "hal/test.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANN/L1Address.h"

namespace HMF {
namespace FPGA {

using ::HMF::HICANN::L1Address;

/// A hardware pulse packet is a 16 bit label:
/// |  2 bit  |   2 bit   |     3bit    |   3bit   |    6bit   |
/// | unknown | DNCOnFPGA | HICANNOnDNC | GbitLink | L1Address |
struct PulseAddress
	: public boost::equality_comparable<PulseAddress>
	, public boost::less_than_comparable<PulseAddress>
{
	// each FPGA handles mupltiple DNCs (but atm: multiple instances handle a
	// single DNC each)
	typedef Coordinate::DNCOnFPGA dnc_address_t;

	// each DNC handles 8 HICANNs, a spike therefore needs a 3 bit address for
	// it's destination chip
	typedef Coordinate::HICANNOnDNC chip_address_t;

	// address of the corresponding dnc merger on the HICANN where the spike is
	// supposed to be inserted
	typedef Coordinate::GbitLinkOnHICANN  channel_t;

	// the hardware label (the raw address) which is returned by the FPGA
	// should not be used by the user.
	typedef uint16_t label_t;

public:
	// All Halbe Coordinates are set to zero
	static const label_t default_label = 0x1c0;

	// TODO: SJ: Does a public default constructor make any sense in this
	// context?
	PYPP_CONSTEXPR PulseAddress(label_t const label = default_label) :
		mData(label)
	{}

	PulseAddress(
		dnc_address_t const dnc,
		chip_address_t const chip,
		channel_t const chan,
		L1Address const& neuron ) :
			mData( ((dnc.value() & 0x03) << 12) // 2bit
					| (link_to_hw(chip.toEnum().value() & 0x7) << 9) // 3bit
					| (swap_channel(chan.value() & 0x7) << 6) // 3bit
					| (neuron.value() & 0x3f) )   // 6bit
	{}

	bool matches(
		dnc_address_t const& dnc, chip_address_t const& chip, channel_t const& channel) const
	{
#ifndef PYPLUSPLUS
		static constexpr label_t without_neuron_address_mask = 0x3FC0;
		label_t reference = PulseAddress(dnc, chip, channel, L1Address{}).mData;
		return (mData & without_neuron_address_mask) == (reference & without_neuron_address_mask);
#endif // !PYPLUSPLUS
	}

	dnc_address_t getDncAddress() const;
	void setDncAddress(const dnc_address_t & dnc);

	chip_address_t getChipAddress() const;
	void setChipAddress(const chip_address_t & chip);

	L1Address getNeuronAddress() const;
	void setNeuronAddress(L1Address const& n);

	channel_t getChannel() const;
	void setChannel(channel_t const& channel);

	label_t getLabel() const { return mData; }
	void setLabel(const label_t label) { mData= label&0x3fff;} //only 14 bit are valid}

	friend bool operator< (PulseAddress const& a, PulseAddress const& b);

	friend bool operator== (PulseAddress const& a, PulseAddress const& b);

private:
	static PYPP_CONSTEXPR label_t swap_channel(label_t channel)
	{
		return 7-channel;
	}

	static PYPP_CONSTEXPR label_t link_to_hw(label_t link)
	{
		/// CONSTEXPR!
		// [0, 1, 2, 3, 4, 5, 6, 7] -> [0, 2, 4, 6, 1, 3, 5, 7]
		return link < 4 ? link*2 : (link-4)*2 + 1;
	}

	static PYPP_CONSTEXPR label_t link_from_hw(label_t link)
	{
		/// CONSTEXPR!
		// [0, 2, 4, 6, 1, 3, 5, 7] -> [0, 1, 2, 3, 4, 5, 6, 7]
		return link%2 ? (link-1)/2+4 : link/2;
	}

	label_t mData;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		ar & boost::serialization::make_nvp("label", mData);
	}

	FRIEND_TEST(PulseAddress, Converter);
};

std::ostream& operator<< (std::ostream& o, PulseAddress const & p);

} // namespace FPGA
} // namespace HMF
