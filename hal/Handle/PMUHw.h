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

	explicit PMUHw(halco::hicann::v2::IPv4 const & ip) :
		PMU(ip)
	{}

	PMUHw(halco::hicann::v2::PMU const & c, halco::hicann::v2::IPv4 const & ip) :
		PMU(c, ip)
	{}
};

} // namespace Handle
} // namespace HMF
