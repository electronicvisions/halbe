#pragma once

#ifndef PYPLUSPLUS
#include <array>
#endif
#include <bitset>

#include "halco/hicann/v2/fwd.h"
#include "pywrap/compat/rant.hpp"

namespace HMF {
namespace HICANN {

struct FGRow
{
	typedef uint16_t value_type;
	static size_t const fg_columns = 129;

	FGRow();
	~FGRow();

	void setShared(value_type value);
	value_type getShared() const;
	void setNeuron(halco::hicann::v2::NeuronOnFGBlock column, value_type value);
	value_type getNeuron(halco::hicann::v2::NeuronOnFGBlock column) const;

	friend bool operator== (FGRow const & a, FGRow const & b);
	friend bool operator!= (FGRow const & a, FGRow const & b);
	friend std::ostream& operator<< (std::ostream& os, FGRow const& fgb);

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver & ar, unsigned int const);

#ifndef PYPLUSPLUS
	std::array<std::bitset<20>, 65> set_formatter() const;
#endif // PYPLUSPLUS
private:
	rant::integral_range<value_type, 1023> mShared;
#ifndef PYPLUSPLUS
	std::array<rant::integral_range<value_type, 1023>, fg_columns-1> mNeuron;
#endif // PYPLUSPLUS
};

#ifndef PYPLUSPLUS
template<typename Archiver>
void FGRow::serialize(Archiver & ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("shared", mShared)
	   & make_nvp("neuron", mNeuron);
}
#endif

} // HICANN
} // HMF
