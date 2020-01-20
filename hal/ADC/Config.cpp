#include "hal/ADC/Config.h"
#include "halco/hicann/v2/external.h"

namespace HMF {
namespace ADC {

	bool Config::operator==(const Config & other) const
	{
		return input()   == other.input() &&
		       trigger() == other.trigger() &&
		       samples() == other.samples();
	}

	Config::Config(samples_type samples,
	               halco::hicann::v2::ChannelOnADC input,
	               halco::hicann::v2::TriggerOnADC trigger) :
		mInput(input), mTrigger(trigger)
	{
		set_samples(samples);
	}

	Config::Config() :
		mSamples(0),
		mInput(halco::hicann::v2::ChannelOnADC(0)),
		mTrigger(halco::hicann::v2::TriggerOnADC(0))
	{
	}

	halco::hicann::v2::ChannelOnADC Config::input() const
	{
		return mInput;
	}

	halco::hicann::v2::TriggerOnADC Config::trigger() const
	{
		return mTrigger;
	}

	Config::samples_type Config::samples() const
	{
		return mSamples;
	}

	void Config::set_input(const halco::hicann::v2::ChannelOnADC & channel)
	{
		mInput = channel;
	}
	void Config::set_trigger(const halco::hicann::v2::TriggerOnADC & trigger)
	{
		mTrigger = trigger;
	}

	void Config::set_samples(samples_type samples)
	{
		// every sample uses 16bit of the available 256MiB
		if (samples > 128 * 1024 * 1024)
			throw std::invalid_argument("Maximum number of samples exceeded.");
		mSamples = samples;
	}

}	//namespace ADC
}	//namespace HMF
