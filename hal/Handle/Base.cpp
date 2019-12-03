#include "Base.h"

#include <stdexcept>

namespace HMF {
namespace Handle {

Base::~Base()
{}

bool operator==(Base const& a, Base const& b)
{
	return &a == &b;
}

} // end namespace HMF
} // end namespace Handle
