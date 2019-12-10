#include "hal/FPGA/PulseAddress.h"

#include "bitter/integral.h"

#include <iomanip>

namespace HMF {
namespace FPGA {

using namespace ::HMF::Coordinate;

const PulseAddress::label_t PulseAddress::default_label;

PulseAddress::dnc_address_t PulseAddress:: getDncAddress() const
{
	return dnc_address_t(bit::crop<2>(mData, 12));
}

void PulseAddress::setDncAddress(const dnc_address_t & dnc)
{
	label_t tmp = (dnc.value() & 0x3) << 12;
	mData &= ~(label_t(0x3) << 12);
	mData |= tmp;
}

PulseAddress::chip_address_t PulseAddress::getChipAddress() const
{
	label_t tmp = bit::crop<3>(mData, 9);
	return chip_address_t(Enum(link_from_hw(tmp)));
}

void PulseAddress::setChipAddress(const chip_address_t & chip)
{
	label_t tmp = (link_to_hw(chip.toEnum().value()) & 0x7) << 9;
	mData &= ~(label_t(0x7) << 9);
	mData |= tmp;
}

PulseAddress::channel_t PulseAddress::getChannel() const {
	label_t tmp = bit::crop<3>(mData, 6);
	return channel_t(Enum(swap_channel(tmp)));
}

void PulseAddress::setChannel(channel_t const& channel)
{
	label_t tmp = (swap_channel(channel.value() & 0x7)) << 6;
	mData &= ~(label_t(0x7) << 6);
	mData |= tmp;
}

L1Address PulseAddress::getNeuronAddress() const {
	return L1Address(bit::crop<6>(mData, 0));
}

void PulseAddress::setNeuronAddress(L1Address const& n)
{
	label_t tmp = (n.value() & 0x3f);
	mData &= ~(label_t(0x3f));
	mData |= tmp;
}

bool operator< (PulseAddress const& a, PulseAddress const& b)
{
	return a.mData < b.mData;
}

bool operator== (PulseAddress const& a, PulseAddress const& b)
{
	return a.mData == b.mData;
}

std::ostream& operator<< (std::ostream& o, PulseAddress const & p)
{
	std::ios::fmtflags f(o.flags());
	o << "Address: DNC " << p.getDncAddress()
		<< ", HICANN " << p.getChipAddress()
		<< ", CHANNEL " << p.getChannel()
		<< ", NEURON " << p.getNeuronAddress()
		<< ", LABEL 0x" << std::hex << std::setfill('0')
		<< std::setw(sizeof(PulseAddress::label_t)*2) << p.getLabel();
	o.flags(f);
	return o;
}

} // namespace FPGA
} // namespace HMF
