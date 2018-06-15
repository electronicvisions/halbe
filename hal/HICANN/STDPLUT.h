#pragma once

#include <bitset>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/array.h>

#include "pywrap/compat/array.hpp"

namespace HMF {
namespace HICANN {

struct STDPLUT
{
	struct LUT
	{
	public:
		typedef std::bitset<4> value_type;

		//access single LUT element: getter returns a numerical value
		uint8_t     operator[] (uint8_t const& ii) const { return m_lut[ii].to_ulong(); }
		value_type& operator[] (uint8_t const& ii)       { return m_lut[ii]; }

		//return the bitset like this
		value_type bits(uint8_t const& ii) const { return m_lut[ii]; }

		std::bitset<32> extract_low() const;
		std::bitset<32> extract_high() const;

		void import_low(std::bitset<32> data);
		void import_high(std::bitset<32> data);

		bool operator ==(LUT const& b) const { return b.m_lut == m_lut; }
		bool operator!=(LUT const& other) const { return !(*this == other); }

	private:
		std::array<value_type, 16> m_lut;

		friend class boost::serialization::access;
		template<typename Archiver>
		void serialize(Archiver& ar, unsigned int const)
		{
			ar & boost::serialization::make_nvp("lut", m_lut);
		}

		friend std::ostream& operator<< (std::ostream& os, LUT const& o);
	};

	LUT causal;
	LUT acausal;
	LUT combined; //both causal and acausal are active

	STDPLUT() { set_defaults(); }

	bool operator ==(STDPLUT const& b) const { return (b.causal == causal && b.acausal == acausal && b.combined == combined); }
	bool operator!=(STDPLUT const& other) const { return !(*this == other); }

	void set_defaults();

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		ar & boost::serialization::make_nvp("causal", causal);
		ar & boost::serialization::make_nvp("acausal", acausal);
		ar & boost::serialization::make_nvp("combined", combined);
	}

	friend std::ostream& operator<< (std::ostream& os, STDPLUT const& o);
};

} // end namespace HMF
} // end namespace HICANN
