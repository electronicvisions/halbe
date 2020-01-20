#pragma once
#include <memory>
#include <string>

#include "hal/Handle/Base.h"
#include "halco/hicann/v2/wafer.h"
#include "euter/cellparameters.h"

namespace HMF {
//forward declarations
class HAL2ESS;

namespace Handle {

struct Ess : public Base {
	//Constructor
	Ess(halco::hicann::v2::Wafer wafer = halco::hicann::v2::Wafer(), std::string filepath = "");
	~Ess();
	//API
	//set duration, run the simulation and reset it
	void run(long const duration_ns);
	//set the global speedup factor
	void setSpeedup(double const speedup);
	//set the timestep for a single integration
	void setTimestep(double const dt);
	//activate the weight distortion and set its variance
	void setWeightDistortion(bool const enable, double const distortion = 0.);
	//if the timed merger are enabled, the merging of events does not happen immediately
	void enableTimedMerger(bool const enable);
	//if spike debugging is enabled all spikes are recorded in an extra file
	void enableSpikeDebugging(bool const enable);
	/// sets file for pulse statistics
	/// If not specified or filename = "", pulse stats are not written to file
	void setPulseStatisticsFile(std::string file);
	/// Set path to directory containing calibration data as xml-files
	/// If not specified or path = "", the default calibration is used.
	void setCalibPath(std::string path);
	//initializes the Ess without running it, needed for tests
	void initialize();
    //adds the hicann coordinate so it is simulated
    void add_hicann(halco::hicann::v2::HICANNOnWafer hicann);

	/// Returns the wafer coordinate
	halco::hicann::v2::Wafer wafer() const { return mWafer; }

	typedef std::map<halco::hicann::v2::NeuronGlobal,
			PyNNParameters::EIF_cond_exp_isfa_ista> ParameterMap;
	void overwriteNeuronParameters(ParameterMap const& map);

#ifndef PYPLUSPLUS
	HAL2ESS * getESS();
#endif
private:
	/// Wafer coordinate of ESS
	halco::hicann::v2::Wafer mWafer;
#ifndef PYPLUSPLUS
	std::unique_ptr<HAL2ESS> mEss;
#endif
};

} // namespace Handle
} // namespace HMF

