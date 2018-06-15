#pragma once

#include <bitset>
#include <string>

#include <boost/shared_ptr.hpp>

#include "pywrap/compat/macros.hpp"

#ifndef PYPLUSPLUS
#include "hal/Handle/dump/dump_helper.h"

#include <boost/archive/xml_oarchive.hpp>
#endif // !PYPLUSPLUS

namespace HMF {
namespace Handle {

struct DumpMixin;

// TODO:
//	- Allow different files for xml
//	- better file handling?
struct Dump :
	private boost::noncopyable
{
	Dump();

	void enableXmlDump(std::string filename, bool append = false);

private:
	int experiment_id;
	size_t calls;


#ifndef PYPLUSPLUS
	struct XmlArchive {
		XmlArchive(std::string filename, bool append);
		std::unique_ptr<std::ostream> file;
		boost::archive::xml_oarchive archive;
	};
	std::unique_ptr<XmlArchive>					   xml_archive;

	template <typename Handle, typename ... Args>
	void dump(std::string name, const Handle & h, Args const & ... args);

	void gen_experiment_id();
#endif // !PYPLUSPLUS

	friend class DumpMixin;
};


#ifndef PYPLUSPLUS
template <typename Handle, typename ... Args>
void Dump::dump(std::string name, const Handle & h, Args const & ... args) {
	using boost::serialization::make_nvp;

	const auto coordinate = h.coordinate();

	if (xml_archive) {
		xml_archive->archive << make_nvp("type", name);
		dump_helper(xml_archive->archive, coordinate, args...);
	}

	++calls;
}
#endif // !PYPLUSPLUS


} // namespace Handle
} // namespace HMF
