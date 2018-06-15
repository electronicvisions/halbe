#include "hal/HICANN/STDPAnalog.h"
#include <ostream>

namespace HMF {
namespace HICANN {

std::ostream& operator<< (std::ostream& os, STDPAnalog const& o)
{
	os << "STDPAnalog(";
	os << "V_m = " << o.V_m << ", ";
	os << "V_clrc = " << o.V_clrc << ", ";
	os << "V_clra = " << o.V_clra << ", ";
	os << "V_thigh = " << o.V_thigh << ", ";
	os << "V_tlow = " << o.V_tlow << ", ";
	os << "V_br = " << o.V_br;
	os << ")";
	return os;
}

} // end namespace HMF
} // end namespace HICANN
