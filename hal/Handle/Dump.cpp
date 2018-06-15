#include "hal/Handle/Dump.h"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

namespace
{
	std::unique_ptr<std::ostream> open_archive_file(
			const std::string & filename, const std::string & ending,
			std::ios_base::openmode mode)
	{
		typedef boost::iostreams::filtering_stream<boost::iostreams::output>
			fstream;
		const std::string gzending = ending + ".gz";
		const std::string bzip2ending = ending + ".bzip2";

		if (boost::algorithm::ends_with(filename, ending))
		{
			return std::unique_ptr<std::ostream>(
					new std::ofstream(filename, mode));
		}
		else if (boost::algorithm::ends_with(filename, gzending))
		{
			std::unique_ptr<fstream> stream(new fstream());
			stream->push(boost::iostreams::gzip_compressor());
			stream->push(boost::iostreams::file_sink(filename, mode));
			return std::move(stream);
		}
		else if (boost::algorithm::ends_with(filename, gzending))
		{
			std::unique_ptr<fstream> stream(new fstream());
			stream->push(boost::iostreams::bzip2_compressor());
			stream->push(boost::iostreams::file_sink(filename, mode));
			return std::move(stream);
		}
		else
		{
			std::stringstream msg;
		    msg << "Invalid filename '" << filename << "'. "
				<< "Filename must end with "
		        << boost::algorithm::join(std::array<const std::string, 3>{{
					ending, gzending, bzip2ending}}, " or ")
				<< ".";
			throw std::runtime_error(msg.str());
		}
	}
}

namespace HMF {
namespace Handle {

Dump::Dump() :
	experiment_id(-1),
	calls(0)
{
	if(char const * file = std::getenv("HALBE_XMLDUMP_FILE")) {
		bool append = std::getenv("HALBE_XMLDUMP_APPEND");
		enableXmlDump(file, append);
	}
}

static std::ios_base::openmode openmode(bool append)
{
	using namespace std;
	return append ? ios_base::app | ios_base::out : ios_base::out;
}

Dump::XmlArchive::XmlArchive(std::string filename, bool append) :
	file(open_archive_file(filename, ".xml", openmode(append))),
	archive(*file)
{
	if (file->bad())
		throw std::runtime_error("error");
	// TODO logger: std::cout << "Set xml dumpfile to: " << filename<< std::endl;
}

void Dump::enableXmlDump(std::string filename, bool append) {
	xml_archive.reset(new XmlArchive(filename, append));
}

} // namespace Handle
} // namespace HMF
