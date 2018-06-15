#pragma once
#include "hal/Coordinate/HMFGeometry.h"

namespace HMF {
namespace ADC {

	/// Holding information about the channels to be used by the
	/// ADC and the number of samples to record
	struct Config
	{
		typedef uint32_t samples_type;

		Config();
		Config(
			samples_type samples,
			Coordinate::ChannelOnADC input,
			Coordinate::TriggerOnADC trigger);

		Coordinate::ChannelOnADC input() const;
		Coordinate::TriggerOnADC trigger() const;
		samples_type samples() const;
		bool operator==(const Config & other) const;
		bool operator!=(const Config& other) const { return !(*this == other); }

		void set_input(const Coordinate::ChannelOnADC & channel);
		void set_trigger(const Coordinate::TriggerOnADC & trigger);
		void set_samples(samples_type samples);
	private:
		samples_type mSamples;
		Coordinate::ChannelOnADC mInput;
		Coordinate::TriggerOnADC mTrigger;

		friend class boost::serialization::access;
		template<typename Archiver>
		void serialize(Archiver & ar, unsigned int const)
		{
			using namespace boost::serialization;
			ar & make_nvp("samples",         mSamples)
			   & make_nvp("input_channel",   mInput)
			   & make_nvp("trigger_channel", mTrigger);
		}
	};
	
} // end namespace ADC
} // end namespace HMF
