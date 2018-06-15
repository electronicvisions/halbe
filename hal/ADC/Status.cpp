#include "hal/ADC/Status.h"

#include <iostream>
#include <sstream>

namespace HMF {
namespace ADC {

	Status::Status() : trigger_activated(false),
					   triggered(false),
					   start_addr(0),
					   end_addr(0),
					   version_string("-1")
	{}

	std::ostream & operator<<(std::ostream & _out, const Status & s)
	{
		std::ostringstream out;
		out << std::hex;
		out << "start_addr: " << s.start_addr << '\n';
		out << "end_addr: " << s.end_addr << '\n';
		out << "trigger_activated: " << s.trigger_activated << '\n';
		out << "triggered: " << s.triggered << '\n';
		out << "version: " << s.version_string << '\n';
		return _out << out.str();
	}

} // end namespace ADC
} // end namepace HMF
