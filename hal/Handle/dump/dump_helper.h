#pragma once
#include "pywrap/compat/debug.hpp"

#include <string>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>


std::string
string_replace(std::string const& str, std::string const& what, std::string const& with);

template<typename Archive>
void dump_helper(Archive&) {}

template<typename Archive, typename Arg, typename ... Args>
void dump_helper(Archive& ar, Arg & arg, Args & ... args) {
	using namespace boost::serialization;
	std::string name = ZTL::typestring<typename std::decay<Arg>::type>();
	name = string_replace(name, "::", ".");
	name = string_replace(name, " ", "_");
	name = string_replace(name, ",", "-");
	name = string_replace(name, "<", "_LT_");
	name = string_replace(name, ">", "_GT_");
	ar & make_nvp(name.c_str(), arg);
	dump_helper(ar, args...);
}
