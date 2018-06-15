#include "hal/HICANN/FGRow.h"

namespace HMF {
namespace HICANN {

size_t const FGRow::fg_columns;

FGRow::FGRow()
{
}

FGRow::~FGRow()
{
}

void FGRow::setShared(value_type value)
{
	mShared = value;
}

FGRow::value_type FGRow::getShared() const
{
	return mShared;
}

void FGRow::setNeuron(Coordinate::NeuronOnFGBlock neuron, value_type value)
{
	mNeuron.at(neuron) = value;
}

FGRow::value_type FGRow::getNeuron(Coordinate::NeuronOnFGBlock neuron) const
{
	return mNeuron.at(neuron);
}

std::array<std::bitset<20>, 65>
FGRow::set_formatter() const
{
	std::array<std::bitset<20>, 65> r;

	r[0] = static_cast<value_type>(mShared);
	// set mNeuron parameter
	for (size_t nrn=0; nrn<128; ++nrn) {
		r[(nrn+1)/2] |= (static_cast<value_type>(mNeuron[nrn]) << (nrn%2 ? 0 : 10));
	}

	return r;
}

bool operator== (FGRow const & a, FGRow const & b)
{
	return a.mShared == b.mShared && a.mNeuron == b.mNeuron;
}

bool operator!= (FGRow const & a, FGRow const & b)
{
	return !(a==b);
}

std::ostream& operator<< (std::ostream& os, FGRow const& row)
{
	os << "FGRow:";
	os << "\n  mShared: " << row.mShared;
	os << "\n  Neurons: ";
	for (auto const& val : row.mNeuron)
	{
		os.width(4);
		os << val << " ";
	}
	return os;
}


} // HICANN
} // HMF
