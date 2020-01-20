#pragma once

#include <bitset>
#include <boost/serialization/bitset.hpp>

#include "pywrap/compat/macros.hpp"
#include "hal/HMFCommon.h"
#include "hal/strong_typedef.h"

#include "hal/HICANN/GbitLink.h"

#include "halco/hicann/v2/fwd.h"

namespace HMF {
namespace DNC {

using HICANN::GbitLink;

/** configuration of the 8 GbitLinks of one reticle (or DNC).
 */
struct GbitReticle {
	typedef std::array<GbitLink, 8> data_type;
public:
	GbitLink&       operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle);
	GbitLink const& operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle) const;

	bool operator==(const GbitReticle & other) const;
	bool operator!=(const GbitReticle& other) const { return !(*this == other); }
private:
	data_type links;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
};


template<typename Archiver>
void GbitReticle::serialize(Archiver& ar, unsigned int const)
{
	ar & boost::serialization::make_nvp("links", links);
}

std::ostream& operator<< (std::ostream& o, GbitReticle const& gr);

/** configuration of the loopback in the DNC.
 * For each hicann a loopback can be enabled, such that pulses are directly looped back before being released to hicann.
 */
struct Loopback {
	typedef std::array<bool, 8> data_type;
public:
	bool&       operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle);
	bool const& operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle) const;

	bool operator==(const Loopback & other) const;
	bool operator!=(const Loopback& other) const { return !(*this == other); }
private:
	data_type data;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
};

template<typename Archiver>
void Loopback::serialize(Archiver& ar, unsigned int const)
{
	ar & boost::serialization::make_nvp("data", data);
}

std::ostream& operator<< (std::ostream& o, Loopback const& l);

class Status : public ::HMF::StatusBase
{
public:
	void check();
};

} // namespace DNC
} // namespace HMF

