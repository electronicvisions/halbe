#pragma once

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANN/DNCMerger.h"

namespace HMF {
namespace HICANN {

struct DNCMergerLine
{
	static const size_t num_merger = Coordinate::DNCMergerOnHICANN::size;

	DNCMerger&       operator[] (const Coordinate::DNCMergerOnHICANN & ii)
	{ return mMerger[ii]; }
	DNCMerger const& operator[] (const Coordinate::DNCMergerOnHICANN & ii) const
	{ return mMerger[ii]; }

	static Coordinate::DNCMergerOnHICANN loopback_target(const Coordinate::DNCMergerOnHICANN & source);

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
