#pragma once

#include <algorithm>
#include <vector>

#include "pywrap/compat/cstdint.hpp"
#include "HMFCommon.h"

namespace HMF {
namespace Support {
namespace Power {

struct ReticleStatus {
	typedef std::vector<float> voltage_t;
	voltage_t /*const*/ v_mon, v_fet; // bp::indexing_suite doesn't like const members :/
	bool operator==(const ReticleStatus & ) const {
		throw std::runtime_error("ReticleStatus::operator== not implemented");
	}
};


struct PsbVoltages {
    typedef std::vector<float> psbvoltages_t;
    psbvoltages_t board0;
    psbvoltages_t board1;

	bool operator==(const PsbVoltages & ) const {
		throw std::runtime_error("PsbVoltages::operator== not implemented");
	}
};

struct SystemTemperatures {
    typedef std::vector<float> temperatures_t;
    temperatures_t systemps;

	bool operator==(const SystemTemperatures & ) const {
		throw std::runtime_error("SystemTemperatures::operator== not implemented");
	}
};

} // namespace Power
} // namespace Support
} // namespace HMF
