#pragma once

#include <boost/operators.hpp>

#include "hal/Handle/Base.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace HMF {
namespace Handle {

// FIXME: heavy hacking for CK
struct PMU
	: public Base
	, public boost::equality_comparable<PMU>
	, public boost::less_than_comparable<PMU>
{
	PMU();
	explicit PMU(Coordinate::IPv4 const &); // TODO: remove this!
	PMU(Coordinate::PMU const &, Coordinate::IPv4 const &);

	/// Returns the ip.
	Coordinate::IPv4 const & ip() const;

	/// Compare operator checks coordinates
	bool operator<(PMU const& b) const;

	/// Equality operator checks coordinates
	bool operator==(PMU const& b) const;

	/// Cast operator to its IP.
	operator Coordinate::IPv4 const & () const;

	Coordinate::PMU coordinate() const;

private:
	// PMU coordinate
	Coordinate::PMU const coord;

	// IP of the power board
	Coordinate::IPv4 const pwr_ip;
};

} // namespace Handle
} // namespace HMF
