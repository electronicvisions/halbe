#pragma once

#include <boost/operators.hpp>

#include "hal/Handle/Base.h"
#include "halco/hicann/v2/external.h"

namespace HMF {
namespace Handle {

// FIXME: heavy hacking for CK
struct PMU
	: public Base
	, public boost::equality_comparable<PMU>
	, public boost::less_than_comparable<PMU>
{
	PMU();
	explicit PMU(halco::hicann::v2::IPv4 const &); // TODO: remove this!
	PMU(halco::hicann::v2::PMU const &, halco::hicann::v2::IPv4 const &);

	/// Returns the ip.
	halco::hicann::v2::IPv4 const & ip() const;

	/// Compare operator checks coordinates
	bool operator<(PMU const& b) const;

	/// Equality operator checks coordinates
	bool operator==(PMU const& b) const;

	/// Cast operator to its IP.
	operator halco::hicann::v2::IPv4 const & () const;

	halco::hicann::v2::PMU coordinate() const;

private:
	// PMU coordinate
	halco::hicann::v2::PMU const coord;

	// IP of the power board
	halco::hicann::v2::IPv4 const pwr_ip;
};

} // namespace Handle
} // namespace HMF
