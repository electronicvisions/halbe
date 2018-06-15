#include "HMFCommon.h"

#include <map>

namespace HMF {

bool StatusBase::operator==(const StatusBase & other) const
{
	return _crc_count   == other._crc_count &&
		_status_reg  == other._status_reg &&
		_hardware_id == other._hardware_id;
}

} // end namespace HICANN
