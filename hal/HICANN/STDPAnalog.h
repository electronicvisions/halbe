#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>

#include "pywrap/compat/macros.hpp"

namespace HMF {
namespace HICANN {

struct STDPAnalog
{
	uint16_t V_m;      //start load voltage of causal stdp capacitor, (ground for acausal)
	uint16_t V_clrc;   //stdp clr voltage (causal)
	uint16_t V_clra;   //stdp clr voltage (acausal)
	uint16_t V_thigh;  //stdp readout compare voltageB
	uint16_t V_tlow;   //stdp readout compare voltage
	uint16_t V_br;     //bias for stdp readout circuit

	PYPP_CONSTEXPR STDPAnalog() : //TODO: define meaningful standard settings
		V_m(0),       // 0 V
		V_clrc(570),  // 1 V
		V_clra(570),  // 1 V
		V_thigh(110), // 0.2 V
		V_tlow(110),  // 0.2 V
		V_br(512)     // 2 uA
	{}

	bool operator ==(STDPAnalog const& b) const
		{ return (b.V_m==V_m && b.V_clrc==V_clrc && b.V_clra==V_clra &&
		b.V_thigh==V_thigh && b.V_tlow == V_tlow && b.V_br==V_br); }

	bool operator!=(STDPAnalog const& other) const { return !(*this == other); }

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		ar & boost::serialization::make_nvp("V_m", V_m);
		ar & boost::serialization::make_nvp("V_clrc", V_clrc);
		ar & boost::serialization::make_nvp("V_clra", V_clra);
		ar & boost::serialization::make_nvp("V_thigh", V_thigh);
		ar & boost::serialization::make_nvp("V_tlow", V_tlow);
		ar & boost::serialization::make_nvp("V_br", V_br);
	}

	friend std::ostream& operator<< (std::ostream& os, STDPAnalog const& o);
};

} // end namespace HMF
} // end namespace HICANN
