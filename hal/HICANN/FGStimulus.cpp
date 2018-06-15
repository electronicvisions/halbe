#include "hal/HICANN/FGStimulus.h"

#include <algorithm>

namespace HMF {
namespace HICANN {

FGStimulus::FGStimulus(bool const continuous) :
	mContinuous(continuous)
{
	std::fill(begin(), end(), 0);
}

bool FGStimulus::getContinuous() const
{
	return mContinuous;
}

void FGStimulus::setContinuous(bool c)
{
	mContinuous = c;
}

bool FGStimulus::operator== (FGStimulus const& b) const
{
	return mContinuous == b.mContinuous
		&& static_cast<type const&>(*this) == b;
}

bool FGStimulus::operator!= (FGStimulus const& b) const
{
	return !(*this == b);
}

std::ostream& operator<<(std::ostream& os, FGStimulus const& f)
{
	os << "FGStimulus:" << "\n"
	   << "  current:";
	for (auto const& val : f) {
		os << " " << val;
	}
	os << "\n  continuous: " << std::boolalpha << f.getContinuous() << "\n";
	return os;
}

} // HICANN
} // HMF
