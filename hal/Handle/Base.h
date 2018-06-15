#pragma once

#include "pywrap/compat/macros.hpp"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace HMF {
struct Scheriff;

namespace Handle {

/** Base handle
 *
 */
struct Base :
	private boost::noncopyable
{
	virtual ~Base();

	bool useScheriff() const;
	void enableScheriff();
	PYPP_EXCLUDE(Scheriff& get_scheriff() const;)

private:
	// @ECM: ok?
	static boost::shared_ptr<Scheriff> elSheriff;
};

bool operator==(Base const& a, Base const& b);

} // end namespace HMF
} // end namespace Handle
