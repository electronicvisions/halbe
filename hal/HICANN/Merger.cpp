#include "hal/HICANN/Merger.h"

namespace HMF {
namespace HICANN {

const size_t Merger::enable_bit,
	Merger::select_bit;

const uint8_t
	Merger::LEFT_ONLY,
	Merger::RIGHT_ONLY,
	Merger::MERGE;

bool Merger::operator ==(Merger const& b) const
{
	return config == b.config
		&& slow == b.slow;
}

bool Merger::operator !=(Merger const& b) const
{
	return !(*this==b);
}

std::ostream& operator<<(std::ostream& os, const Merger & m)
{
	os << "config = " << (m.config == Merger::LEFT_ONLY ?
			"LEFT_ONLY" : (m.config ==Merger::RIGHT_ONLY ?
			"RIGHT_ONLY" : "MERGE"))
		<< ", slow = " << (unsigned int)m.slow;
	return os;
}

} // HICANN
} // HMF
