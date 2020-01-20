#pragma once
#include <stdint.h>
#include "halco/hicann/v2/external.h"

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
			halco::hicann::v2::ChannelOnADC input,
			halco::hicann::v2::TriggerOnADC trigger);

		halco::hicann::v2::ChannelOnADC input() const;
		halco::hicann::v2::TriggerOnADC trigger() const;
		samples_type samples() const;
		bool operator==(const Config & other) const;
		bool operator!=(const Config& other) const { return !(*this == other); }

		void set_input(const halco::hicann::v2::ChannelOnADC & channel);
		void set_trigger(const halco::hicann::v2::TriggerOnADC & trigger);
		void set_samples(samples_type samples);
	private:
		samples_type mSamples;
		halco::hicann::v2::ChannelOnADC mInput;
		halco::hicann::v2::TriggerOnADC mTrigger;

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
