#pragma once
// GCCXML has problems with atomics -> removed before boost serialization is included
#ifdef PYPLUSPLUS
#undef _GLIBCXX_ATOMIC_BUILTINS
#endif // PYPLUSPLUS
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include "halco/common/geometry.h"

namespace HMF {
namespace ADC {

struct USBSerial
{
	USBSerial();
	explicit USBSerial(std::string const serial);

	std::string const get_serial() const;
	std::string const value() const;

	void set_serial(std::string const serial);

	bool operator==(USBSerial const& other) const;

	bool operator!=(USBSerial const& other) const;

	friend std::ostream& operator<<(std::ostream& o, USBSerial const& b);

	size_t hash() const;

private:
	// vmodule uses string (and hopefully handles it correctly :p)
	std::string m_serial;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver & ar, unsigned int const) {
		using namespace boost::serialization;
		ar & make_nvp("m_serial", m_serial);
	}
};

} // end namespace ADC
} // end namespace HMF

namespace std {
	HALCO_GEOMETRY_HASH_CLASS(HMF::ADC::USBSerial)
}
