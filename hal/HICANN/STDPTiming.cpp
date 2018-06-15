#include "hal/HICANN/STDPTiming.h"
#include <ostream>

namespace HMF {
namespace HICANN {

std::ostream& operator<< (std::ostream& os, STDPTiming const& o)
{
	os << "STDPTiming(";
	os << "endel = " << o.endel << ", ";
	os << "slen = " << o.slen << ", ";
	os << "outdel = " << o.outdel << ", ";
	os << "predel = " << o.predel << ", ";
	os << "wrdel = " << o.wrdel << ")";
	return os;
}

} // end namespace HMF
} // end namespace HICANN
