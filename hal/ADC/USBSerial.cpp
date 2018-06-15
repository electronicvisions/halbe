#include "hal/ADC/USBSerial.h"
#include "pywrap/compat/hash.hpp"

namespace HMF {
namespace ADC {

USBSerial::USBSerial() : m_serial() {}

USBSerial::USBSerial(std::string const serial) : m_serial(serial) {}

std::string const USBSerial::get_serial() const
{
	return m_serial;
}

std::string const USBSerial::value() const
{
	return get_serial();
}

void USBSerial::set_serial(std::string const serial)
{
	m_serial = serial;
}

bool USBSerial::operator==(USBSerial const& other) const
{
	return get_serial() == other.get_serial();
}

bool USBSerial::operator!=(USBSerial const& other) const
{
	return !(*this == other);
}

std::ostream& operator<<(std::ostream& o, USBSerial const& b)
{
	o << "USBSerial: " << b.get_serial();
	return o;
}

size_t USBSerial::hash() const
{
	// We include the type name in the hash to reduce the number of hash collisions in
	// python code, where __hash__ is used in heterogeneous containers.
	static const size_t seed = boost::hash_value(typeid(USBSerial).name());
	size_t hash = seed;
	boost::hash_combine(hash, get_serial());
	return hash;
}

} // end namespace ADC
} // end namespace HMF
