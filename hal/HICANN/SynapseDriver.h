#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/bitset.hpp>

#include "pywrap/compat/macros.hpp"
#include "halco/hicann/v2/fwd.h"
#include "hal/HICANN/RowConfig.h"

namespace HMF {
namespace HICANN {

/**
 * Configuration of one synapse driver.
 * holds the config data for one synapse driver, which is shared for both synapse lines.
 */
struct SynapseDriver
{
public:
	static size_t const
		num_rows     = 2,
		num_cap      = 3;
	static const bool
		FACILITATION = 0,
		DEPRESSION   = 1;

	PYPP_DEFAULT(SynapseDriver());
	PYPP_DEFAULT(SynapseDriver(SynapseDriver const&));
	PYPP_DEFAULT(SynapseDriver& operator=(SynapseDriver const&));

	void disable();
	/// Allow stimulus from l1
	void set_l1();
	/// From L1 and give to other
	void set_l1_mirror();
	/// Give to other
	void set_mirror();
	/// Give to other, but do not enable the driver
	void set_mirror_only();
	/// Receive from other
	void set_listen();

	bool is_enabled() const;
	bool is_l1() const;
	bool is_l1_mirror() const;
	bool is_mirror() const;
	bool is_mirror_only() const;
	bool is_listen() const;

	void set_stf();
	void set_std();
	void disable_stp();
	bool is_stf() const;
	bool is_std() const;

	bool operator==(SynapseDriver const& b) const;
	bool operator!=(SynapseDriver const& b) const;

	RowConfig const& operator[] (halco::hicann::v2::RowOnSynapseDriver const&) const;
	RowConfig&       operator[] (halco::hicann::v2::RowOnSynapseDriver const&);

	// FIXME: shouldn't be public. However, many tests use them directly. fix in future
	std::bitset<num_cap> stp_cap;            //!< select size of capacitor in STP circuit

	PYPP_INIT(bool enable, false);           //!< enable this synapse driver
	PYPP_INIT(bool connect_neighbor, false); //!< connect neighboring synapse driver (connect to upper in top half, lower in bottom half)
	PYPP_INIT(bool locin, false);            //!< select input from L1
	PYPP_INIT(bool stp_enable, false);       //!< enable short-term plasticity (STP) for this syndriver
	PYPP_INIT(bool stp_mode, false);         //!< set STP mode for this syndriver. 0: facilitation, 1: depression

private:
	std::array<RowConfig, num_rows> mRowConfig;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);

	friend std::ostream& operator<<(std::ostream& os, const SynapseDriver & m);
};



template<typename Archiver>
void SynapseDriver::serialize(Archiver& ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("enable", enable)
	   & make_nvp("connect_neighbor", connect_neighbor)
	   & make_nvp("locin", locin)
	   & make_nvp("stp_enable", stp_enable)
	   & make_nvp("stp_mode", stp_mode)
	   & make_nvp("stp_cap", stp_cap)
	   & make_nvp("rowconfig", mRowConfig);
}

} // HICANN
} // HMF
