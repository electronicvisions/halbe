#pragma once

#include <boost/noncopyable.hpp>

#include "hal/Handle/Base.h"
#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/external.h"

namespace HMF {
namespace Handle {

struct FPGA;

/**
 * @class HICANN
 *
 * @brief (Ultra-Minimal) Handle class that encapsulates a single HICANN connection.
 *
 * @note This will(!) change a lot (issueCmd(), etc.)! Purpose: Adapt/think about parameters/comm hiding.
 */
// FIXME: This is not derived from Handle::Base? (any more?)
struct HICANN
	: private boost::noncopyable
{
	virtual ~HICANN() {}

	/// Returns the coordinate.
	halco::hicann::v2::HICANNGlobal const & coordinate() const {
		return coord;
	}

	/// conversion function (same as explicit cast above) to DNCGlobal
	halco::hicann::v2::DNCGlobal to_DNCGlobal() const {
		return coord.toDNCGlobal();
	}

	/// Cast operator to its DNC-local coordinate.
	halco::hicann::v2::HICANNOnDNC to_HICANNOnDNC () const {
		return coordinate().toHICANNOnDNC();
	}

	/// Equality operator checks coordinates and highspeed
	bool operator==(HICANN const& b) const;
	bool operator!=(HICANN const& other) const { return !(*this == other); }

	/// Returns if for a given HICANN the highspeed connection is required
	bool highspeed() const;

protected:
	/// Construct a HICANN that is connected to FPGA f
	explicit HICANN(const halco::hicann::v2::HICANNGlobal & h, bool request_highspeed = true);

private:
	// HICANN coordinate
	halco::hicann::v2::HICANNGlobal const coord;
	bool const m_highspeed;
}; // class HICANN


} // namespace Handle
} // namespace HMF
