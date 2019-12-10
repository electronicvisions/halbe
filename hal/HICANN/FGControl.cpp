#include "hal/HICANN/FGControl.h"
#include "pythonic/enumerate.h"

using namespace HMF::Coordinate;

namespace HMF {
namespace HICANN {

FGControl::value_type
FGControl::getNeuron(
	NeuronOnHICANN const& nrn,
	neuron_parameter const np) const
{
	auto fgblock = nrn.toNeuronFGBlock();
	return getBlock(fgblock).getNeuron(
		fgblock, nrn.toNeuronOnFGBlock(), np);
}

void FGControl::setNeuron(
	NeuronOnHICANN const& nrn,
	HICANN::neuron_parameter const np,
	value_type const& value)
{
	auto fgblock = nrn.toNeuronFGBlock();
	_getBlock(fgblock).setNeuron(
		fgblock, nrn.toNeuronOnFGBlock(), np, value);
}

FGControl::value_type FGControl::getShared(
	FGBlockOnHICANN const& b,
	HICANN::shared_parameter const np) const
{
	return getBlock(b).getShared(b, np);
}

void FGControl::setShared(
	FGBlockOnHICANN const b,
	HICANN::shared_parameter const np,
	value_type const& value)
{
	return _getBlock(b).setShared(b, np, value);
}

bool operator== (FGControl const& a, FGControl const& b)
{
	return a.mBlock == b.mBlock;
}

bool operator!= (FGControl const& a, FGControl const& b)
{
	return !(a == b);
}

FGControl::FGControl() :
	mBlock({{
		   FGBlock(FGBlockOnHICANN(Enum(0))),
		   FGBlock(FGBlockOnHICANN(Enum(1))),
		   FGBlock(FGBlockOnHICANN(Enum(2))),
		   FGBlock(FGBlockOnHICANN(Enum(3))),
	   }})
{}

size_t FGControl::size() const
{
	return mBlock.size();
}

FGBlock& FGControl::operator[] (Coordinate::FGBlockOnHICANN const& b)
{
	return mBlock[b.toEnum()];
}

FGBlock const& FGControl::operator[] (Coordinate::FGBlockOnHICANN const& b) const
{
	return mBlock[b.toEnum()];
}

FGBlock& FGControl::getBlock(
	FGBlockOnHICANN const& b)
{
	return mBlock[b.toEnum()];
}

FGBlock const& FGControl::getBlock(
	FGBlockOnHICANN const& b) const
{
	return mBlock[b.toEnum()];
}

void FGControl::setBlock(
	FGBlockOnHICANN const& b,
	FGBlock const& fg)
{
	_getBlock(b) = fg;
}

FGBlock& FGControl::_getBlock(
	FGBlockOnHICANN const& b)
{
	return mBlock[b.toEnum()];
}

std::ostream& operator<<(std::ostream& os, FGControl const& fgc)
{
	os << "FGControl\n";

	for (auto it : fgc.mBlock) {
		os << it << "\n";
	}
	return os;
}

} // HICANN
} // HMF
