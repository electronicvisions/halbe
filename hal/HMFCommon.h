#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/array.h>

#include "pywrap/compat/array.hpp"
#include "halco/common/geometry.h"
#include "hal/test.h"

namespace HMF {

///base status class: every instance contains crc and status info
///TODO: find out what else one can read out
class StatusBase
{
public:
	virtual void check() = 0;

	uint8_t getCRCCount() const { return _crc_count; }
	void setCRCCount(uint8_t crc) { _crc_count = crc; }

	uint64_t getHardwareId() const { return _hardware_id; }
	void setHardwareId(uint64_t id) { _hardware_id = id; }

	uint8_t  getStatusReg() const { return _status_reg; }
	void setStatusReg(uint8_t r) { _status_reg = r; }

	PYPP_CONSTEXPR StatusBase() :
		_crc_count(0),
		_status_reg(0),
		_hardware_id(0)
	{}

	bool operator==(const StatusBase & other) const;
	bool operator!=(const StatusBase& other) const { return !(*this == other); }

private:
	uint8_t _crc_count;
	uint8_t _status_reg;
	uint64_t _hardware_id;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, const unsigned int)
	{
		ar & boost::serialization::make_nvp("crc_count", _crc_count)
		   & boost::serialization::make_nvp("status_reg", _status_reg)
		   & boost::serialization::make_nvp("hardware_id", _hardware_id);
	}
};

} // namespace HMF

