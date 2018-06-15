#pragma once

#include <string>
#include <sstream>
#include <boost/python.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace HMF {
namespace pyplusplus {

template <typename T>
struct pickle_suite : boost::python::pickle_suite
{
	static boost::python::object
	getstate(T const& obj)
	{
		std::ostringstream os;
		{
			boost::archive::binary_oarchive oa(os);
			oa << obj;
		}
		return boost::python::str(os.str());
	}

	static void
	setstate(T & obj, boost::python::object state)
	{
		namespace bp = boost::python;
		std::string st = bp::extract<std::string>(state)();
		std::istringstream is(st);
		boost::archive::binary_iarchive ia(is);
		ia >> obj;
	}
};

} // end namespace pyplusplus
} // end namespace HMF
