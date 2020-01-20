#pragma once

#include "halco/hicann/v2/l1.h"
#include "hal/HICANN/DNCMerger.h"

namespace HMF {
namespace HICANN {

struct DNCMergerLine
{
	static const size_t num_merger = halco::hicann::v2::DNCMergerOnHICANN::size;

	DNCMerger&       operator[] (const halco::hicann::v2::DNCMergerOnHICANN & ii)
	{ return mMerger[ii]; }
	DNCMerger const& operator[] (const halco::hicann::v2::DNCMergerOnHICANN & ii) const
	{ return mMerger[ii]; }

	static halco::hicann::v2::DNCMergerOnHICANN loopback_target(const halco::hicann::v2::DNCMergerOnHICANN & source);

	bool operator ==(DNCMergerLine const& b) const
		{ return mMerger==b.mMerger; }
	bool operator!=(DNCMergerLine const& other) const { return !(*this == other); }

	friend std::ostream& operator<< (std::ostream& o, const DNCMergerLine & dn);
private:
	std::array<DNCMerger, num_merger>   mMerger;
	//convention: enabling loopbacks according to source channel number!
	//the loopback can only function on channels 0</>1, 2</>3, 4</>5, 6</>7

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		ar & boost::serialization::make_nvp("merger", mMerger);
	}
};

} // HICANN
} // HMF
