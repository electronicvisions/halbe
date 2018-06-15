#pragma once

#include "hal/HICANN/FGBlock.h"

namespace HMF {
namespace HICANN {

struct FGControl
{
	typedef FGBlock::value_type value_type;

	///neuron parameter access
	value_type getNeuron(Coordinate::NeuronOnHICANN const& nrn,
						 neuron_parameter const np) const;
	void setNeuron(Coordinate::NeuronOnHICANN const& nrn,
				   HICANN::neuron_parameter const np,
				   value_type const& value);


	///shared parameter access through HMF::HICANN::FGBlockOnHICANN coordinate
	value_type getShared(Coordinate::FGBlockOnHICANN const& b,
						 HICANN::shared_parameter const np) const;
	void setShared(Coordinate::FGBlockOnHICANN const b,
				   HICANN::shared_parameter const np,
				   value_type const& value);

	friend bool operator== (FGControl const& a, FGControl const& b);
	friend bool operator!= (FGControl const& a, FGControl const& b);

	FGControl();

	PYPP_DEFAULT(FGControl(FGControl const&));
	PYPP_DEFAULT(FGControl& operator=(FGControl const&));

	size_t size() const;

	FGBlock&       operator[] (Coordinate::FGBlockOnHICANN const& b);
	FGBlock const& operator[] (Coordinate::FGBlockOnHICANN const& b) const;

#ifndef PYPLUSPLUS
	FGBlock& getBlock(Coordinate::FGBlockOnHICANN const& b);
#endif // PYPLUSPLUS
	FGBlock const& getBlock(Coordinate::FGBlockOnHICANN const& b) const;
	void setBlock(Coordinate::FGBlockOnHICANN const& b, FGBlock const& fg);

private:
	FGBlock& _getBlock(Coordinate::FGBlockOnHICANN const& b);

	std::array<FGBlock, FGBlock::fg_blocks> mBlock;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		using namespace boost::serialization;
		ar & make_nvp("blocks", mBlock);
	}

	friend std::ostream& operator<<(std::ostream& os, FGControl const& fgc);
};

} // HICANN
} // HMF
