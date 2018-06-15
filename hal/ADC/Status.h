#pragma once

#include <string>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>

namespace HMF {
namespace ADC {

	struct Status
	{
		Status();
		bool trigger_activated;
		bool triggered;
		uint32_t start_addr;
		uint32_t end_addr;
		std::string version_string;

		friend std::ostream & operator<<(std::ostream & out, const Status & s);

	private:
		friend class boost::serialization::access;
		template <typename Archiver>
		void serialize(Archiver& ar, unsigned int const)
		{
			using namespace boost::serialization;
			ar & make_nvp("trigger_activated", trigger_activated)
			   & make_nvp("triggered", triggered)
			   & make_nvp("start_addr", start_addr)
			   & make_nvp("end_addr", end_addr)
			   & make_nvp("version_string", version_string);
		}
	};

}	// end namespace ADC
}	// end namespace HMF
