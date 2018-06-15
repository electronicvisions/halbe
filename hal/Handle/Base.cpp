#include "Base.h"

#include <stdexcept>

#include "scheriff/Scheriff.h"

namespace HMF {
namespace Handle {

Base::~Base()
{}

bool Base::useScheriff() const
{
       return static_cast<bool>(elSheriff);
}

void Base::enableScheriff()
{
	elSheriff.reset(new Scheriff());
	elSheriff->start();
}

Scheriff& Base::get_scheriff() const
{
	if (!elSheriff)
	{
		throw std::runtime_error("The Sheriff is currenlty not in town...");
	}
	return *elSheriff;
}

bool operator==(Base const& a, Base const& b)
{
	return &a == &b;
}

boost::shared_ptr<Scheriff> Base::elSheriff;

} // end namespace HMF
} // end namespace Handle
