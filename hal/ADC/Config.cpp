#include "hal/ADC/Config.h"
namespace HMF {
namespace ADC {

	bool Config::operator==(const Config & other) const
	{
		return input()   == other.input() &&
		       trigger() == other.trigger() &&
		       samples() == other.samples();
	}

	Config::Config(samples_type samples,
	               Coordinate::ChannelOnADC input,
	               Coordinate::TriggerOnADC trigger) :
		mInput(input), mTrigger(trigger)
	{
		set_samples(samples);
	}

	Config::Config() :
		mSamples(0),
		mInput(Coordinate::ChannelOnADC(0)),
		mTrigger(Coordinate::TriggerOnADC(0))
	{
	}

	Coordinate::ChannelOnADC Config::input() const
	{
		return mInput;
	}

	Coordinate::TriggerOnADC Config::trigger() const
	{
		return mTrigger;
	}

	Config::samples_type Config::samples() const
	{
		return mSamples;
	}

	void Config::set_input(const Coordinate::ChannelOnADC & channel)
	{
		mInput = channel;
	}
	void Config::set_trigger(const Coordinate::TriggerOnADC & trigger)
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
