#include "hal/Handle/Ess.h"
#include "ESS/halbe_to_ess.h"

namespace HMF {

namespace Handle {

Ess::Ess(halco::hicann::v2::Wafer wafer, std::string filepath)
    : mWafer(wafer), mEss(new HAL2ESS(wafer, filepath))
{}

Ess::~Ess()
{}

void Ess::run(long const duration_ns)
{
	mEss->initialize_sim();
	mEss->run_sim(duration_ns);
}

void Ess::setSpeedup(double const speedup)
{
	mEss->set_speedup(speedup);
}

void Ess::setTimestep(double const dt)
{
	mEss->set_timestep(dt);
}

void Ess::setWeightDistortion(bool const enable, double const distortion)
{
	mEss->set_weight_distortion(enable,distortion);
}

void Ess::enableTimedMerger(bool const enable)
{
	mEss->enable_timed_merger(enable);
}

void Ess::enableSpikeDebugging(bool const enable)
{
	mEss->enable_spike_debugging(enable);
}

void Ess::setPulseStatisticsFile(std::string file)
{
	mEss->set_pulse_statistics_file(file);
}

void Ess::setCalibPath(std::string path)
{
	mEss->set_calib_path(path);
}


HAL2ESS * Ess::getESS()
{
	return mEss.get();
}

void Ess::initialize()
{
	mEss->initialize_sim();
}
    
void Ess::add_hicann(halco::hicann::v2::HICANNOnWafer hicann)
{
    mEss->instantiate_hicann(hicann);
}

void Ess::overwriteNeuronParameters(ParameterMap const&)
{
}

} //end namepace Handle

} //end namespace HMF
