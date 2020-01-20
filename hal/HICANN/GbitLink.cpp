#include "hal/HICANN/GbitLink.h"

#include <map>

#include "halco/hicann/v2/external.h"

namespace HMF {
namespace HICANN {

GbitLink::GbitLink() :
	dirs({{Direction::OFF, Direction::OFF, Direction::OFF, Direction::OFF,
		Direction::OFF, Direction::OFF, Direction::OFF, Direction::OFF}})
	, timestamp_enable(false)
{}

GbitLink::Direction & GbitLink::operator[](const halco::hicann::v2::GbitLinkOnHICANN & ii)
{
	return dirs[ii];
}

const GbitLink::Direction & GbitLink::operator[](const halco::hicann::v2::GbitLinkOnHICANN & ii) const
{
	return dirs[ii];
}


const std::map<GbitLink::Direction, std::string > gbit_link_names = {{GbitLink::Direction::OFF, "OFF"}, {GbitLink::Direction::TO_DNC, "TO_DNC"}, {GbitLink::Direction::TO_HICANN,"TO_HICANN"}};

std::ostream& operator<< (std::ostream& o, GbitLink const& gl) {
	o << "GbitLink:" << std::endl;
	for(size_t nn=0;nn<gl.dirs.size(); ++nn){
		o << nn << ": " <<  gbit_link_names.find(gl.dirs[nn])->second << std::endl;
	}
	o << "timestamp_enable = "<< gl.timestamp_enable << std::endl;
	return o;
}

} // end namespace HICANN
} // end namespace HMF
