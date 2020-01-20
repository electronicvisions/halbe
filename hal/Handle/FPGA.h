#pragma once

#include <string>

#ifndef PYPLUSPLUS
#include <memory>
#endif

#include <boost/shared_ptr.hpp>

#include "hal/Handle/Base.h"
#include "hal/Handle/HICANN.h"

#include "halco/hicann/v2/external.h"
#include "halco/common/typed_array.h"

// fwd decl
struct SpinnController;
struct RealtimeComm;

namespace HMF {
struct PowerBackend;

namespace Handle {

struct HICANN;

// Base class for FPGAs
struct FPGA : public Base
{
	typedef halco::hicann::v2::DNCOnFPGA dnc_coord_t;
	typedef halco::hicann::v2::HICANNOnDNC hicann_coord_t;
	typedef boost::shared_ptr<HICANN> hicann_handle_t;

	/// Dummy destructor needed by unique_ptr member.
	virtual ~FPGA();

	/// Return number of connected HICANNs on a specific DNC
	size_t hicann_count(dnc_coord_t const d) const;

	/// Returns if the given HICANN is active
	bool hicann_active(dnc_coord_t const d, hicann_coord_t const h) const;

	/// Returns if for a given HICANN the highspeed connection is required
	bool hicann_highspeed(dnc_coord_t const d, hicann_coord_t const h) const;

	/// Returns if the given DNC is activ
	bool dnc_active(dnc_coord_t const d) const;

	/// Returns the coordinate.
	halco::hicann::v2::FPGAGlobal const & coordinate() const {
		return coord;
	}

	/// Returns the wafer coordinate.
	halco::hicann::v2::Wafer wafer() const {
		return coord.toWafer();
	}

	/// Translates from local DNC coordinate to global DNC coordinate
	halco::hicann::v2::DNCGlobal dnc(dnc_coord_t const d) const {
		using namespace halco::hicann::v2;
		FPGAGlobal fpga = coordinate();
		return DNCGlobal(d.toDNCOnWafer(fpga), fpga.toWafer());
	}

	/// Translates from local DNC and HICANN coordinates to global HICANN coordinate
	halco::hicann::v2::HICANNGlobal hicann(dnc_coord_t const d, hicann_coord_t const h) const {
		using namespace halco::hicann::v2;
		DNCOnWafer dnc = d.toDNCOnWafer(coordinate());
		HICANNOnWafer hicann = h.toHICANNOnWafer(dnc);
		return HICANNGlobal(hicann, coordinate().toWafer());
	}

	/// Equality operator checks coordinates
	bool operator==(FPGA const& other) const;
	bool operator!=(FPGA const& other) const { return !(*this == other); }

	/// Cast operator to its coordinate.
	operator halco::hicann::v2::FPGAGlobal const & () const {
		return coordinate();
	}

	/// Returns a HICANN handle that representes a connection to a HICANN that
	/// is accessible from this FPGA.
	hicann_handle_t get(dnc_coord_t const& d, hicann_coord_t const& h);

	/// Returns a HICANN handle that representes a connection to a HICANN that
	/// is accessible from this FPGA.
	hicann_handle_t get(halco::hicann::v2::HICANNOnWafer const& hicann);

	template <typename FPGAType>
	FPGAType & cast_to();

	template <typename FPGAType>
	const FPGAType & cast_to() const;

	/// Checks if the FPGA is the master FPGA for global systime synchronization
	bool isMaster() const;

	/// Checks if FPGA operates in listen global mode, i.e. synchronized multi FPGA Experiment
	bool getListenGlobalMode() const;

	void setListenGlobalMode(bool listen);

	typedef std::string license_t;
#ifndef PYPLUSPLUS
	/// Returns the expected license. Empty if no license is needed.
	virtual std::optional<license_t> expected_license() const;
	// Returns if license is valid. Defaults to true.
	virtual bool license_valid() const { return true; }
#endif

protected:
	FPGA(halco::hicann::v2::FPGAGlobal const c);

	/// Mark the DNC as active
	void activate_dnc(const dnc_coord_t & dnc);

	/// Add a HICANN Handle to the FPGAHandle. To construct the handle create_hicann is called
	void add_hicann(const dnc_coord_t & dnc, const hicann_coord_t & hicann, bool request_highspeed = true);

private:
#ifndef PYPLUSPLUS
	virtual hicann_handle_t create_hicann(halco::hicann::v2::HICANNGlobal const& h, bool request_highspeed = true) = 0;
#endif

	// FPGA coordinate
	halco::hicann::v2::FPGAGlobal const coord;

	// Active DNCs
	std::bitset<hicann_coord_t::enum_type::size> active_dncs;

	// attached DNCs
	std::array<halco::hicann::v2::DNCGlobal, dnc_coord_t::size> const dnc_coords;

	// attached HICANNs
	std::array< std::array<halco::hicann::v2::HICANNGlobal, hicann_coord_t::enum_type::size>, dnc_coord_t::end> const hicann_coords;

	// FPGA operates in listen global Mode, i.e. synchronized multi FPGA Experiment
	bool m_listen_global;

	// attached HICANNs
	// FIXME: this array should be list of actually used hicanns and not all available hicanns
	halco::common::typed_array<halco::common::typed_array<hicann_handle_t, hicann_coord_t>, dnc_coord_t>
	    hicanns;

	// HICANNs where a high-speed connection is required
	halco::common::typed_array<halco::common::typed_array<hicann_handle_t, hicann_coord_t>, dnc_coord_t>
	    highspeed_hicanns;

	// HICANNs that can be used (e.g. return correct JTAG id)
	halco::common::typed_array<halco::common::typed_array<hicann_handle_t, hicann_coord_t>, dnc_coord_t>
	    usable_hicanns;
};

template <typename FPGAType>
FPGAType & FPGA::cast_to()
{
	return dynamic_cast<FPGAType &>(*this);
}

template <typename FPGAType>
const FPGAType & FPGA::cast_to() const
{
	return dynamic_cast<const FPGAType & >(*this);
}

PYPP_INSTANTIATE(boost::shared_ptr<FPGA>)

template <typename HICANNHandle>
struct FPGAMixin : public virtual FPGA
{
public:
	typedef FPGAMixin<HICANNHandle> Base;

	/// Returns a HICANN handle that representes a connection to a HICANN that
	/// is accessible from this FPGA.
	boost::shared_ptr<HICANNHandle> get(dnc_coord_t const& d, hicann_coord_t const& h)
	{
		return boost::dynamic_pointer_cast<HICANNHandle>(FPGA::get(d, h));
	}

	boost::shared_ptr<HICANNHandle> get(halco::hicann::v2::HICANNGlobal const& h)
	{
		return boost::dynamic_pointer_cast<HICANNHandle>(FPGA::get(h));
	}

protected:
	FPGAMixin(halco::hicann::v2::FPGAGlobal const& c) : FPGA(c) {}
};

} // namespace Handle
} // namespace HMF
