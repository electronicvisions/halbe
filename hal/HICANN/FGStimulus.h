#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.h>
#include "hal/HICANN/FGBlock.h"
#include "pywrap/compat/rant.hpp"

namespace HMF {
namespace HICANN {

/// Describes the current stimulus for neurons
/// @note: the pulselength of the current stimulus must be configured
///        via FGConfig
struct FGStimulus :
	public std::array<FGBlock::value_type, FGBlock::fg_columns>
{
	typedef std::array<FGBlock::value_type, FGBlock::fg_columns> type;

	FGStimulus(bool continuous = true);
	PYPP_DEFAULT(FGStimulus(FGStimulus const&));
	PYPP_DEFAULT(FGStimulus& operator=(FGStimulus const&));

	bool getContinuous() const;
	void setContinuous(bool c);

	bool operator== (FGStimulus const& b) const;
	bool operator!= (FGStimulus const& b) const;

private:
	bool mContinuous;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver & ar, unsigned int const)
	{
		using namespace boost::serialization;
		ar & make_nvp("current", base_object<type>(*this))
		   & make_nvp("continuous", mContinuous);
	}

	friend std::ostream& operator<<(std::ostream& os, FGStimulus const& f);
};

} // HICANN
} // HMF
