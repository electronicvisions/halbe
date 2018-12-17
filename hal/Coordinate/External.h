#pragma once

#include "halco/hicann/v2/external.h"

#include "pywrap/compat/array.hpp"

// Ensure that "pywrap/compat/array.hpp" is included before this and hope...
#define BOOST_ASIO_HAS_STD_ARRAY
#include <boost/asio/ip/address.hpp>

namespace HMF {
namespace Coordinate {

using halco::hicann::v2::GbitLinkOnHICANN;
using halco::hicann::v2::HighspeedLinkOnDNC;
using halco::hicann::v2::HighspeedLinkOnWafer;
using halco::hicann::v2::DNCOnWafer;
using halco::hicann::v2::DNCOnFPGA;
using halco::hicann::v2::PowerCoordinate;
using halco::hicann::v2::DNCGlobal;
using halco::hicann::v2::UDPPort;
using halco::hicann::v2::TCPPort;
using halco::hicann::v2::Host;
using halco::hicann::v2::PMU;
using halco::hicann::v2::FPGAOnWafer;
using halco::hicann::v2::FPGAGlobal;
using halco::hicann::v2::AnalogOnHICANN;
using halco::hicann::v2::ChannelOnADC;
using halco::hicann::v2::TriggerOnADC;
using halco::hicann::v2::IPv4;
using halco::hicann::v2::IPv4_array_t;

} // Coordinate
} // HMF
