#pragma once

#include "PMU.h"

#include <memory>
#include "Ess.h"

namespace HMF {
//forard declaration
	class HAL2ESS;
namespace Handle {

struct PMUEss : public PMU
{
	explicit PMUEss(std::shared_ptr<Ess> ess) :
		PMU(),
		mEss(ess)
	{}

#ifndef PYPLUSPLUS
	HAL2ESS & ess();
#endif
private:
	std::shared_ptr<Ess> mEss;
};

}	//end namespace Handle
}	//end namespace HMF

