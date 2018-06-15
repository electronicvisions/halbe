#pragma once

#include "hal/Handle/PMU.h"

namespace HMF {
namespace Handle {

struct PMUHw:
	public PMU
{
	PMUHw() :
		PMU()
	{}

	explicit PMUHw(Coordinate::IPv4 const & ip) :
		PMU(ip)
	{}

	PMUHw(Coordinate::PMU const & c, Coordinate::IPv4 const & ip) :
		PMU(c, ip)
	{}
};

} // namespace Handle
} // namespace HMF
