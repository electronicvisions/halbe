#pragma once

#include "pywrap/compat/macros.hpp"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace HMF {

namespace Handle {

/** Base handle
 *
 */
struct Base :
	private boost::noncopyable
{
	virtual ~Base();

};

bool operator==(Base const& a, Base const& b);

} // end namespace HMF
} // end namespace Handle
