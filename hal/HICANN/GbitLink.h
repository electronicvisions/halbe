#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/bitset.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANN/L1Address.h"

namespace HMF {
namespace HICANN {

struct GbitLink //DNC<->HICANN channels
{
	PYPP_CLASS_ENUM(Direction) { OFF, TO_DNC, TO_HICANN };

	std::array<Direction, 8> dirs;

	/** enables the use of the pulse timestamps.
	 * if set to True:
	 *   the release timestamp of a Pulse is used to release the time at the given time.
	 * if set to False:
	 *   the release timestamp of a Pulse is discarded. The pulse is directly forwarded.
	 * In the HICANN, this affects only the down transmission (from DNC to HICANN).
	 * In the DNC this enables/disables the use of the heap memory for the down transmission.
	 * TODO: what happens at up transmission if set to false?
	 */
	bool timestamp_enable;

	GbitLink();

	Direction & operator[](const Coordinate::GbitLinkOnHICANN & ii);
	const Direction & operator[](const Coordinate::GbitLinkOnHICANN & ii) const;

	bool operator ==(GbitLink const& b) const
	    { return dirs==b.dirs && timestamp_enable==b.timestamp_enable; }
	bool operator!=(GbitLink const& other) const { return !(*this == other); }

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		ar & boost::serialization::make_nvp("dirs", dirs)
		   & boost::serialization::make_nvp("timestamp_enable", timestamp_enable);
	}
	friend std::ostream& operator<< (std::ostream& o, GbitLink const& gl);
};

} // end namespace HICANN
} // end namespace HMF

