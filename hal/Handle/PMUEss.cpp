#include "hal/Handle/PMUEss.h"
#include "ESS/halbe_to_ess.h"

namespace HMF {
namespace Handle {
	
HAL2ESS & PMUEss::ess()	
{
   	return *(mEss->getESS());
}

}	//end namespace Handle
}	//end namespace HMF
