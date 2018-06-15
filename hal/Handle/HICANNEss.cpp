#include "HICANNEss.h"

#include "ESS/halbe_to_ess.h"

namespace HMF {
namespace Handle {

HAL2ESS & HICANNEss::ess()
{
	return *(mEss->getESS());
}

}	//Handle
}	//HMF
