#pragma once

#include "hal/HICANN/Merger.h"

namespace HMF {
namespace HICANN {

struct DNCMerger : public Merger
{
public:
	PYPP_CONSTEXPR DNCMerger(unsigned long long config = Merger::MERGE, bool slow = false) :
		Merger(config, slow), loopback(false) {}

	bool operator ==(DNCMerger const& b) const {
		return Merger::operator==(b) && loopback == b.loopback;
	}

	bool operator!=(DNCMerger const& other) const { return !(*this == other); }

	bool loopback;

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		using namespace boost::serialization;
		ar & make_nvp("base", base_object<Merger>(*this))
		   & boost::serialization::make_nvp("loopback", loopback);
	}
	friend std::ostream& operator<<(std::ostream& os, const Merger & m);
};

} // HICANN
} // HMF
