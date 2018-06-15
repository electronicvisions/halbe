#include "hal/Handle/dump/dump_helper.h"

std::string
string_replace(std::string const& str, std::string const& what, std::string const& with)
{
	std::string tmp(str);
	for (size_t cpos = tmp.find(what, 0); (cpos != std::string::npos) && (cpos < tmp.size());
	     cpos = tmp.find(what, cpos)) {
		tmp.replace(cpos, what.size(), with);
		cpos += with.size(); // don't search twice
	};
	return tmp;
}
