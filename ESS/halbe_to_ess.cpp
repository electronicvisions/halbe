#include <assert.h>
#include <math.h>
#include <iostream>

#include "bitter/integral.h"

#include "halbe_to_ess.h"
#include "halbe_to_ess_formatter.h"

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Coordinate/iter_all.h"

#include "hal/Handle/HICANNEss.h"
#include "hal/Handle/ADCEss.h"

//Includes from hal/HICANN
#include "hal/HICANN/SynapseDriver.h"
#include "hal/HICANN/FGControl.h"
#include "hal/HICANN/MergerTree.h"
#include "hal/HICANN/DNCMerger.h"
#include "hal/HICANN/GbitLink.h"
#include "hal/HICANN/L1Address.h"
#include "hal/HICANN/DNCMergerLine.h"
#include "hal/Coordinate/Merger0OnHICANN.h"
#include "hal/Coordinate/Merger1OnHICANN.h"
#include "hal/Coordinate/Merger2OnHICANN.h"
#include "hal/Coordinate/Merger3OnHICANN.h"

#include "HMF/NeuronCalibration.h"

//Includes for Parameter Transformations
#include "HMF/HWNeuronParameter.h"
#include "euter/cellparameters.h"

#include "pythonic/zip.h"

#include "systemsim/HALaccess.h"
#include "systemsim/stage2_virtual_hardware.h"

//includes for getter from ess
#include "systemsim/wafer.h"
#include "systemsim/hw_neuron.h"
#include "systemsim/bg_event_generator.h"
#include "systemsim/syndriver.h"
#include "systemsim/ADEX.h"
#include "systemsim/merger_timed.h"
#include "systemsim/l2_fpga.h"
#include "systemsim/hicann_behav_V2.h"
#include "systemsim/l1_behav_V2.h"
#include "systemsim/anncore_behav.h"
#include "systemsim/dnc_if.h"
#include "systemsim/spl1_merger.h"
#include "systemsim/lost_event_logger.h"


// undefine NDEBUG in this compile unit (workaround for disabled asserts)
#ifdef NDEBUG
#undef NDEBUG
#endif


// helper functions

// creates a directory with beginning "/tmp/ess_", which does not yet exists, and returns path as string
std::string create_unique_tmp_directory() {
	using namespace boost::filesystem;
	path tmp_dir;
	bool created_by_me = false;
	do {
		tmp_dir  = temp_directory_path()/unique_path("ess_%%%%-%%%%-%%%%-%%%%");
		created_by_me = create_directory(tmp_dir); // returns true if a new directory was created, otherwise false.
	}
	while ( !created_by_me );
	return tmp_dir.native();
}

// deletes directory in destructor
class DirectoryDeleter {
public:
	DirectoryDeleter(std::string path): mPath(path){}
	virtual ~DirectoryDeleter() {
		using namespace boost::filesystem;
		remove_all(path(mPath)); // deletes if exists
	}
private:
	std::string mPath;
};

static std::vector<DirectoryDeleter> directory_deleters;

/// creates tmp directory for ESS if filepath is empty.
/// If a new directory was created, this directory is registered for deletion
/// at the and of the program.
/// Otherwise, `filepath` is returned.
std::string create_tmp_directory_if_necessary(std::string const & filepath)
{
	if ( filepath.empty() ) {
		std::string ess_dir = create_unique_tmp_directory();
		directory_deleters.emplace_back( ess_dir );
		return ess_dir;
	} else {
		return filepath;
	}
}


namespace HMF
{
//functions for internal structures

const int HAL2ESS::num_wafer;

//Constructor
HAL2ESS::HAL2ESS(Coordinate::Wafer wafer, std::string filepath)
    : mWafer{wafer}
    , mNumFPGAs(wafer.isKintex() ? 48 : 12)
    , mrun{false}
    , mfilepath(create_tmp_directory_if_necessary(filepath))
    , mhal_access{new HALaccess(wafer.value(), mfilepath)}
    , mvirtual_hw{new Stage2VirtualHardware{"Virtual_FACETS_Stage2_Hardware", num_wafer, mfilepath}}
    , mpulse_statistics_file("")
    , mFPGAdata(mNumFPGAs)
    , mFPGAConfig(mNumFPGAs)
{
	LOG4CXX_INFO(_logger, "init ess, storing files in '" << filepath << "'");
}

HAL2ESS::~HAL2ESS()
{
    LOG4CXX_INFO(_logger, "destructor called");
}

//***********************************
//functions to control the simulation
//***********************************

void HAL2ESS::initialize_sim()
{
	// Load calib data before building the hardware
    mhal_access->initCalib();

	unsigned int id = 0;	//the identification of the wafer beloning to the pcb. only single wafer-systems are supported yet
	const unsigned int hicann_x = mhal_access->wafer().num_x_hicanns;
	const unsigned int hicann_y = mhal_access->wafer().num_y_hicanns;
	const unsigned int dnc_count = mhal_access->wafer().num_dncs;
	const unsigned int fpga_count = mNumFPGAs;
	std::vector<std::vector<int> > dnc_to_fpga;
	std::vector<std::vector<boost::tuples::tuple<bool, unsigned int, int, int> > > hic_config;

	//initialize hic_config
	boost::tuples::tuple<bool, unsigned int, int, int> init(false,0,0,0);
	std::vector<boost::tuples::tuple<bool, unsigned int, int, int> > temp;
	temp.resize(hicann_x,init);
	hic_config.resize(hicann_y,temp);

	//initialize dnc_to_fpga
	dnc_to_fpga.resize(fpga_count, std::vector<int>() );

	for (auto & fp : dnc_to_fpga) {
		fp.resize(4, -1);
	}

	//configure hic_config and dnc_to_fpga
	for(size_t n_y = 0; n_y < hicann_y ; ++n_y )
	{
		for(size_t n_x = 0; n_x < hicann_x ; ++n_x )
		{
			auto it = std::find(Coordinate::HICANNOnWaferGrid.begin(), Coordinate::HICANNOnWaferGrid.end(), std::pair<int,int>(n_x, n_y));
			if( it != Coordinate::HICANNOnWaferGrid.end())	//check if nx and ny is a valid combination
			{
				//determine the hicann_id
				halco::common::XRanged<35,0> x(n_x);
				halco::common::YRanged<15,0> y(n_y);
				Coordinate::HICANNGlobal hicann_c(x, y, mWafer);
				unsigned int hicann_id = hicann_c.toHICANNOnWafer().toEnum();
				//determeine if hicann was initialized
				bool available = mhal_access->wafer().hicanns[hicann_id].available;

                //determine the parent DNC of this hicann
				int dnc_id = hicann_c.toDNCOnWafer().id();
                //determine the Hicann-DNC-Channel and the DNC-FPGA-Channel
				auto hod = hicann_c.toHICANNOnDNC();
				int hicann_channel  = hod.x()*2 + hod.y(); // convert to numbering on DNC
				int channel = hicann_c.toDNCOnFPGA();
				
				//write configuration data to hic_config
	            LOG4CXX_DEBUG(_logger, "config channel (=dnc_on_fpga id) =" << channel << " hicann_channel: " << hicann_channel << " for hicann " << hicann_id << " dnc: " << dnc_id );
				boost::tuples::tuple<bool, unsigned int, int, int> conf(available, hicann_id, dnc_id, hicann_channel);
				hic_config[n_y][n_x] = conf;

				//config dnc_to_fpga
                //TODO Log this
                Coordinate::FPGAOnWafer fpga_c = hicann_c.toFPGAOnWafer();
				size_t fpga_id = fpga_c.value();
				dnc_to_fpga.at(fpga_id).at(channel) = dnc_id;

				assert(dnc_to_fpga.at(fpga_id).size() <= 4);
			}
		}
	}

	//write config to Stage2Virtual_HW Instance
	mvirtual_hw->initialize_pcb(id, hicann_x, hicann_y, dnc_count, fpga_count, dnc_to_fpga, hic_config, mhal_access.get(), mFPGAConfig, mDNCConfig);
}

//setting the speedup
void HAL2ESS::set_speedup(double const speedup)
{
    mhal_access->getGlobalHWParameters().speedup = speedup;
}

//setting the timestep for the numerical simulation of the neuron 
void HAL2ESS::set_timestep(double const dt)
{
    mhal_access->getGlobalHWParameters().timestep = dt;
}

//enabling and setting the weight distortion
void HAL2ESS::set_weight_distortion(bool const enable, double const distortion)
{
    mhal_access->getGlobalHWParameters().enable_weight_distortion = enable;
    mhal_access->getGlobalHWParameters().weight_distortion = distortion;
}

// choose timed or non-timed merger
void HAL2ESS::enable_timed_merger(bool enable)
{
    mhal_access->getGlobalHWParameters().enable_timed_merger = enable;
}

void HAL2ESS::enable_spike_debugging(bool const enable)
{
    mhal_access->getGlobalHWParameters().enable_spike_debugging = enable;
}

void HAL2ESS::set_pulse_statistics_file(std::string file)
{
	mpulse_statistics_file  = file;
}

void HAL2ESS::set_calib_path(std::string path)
{
	mhal_access->setCalibPath(path);
}



//sets the duration of the simulation and then runs it
void HAL2ESS::run_sim(long duration_in_ns)
{
    mrun = true;
	mvirtual_hw->set_sim_duration(duration_in_ns);
	mvirtual_hw->run();
	LostEventLogger::set_loglevel(Logger::INFO);
	LostEventLogger::summary();
	if (mpulse_statistics_file != std::string(""))
		LostEventLogger::print_summary_to_file(mpulse_statistics_file);
}

//*****************
//backend functions
//*****************

//**************
//HICANN-Backend
//**************

//sets the configuration of a crossbar row TODO Check if HALBE-numering of h-wires matches ESS-numbering of H-Wires
void HAL2ESS::set_crossbar_switch_row(Handle::HICANN const& h, Coordinate::HLineOnHICANN y_, geometry::Side s, HICANN::CrossbarRow const & switches)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	size_t hic_id = static_cast<size_t>(e);

	size_t hbus = revert_hbus(y_);
	size_t side;
    
    if(s == geometry::left) //left side, numbering of vlines identical 
    {
        side = 0;
	    LOG4CXX_DEBUG(_logger, "set_crossbar_switch_row: Transformed HALbe-Address: Side " << s << " HLine " << y_.value() << " to ESS-Address: Side: " << side << " Row: " <<  hbus );
	    auto &config = mhal_access->wafer().hicanns[hic_id].crossbar_config[side];
	    assert(switches.size() == config[hbus].size() );
	    for(size_t i = 0; i < switches.size(); ++i)
	    {
            Coordinate::VLineOnHICANN vline = y_.toVLineOnHICANN(s, Coordinate::Enum{i});
            size_t v = revert_vbus(vline)/32;
	        LOG4CXX_TRACE(_logger, "set_crossbar_switch_row: Transformed HALbe-Address: VLine " << vline.value() << " to ESS-Address: Line: " << v );
            config[hbus].at(v) = switches[i];
	    }
    }
	else                    //right side, keep in mind different numbering of vlines in ESS and HALBE
    {
		side = 1;
	    LOG4CXX_DEBUG(_logger, "set_crossbar_switch_row: Transformed HALbe-Address: Side " << s << " HLine " << y_.value() << " to ESS-Address: Side: " << side << " Row: " <<  hbus );
	    auto &config = mhal_access->wafer().hicanns[hic_id].crossbar_config[side];
	    assert(switches.size() == config[hbus].size() );
	    for(size_t i = 0; i < switches.size(); ++i)
	    {
            Coordinate::VLineOnHICANN vline = y_.toVLineOnHICANN(s, Coordinate::Enum{i});
		    size_t v = (revert_vbus(vline)%128)/32;
	        LOG4CXX_TRACE(_logger, "set_crossbar_switch_row: Transformed HALbe-Address: VLine " << vline.value() << " to ESS-Address: Line: " << v );
            config[hbus].at(v) = switches[i];
    	}   
    }
}


//gets the configuration of a crossbar switch. fetches the configuration from the HAL2ESS datastructure and the ESS
HICANN::CrossbarRow HAL2ESS::get_crossbar_switch_row(Handle::HICANN const& h, Coordinate::HLineOnHICANN y_, geometry::Side s)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	HICANN::CrossbarRow returnval;
	size_t hbus = revert_hbus(y_);
    size_t side;

    if(s == geometry::left) //left side, numbering of vlines identical
    {
        side = 0;
        const auto &config = mhal_access->wafer().hicanns[hic_id].crossbar_config[side];
	    assert(returnval.size() == config[hbus].size());
	    for(size_t i = 0; i < returnval.size(); ++i)
	    {
            Coordinate::VLineOnHICANN vline = y_.toVLineOnHICANN(s, Coordinate::Enum{i});
		    size_t v = revert_vbus(vline)/32;
            returnval[i] = config[hbus].at(v);
	    }
    }
    else                    //right side, keep in mind different numbering of vlines
    {
        side = 1;
        const auto &config = mhal_access->wafer().hicanns[hic_id].crossbar_config[side];
	    assert(returnval.size() == config[hbus].size());
	    for(size_t i = 0; i < returnval.size(); ++i)
	    {
            Coordinate::VLineOnHICANN vline = y_.toVLineOnHICANN(s, Coordinate::Enum{i});
		    size_t v = (revert_vbus(vline)%128)/32;
            returnval[i] = config[hbus].at(v);
        }
    }
	// getting the cbsrow from the ESS and asserting equality
	HICANN::CrossbarRow returnval_ESS = get_crossbar_switch_row_ESS(h,y_,s);
	(void) returnval_ESS; 	//avoid warning
	assert(returnval == returnval_ESS);
	return returnval;
}


//sets the configuration of a syndriver-switch-row. in HALBE the numbering is done from 0 to 223 and there is no distinction between up/down whereas in the ESS the numbering is done from 0 to 111 and there is a distinction between up/down TODO check if my translation between these different numberings is correct
void HAL2ESS::set_syndriver_switch_row(Handle::HICANN const& h, Coordinate::SynapseSwitchRowOnHICANN const& s, HICANN::SynapseSwitchRow const& switches)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	//convert HALBE-Numbering to ESS-Numbering
	size_t synbl;
	size_t addr;
    format_synswitch(s, synbl, addr);
	LOG4CXX_TRACE(_logger,
	              "set_syndriver_switch_row: Transformed HALbe-Address: Side "
	                  << s.toSideHorizontal() << " SynSwitchRow: " << s.line().value()
	                  << " to ESS-Address: Block: " << synbl << " Row: " << addr);

	//copy configuration
	auto &config = mhal_access->wafer().hicanns[hic_id].synswitch_config[synbl][addr];
	assert(config.size() == switches.size());
	for(size_t i = 0; i < config.size(); ++i)
	{
        size_t j;
        if(s.toSideHorizontal() == geometry::left)    //TODO check if this is correct!
            j = i;
        else 
            j = 15 - i;
        config.at(j) = switches[i];
	}
}


//gets the configuration of a syndriver-switch-row from the ESS and the HAL2ESS datastructure
HICANN::SynapseSwitchRow HAL2ESS::get_syndriver_switch_row(Handle::HICANN const& h, Coordinate::SynapseSwitchRowOnHICANN const& s)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	//convert HALBE-Numbering to ESS-Numbering
	size_t synbl;
	size_t addr;
    format_synswitch(s, synbl, addr);

	HICANN::SynapseSwitchRow returnval;
	const auto &config = mhal_access->wafer().hicanns[hic_id].synswitch_config[synbl][addr];
	assert(config.size() == returnval.size());
	for(size_t i = 0; i < config.size(); ++i)
	{
        size_t j;
        if(s.toSideHorizontal() == geometry::left)    //TODO check if this is correct!
            j = i;
        else 
            j = 15 - i;
		returnval[i] = config.at(j);
	}
	// getting the synswitch row from te ess and asserting equality
	HICANN::SynapseSwitchRow returnval_ESS = get_syndriver_switch_row_ESS(h, s);
	(void) returnval_ESS;
	assert(returnval == returnval_ESS);
	return returnval;
}

//sets the weights for a row of synapses 
void HAL2ESS::set_weights_row(Handle::HICANN const& h, Coordinate::SynapseRowOnHICANN const& s, HICANN::WeightRow const& weights)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	//calculate the address
	size_t row_addr = format_synapse_row(s);
    LOG4CXX_DEBUG(_logger, "set_weights_row: Transformed HALbe-Address: SynapseRow " << s.value() << " to ESS-Address: Row: " << row_addr );

	for(size_t i = 0; i < weights.size(); ++i)
	{
        std::bitset<4> w(weights[i].value());
        mhal_access->wafer().hicanns[hic_id].set_syn_weight(w, row_addr, i);
	    if(w.to_ulong() != 0)
        {
            LOG4CXX_DEBUG(_logger, "set_weights_row: Weight in column " << i << " set to " << w.to_ulong() );
        }
    }
}

void HAL2ESS::set_weights_row(std::vector<boost::shared_ptr<Handle::HICANN> > handles, Coordinate::SynapseRowOnHICANN const& s, std::vector<HICANN::WeightRow> const& data)
{
	for (auto v: pythonic::zip(handles, data)) {
		set_weights_row(*(v.first), s, v.second);
	}
}

//gets the weights for a row of synapses from the ESS and the HAL2ESS datastructure
HICANN::WeightRow HAL2ESS::get_weights_row(Handle::HICANN const& h, Coordinate::SynapseRowOnHICANN const& s)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	HICANN::WeightRow returnval;

	//calculate the address
	size_t row_addr = format_synapse_row(s);

	for(size_t i = 0; i < returnval.size(); ++i)
	{
		auto w = HICANN::SynapseWeight::from_bitset(mhal_access->wafer().hicanns[hic_id].get_syn_weight(row_addr, i));
		returnval[i] = w;
	}
	//getting the weight row from the ESS and asserting equality
	HICANN::WeightRow returnval_ESS = get_weights_row_ESS(h,s);
	(void) returnval_ESS;
	assert(returnval == returnval_ESS);
	return returnval_ESS;
}

//sets the decoder-value 
void HAL2ESS::set_decoder_double_row(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s, HICANN::DecoderDoubleRow const& data)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	//calculate the address
    Coordinate::SynapseRowOnHICANN top_row{s, Coordinate::RowOnSynapseDriver{geometry::top}};
    Coordinate::SynapseRowOnHICANN bot_row{s, Coordinate::RowOnSynapseDriver{geometry::bottom}};
    size_t top = format_synapse_row(top_row);
	size_t bot = format_synapse_row(bot_row);
    
    LOG4CXX_DEBUG(_logger, "set_decoder_double_row: Transformed HALbe-Address: SynapseDriver " << s.line().value() << " to ESS-Address: TopRow: " << top << " BotRow: " << bot );

	auto &hica = mhal_access->wafer().hicanns[hic_id];
	for(size_t i = 0; i < data[geometry::top].size(); ++i)
	{
        if (data[geometry::top][i].value() != 1) // 1 = mapping blocking value TODO shouldnt be a magic number here
            LOG4CXX_DEBUG(_logger, "set_decoder_double_row: top row decoder set to " << (int)data[geometry::top][i].value() << " in column " << i );
	    std::bitset<4> addr_top(data[geometry::top][i].value());
        hica.set_syn_address(addr_top, top, i);
        if (data[geometry::bottom][i].value() != 1) // 1 = mapping blocking value TODO shouldnt be a magic number here
            LOG4CXX_DEBUG(_logger, "set_decoder_double_row: bottom row decoder set to " << (int)data[geometry::bottom][i].value() << " in column " << i );
        std::bitset<4> addr_bot(data[geometry::bottom][i].value());
        hica.set_syn_address(addr_bot, bot, i);
	}
}

void HAL2ESS::set_decoder_double_row(std::vector<boost::shared_ptr<Handle::HICANN> > handles, Coordinate::SynapseDriverOnHICANN const& syndrv, std::vector<HICANN::DecoderDoubleRow> const& data)
{
	for (auto v: pythonic::zip(handles, data)) {
		set_decoder_double_row(*(v.first), syndrv, v.second);
	}
}

//gets the decoder value
HICANN::DecoderDoubleRow HAL2ESS::get_decoder_double_row(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s)
{
	HICANN::DecoderDoubleRow returnval;

	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	//calculate the address
    Coordinate::SynapseRowOnHICANN top_row{s, Coordinate::RowOnSynapseDriver{geometry::top}};
    Coordinate::SynapseRowOnHICANN bot_row{s, Coordinate::RowOnSynapseDriver{geometry::bottom}};
    size_t top = format_synapse_row(top_row);
	size_t bot = format_synapse_row(bot_row);

	auto &hica = mhal_access->wafer().hicanns[hic_id];
	for(size_t i = 0; i < returnval[geometry::top].size(); ++i)
	{
		auto addr_top = HICANN::SynapseDecoder::from_bitset(hica.get_syn_address(top, i));
        returnval[geometry::top][i] = addr_top;
		auto addr_bot = HICANN::SynapseDecoder::from_bitset(hica.get_syn_address(bot, i));
		returnval[geometry::bottom][i] = addr_bot;
	}
	//getting the decoder double row from the ESS and asserting equality
	HICANN::DecoderDoubleRow returnval_ESS = get_decoder_double_row_ESS(h,s);
	(void) returnval_ESS;
	assert(returnval == returnval_ESS);
	return returnval;
}

//sets a synapse-row -> synapse driver and the two corresponding rows
void HAL2ESS::set_synapse_driver(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s, HICANN::SynapseDriver const& drv_row)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	//determine the block and the address inside the block TODO check if addressing is correct
	size_t side = to_synblock(s);
	size_t addr = to_drvaddr(s);

	LOG4CXX_DEBUG(_logger, "set_synapse_driver: Transformed HALbe-Address: " << s.id() << " to ESS-Address: Side: " << side << " Address inside Block: " <<  addr );
	LOG4CXX_TRACE(_logger, s);
	LOG4CXX_TRACE(_logger, drv_row);

	//syndriver config
	auto &syndriver = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].syndriver[addr];

    //syndriver enable/mirroring config
    if( drv_row.is_enabled() )
    {
	    syndriver.enable = true;
	    if( drv_row.is_l1() )
        {
	        syndriver.locin  = true;
            syndriver.mirror = false;	
	        LOG4CXX_DEBUG(_logger, "set_synapse_driver: Driver is l1" );
        }
        else if( drv_row.is_l1_mirror() )
        {
	        syndriver.locin  = true;
            syndriver.mirror = true;	
	        LOG4CXX_DEBUG(_logger, "set_synapse_driver: Driver is l1_mirror" );
        }
        else if( drv_row.is_mirror() )
        {
	        syndriver.locin  = false;
            syndriver.mirror = true;	
	        LOG4CXX_DEBUG(_logger, "set_synapse_driver: Driver is mirror" );
        }
        else if( drv_row.is_listen() )
        {
	        syndriver.locin  = false;
            syndriver.mirror = false;	
	        LOG4CXX_DEBUG(_logger, "set_synapse_driver: Driver is listen" );
        }
    }
    else
    {
        syndriver.enable = false;
        syndriver.locin  = false;
        if( drv_row.is_mirror_only() )
        {
            syndriver.mirror = true;
	        LOG4CXX_DEBUG(_logger, "set_synapse_driver: Driver is mirror_only" );
        }
        else
        {
            syndriver.mirror = false;
	        LOG4CXX_DEBUG(_logger, "set_synapse_driver: Driver is disabled" );
        }
    }
    
    //configure the STP stuff
    if( drv_row.is_stf())
    {
        syndriver.stp_enable = true;
	    syndriver.stp_mode   = 1;
    }
    else if( drv_row.is_std())
    {
        syndriver.stp_enable = true;
	    syndriver.stp_mode   = 0;
    }
    else
    {
        syndriver.stp_enable = false;
    }
	//set stp_cap
    unsigned char aux = 0;
    syndriver.stp_cap = aux;
    for(size_t ii = 0; ii < drv_row.stp_cap.size(); ++ii)
    {
        if(drv_row.stp_cap[ii] == true)
        syndriver.stp_cap |= bit::set(aux,ii);
    }

    // set the synderiver rows
    ESS::syndr_row & top_row = syndriver.top_row_cfg;
    ESS::syndr_row & bot_row = syndriver.bottom_row_cfg;
    //config the lines
    // get the synderiver row configuration
    Coordinate::RowOnSynapseDriver top_line;
    Coordinate::RowOnSynapseDriver bot_line;
    //upper half : 
    // bottom lines correspond to each other in ESS and HALbe
	// ESS counts from center of the chip to the top. Even rows in ESS are bottom rows in halbe
    if (addr < 56)
    {
        top_line = Coordinate::RowOnSynapseDriver(Coordinate::top);
        bot_line = Coordinate::RowOnSynapseDriver(Coordinate::bottom);
    }
    //lower half : 
    //addressing in ESS and HALbe are inverted
	// ESS counts from center of the chip to the bottom. Even rows in ESS are top rows in halbe
	else if (addr >= 56)
    {
        top_line = Coordinate::RowOnSynapseDriver(Coordinate::bottom);
        bot_line = Coordinate::RowOnSynapseDriver(Coordinate::top);
	}

    Coordinate::SynapseRowOnHICANN top_synrow{s,top_line};
    Coordinate::SynapseRowOnHICANN bot_synrow{s,bot_line};

    //set the decoder values
    top_row.preout_even = drv_row[top_line].get_decoder(Coordinate::top).value();
	LOG4CXX_DEBUG(_logger, "set_synapse_driver: even recorder belonging to " << top_synrow << " set to " <<(int) drv_row[top_line].get_decoder(Coordinate::top).value() );
    top_row.preout_odd  = drv_row[top_line].get_decoder(Coordinate::bottom).value();
	LOG4CXX_DEBUG(_logger, "set_synapse_driver: odd recorder belonging to " << top_synrow << " set to " << (int)drv_row[top_line].get_decoder(Coordinate::bottom).value() );
    bot_row.preout_even = drv_row[bot_line].get_decoder(Coordinate::top).value();
	LOG4CXX_DEBUG(_logger, "set_synapse_driver: even recorder belonging to " << bot_synrow << " set to " <<(int) drv_row[bot_line].get_decoder(Coordinate::top).value() );
    bot_row.preout_odd  = drv_row[bot_line].get_decoder(Coordinate::bottom).value();
	LOG4CXX_DEBUG(_logger, "set_synapse_driver: odd recorder belonging to " << bot_synrow << " set to " << (int)drv_row[bot_line].get_decoder(Coordinate::bottom).value() );
    //set the synapse types
	top_row.senx = drv_row[top_line].get_syn_in(Coordinate::left);	
	top_row.seni = drv_row[top_line].get_syn_in(Coordinate::right);	
	bot_row.senx = drv_row[bot_line].get_syn_in(Coordinate::left);	
	bot_row.seni = drv_row[bot_line].get_syn_in(Coordinate::right);	

	// set sel_Vgmax and gmax_div
	top_row.sel_Vgmax = drv_row[top_line].get_gmax();
	bot_row.sel_Vgmax = drv_row[bot_line].get_gmax();
	top_row.gmax_div_x = drv_row[top_line].get_gmax_div(Coordinate::left);
	top_row.gmax_div_i = drv_row[top_line].get_gmax_div(Coordinate::right);
	bot_row.gmax_div_x = drv_row[bot_line].get_gmax_div(Coordinate::left);
	bot_row.gmax_div_i = drv_row[bot_line].get_gmax_div(Coordinate::right);
}


//gets the configuration of a synapse driver and the two corresponding synrows
HICANN::SynapseDriver HAL2ESS::get_synapse_driver(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	HICANN::SynapseDriver returnval;

	//determine the block and the address inside the block TODO check if addressing is correct
	size_t side = to_synblock(s);
	size_t addr = to_drvaddr(s);

	//syndriver config
	auto &syndriver = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].syndriver[addr];
    
    if (syndriver.enable)
    {
        if( syndriver.locin == true && syndriver.mirror == false )
        {
            returnval.set_l1();
        }
        if( syndriver.locin == true && syndriver.mirror == true )
        {
            returnval.set_l1_mirror();
        }
        if( syndriver.locin == false && syndriver.mirror == true )
        {
            returnval.set_mirror();
        }
        if( syndriver.locin == false && syndriver.mirror == false )
        {
            returnval.set_listen();
        }
    }
    else
    {
        if(syndriver.mirror == true )
        {
            returnval.set_mirror_only();
        }
        else
        {
            returnval.disable();
        }
    }
	if(syndriver.stp_enable)
    {
        if(syndriver.stp_mode == 0)
        {
            returnval.set_std();
        }
        else if (syndriver.stp_mode == 1)
        {
            returnval.set_stf();
        }
    }
    else
    {
        returnval.disable_stp();
    }
    returnval.stp_cap = syndriver.stp_cap;

    // get the synderiver row configuration
    const ESS::syndr_row & top_row = syndriver.top_row_cfg;
    const ESS::syndr_row & bot_row = syndriver.bottom_row_cfg;
    Coordinate::RowOnSynapseDriver top_line;
    Coordinate::RowOnSynapseDriver bot_line;
    //upper half : 
    //addressing in ESS and HALbe are inverted
    if (addr < 56)
    {
        top_line = Coordinate::RowOnSynapseDriver(Coordinate::bottom);
        bot_line = Coordinate::RowOnSynapseDriver(Coordinate::top);
    }
    //lower half : 
    //addressing in ESS and HALbe are the same
	else if (addr >= 56)
    {
        top_line = Coordinate::RowOnSynapseDriver(Coordinate::top);
        bot_line = Coordinate::RowOnSynapseDriver(Coordinate::bottom);
	}
    //get the Decoder values
	returnval[top_line].set_decoder(Coordinate::top   , HICANN::DriverDecoder(top_row.preout_even));
	returnval[top_line].set_decoder(Coordinate::bottom, HICANN::DriverDecoder(top_row.preout_odd));
	returnval[bot_line].set_decoder(Coordinate::top   , HICANN::DriverDecoder(bot_row.preout_even));
	returnval[bot_line].set_decoder(Coordinate::bottom, HICANN::DriverDecoder(bot_row.preout_odd));
    //get the synapse types
    returnval[top_line].set_syn_in(geometry::left, top_row.senx);
	returnval[top_line].set_syn_in(geometry::right,top_row.seni);
    returnval[bot_line].set_syn_in(geometry::left, bot_row.senx);
	returnval[bot_line].set_syn_in(geometry::right,bot_row.seni);
	
    //getting the synapse driver from the ESS and asserting equality
	HICANN::SynapseDriver returnval_ESS = get_synapse_driver_ESS(h,s);
	(void) returnval_ESS;
	//assert(returnval == returnval_ESS);
	return returnval_ESS;
}


//sets the configuration of a denmem quad, aout and current_input are written, but not necessary for ESS! TODO check interconnections
void HAL2ESS::set_denmem_quad(Handle::HICANN const& h, Coordinate::QuadOnHICANN qb, HICANN::NeuronQuad const& nquad)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	//Configure the quad intraconnectivity
    auto& connection_config = mhal_access->wafer().hicanns[hic_id].connection_config[qb.value()];
    //vertical connections
    connection_config.vert[0] = nquad.getVerticalInterconnect(Coordinate::NeuronOnQuad::x_type{Coordinate::Enum{0}});
    connection_config.vert[1] = nquad.getVerticalInterconnect(Coordinate::NeuronOnQuad::x_type{Coordinate::Enum{1}});
    //horizontal connections
    connection_config.hori[0] = nquad.getHorizontalInterconnect(Coordinate::NeuronOnQuad::y_type{Coordinate::Enum{0}});
    connection_config.hori[1] = nquad.getHorizontalInterconnect(Coordinate::NeuronOnQuad::y_type{Coordinate::Enum{1}});
	LOG4CXX_DEBUG(_logger, "set_denmem_quad ("<< h.coordinate() << ", " << qb);
	LOG4CXX_DEBUG(_logger, "Interconnects within Quad:");
	LOG4CXX_DEBUG(_logger, "vert[0]=" << connection_config.vert[0]);
	LOG4CXX_DEBUG(_logger, "vert[1]=" << connection_config.vert[1]);
	LOG4CXX_DEBUG(_logger, "hori[0]=" << connection_config.hori[0]);
	LOG4CXX_DEBUG(_logger, "hori[1]=" << connection_config.hori[1]);

	//Configure the denmen
    for(auto nrn_on_quad : Coordinate::iter_all<Coordinate::NeuronOnQuad>())
    {
        Coordinate::NeuronOnHICANN nrn(qb,nrn_on_quad);									    //calculate the coordinate on hic
	    auto& neuron = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[nrn.id()];	//reference to the neuron
	    LOG4CXX_DEBUG(_logger, "set_denmem_quad: Configuration for Neuron " << nrn.id() << " on HICANN " << h.coordinate().toHICANNOnWafer().id() );
        neuron.activate_firing = nquad[nrn_on_quad].activate_firing();
	    LOG4CXX_DEBUG(_logger, "activate_firing was set to " << neuron.activate_firing );
        neuron.enable_output = nquad[nrn_on_quad].enable_aout();
	    LOG4CXX_DEBUG(_logger, "enable_analog_output was set to " << neuron.enable_output );
        neuron.enable_curr_input = nquad[nrn_on_quad].enable_current_input();
	    LOG4CXX_DEBUG(_logger, "enable_curr_input was set to " << neuron.enable_curr_input );
        neuron.enable_spl1_output = nquad[nrn_on_quad].enable_spl1_output();
	    LOG4CXX_DEBUG(_logger, "enable_spl1_output was set to " << neuron.enable_spl1_output );
        HICANN::L1Address l1 = nquad[nrn_on_quad].address();
        neuron.l1_address = static_cast<unsigned int>(l1);
	    LOG4CXX_DEBUG(_logger, "l1 address is " << neuron.l1_address );
        neuron.enable_fire_input = nquad[nrn_on_quad].enable_fire_input();
	    LOG4CXX_DEBUG(_logger, "enable_fire_input was set to " << neuron.enable_fire_input );
    }
}


//getter for the configuration of a neuron quad. the L1 address is fetched from the HAL2ESS datastructure and the ESS
HICANN::NeuronQuad HAL2ESS::get_denmem_quad(Handle::HICANN const& h, Coordinate::QuadOnHICANN qb)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	HICANN::NeuronQuad returnvalue;

	//get Interconnections
    const auto& connection_config = mhal_access->wafer().hicanns[hic_id].connection_config[qb.value()];
    //vertical connections
    returnvalue.setVerticalInterconnect(Coordinate::NeuronOnQuad::x_type{Coordinate::Enum{0}}, connection_config.vert[0]);
    returnvalue.setVerticalInterconnect(Coordinate::NeuronOnQuad::x_type{Coordinate::Enum{1}}, connection_config.vert[1]);
    //horizontal connections
    returnvalue.setHorizontalInterconnect(Coordinate::NeuronOnQuad::y_type{Coordinate::Enum{0}}, connection_config.hori[0]);
    returnvalue.setHorizontalInterconnect(Coordinate::NeuronOnQuad::y_type{Coordinate::Enum{1}}, connection_config.hori[1]);

	//get config of the denmem
    for(size_t ii = 0; ii <= 3; ii++)
    {
        Coordinate::NeuronOnQuad nrn_on_quad{Coordinate::NeuronOnQuad::enum_type{ii}};
        Coordinate::NeuronOnHICANN nrn_on_hic{qb, nrn_on_quad};
        auto& nrn = returnvalue[nrn_on_quad];
        const auto& nrn_conf = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[nrn_on_hic.id()];
		//get L1-address from datacontainer and from ESS and assert their equality
		auto L1Addr = HICANN::L1Address(nrn_conf.l1_address);
        auto L1Addr_ESS = get_L1Address_ESS(h, nrn_on_hic);
		(void) L1Addr_ESS;
		assert(L1Addr == L1Addr_ESS);
		nrn.address(L1Addr);
        //get flags from datacontainer
		nrn.activate_firing(nrn_conf.activate_firing);
        nrn.enable_aout(nrn_conf.enable_output);
        nrn.enable_current_input(nrn_conf.enable_curr_input);
        nrn.enable_fire_input(nrn_conf.enable_fire_input);
        nrn.enable_spl1_output(nrn_conf.enable_spl1_output);
    }

	return returnvalue;
}


//set the capacitance for a hole side of neurons
void HAL2ESS::set_neuron_config(Handle::HICANN const& h, HICANN::NeuronConfig const& nblock)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);
	if(nblock.bigcap[geometry::top] == true)
	{
		for(size_t i = 0; i < 256; ++i)		//set values for upper block
		{
			mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[i].cap = true;
        }
	}
	else
	{
		for(size_t i = 0; i < 256; ++i)		//set values for upper block
		{
			mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[i].cap = false;
        }
	}
	if(nblock.bigcap[geometry::bottom] == true)
	{
		for(size_t i = 256; i < 512; ++i)		//set values for lower block
		{
			mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[i].cap = true;
        }
	}
	else
	{
		for(size_t i = 256; i < 512; ++i)		//set values for lower block
		{
			mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[i].cap = false;
        }
	}
}


//get the capacitances, only from HAL2ESS datastructure, not from ESS yet
HICANN::NeuronConfig HAL2ESS::get_neuron_config(Handle::HICANN const& h)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	HICANN::NeuronConfig returnval{};

	auto const& upper_cap = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[0].cap;
	if(upper_cap == true)
		returnval.bigcap[geometry::top] = true;
	else if (upper_cap == false)
	{
		returnval.bigcap[geometry::top] = false;
	}
	auto const& lower_cap = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[256].cap;
	if(lower_cap == true)
		returnval.bigcap[geometry::bottom] = true;
	else if (lower_cap == false)
	{
		returnval.bigcap[geometry::bottom] = false;
	}
    return returnval;
}

// ESS is assumed to always succeed in writing analog values.
// Return empty list of errors.
HICANN::FGErrorResultRow HAL2ESS::wait_fg(Handle::HICANN &, Coordinate::FGBlockOnHICANN const & )
{
	return HICANN::FGErrorResultRow();
}

HICANN::FGErrorResultQuadRow HAL2ESS::wait_fg(Handle::HICANN &)
{
	// return empty error vectors to indicate that writing was successful
	return HICANN::FGErrorResultQuadRow();
}

//sets the fg values for a single fg-block
void HAL2ESS::set_fg_values(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& b, HICANN::FGBlock const& fg)
{
    //Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

    HICANN::FGControl fgc{};
	fgc[b] = fg;

	size_t fg_addr = static_cast<size_t>(b.id());
	
    LOG4CXX_DEBUG(_logger, "set_fg_values called for fg_block " << fg_addr );

    //**********************************************
    //set shared Parameters for Synapses and Syndriver
    //***********************************************

    // determine the coorect Syndriver address in the ESS
    //the side (left/right) corresponds to the side of the FGBlock
    int side = b.x();
    //top/bot corresponds to vertical position of fgblock
    int vert = b.y();
    ESS::SyndriverParameterHW& shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
    //set the HW-values
    shrd_param.V_stdf = fgc.getShared(b,HICANN::shared_parameter::V_stdf);
    shrd_param.V_dep  = fgc.getShared(b,HICANN::shared_parameter::V_dep);
    shrd_param.V_fac  = fgc.getShared(b,HICANN::shared_parameter::V_fac);
    shrd_param.V_dtc  = fgc.getShared(b,HICANN::shared_parameter::V_dtc);
    
    //****************************
    //set shared Neuron Parameters
    //****************************
    
    for(size_t i = 0; i < Coordinate::QuadOnHICANN::size; ++i) //Number Quads on HICANN = 128
    {
        Coordinate::QuadOnHICANN quad((Coordinate::Enum(i)));
        Coordinate::NeuronOnQuad nrn_on_quad((Coordinate::Enum(fg_addr)));
        Coordinate::NeuronOnHICANN nrn{quad, nrn_on_quad};
		size_t addr = static_cast<size_t>(nrn.id());
        auto &V_reset = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[addr].V_reset;
        //set V_reset (shared parameter) different block for shared param!
		HICANN::shared_parameter shrd = HICANN::shared_parameter::V_reset;
		V_reset = fgc.getShared(b,shrd);
    }
    
    //*********************
    //set Neuron Parameters
	//*********************

    size_t addr_offset = fg_addr*128; //calculating the address_offset
    for(size_t addr = addr_offset; addr < HICANN::FGBlock::fg_columns + addr_offset - 1; addr++)
	{
		//detemine coordinate of current neuron
		Coordinate::NeuronOnHICANN nrn{Coordinate::Enum{addr}};
		auto &param_for_ess = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[addr].neuron_parameters;
		//fill hw_params    Only write parameter that are used in the ESS i.e. not the technical parameter I_bexp, I_convi, I_convx, V_syni, V_synx, I_intbbi, I_intbbx, I_spikeamp
		HICANN::neuron_parameter type = HICANN::neuron_parameter::I_gl;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
        type = HICANN::neuron_parameter::E_l;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::I_gladapt;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::I_radapt;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::I_fire;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::V_exp;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::I_rexp;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::E_syni;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::E_synx;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::V_syntci;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::V_syntcx;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
		type = HICANN::neuron_parameter::I_pl;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
        type = HICANN::neuron_parameter::V_t;
		param_for_ess.setParam(type, fgc.getNeuron(nrn, type));
	}
}
    
HICANN::FGErrorResultQuadRow HAL2ESS::set_fg_row_values(Handle::HICANN & h, Coordinate::FGRowOnFGBlock row, HICANN::FGControl const& fg, bool const, bool const)
{
    //Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

    LOG4CXX_DEBUG(_logger, "set_fg_row_values called for row " << row );

    //iterate over the blocks
    for ( auto fg_block : Coordinate::iter_all<Coordinate::FGBlockOnHICANN>() )
    {
        // get the block coordinates
        size_t side = fg_block.x();
        //top/bot corresponds to vertical position of fgblock
        size_t vert = fg_block.y();
        
        // get the shared parameter
        // only the shared parameters V_reset, V_stdf, V_dep, V_fac and V_dtc are used
        try
        {
            HICANN::shared_parameter shared_param_type = HICANN::getSharedParameter(fg_block,row);
            // V_stdf
            if (shared_param_type == HICANN::shared_parameter::V_stdf)
            {
                ESS::SyndriverParameterHW & shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
                shrd_param.V_stdf = fg.getShared(fg_block,shared_param_type);
            }
            // V_dep
            else if (shared_param_type == HICANN::shared_parameter::V_dep)
            {
                ESS::SyndriverParameterHW & shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
                shrd_param.V_dep = fg.getShared(fg_block,shared_param_type);
            }
            // V_fac
            else if (shared_param_type == HICANN::shared_parameter::V_fac)
            {
                ESS::SyndriverParameterHW & shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
                shrd_param.V_fac = fg.getShared(fg_block,shared_param_type);
            }
            // V_dtc
            else if (shared_param_type == HICANN::shared_parameter::V_dtc)
            {
                ESS::SyndriverParameterHW & shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
                shrd_param.V_dtc = fg.getShared(fg_block,shared_param_type);
            }
            // V_reset (shared neuron parameter)
            else if (shared_param_type == HICANN::shared_parameter::V_reset)
            {
                //iterate over the quads
                for (auto quad : Coordinate::iter_all<Coordinate::QuadOnHICANN>())
                {
                    Coordinate::NeuronOnQuad nrn_on_quad( Coordinate::Enum(fg_block.id()) );
                    Coordinate::NeuronOnHICANN nrn(quad, nrn_on_quad);
	            	size_t addr = nrn.id();
                    auto & nrn_ess  = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[addr];
	            	nrn_ess.V_reset = fg.getShared(fg_block,shared_param_type);
                }
            }
        }
        catch (std::out_of_range)
        {}
        
        //get the neuron parameter
        // try to get the type, if this row is not connected continue
        try
        {
            HICANN::neuron_parameter nrn_param_type = HICANN::getNeuronParameter(fg_block,row);
            // set the neuron parameter
            for (auto nrn_fg : Coordinate::iter_all<Coordinate::NeuronOnFGBlock>())
            {
                Coordinate::NeuronOnHICANN nrn = nrn_fg.toNeuronOnHICANN(fg_block);
                size_t nrn_id = nrn.id();
		        auto & nrn_params = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[nrn_id].neuron_parameters;
                auto const param = fg.getNeuron(nrn, nrn_param_type);
                nrn_params.setParam(nrn_param_type, param);
            }
        }
        catch (std::out_of_range)
        {
            continue;
        }
    }

	// return empty error vectors to indicate that writing was successful
	return HICANN::FGErrorResultQuadRow{};
}

HICANN::FGErrorResultQuadRow HAL2ESS::set_fg_row_values(Handle::HICANN & h, Coordinate::FGBlockOnHICANN fg_block,
		Coordinate::FGRowOnFGBlock row,
		HICANN::FGRow const& fg, bool const, bool const)
{
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	// get the block coordinates
	size_t side = fg_block.x();
	//top/bot corresponds to vertical position of fgblock
	size_t vert = fg_block.y();

	// get the shared parameter
	// only the shared parameters V_reset, V_stdf, V_fac and V_dtc are used
	try
	{
		HICANN::shared_parameter shared_param_type = HICANN::getSharedParameter(fg_block,row);
		// V_stdf
		if (shared_param_type == HICANN::shared_parameter::V_stdf)
		{
			ESS::SyndriverParameterHW & shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
			shrd_param.V_stdf = fg.getShared();
		}
		// V_fac
		else if (shared_param_type == HICANN::shared_parameter::V_fac)
		{
			ESS::SyndriverParameterHW & shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
			shrd_param.V_fac = fg.getShared();
		}
		// V_dtc
		else if (shared_param_type == HICANN::shared_parameter::V_dtc)
		{
			ESS::SyndriverParameterHW & shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];
			shrd_param.V_dtc = fg.getShared();
		}
		// V_reset (shared neuron parameter)
		else if (shared_param_type == HICANN::shared_parameter::V_reset)
		{
			//iterate over the quads
			for (auto quad : Coordinate::iter_all<Coordinate::QuadOnHICANN>())
			{
				Coordinate::NeuronOnQuad nrn_on_quad( Coordinate::Enum(fg_block.id()) );
				Coordinate::NeuronOnHICANN nrn(quad, nrn_on_quad);
				size_t addr = nrn.id();
				auto & nrn_ess  = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[addr];
				nrn_ess.V_reset = fg.getShared();
			}
		}
	}
	catch (std::out_of_range)
	{}

	//get the neuron parameter
	// try to get the type, if this row is not connected continue
	try
	{
		HICANN::neuron_parameter nrn_param_type = HICANN::getNeuronParameter(fg_block,row);
		// set the neuron parameter
		for (auto nrn_fg : Coordinate::iter_all<Coordinate::NeuronOnFGBlock>())
		{
			Coordinate::NeuronOnHICANN nrn = nrn_fg.toNeuronOnHICANN(fg_block);
			size_t nrn_id = nrn.id();
			auto & nrn_params = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[nrn_id].neuron_parameters;
			auto const param = fg.getNeuron(nrn_fg);
			nrn_params.setParam(nrn_param_type, param);
		}
	}
	catch (std::out_of_range)
	{}

	// return empty error vectors to indicate that writing was successful
	return HICANN::FGErrorResultQuadRow{};
}

HICANN::FGErrorResultQuadRow HAL2ESS::set_fg_row_values(
		Handle::HICANN & h,
		HICANN::FGRowOnFGBlock4 rows,
		HICANN::FGRow4 data,
		bool const writeDown,
		bool const blocking)
{
	for (auto block : Coordinate::iter_all<Coordinate::FGBlockOnHICANN>())
	{
		set_fg_row_values(h, block, rows.at(block.id()), data.at(block.id()),
				          writeDown, blocking);
	}

	// return empty error vectors to indicate that writing was successful
	return HICANN::FGErrorResultQuadRow{};
}

//sets the fg values for a full FGControl
void HAL2ESS::set_fg_values(Handle::HICANN const& h, HICANN::FGControl const& fg)
{
	for (size_t i = 0; i < fg.size(); i++)
	{
        Coordinate::FGBlockOnHICANN block{geometry::Enum{i}};
        set_fg_values(h, block, fg[block]);
	}
}

//gets the analog fg_values, not possible that way in real Hardware
//not a fuction of HICANNBackend any,ore, but still here for tests
HICANN::FGBlock HAL2ESS::get_fg_values(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& addr)
{
	LOG4CXX_DEBUG(_logger, "get_fg_values for FGBlock " << addr.id() << " on HICANN " << h.coordinate().toHICANNOnWafer().id() );
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);
	//initialize FGcontrol
	HICANN::FGBlock fgb{};
    HICANN::FGControl fgc{};
	fgc[addr] = fgb;

	//calculate the address offset and cast the fg-addr to size_t
	size_t addr_offset = addr.id()*128;
	size_t fg_addr = static_cast<size_t>(addr.id());
    
	LOG4CXX_DEBUG(_logger, "Shared parameter:");
    //look for a neuron which gets shrd params from this block
    Coordinate::QuadOnHICANN quad{Coordinate::Enum{0}};
    Coordinate::NeuronOnQuad nrn_on_quad{Coordinate::Enum{fg_addr}};
    Coordinate::NeuronOnHICANN shrd_nrn{quad, nrn_on_quad};

	//getting V_reset = shared neuron parameter
    const auto &V_reset = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[static_cast<size_t>(shrd_nrn.id())].V_reset;
    auto shrd = HICANN::shared_parameter::V_reset;
	LOG4CXX_DEBUG(_logger, "V_reset = " << V_reset);
    fgc.setShared(addr, shrd, V_reset);

    // determine the coorect Syndriver address in the ESS
    //the side (left/right) corresponds to the side of the FGBlock
    int side = addr.x();
    //top/bot corresponds to vertical position of fgblock
    int vert = addr.y();
    const auto &shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];

	//get STP Parameter
    shrd = HICANN::shared_parameter::V_fac;
	LOG4CXX_DEBUG(_logger, "V_fac = " << shrd_param.V_fac);
	fgc.setShared(addr, shrd, shrd_param.V_fac);
	shrd = HICANN::shared_parameter::V_stdf;
	LOG4CXX_DEBUG(_logger, "V_stdf = " << shrd_param.V_stdf);
	fgc.setShared(addr, shrd, shrd_param.V_stdf);
	shrd = HICANN::shared_parameter::V_dtc;
	LOG4CXX_DEBUG(_logger, "V_dtc = " << shrd_param.V_dtc);
    fgc.setShared(addr, shrd, shrd_param.V_dtc);

	//getting the local neuron parameters
	for(size_t nrn_addr = addr_offset; nrn_addr < HICANN::FGBlock::fg_columns + addr_offset - 1; nrn_addr++)
	{
	    LOG4CXX_DEBUG(_logger, "Neuron Parameter for neuron " << nrn_addr << " :");
		const auto& param = mhal_access->wafer().hicanns[hic_id].neurons_on_hicann[nrn_addr].neuron_parameters;
		Coordinate::NeuronOnHICANN nrn{Coordinate::Enum{nrn_addr}};
        
		HICANN::neuron_parameter type = HICANN::neuron_parameter::I_gl;
	    LOG4CXX_DEBUG(_logger, "I_gl = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::E_l;
	    LOG4CXX_DEBUG(_logger, "E_l = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::E_syni;
	    LOG4CXX_DEBUG(_logger, "E_syni = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::E_synx;
	    LOG4CXX_DEBUG(_logger, "E_synx = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::I_fire;
	    LOG4CXX_DEBUG(_logger, "I_fire = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::I_gladapt;
	    LOG4CXX_DEBUG(_logger, "I_gladapt = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::I_pl;
	    LOG4CXX_DEBUG(_logger, "I_pl = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::I_radapt;
	    LOG4CXX_DEBUG(_logger, "I_radapt = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::I_rexp;
	    LOG4CXX_DEBUG(_logger, "I_rexp = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::V_exp;
	    LOG4CXX_DEBUG(_logger, "V_exp = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::V_syntci;
	    LOG4CXX_DEBUG(_logger, "V_syntci = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::V_syntcx;
	    LOG4CXX_DEBUG(_logger, "V_syntcx = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
		type = HICANN::neuron_parameter::V_t;
	    LOG4CXX_DEBUG(_logger, "V_t = " << param.getParam(type));
        fgc.setNeuron(nrn, type, param.getParam(type));
	}
    auto returnval_ESS = get_fg_values_ESS(h,addr);
    //assert(fgc[addr] == returnval_ESS);
    return returnval_ESS;
}

void HAL2ESS::set_fg_config(
		Handle::HICANN & h,
		Coordinate::FGBlockOnHICANN const& b,
		const HICANN::FGConfig & cfg)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);
    size_t fg_addr = b.id();

	LOG4CXX_DEBUG(_logger, "set_fg_config called for FBBlock " << fg_addr
			               << "(note: only pulselenght is used)");

    auto &stimulus = mhal_access->wafer().hicanns[hic_id].stimulus_config[fg_addr];
    stimulus.PulseLength = cfg.pulselength.to_ulong();
}

HICANN::FGConfig HAL2ESS::get_fg_config(
		Handle::HICANN const& h,
		Coordinate::FGBlockOnHICANN const& b)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);
	size_t fg_addr = b.id();

	LOG4CXX_DEBUG(_logger, "get_fg_config called for FBBlock " << fg_addr
			               << "(note: only pulselenght is read)");

	auto &stimulus = mhal_access->wafer().hicanns[hic_id].stimulus_config[fg_addr];

	ESS_DUMMY();
	HICANN::FGConfig cfg;

	cfg.pulselength = stimulus.PulseLength;
	return cfg;
}

//sets the configuration for the current stimuli
void HAL2ESS::set_current_stimulus(
		Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& b,
		HICANN::FGStimulus const& stim)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

    size_t fg_addr = b.id();
	LOG4CXX_DEBUG(_logger, "set_current_stimulus called for FBBlock "
		                   << fg_addr);

    auto &stimulus = mhal_access->wafer().hicanns[hic_id].stimulus_config[fg_addr];

    stimulus.Continuous = stim.getContinuous();

    LOG4CXX_DEBUG(_logger, "Continuous set to " << stim.getContinuous()
			               << ". Pulselength was set by set_fg_config to "
						   << stimulus.PulseLength);

    auto &curr = stimulus.Currents;
    assert(curr.size() == stim.size());
    for(size_t i = 0; i < stim.size(); ++i)
    {
        curr[i] = stim[i];
        if (curr[i] != 0)
            LOG4CXX_DEBUG(_logger, "Pulse  " << i
					               << " is set to non-zero value "
								   << (int)curr[i]);
    }
}

//gets the configuration of the current stimuli
//TODO get from ESS, too
HICANN::FGStimulus HAL2ESS::get_current_stimulus(
		Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& b)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

    HICANN::FGStimulus returnval;

    size_t fg_addr = b.id();
    auto const& stimulus = mhal_access->wafer().hicanns[hic_id].stimulus_config[fg_addr]; 

    returnval.setContinuous(stimulus.Continuous);

    auto const& curr = stimulus.Currents;
    assert(curr.size() == returnval.size());
    for(size_t i = 0; i < returnval.size(); ++i)
        returnval[i] = curr[i];

    return returnval;
}

// bit order differs from HALBE to GMAccess for RepeaterConfig!!

//sets a vertical repeater
void HAL2ESS::set_repeater(Handle::HICANN const& h, Coordinate::VRepeaterOnHICANN r, HICANN::VerticalRepeater const& rc)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	size_t y = static_cast<size_t>(r.toVLineOnHICANN());

	//determine the repeater block
	ESS::RepeaterLocation rep_block = to_repblock(r.toRepeaterBlockOnHICANN());

	//determine the address inside block
	size_t addr = to_repaddr(r.toRepeaterBlockOnHICANN(), y);

	LOG4CXX_DEBUG(_logger,
	              "set_repeater: Transformed HALbe-Address: VRepeaterLine "
	                  << r.toVLineOnHICANN().value()
	                  << " to ESS-Address: Block: " << rep_block
	                  << " Address inside Block: " << addr);

	auto &rep_conf = mhal_access->wafer().hicanns[hic_id].repeater_config[rep_block].repeater[addr];

	unsigned char recen = 0x80;	//recen-bit -> bit-7 = 1
	unsigned char dir = 0x40;	//dir-bit ->  bit-6 = 1
	unsigned char off = 0;

	//determine the recen-bit -> MODE = IDLE -> recen = 0; MODE = FORWARDING -> recen = 1
	if(rc.getMode() == HICANN::Repeater::Mode::IDLE)
		rep_conf = off;
	else if(rc.getMode() == HICANN::Repeater::Mode::FORWARDING)
		rep_conf = recen;

	//TODO Check if other modes are important for the ESS, as far ass i can see they are not...

	//determine the direction bit
	if(y < 128) //left HICANN-Side
	{
		if(y%2) //odd lines are operated by the top repeater-blocks => inward direction is down
			rep_conf |= (rc.getBottom()) ? dir : off;
		else			//even lines are operated by bottom repeater blocks => inward direction is up
			rep_conf |= (rc.getTop()) ? dir : off;
	}
	else //right HICANN-Side
	{
		if(y%2)	//odd lines are operated by bottom repeater blocks => inward direction is up
			rep_conf |= (rc.getTop()) ? dir : off;
		else			//even lines are operated by top repeater block => inward direction is down
			rep_conf |= (rc.getBottom()) ? dir : off;
	}
}


//gets configuration of a vertical repeater from HAL2ESS datastructure and ESS
HICANN::VerticalRepeater HAL2ESS::get_repeater(Handle::HICANN const& h, Coordinate::VRepeaterOnHICANN r)
{
	HICANN::VerticalRepeater returnval;

	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	size_t y = static_cast<size_t>(r.toVLineOnHICANN());

	//determine the repeater block
	ESS::RepeaterLocation rep_block = to_repblock(r.toRepeaterBlockOnHICANN());

	//determine the address inside block
	size_t addr = to_repaddr(r.toRepeaterBlockOnHICANN(), y);

	const auto &config = mhal_access->wafer().hicanns[hic_id].repeater_config[rep_block].repeater[addr];

	//determine the mode
	if(bit::test(config,7) == 0)	//recen = 0
		returnval.setIdle();
	else if(bit::test(config,7) == 1) //recen = 1
	{
		//determine the direction
		if(y < 128)
		{ //left HICANN side
			if(y % 2)
			{
				if(bit::test(config,6) == 1)
					returnval.setForwarding(geometry::bottom);
				else
					returnval.setForwarding(geometry::top);
			}
			else
			{
				if(bit::test(config,6) == 1)
					returnval.setForwarding(geometry::top);
				else
					returnval.setForwarding(geometry::bottom);
			}
		}
		else
		{ //right HICANN-Side
			if(y % 2)
			{
				if(bit::test(config,6) == 1)
					returnval.setForwarding(geometry::top);
				else
					returnval.setForwarding(geometry::bottom);
			}
			else
			{
				if(bit::test(config,6) == 1)
					returnval.setForwarding(geometry::bottom);
				else
					returnval.setForwarding(geometry::top);
			}
		}
	}
	//get the repeater config from the ESS and assert equality
	HICANN::VerticalRepeater returnval_ESS = get_repeater_ESS(h, r);
	(void) returnval_ESS;
	assert(returnval == returnval_ESS);
	return returnval;
}


//sets a horizontal repeater
void HAL2ESS::set_repeater(Handle::HICANN const& h, Coordinate::HRepeaterOnHICANN r, HICANN::HorizontalRepeater const& rc)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	size_t x = static_cast<size_t>(r.toHLineOnHICANN());

	//determine the repeater block
	ESS::RepeaterLocation rep_block = to_repblock(r.toRepeaterBlockOnHICANN());
	//determine the address inside block
	size_t addr = to_repaddr(r.toRepeaterBlockOnHICANN(), x);
    LOG4CXX_DEBUG(_logger,
			"set_repeater: Transformed HALbe-Address: HRepeaterLine " <<
			r.toHLineOnHICANN().value() << " to ESS-Address: Block: " <<
			rep_block << " Address inside Block: " <<  addr );

    //get configuration for ESS
	auto& rep_conf = mhal_access->wafer().hicanns[hic_id].repeater_config[rep_block].repeater[addr];

	unsigned char recen = 0x80;	//recen-bit -> bit-7 = 1
	unsigned char dir = 0x40;	//dir-bit ->  bit-6 = 1
	unsigned char tinen = 0x10; //tinen-bit -> bit4 = 1
	unsigned char off = 0;

	assert (bool(x % 2) == bool(rep_block == ESS::REP_R));

	//determine the recen-bit -> MODE = IDLE -> recen = 0; MODE = FORWARDING -> recen = 1
	if(rc.getMode() == HICANN::Repeater::Mode::IDLE)
		rep_conf = off;
	else if(rc.getMode() == HICANN::Repeater::Mode::FORWARDING)
	{
        LOG4CXX_DEBUG(_logger, "set_repeater: forwarding for H-repeater" << r << " on " << h.coordinate() << " left " << rc.getLeft() << " right " << rc.getRight() );
		// FIXME: PM: what about sending repeaters in forward mode? is recen/tinen semantics the same as in hicann doc? (compare set_repeater[_helper] in halbe)
		// FIXME: PM: adapt repeater representation in the ess that conforms to HALBE/HW settings
		if (addr % 4 || rep_block == ESS::REP_R) {
			rep_conf = recen;
			//determine the direction
			//if (x % 2) //odd lines are operated by right repeater block => inwards direction is to the left
			if (rep_block == ESS::REP_R) {
				rep_conf |= (rc.getLeft()) ? dir : off;
			}
			else {      //even lines are operated by left repeater block => inwards direction is to the right
				rep_conf |= (rc.getRight()) ? dir : off;
			}
		}
		else {  // SPL1
				// backward == (tinen=0;recen=0;dir=1)
				// forward  == (tinen=0;recen=1;dir=0)
			unsigned char backward = dir;
			unsigned char forward = recen;
			assert (!(rc.getLeft() && rc.getRight())); // we can't  handle this
			if (rep_block == ESS::REP_R) {
				rep_conf = (rc.getLeft()) ? backward : forward;
			}
			else {
				rep_conf = (rc.getRight()) ? backward : forward;
			}
		}
	}
	//only for sending repeater
	else if(rc.getMode() == HICANN::Repeater::Mode::OUTPUT && r.isSending())
	{
        LOG4CXX_DEBUG(_logger,
				"set_repeater: output for H-repeater" << r << " on " <<
				h.coordinate() << " left " << rc.getLeft() << " right " <<
				rc.getRight() );
		rep_conf = tinen;
		if(rc.getRight()) //if dir->right = 1 set dir-bit
			rep_conf |= dir;
		if(rc.getLeft()) //if dir->left = 1 set recen-bit
			rep_conf |= recen;
	}
}


//gets a horizontal repeater from the HAL2ESS datastructure and the ESS
HICANN::HorizontalRepeater HAL2ESS::get_repeater(
		Handle::HICANN const& h, Coordinate::HRepeaterOnHICANN r)
{
	HICANN::HorizontalRepeater returnval;

	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	size_t x =  static_cast<size_t>(r.toHLineOnHICANN());
	//determine the repeater block
	ESS::RepeaterLocation rep_block = to_repblock(r.toRepeaterBlockOnHICANN());
	//determine the address inside block
	size_t addr = to_repaddr(r.toRepeaterBlockOnHICANN(), x);
	//get configuration for ESS
	const auto& config = mhal_access->wafer().hicanns[hic_id].repeater_config[rep_block].repeater[addr];

	//determine the mode
	if(bit::test(config,7) == 0 && bit::test(config,6) == 0 && bit::test(config,4) == 0)
		returnval.setIdle();
	else if(bit::test(config,7) == 1 && bit::test(config,4) == 0)
	{
		if(x%2)
		{
			if(bit::test(config,6) == 1)
				returnval.setForwarding(geometry::left);
			else
				returnval.setForwarding(geometry::right);
		}
		else
		{
			if(bit::test(config,6) == 1)
					returnval.setForwarding(geometry::right);
			else
					returnval.setForwarding(geometry::left);
		}
	}
	else if(bit::test(config,4) == 1 && r.isSending())	//only for sending repeater
	{
		if(bit::test(config,6) == 1)
			returnval.setOutput(geometry::right);
		if(bit::test(config,7) == 1)
			returnval.setOutput(geometry::left);
	}
	//get the repeater from the ESS and assert equality
	HICANN::HorizontalRepeater returnval_ESS = get_repeater_ESS(h,r);
	(void) returnval_ESS;
	assert(returnval == returnval_ESS);
	return returnval;
}


//configures the first 3 bit of the 15 mergers of the merger tree
void HAL2ESS::set_merger_tree(Handle::HICANN const& h, HICANN::MergerTree const& m)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;
    
    //BG-Merger
    for(uint8_t i = 0; i < Coordinate::Merger0OnHICANN::end; i++)
    {
        Coordinate::Merger0OnHICANN merger{i};
        mergerconfig[0][format_merger(i)] = m[merger].config[HICANN::Merger::enable_bit];
        mergerconfig[1][format_merger(i)] = m[merger].config[HICANN::Merger::select_bit];
        mergerconfig[2][format_merger(i)] = m[merger].slow;
    }
    
    //get config for level0 merger 
	for(uint8_t i = 0; i < Coordinate::Merger1OnHICANN::end; i++)
	{
        Coordinate::Merger1OnHICANN merger{i};
		size_t off = Coordinate::Merger0OnHICANN::end;
        mergerconfig[0][format_merger(i+off)] = m[merger].config[HICANN::Merger::enable_bit];	//enable_bit
        mergerconfig[1][format_merger(i+off)] = m[merger].config[HICANN::Merger::select_bit];	//slect_bit
		mergerconfig[2][format_merger(i+off)] = m[merger].slow;								//slow_bit
	}
	//get config for level1 merger
    for(uint8_t i = 0; i < Coordinate::Merger2OnHICANN::end; i++)
	{
        Coordinate::Merger2OnHICANN merger{i};
		size_t off = Coordinate::Merger0OnHICANN::end + Coordinate::Merger1OnHICANN::end;
        mergerconfig[0][format_merger(i+off)] = m[merger].config[HICANN::Merger::enable_bit];	//enable_bit
        mergerconfig[1][format_merger(i+off)] = m[merger].config[HICANN::Merger::select_bit];	//slect_bit
		mergerconfig[2][format_merger(i+off)] = m[merger].slow;								//slow_bit
	}
    //get config for level 2 merger
    for(uint8_t i = 0; i < Coordinate::Merger3OnHICANN::end; i++)
	{
        Coordinate::Merger3OnHICANN merger{i};
		size_t off = Coordinate::Merger0OnHICANN::end + Coordinate::Merger1OnHICANN::end + Coordinate::Merger2OnHICANN::end;
        mergerconfig[0][format_merger(i+off)] = m[merger].config[HICANN::Merger::enable_bit];	//enable_bit
        mergerconfig[1][format_merger(i+off)] = m[merger].config[HICANN::Merger::select_bit];	//slect_bit
		mergerconfig[2][format_merger(i+off)] = m[merger].slow;								//slow_bit
	}
}


//reads out the mergertreeconfig
//TODO get from ESS too
HICANN::MergerTree HAL2ESS::get_merger_tree(Handle::HICANN const& h)
{
	HICANN::MergerTree returnvalue{};

	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	const auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;

    //BG-Merger
    for(uint8_t i = 0; i < Coordinate::Merger0OnHICANN::end; i++)
    {
        Coordinate::Merger0OnHICANN merger{i};
        returnvalue[merger].config[HICANN::Merger::enable_bit] = mergerconfig[0][format_merger(i)];
        returnvalue[merger].config[HICANN::Merger::select_bit] = mergerconfig[1][format_merger(i)];
        returnvalue[merger].slow = mergerconfig[2][format_merger(i)];
    }

    //level 0 Merger
	for(uint8_t i = 0; i < Coordinate::Merger1OnHICANN::end; i++)
	{
        Coordinate::Merger1OnHICANN merger{i};
		size_t off = Coordinate::Merger0OnHICANN::end;
        returnvalue[merger].config[HICANN::Merger::enable_bit] = mergerconfig[0][format_merger(i+off)];
		returnvalue[merger].config[HICANN::Merger::select_bit] = mergerconfig[1][format_merger(i+off)];
		returnvalue[merger].slow = mergerconfig[2][format_merger(i+off)];
	}
    
    //level 1 Merger
	for(uint8_t i = 0; i < Coordinate::Merger2OnHICANN::end; i++)
	{
        Coordinate::Merger2OnHICANN merger{i};
		size_t off = Coordinate::Merger0OnHICANN::end + Coordinate::Merger1OnHICANN::end;
        returnvalue[merger].config[HICANN::Merger::enable_bit] = mergerconfig[0][format_merger(i+off)];
		returnvalue[merger].config[HICANN::Merger::select_bit] = mergerconfig[1][format_merger(i+off)];
		returnvalue[merger].slow = mergerconfig[2][format_merger(i+off)];
	}
    
    //level 2 Merger
	for(uint8_t i = 0; i < Coordinate::Merger3OnHICANN::end; i++)
	{
        Coordinate::Merger3OnHICANN merger{i};
        size_t off = Coordinate::Merger0OnHICANN::end + Coordinate::Merger1OnHICANN::end + Coordinate::Merger2OnHICANN::end;
        returnvalue[merger].config[HICANN::Merger::enable_bit] = mergerconfig[0][format_merger(i+off)];
		returnvalue[merger].config[HICANN::Merger::select_bit] = mergerconfig[1][format_merger(i+off)];
		returnvalue[merger].slow = mergerconfig[2][format_merger(i+off)];
	}
	return returnvalue;
}


//sets the dnc-merger-config
void HAL2ESS::set_dnc_merger(Handle::HICANN const& h, HICANN::DNCMergerLine const& m)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;

	for(uint8_t j = 0; j < HICANN::DNCMergerLine::num_merger; ++j)
	{
		// FIXME: PM: use HICANNBackendHelper here
		uint8_t i = 7 - j;
        Coordinate::DNCMergerOnHICANN merger{j};
        mergerconfig[3][i] = m[merger].loopback;    					    //loopback_enable bit is stored in address 3 from bit 0 to 7
		mergerconfig[3][8+i] = m[merger].config[HICANN::Merger::select_bit];//select_bit is stored in address 3 from bit 8 to 15L
		mergerconfig[15][i] = m[merger].config[HICANN::Merger::enable_bit];	//enable_bit is stored in address 15 from bit 0 to 7
		mergerconfig[15][i+8] = m[merger].slow;								//slow_bit is stored in address 15 from bit 8 to 15
	}
}


//gets the dnc-merger-config
//TODO get from ESS too
HICANN::DNCMergerLine HAL2ESS::get_dnc_merger(Handle::HICANN const& h)
{
	HICANN::DNCMergerLine returnval{};

	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	const auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;

	for(uint8_t j = 0; j < HICANN::DNCMergerLine::num_merger; ++j)
	{
		// FIXME: PM: use HICANNBackendHelper here
		uint8_t i = 7 - j;
        Coordinate::DNCMergerOnHICANN merger{j};
        returnval[merger].loopback = mergerconfig[3][i];
		returnval[merger].config[HICANN::Merger::select_bit] = mergerconfig[3][8+i];
		returnval[merger].config[HICANN::Merger::enable_bit] = mergerconfig[15][i];
		returnval[merger].slow = mergerconfig[15][8+i];
	}
	return returnval;
}

//sets the directions of the 8  dnc<->hicann channels
void HAL2ESS::set_gbit_link(Handle::HICANN const& h, HICANN::GbitLink const& link)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);
    
    LOG4CXX_DEBUG(_logger, "set_gbit_link: hicann " << hic_id);

	auto &enable = mhal_access->wafer().hicanns[hic_id].dnc_link_enable;
	auto &directions = mhal_access->wafer().hicanns[hic_id].dnc_link_directions;

	assert(link.dirs.size() == directions.size());
	for(size_t i = 0; i < link.dirs.size(); ++i)
	{
		// FIXME: PM: use HICANNBackendHelper here
		int ess_idx = 7 - i;
        LOG4CXX_DEBUG(_logger, "set_gbit_link: HALbe-Coordinate: " << i << " ESS-Coordinate: " << ess_idx );

		if(link.dirs[i] == HICANN::GbitLink::Direction::OFF)		//set enable[i] = false, direction is not defined here
		{
			enable[ess_idx] = false;
			directions[ess_idx] = ESS::DNCIfDirections::OFF;
            LOG4CXX_DEBUG(_logger, "DNCInterface OFF");
		}
		if(link.dirs[i] == HICANN::GbitLink::Direction::TO_HICANN)	//set enable[i] = true and directions[i] = true -> to hicann
		{
			enable[ess_idx] = true;
			directions[ess_idx] = ESS::DNCIfDirections::TO_HICANN;
            LOG4CXX_DEBUG(_logger, "DNCInterface TO_HICANN");
		}
		if(link.dirs[i] == HICANN::GbitLink::Direction::TO_DNC)	//set enable[i] = true and directions[i] = false -> to dnc
		{
			enable[ess_idx] = true;
			directions[ess_idx] = ESS::DNCIfDirections::TO_DNC;
            LOG4CXX_DEBUG(_logger, "DNCInterface TO_DNC");
		}
	}
}


//sets the phase, as far as i can tell this is not properly implemented in GMAcces, i implemented it anyhow, hope it wont hurt....
void HAL2ESS::set_phase(Handle::HICANN const& h, phase_t phase)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;

	for(size_t i = 0; i < phase.size(); ++i)
	{
		mergerconfig[5][i] = phase[i];	//phase is stored in adress 5, bits 0 to 7. bits 8 to 15 are not connected.
	}

}


//gets the phase
//TODO get from ESS too
HAL2ESS::phase_t HAL2ESS::get_phase(Handle::HICANN const& h)
{
	phase_t returnval{};

	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	const auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;

	for(size_t i = 0; i < returnval.size(); ++i)
	{
		returnval[i] = mergerconfig[5][i];
	}
	return returnval;
}


//configures the background generators ( bit of the merger tree)
void HAL2ESS::set_background_generator(Handle::HICANN const& h, HICANN::BackgroundGeneratorArray const& bg)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;

	for(size_t i = 0; i < bg.size(); i++)
	{
		mergerconfig[4][format_merger(i)] = bg[i].enable(); //set the reset_n-bit which is stored in address 4 bits 0 to 7
		mergerconfig[4][format_merger(i)+bg.size()] = bg[i].random(); //set the random bit, which is stored in address 4 bits 7 to 15
		//set the period
		std::vector<bool> out(16);
		for(size_t ii = 0; ii < out.size(); ++ii)
		{
			out[ii] = bit::test(bg[i].period(),ii);
		}
		mergerconfig[7+format_merger(i)] = out; //periods for bg 0 to 7 in 7 to 14
	}
	//set the l1_addresses
	for(size_t j = 0; j < 4; ++j)
	{
		std::bitset<6> adr_even{bg[2*j].address().value()};
		std::bitset<6> adr_odd{bg[2*j+1].address().value()};
		for(size_t jj = 0; jj < adr_even.size(); ++jj)
		{
			mergerconfig[16+format_merger(2*j+1)/2][jj] = adr_odd[jj];		//l1address of even bg are written to bits 0 to 5
			mergerconfig[16+format_merger(2*j)/2][jj+8] =	adr_even[jj];	//l1address of odd bg are written to bits 8 to 13
		}
	}
	//set the common seed, as far as i can tell, this is not done in GMAcces. i implemented it anyhow, hope it won t hurt...
	for(size_t i = 0; i < mergerconfig[6].size(); ++i)
	{
		mergerconfig[6][i] = bit::test(bg[0].seed(),i);	//the seed is common, so it doesnt matter which backgroundgenerator
	}
}


//reads out the bg-config from HAL2ESS datastructure and from the ESS
HICANN::BackgroundGeneratorArray HAL2ESS::get_background_generator(Handle::HICANN const& h)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);

	HICANN::BackgroundGeneratorArray returnval;

	const auto &mergerconfig = mhal_access->wafer().hicanns[hic_id].merger_tree_config;

	for(size_t i = 0; i < returnval.size(); i++)
	{
		returnval[i].enable(mergerconfig[4][format_merger(i)]);					//get the enable bit
		returnval[i].random(mergerconfig[4][format_merger(i)+returnval.size()]);	//get the random bit
		//get the period
		uint16_t p = 0;
		for(size_t ii = 0; ii < mergerconfig[7+format_merger(i)].size(); ++ii)
		{
			if(mergerconfig[7+format_merger(i)][ii])
				p += pow(2,ii);
		}
		returnval[i].period(p);
	}

	//get the l1address
	std::bitset<6> adr_even{};
	std::bitset<6> adr_odd{};
	for(size_t i = 0; i < 4; i++)
	{
		for(size_t ii = 0; ii < adr_even.size(); ++ii)
		{
			adr_odd[ii] = mergerconfig[16+format_merger(2*i+1)/2][ii];
			adr_even[ii] = mergerconfig[16+format_merger(2*i)/2][ii+8];
		}
		returnval[2*i].address(HICANN::L1Address(adr_even.to_ulong()));
		returnval[2*i+1].address(HICANN::L1Address(adr_odd.to_ulong()));
	}

	//get the common seed
	uint16_t seeed = 0;
	for(size_t i = 0; i < mergerconfig[6].size(); ++i)
	{
		if(mergerconfig[6][i])
			seeed += pow(2,i);
	}
	for(size_t i = 0; i < returnval.size(); ++i)
	{
		returnval[i].seed(seeed);
	}
	//get the BGarray from the ESS and assert equality
	HICANN::BackgroundGeneratorArray returnval_ESS = get_background_generator_ESS(h);
	(void) returnval_ESS;
	//seed is not gettable from ess and the enable bit is not read out correctly yet TODO...
	for(size_t i = 0; i < returnval.size(); ++i)
	{
		//assert(returnval[i].enable() == returnval_ESS[i].enable());
		assert(returnval[i].random() == returnval_ESS[i].random());
		assert(returnval[i].address() == returnval_ESS[i].address());
		assert(returnval[i].period() == returnval_ESS[i].period());
	}

	return returnval;
}
    
void HAL2ESS::set_analog(Handle::HICANN const& h, HICANN::Analog const& a)
{
    //get reticle id of this hicann a.k.a DNC id
	size_t reticle = h.coordinate().toDNCGlobal().toDNCOnWafer().toEnum();
    auto & analog_config = mhal_access->wafer().adcs;
    for( auto analog : Coordinate::iter_all<Coordinate::AnalogOnHICANN>() )
    {
        std::bitset<4> analog_input;
		auto & config = analog_config.at(2*reticle + analog.value());
        analog_input[0] = a.get_membrane_bot_odd(analog);
		analog_input[1] = a.get_membrane_top_odd(analog);
		analog_input[2] = a.get_membrane_bot_even(analog);
		analog_input[3] = a.get_membrane_top_even(analog);
        //check if an input was activated
        unsigned int check = analog_input.to_ulong();
        if (check == 0)
        {
            LOG4CXX_DEBUG(_logger, "set_analog called on HICANN " << h.coordinate() << " no analog input activated for channel" << analog );
        }
        //make sure only one is activated //TODO does this correspond to hardware?
        if (check == 3 || check == 5 || check == 6 || check == 7)
        {
            LOG4CXX_WARN(_logger, "set_analog called on HICANN " << h.coordinate() << 
                    " but more than one analog input activated for channel" << analog 
                    << " no input was activated" );
            analog_input = std::bitset<4>(0);
        }
        config.input = analog_input;
    }
}

HICANN::Analog HAL2ESS::get_analog(Handle::HICANN const& h)
{
	size_t reticle = h.coordinate().toDNCGlobal().toDNCOnWafer().toEnum();
    const auto & analog_config = mhal_access->wafer().adcs;
    HICANN::Analog returnval;
    for( auto analog : Coordinate::iter_all<Coordinate::AnalogOnHICANN>() )
    {
        size_t addr = 2*reticle + analog.value();
        std::bitset<4> input = analog_config.at(addr).input;
        if(input[0] == true)
        {
            returnval.set_membrane_bot_odd(analog);
        }
        else if(input[1] == true)
        {
            returnval.set_membrane_top_odd(analog);
        }
        else if(input[2] == true)
        {
            returnval.set_membrane_bot_even(analog);
        }
        else if(input[3] == true)
        {
            returnval.set_membrane_top_even(analog);
        }
    }
    return returnval;
}


//initializes a hicann
void HAL2ESS::init(Handle::HICANN const& h, bool)
{
    //TODO this function should not reset everything, but have same behaviour as on HW
    instantiate_hicann(h.coordinate());
}


//initializes a hicann
void HAL2ESS::instantiate_hicann(Coordinate::HICANNOnWafer const& h)
{
    assert(h.id() < ESS::size::hicanns_on_wafer);
	unsigned int ident = h.id();
    LOG4CXX_DEBUG(_logger, "Init called for HICANN " << ident);
	ESS::hicann hic{ident};
	hic.available = true;
	mhal_access->wafer().hicanns.at(ident) = hic;
}
	
//calls init, but additionally sets the PLL Frequency
void HAL2ESS::reset(Handle::HICANN const& h, uint8_t PLL_frequency)
{
	//Calculate the hicann coordinate
	auto e = h.coordinate().toHICANNOnWafer().id();
	auto hic_id = static_cast<size_t>(e);
    init(h);
    mhal_access->wafer().hicanns[hic_id].PLL_freq = PLL_frequency; 
	
    LOG4CXX_DEBUG(_logger, "Reset called for HICANN " << hic_id << ". Setting PLL-Frequency to " << (int) PLL_frequency << " MHz");
}

//************
//DNC Backend
//************

void HAL2ESS::set_hicann_directions(Handle::FPGA & f, Coordinate::DNCOnFPGA const& dof, DNC::GbitReticle const& gbit_reticle ){
	size_t dnc_id = f.dnc(dof).toDNCOnWafer().toEnum();
	std::bitset<64> & dirs = mDNCConfig[dnc_id].hicann_directions;
	// same conversion as for real hardware
	for (auto hicann : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() ) {
		auto link = gbit_reticle[hicann];
		size_t j = hicann.x()*2 + hicann.y(); // convert to numbering on DNC
		size_t hicann_offset = j*8;
		std::bitset<8> temp;
		///swap the direction bits according to new coordinates
		for (int i=0; i<8; i++){ //looping over channels
			dirs.set( hicann_offset + 7-i, (link.dirs[i]==HMF::HICANN::GbitLink::Direction::TO_HICANN)?true:false );
		}
	}
	// TODO: timestamp enable (currently always enabled on ESS)
}

//************
//FPGA Backend
//************

void HAL2ESS::reset(Handle::FPGA const& )
{}

void HAL2ESS::set_fpga_background_generator(Handle::FPGA const&, Coordinate::DNCOnFPGA const, FPGA::BackgroundGenerator const&)
{}

void HAL2ESS::write_playback_pulses(Handle::FPGA const& f, FPGA::PulseEventContainer const& st, FPGA::PulseEvent::spiketime_t /*runtime*/, uint16_t fpga_hicann_delay)
{
	size_t fpga_id = f.coordinate().value();
	auto & playback_pulses = mFPGAConfig[fpga_id].playback_pulses;

	// several calls of write_playback_pulses are currently not allowed here, as we don't track the last fpga release time in this function.
	// Hence, the existing pulses are cleared. 
	if ( playback_pulses.size() ) {
		LOG4CXX_WARN(_logger, "write_playback_pulses (..): There are already " << playback_pulses.size() << " pulses, which will be overwritten!");
		playback_pulses.clear();
	}

	// transform pulses
	// NOTE: this code was adapted from systemsim/s2c_systemsim::flush in master branch

	// The l2 fpga can send maximally 1 Pulse per clock cycle (8 ns).
	// We filter out the events coming too close, otherwise the spiketrain would be stretched in the FPGA.
	// We assume a maximum delay time, by which pulse may be released after there normal release time.
	// Such pulses reach the DNC still sufficiently early, and are released there according to their timestamp.
	// Pulses that would need to be delayed by more than that time, are dropped!
	// TODO: find a good number for this limit! currently we use 100 ns = 1ms @ 10^4
	const unsigned int max_delay_time = 100; // maximum time in nano seconds by which pulses can be delayed before they are dropped.
	const unsigned int max_delay_time_in_fpga_clks = max_delay_time/SYS_CLK_PER_L2_FPGA_NS; // maximum time by which pulses can be delayed before they are dropped in FPGA clk cycles

	uint64_t prev_rel_time_in_fpga_clks = 0; // previous release time in fpga clock cycles (SYS_CLK_PER_L2_FPGA_NS=8ns)

	static unsigned int num_times_warning_called = 0;

	const size_t num_pulses = st.size();
	for (size_t ii=0; ii<num_pulses; ++ii)
	{
		auto pulse_event = st[ii];
		LostEventLogger::count_pre_sim();

		if (pulse_event.getTime() < fpga_hicann_delay*2)
			throw std::runtime_error("write_playback_pulses: the time of the PulseEvent in the spike list has to be greater or equal than fpga_hicann_delay*2");
		uint64_t rel_time_in_fpga_clks = pulse_event.getTime()/2 - fpga_hicann_delay; // 1 fpga clk = 2 dnc clk

		int delta_time_in_fpga_cycles = rel_time_in_fpga_clks - prev_rel_time_in_fpga_clks;

		if ( delta_time_in_fpga_cycles >= 1) {
			// everything fine, pulses are not too close
		}
		else if ( delta_time_in_fpga_cycles + max_delay_time_in_fpga_clks >= 1 ) {
			// do it anyway
			delta_time_in_fpga_cycles = 1;
		}
		else {
			// drop event
			LostEventLogger::log_pre_sim();
			if (num_times_warning_called < 5 ) {
				LOG4CXX_WARN(_logger, "write_playback_pulses (..): Pulses come to close. Have to drop pulse " << pulse_event);
				num_times_warning_called++;
				if (num_times_warning_called == 5 ){
					LOG4CXX_WARN(_logger, "\tFuture warnings will be suppressed");
				}
			}
			continue;
		}

		playback_pulses.push_back(ESS::playback_entry(delta_time_in_fpga_cycles, pulse_event));

		prev_rel_time_in_fpga_clks += delta_time_in_fpga_cycles;
	}
}

void HAL2ESS::start_experiment(Handle::FPGA const& f)
{
	size_t fpga_id = f.coordinate().value();
	mFPGAConfig[fpga_id].record = true; // enable trace module
}

bool HAL2ESS::get_pbmem_buffering_completed(Handle::FPGA & /*f*/)
{
	// simulation is already ready (HostARQ isn't simulated) ;)
	return true;
}

//returns the PulseEventContainer with the Events that where read out during the simulation
//call after the simulation has run!!
FPGA::AlmostSortedPulseEvents HAL2ESS::read_trace_pulses(Handle::FPGA const& f, FPGA::PulseEvent::spiketime_t /*runtime*/, bool drop_background_events)
{
	size_t fpga_id = f.coordinate().value();
	mvirtual_hw->pcb_i.at(0)->get_fpga(fpga_id)->setStopTrace(true);

	size_t dropped_events = 0;
	FPGA::AlmostSortedPulseEvents::container_type rv;
	static const size_t  dnc_cycle_length = (1<<TIMESTAMP_WIDTH); // number of dnc cycles after which the counter overflow occurs.
	if(mvirtual_hw->pcb_i[0]->get_fpga(fpga_id)->getStopTrace()) {
		auto trace_pulses = mvirtual_hw->pcb_i[0]->get_fpga(fpga_id)->getTracePulses();
		// NOTE: this code was adapted from systemsim/s2c_systemsim::receive() in master branch
		for (auto pulse : trace_pulses ) {
			auto fpga_time = pulse.fpga_time;
			auto pulse_event = pulse.event;

			if (drop_background_events && pulse_event.getNeuronAddress().value() == 0) {
				++dropped_events;
				continue;
			}

			auto time_stamp = pulse_event.getTime();
			// compute overflow count from fpga time
			uint64_t fpga_time_in_dnc_clk = 2*fpga_time;
			long int overflow_count = fpga_time_in_dnc_clk / dnc_cycle_length;
			// check if recorded event is from cycle before
			if ( (fpga_time_in_dnc_clk % dnc_cycle_length) < time_stamp )
				--overflow_count;
			pulse_event.setTime( time_stamp + overflow_count*dnc_cycle_length);
			rv.push_back(pulse_event);
		}
	}
	else
	{
		LOG4CXX_WARN(_logger, "read_trace_pulses:fpga_id = " << fpga_id << " You tried to read out traces before calling stop_trace_memory" );
	}
	return FPGA::AlmostSortedPulseEvents(std::move(rv), dropped_events);
}

//************
//ADC Backend
//************
    
ADC::Status HAL2ESS::get_status(Handle::ADC & h)
{
    ADC::Status returnval;
    //get the adc coordinate
    std::string adc_string = h.coordinate().get_serial();
    size_t const adc_coord_ess = std::stoi(adc_string);
    auto const & adc_conf = mhal_access->wafer().adcs.at(adc_coord_ess);
    //determine if the adc was activated and set the trigge_activated bit accordingly
    returnval.trigger_activated = adc_conf.enable; 
    //determine if ADC has triggered yet i.e. if data was recorded 
    //therefore check whether the ESS was executed yet
    returnval.triggered = (mrun && adc_conf.enable);
    return returnval;
}	

void HAL2ESS::config(Handle::ADC & h , ADC::Config const& cfg)
{
    auto channel = cfg.input();
    if (channel != 0)
    {
		LOG4CXX_ERROR(_logger, "ADC::config:adc_id = " << h.coordinate().get_serial()
                << " you tried to configure an ADC with channel " << channel << " != 0"  );
        exit(-1);
    }
    std::string adc_string = h.coordinate().get_serial();
    //convert string to int
    size_t const adc_coord_ess = std::stoi(adc_string);
    uint32_t const samples = cfg.samples();
    mhal_access->configADC(adc_coord_ess,samples);
}

void HAL2ESS::prime(Handle::ADC & h)
{
    std::string adc_string = h.coordinate().get_serial();
    //convert string to int
    size_t const adc_coord_ess = std::stoi(adc_string);
    mhal_access->primeADC(adc_coord_ess);
}

//returns the sample rate in Hz //TODO correct unit?
//ín the ESS this value is globally determined by the timestep of the integration done in the neuron modell
double HAL2ESS::get_sample_rate(Handle::ADC &)
{
    //a datapoint is written in the ESS every nanosecond
    double rate = 1.e9;
    return rate;
}

// returns the temperature in degree Celsius
float HAL2ESS::get_temperature(Handle::ADC &)
{
	ESS_DUMMY();
	float temperature = 0.0;
	return temperature;
} 

//returns trace in 16 bit from 0 to 1.8 V
HAL2ESS::raw_data_type HAL2ESS::get_trace(Handle::ADC & h)
{
    
    std::string adc_string = h.coordinate().get_serial();
    //convert string to int
    size_t const adc_coord_ess = std::stoi(adc_string);
	LOG4CXX_DEBUG(_logger, "get_trace() for ADC: HALbe Coordinate " << h.coordinate() << " ESS-Coordinate " << adc_coord_ess << " called.");
    return mhal_access->getADCTrace(adc_coord_ess);
}

//returns coordiante of the ADC board
ADC::USBSerial HAL2ESS::get_board_id(Handle::ADC & h)
{
    return h.coordinate();
}

//************************
//end of backend functions
//************************
    
//**********************
//Public Debug Functions
//**********************

PyNNParameters::EIF_cond_exp_isfa_ista HAL2ESS::get_bio_parameter(Handle::HICANN const& h, Coordinate::NeuronOnHICANN const& nrn ) const
{
    //determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
    // nrn id
    size_t nrn_id = nrn.id();
    LOG4CXX_DEBUG(_logger, "HAL2ESS:get_bio_parameter for neuron " << nrn_id << " on HICANN " << coord.toHICANNOnWafer().id() );
    //get the ESS-Neuron-Parameter
    const auto& ESS_param = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->anncore_behav_i->get_neuron_parameter(nrn_id);
    LOG4CXX_TRACE(_logger, "Reporting Parameter in technical HWRegime" );
    LOG4CXX_TRACE(_logger, "tau_refrac = " << ESS_param.tau_refrac  << " ms"  );
    LOG4CXX_TRACE(_logger, "a          = " << ESS_param.a           << " nS"  );
    LOG4CXX_TRACE(_logger, "tau_m      = " << ESS_param.tau_m       << " ms"  );
    LOG4CXX_TRACE(_logger, "e_rev_E    = " << ESS_param.e_rev_E     << " mV"  );
    LOG4CXX_TRACE(_logger, "e_rev_I    = " << ESS_param.e_rev_I     << " mV"  );
    LOG4CXX_TRACE(_logger, "cm         = " << ESS_param.cm          << " nF"  );
    LOG4CXX_TRACE(_logger, "delta_T    = " << ESS_param.delta_T     << " mV"  );
    LOG4CXX_TRACE(_logger, "v_thresh   = " << ESS_param.v_thresh    << " mV"  );
    LOG4CXX_TRACE(_logger, "b          = " << ESS_param.b           << " pA"  );
    LOG4CXX_TRACE(_logger, "tau_syn_E  = " << ESS_param.tau_syn_E   << " ms"  );
    LOG4CXX_TRACE(_logger, "tau_syn_I  = " << ESS_param.tau_syn_I   << " ms"  );
    LOG4CXX_TRACE(_logger, "v_reset    = " << ESS_param.v_reset     << " mV"  );
    LOG4CXX_TRACE(_logger, "v_spike    = " << ESS_param.v_spike     << " mV"  );
    LOG4CXX_TRACE(_logger, "tau_w      = " << ESS_param.tau_w       << " ms"  );
    LOG4CXX_TRACE(_logger, "v_rest     = " << ESS_param.v_rest      << " mV"  );
    // scale the ESS_params (which are in the technical regime) to the bio regime
    auto returnval = mhal_access->getScaledBioParameter(ESS_param);
    LOG4CXX_TRACE(_logger, "Reporting Parameter in biological HWRegime" );
    LOG4CXX_TRACE(_logger, "tau_refrac = " << returnval.tau_refrac  << " ms"  );
    LOG4CXX_TRACE(_logger, "a          = " << returnval.a           << " nS"  );
    LOG4CXX_TRACE(_logger, "tau_m      = " << returnval.tau_m       << " ms"  );
    LOG4CXX_TRACE(_logger, "e_rev_E    = " << returnval.e_rev_E     << " mV"  );
    LOG4CXX_TRACE(_logger, "e_rev_I    = " << returnval.e_rev_I     << " mV"  );
    LOG4CXX_TRACE(_logger, "cm         = " << returnval.cm          << " nF"  );
    LOG4CXX_TRACE(_logger, "delta_T    = " << returnval.delta_T     << " mV"  );
    LOG4CXX_TRACE(_logger, "v_thresh   = " << returnval.v_thresh    << " mV"  );
    LOG4CXX_TRACE(_logger, "b          = " << returnval.b           << " pA"  );
    LOG4CXX_TRACE(_logger, "tau_syn_E  = " << returnval.tau_syn_E   << " ms"  );
    LOG4CXX_TRACE(_logger, "tau_syn_I  = " << returnval.tau_syn_I   << " ms"  );
    LOG4CXX_TRACE(_logger, "v_reset    = " << returnval.v_reset     << " mV"  );
    LOG4CXX_TRACE(_logger, "v_spike    = " << returnval.v_spike     << " mV"  );
    LOG4CXX_TRACE(_logger, "tau_w      = " << returnval.tau_w       << " ms"  );
    LOG4CXX_TRACE(_logger, "v_rest     = " << returnval.v_rest      << " mV"  );
    return returnval;
}
    
PyNNParameters::EIF_cond_exp_isfa_ista HAL2ESS::get_technical_parameter(Handle::HICANN const& h, Coordinate::NeuronOnHICANN const& nrn ) const
{
    //determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
    // nrn id
    size_t nrn_id = nrn.id();
    LOG4CXX_DEBUG(_logger, "HAL2ESS:get_technical_parameter for neuron " << nrn_id << " on HICANN " << coord.toHICANNOnWafer().id() );
    //get the ESS-Neuron-Parameter
    const auto& returnval = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->anncore_behav_i->get_neuron_parameter(nrn_id);
    LOG4CXX_TRACE(_logger, "Reporting Parameter in technical HWRegime" );
    LOG4CXX_TRACE(_logger, "tau_refrac = " << returnval.tau_refrac  << " ms"  );
    LOG4CXX_TRACE(_logger, "a          = " << returnval.a           << " nS"  );
    LOG4CXX_TRACE(_logger, "tau_m      = " << returnval.tau_m       << " ms"  );
    LOG4CXX_TRACE(_logger, "e_rev_E    = " << returnval.e_rev_E     << " mV"  );
    LOG4CXX_TRACE(_logger, "e_rev_I    = " << returnval.e_rev_I     << " mV"  );
    LOG4CXX_TRACE(_logger, "cm         = " << returnval.cm          << " nF"  );
    LOG4CXX_TRACE(_logger, "delta_T    = " << returnval.delta_T     << " mV"  );
    LOG4CXX_TRACE(_logger, "v_thresh   = " << returnval.v_thresh    << " mV"  );
    LOG4CXX_TRACE(_logger, "b          = " << returnval.b           << " pA"  );
    LOG4CXX_TRACE(_logger, "tau_syn_E  = " << returnval.tau_syn_E   << " ms"  );
    LOG4CXX_TRACE(_logger, "tau_syn_I  = " << returnval.tau_syn_I   << " ms"  );
    LOG4CXX_TRACE(_logger, "v_reset    = " << returnval.v_reset     << " mV"  );
    LOG4CXX_TRACE(_logger, "v_spike    = " << returnval.v_spike     << " mV"  );
    LOG4CXX_TRACE(_logger, "tau_w      = " << returnval.tau_w       << " ms"  );
    LOG4CXX_TRACE(_logger, "v_rest     = " << returnval.v_rest      << " mV"  );
    // scale the ESS_params (which are in the technical regime) to the bio regime
    return returnval;
}

//*********
//ESS-Getter
//**********

//gets the hicann_id
unsigned int HAL2ESS::get_hic_id_ESS(Handle::HICANN const& h) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
	const auto& hica = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord];
	unsigned int id_ess = hica->get_id();

	return id_ess;
}

//gets the hicann x-coordinate
unsigned int HAL2ESS::get_hic_x_ESS(Handle::HICANN const& h) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
	const auto& hica = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord];
	unsigned int id_ess = hica->get_x();

	return id_ess;
}

//gets the hicann y-coordinate
unsigned int HAL2ESS::get_hic_y_ESS(Handle::HICANN const& h) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
	const auto& hica = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord];
	unsigned int id_ess = hica->get_y();

	return id_ess;
}

//gets the L1-Address from the ESS->hw_neuron
HICANN::L1Address HAL2ESS::get_L1Address_ESS(Handle::HICANN const& h, Coordinate::NeuronOnHICANN const& neuron) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
	//determine the neuron address
	size_t nrn_id = static_cast<size_t>(neuron.id());
	//get the l1_address
	unsigned int l1 = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->anncore_behav_i->get_6_bit_address(nrn_id);
	HICANN::L1Address returnval(l1);
	return returnval;
}

//gets a CBS-row from the ESS
HICANN::CrossbarRow HAL2ESS::get_crossbar_switch_row_ESS(Handle::HICANN const& h, Coordinate::HLineOnHICANN y_, geometry::Side s) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());

	const auto& l1_config = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->l1_behav_i;
	
	switch_loc cbs;
	HICANN::CrossbarRow returnval;
	size_t row_addr = revert_hbus(y_);
	
    if(s == geometry::left)
    {
        cbs = switch_loc::CBL;
	    const std::vector<bool>& switchrow = l1_config->get_switch_row_config(cbs, row_addr);
        assert(returnval.size() == switchrow.size());
	    for(size_t i = 0; i < returnval.size(); ++i)
	    {
            Coordinate::VLineOnHICANN vline = y_.toVLineOnHICANN(
					s, Coordinate::Enum{i});
            size_t v = revert_vbus(vline)/32;
            returnval[i] = switchrow.at(v);
        }
    }
    else
    {
		cbs = switch_loc::CBR;
	    const std::vector<bool>& switchrow = l1_config->get_switch_row_config(cbs, row_addr);
        assert(returnval.size() == switchrow.size());
	    for(size_t i = 0; i < returnval.size(); ++i)
	    {
            Coordinate::VLineOnHICANN vline = y_.toVLineOnHICANN(
					s, Coordinate::Enum{i});
            size_t v = (revert_vbus(vline)%128)/32;
            returnval[i] = switchrow.at(v);
	    }
    }
	return returnval;
}

HICANN::BackgroundGeneratorArray HAL2ESS::get_background_generator_ESS(Handle::HICANN const& h) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());

    HICANN::BackgroundGeneratorArray returnval;
	size_t offset = returnval.size()-1;
    for(size_t i = 0; i < returnval.size(); ++i)
    {
	    const auto& bg = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->spl1_merger_i->get_background(offset-i);
	    const auto& bg_merger = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->spl1_merger_i->get_bg_merger(format_merger(i));
        returnval[i].enable(bg_merger.get_enable());
        returnval[i].random(bg.get_random());
        //converting the address
        returnval[i].address(HICANN::L1Address(bg.get_addr()));
        returnval[i].period(bg.get_period() );
    }
    
    return returnval;
}

HICANN::HorizontalRepeater HAL2ESS::get_repeater_ESS(Handle::HICANN const& h, Coordinate::HRepeaterOnHICANN r) const
{
    HICANN::HorizontalRepeater returnval;

    //determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());

	const auto& l1_config = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->l1_behav_i;
	
    size_t x = static_cast<size_t>(r.toHLineOnHICANN());

	//determine the repeater block
	ESS::RepeaterLocation rep_block = to_repblock(r.toRepeaterBlockOnHICANN());

	//determine the address inside block
	size_t addr = to_repaddr(r.toRepeaterBlockOnHICANN(), x);

	const auto& config = l1_config->get_repeater_config(rep_block, addr);

	//determine the mode
	if(bit::test(config,7) == 0 && bit::test(config,6) == 0 && bit::test(config,4) == 0)
		returnval.setIdle();
	else if(bit::test(config,7) == 1 && bit::test(config,4) == 0)
	{
		//determine the direction
		if(x%2)
		{
			if(bit::test(config,6) == 1)
				returnval.setForwarding(geometry::left);
			else
				returnval.setForwarding(geometry::right);
		}
		else
		{
			if(bit::test(config,6) == 1)
				returnval.setForwarding(geometry::right);
			else
				returnval.setForwarding(geometry::left);
		}
	}
	else if(bit::test(config,4) == 1 && r.isSending())	//only for sending repeater
	{
		if(bit::test(config,6) == 1)
			returnval.setOutput(geometry::right);
		if(bit::test(config,7) == 1)
			returnval.setOutput(geometry::left);
	}
	return returnval;
}

HICANN::VerticalRepeater HAL2ESS::get_repeater_ESS(Handle::HICANN const& h, Coordinate::VRepeaterOnHICANN r) const
{
    HICANN::VerticalRepeater returnval;

    //determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());

	const auto& l1_config = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->l1_behav_i;

	size_t y = static_cast<size_t>(r.toVLineOnHICANN());

	//determine the repeater block
	ESS::RepeaterLocation rep_block = to_repblock(r.toRepeaterBlockOnHICANN());

	//determine the address inside block
	size_t addr = to_repaddr(r.toRepeaterBlockOnHICANN(), y);

	const auto& rep_conf = l1_config->get_repeater_config(rep_block, addr);

	//determine the mode
	if(bit::test(rep_conf,7) == 0)		//recen = 0
		returnval.setIdle();
	else if(bit::test(rep_conf,7) == 1)	//recen = 1
	{
		if(y < 128)
		{ //left HICANN side
			if(y % 2)
			{
				if(bit::test(rep_conf,6) == 1)
					returnval.setForwarding(geometry::bottom);
				else
					returnval.setForwarding(geometry::top);
			}
			else
			{
				if(bit::test(rep_conf,6) == 1)
					returnval.setForwarding(geometry::top);
				else
					returnval.setForwarding(geometry::bottom);
			}
		}
		else
		{ //right HICANN-Side
			if(y % 2)
			{
				if(bit::test(rep_conf,6) == 1)
					returnval.setForwarding(geometry::top);
				else
					returnval.setForwarding(geometry::bottom);
			}
			else
			{
				if(bit::test(rep_conf,6) == 1)
					returnval.setForwarding(geometry::bottom);
				else
					returnval.setForwarding(geometry::top);
			}
		}
	}
	return returnval;
}

HICANN::SynapseSwitchRow HAL2ESS::get_syndriver_switch_row_ESS(Handle::HICANN const& h, Coordinate::SynapseSwitchRowOnHICANN const& s) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
    
    //convert HALBE-Numbering to ESS-Numbering
	size_t synbl;
	size_t addr;
    format_synswitch(s,synbl,addr);

	HICANN::SynapseSwitchRow returnval;
	
    const auto& l1_config = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->l1_behav_i;
	//determine the switch location
	switch_loc syn;
	switch(synbl)
    {
        case(0)	: { syn = switch_loc::SYNBL; break;}
        case(1) : { syn = switch_loc::SYNTL; break;}
        case(2) : { syn = switch_loc::SYNBR; break;}
        case(3) : { syn = switch_loc::SYNTR; break;}
        default : { throw std::runtime_error("Undefined synapse switch case");}
    }
    const std::vector<bool>& config = l1_config->get_switch_row_config(syn, addr);
    
    assert(config.size() == returnval.size());
    for(size_t i = 0; i < config.size(); ++i)
	{
        size_t j;
        if(s.toSideHorizontal() == geometry::left)    //TODO check if this is correct!
            j = i;
        else 
            j = 15 - i;
		returnval[i] = config.at(j);
	}
	return returnval;
}

//gets the configuration of a syndriver from the ESS
HICANN::SynapseDriver HAL2ESS::get_synapse_driver_ESS(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());

	HICANN::SynapseDriver returnval;

    //determine address of the driver in the ESS
    //address on side (0-111)
    size_t addr = to_drvaddr(s);
    //side left = 0, right = 1
    size_t side = to_synblock(s);  
    //address in the ess, right side has offset +112
    size_t essaddr = addr + side*112;

    //get syndriver config from ess
	auto& syndriver = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->anncore_behav_i->get_syndriver(essaddr);

    //configuration of input
    if ( syndriver.get_enable() )
    {
        if (syndriver.get_l1() == true && syndriver.get_mirror() == false)
            returnval.set_l1();
        else if (syndriver.get_l1() == true && syndriver.get_mirror() == true)
            returnval.set_l1_mirror();
        else if (syndriver.get_l1() == false && syndriver.get_mirror() == true)
            returnval.set_mirror();
        else if (syndriver.get_l1() == false && syndriver.get_mirror() == false)
            returnval.set_listen();
    }
    else
    {
        if (syndriver.get_mirror() == true)
            returnval.set_mirror_only();
        else
            returnval.disable();
    }
    //stp config
    if (syndriver.get_stp_enable())
    {
        if(syndriver.get_mode() == 1)
            returnval.set_stf();
        else if (syndriver.get_mode() == 0)
            returnval.set_std();
    }
    else
    {
        returnval.disable_stp();
    }
    std::bitset<3> cap_temp(syndriver.get_stp_cap());
    returnval.stp_cap = cap_temp;
    
    //config the lines
    // get the synderiver row configuration
    Coordinate::RowOnSynapseDriver top_line;
    Coordinate::RowOnSynapseDriver bot_line;
    //upper half : 
    // bottom lines correspond to each other in ESS and HALbe
	// ESS counts from center of the chip to the top. Even rows in ESS are bottom rows in halbe
    if (addr < 56)
    {
        top_line = Coordinate::RowOnSynapseDriver(Coordinate::top);
        bot_line = Coordinate::RowOnSynapseDriver(Coordinate::bottom);
    }
    //lower half : 
    //addressing in ESS and HALbe are inverted
	// ESS counts from center of the chip to the bottom. Even rows in ESS are top rows in halbe
	else if (addr >= 56)
    {
        top_line = Coordinate::RowOnSynapseDriver(Coordinate::bottom);
        bot_line = Coordinate::RowOnSynapseDriver(Coordinate::top);
	}
    auto& drv_line_top = returnval[top_line];
    auto& drv_line_bot = returnval[bot_line];
    //get the decoder values
    drv_line_top.set_decoder(Coordinate::top,    HICANN::DriverDecoder(syndriver.get_pre_out(1)) );
    drv_line_top.set_decoder(Coordinate::bottom, HICANN::DriverDecoder(syndriver.get_pre_out(3)) );
    drv_line_bot.set_decoder(Coordinate::top,    HICANN::DriverDecoder(syndriver.get_pre_out(0)) );
    drv_line_bot.set_decoder(Coordinate::bottom, HICANN::DriverDecoder(syndriver.get_pre_out(2)) );
    //get the synapse types
    if(syndriver.get_syn_type(1) == 1) {                // get_syn_type(1) = top_row, value = 1 -> inhibitory
        drv_line_top.set_syn_in(geometry::right, true);
    } else if(syndriver.get_syn_type(1) == 0) {         // get_syn_type(1) = top_row, value = 0 -> excitatory
        drv_line_top.set_syn_in(geometry::left, true);
    }
    if(syndriver.get_syn_type(0) == 1) {                // get_syn_type(0) = bot_row,  value = 1 -> inhibitory
        drv_line_bot.set_syn_in(geometry::right, true);
    } else if(syndriver.get_syn_type(0) == 0) {         // get_syn_type(0) = bot_row, value = 0 -> excitatory
        drv_line_bot.set_syn_in(geometry::left, true);
    }

    // TODO check gmax settings
    drv_line_top.set_gmax(syndriver.get_sel_Vgmax(1));
    drv_line_bot.set_gmax(syndriver.get_sel_Vgmax(0));
    drv_line_top.set_gmax_div(Coordinate::left,  syndriver.get_gmax_div(1,0));
    drv_line_top.set_gmax_div(Coordinate::right, syndriver.get_gmax_div(1,1));
    drv_line_bot.set_gmax_div(Coordinate::left,  syndriver.get_gmax_div(0,0));
    drv_line_bot.set_gmax_div(Coordinate::right, syndriver.get_gmax_div(0,1));
    return returnval;
}

HICANN::DecoderDoubleRow HAL2ESS::get_decoder_double_row_ESS(Handle::HICANN const& h, Coordinate::SynapseDriverOnHICANN const& s) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());

	const auto& decoder_vals = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->anncore_behav_i->getAddressDecoders();
    
    HICANN::DecoderDoubleRow returnval;

	//calculate the address
    Coordinate::SynapseRowOnHICANN top_row{s, Coordinate::RowOnSynapseDriver{geometry::top}};
    Coordinate::SynapseRowOnHICANN bot_row{s, Coordinate::RowOnSynapseDriver{geometry::bottom}};
    size_t top = format_synapse_row(top_row) * returnval[geometry::top].size();
	size_t bot = format_synapse_row(bot_row) * returnval[geometry::bottom].size();

    for(size_t i = 0; i < returnval[geometry::top].size(); ++i)
    {
        HICANN::SynapseDecoder decoder{static_cast<uint8_t>(decoder_vals[top + i])};
        returnval[geometry::top][i] = decoder;
        assert(decoder.value() == decoder_vals[top + i]);
        decoder = HICANN::SynapseDecoder{static_cast<uint8_t>(decoder_vals[bot+i])};
        returnval[geometry::bottom][i] = decoder;
        assert(decoder.value() == decoder_vals[bot + i]);
    }
    return returnval;
}

//get the synapse weights from the ESS
HICANN::WeightRow HAL2ESS::get_weights_row_ESS(Handle::HICANN const& h, Coordinate::SynapseRowOnHICANN const& s) const
{
	//determine the HICANN x and y - id
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());

	const auto& weights = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->anncore_behav_i->getWeights();

    HICANN::WeightRow returnval;

    size_t addr = format_synapse_row(s) * returnval.size();

    for(size_t i = 0; i < returnval.size(); ++i)
    {
        HICANN::SynapseWeight w{static_cast<uint8_t>(weights[addr + i])};
        returnval[i] = w;
        assert(w.value() == weights[addr + i]);
    }

    return returnval;
}


//gets the fg_values from the ess
HICANN::FGBlock HAL2ESS::get_fg_values_ESS(Handle::HICANN const& h, Coordinate::FGBlockOnHICANN const& addr) const
{
	LOG4CXX_DEBUG(_logger, "get_fg_values_ESS for FGBlock " << addr.id() << " on HICANN " << h.coordinate().toHICANNOnWafer().id() );
    //determine the HICANN x and y - id, and enum
	Coordinate::HICANNGlobal coord = h.coordinate();
	size_t x_coord = static_cast<size_t>(coord.x());
	size_t y_coord = static_cast<size_t>(coord.y());
	size_t hic_id = static_cast<size_t>(coord.toHICANNOnWafer().id());
	LOG4CXX_DEBUG(_logger, "get_fg_values for FGBlock " << addr.id() << " on HICANN " << h.coordinate().toHICANNOnWafer().id() );

	//initialize FGcontrol
	HICANN::FGBlock fgb;
    HICANN::FGControl fgc;
	fgc[addr] = fgb;

	//calculate the address offset and cast the fg-addr to size_t
	size_t addr_offset = addr.id()*128;
    //getting the global parameters
	LOG4CXX_DEBUG(_logger, "Global Parameters" );
    // determine the coorect Syndriver address in the ESS
    //the side (left/right) corresponds to the side of the FGBlock
    int side = addr.x();
    //top/bot corresponds to vertical position of fgblock
    int vert = addr.y();
    const auto &shrd_param = mhal_access->wafer().hicanns[hic_id].syndriver_config[side].synapse_params_hw[vert];

	//get STP Parameters: The ESS only uses transformed parameters, which are not stored.
	// hence, we get the stored parameters from the hal2ess-container.
    auto shrd = HICANN::shared_parameter::V_fac;
	LOG4CXX_DEBUG(_logger, "V_fac = " << shrd_param.V_fac);
	fgc.setShared(addr, shrd, shrd_param.V_fac);
    shrd = HICANN::shared_parameter::V_dep;
	LOG4CXX_DEBUG(_logger, "V_dep = " << shrd_param.V_dep);
	fgc.setShared(addr, shrd, shrd_param.V_dep);
	shrd = HICANN::shared_parameter::V_stdf;
	LOG4CXX_DEBUG(_logger, "V_stdf = " << shrd_param.V_stdf);
	fgc.setShared(addr, shrd, shrd_param.V_stdf);
	shrd = HICANN::shared_parameter::V_dtc;
	LOG4CXX_DEBUG(_logger, "V_dtc = " << shrd_param.V_dtc);
    fgc.setShared(addr, shrd, shrd_param.V_dtc);

    //getting the local neuron parameters
	for(size_t nrn_addr = addr_offset; nrn_addr < HICANN::FGBlock::fg_columns + addr_offset - 1; nrn_addr++)
	{
	    LOG4CXX_DEBUG(_logger, "Neuron parameters for neuron " << nrn_addr  );
        Coordinate::NeuronOnHICANN nrn{Coordinate::Enum{nrn_addr}};
        
        //send parameter to calbtic to get HW parameter
        const auto& ESS_param = mvirtual_hw->pcb_i[0]->get_wafer()->hicann_i[y_coord][x_coord]->anncore_behav_i->get_neuron_parameter(nrn_addr);
        auto HW_param = mhal_access->getHWNeuronParameter(ESS_param,x_coord,y_coord,nrn_addr); 
        
        HICANN::neuron_parameter type = HICANN::neuron_parameter::I_gl;
	    LOG4CXX_DEBUG(_logger, "I_gl = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
        type = HICANN::neuron_parameter::E_l;
	    LOG4CXX_DEBUG(_logger, "E_l = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::E_syni;
	    LOG4CXX_DEBUG(_logger, "E_syni = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::E_synx;
	    LOG4CXX_DEBUG(_logger, "E_synx = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::I_fire;
	    LOG4CXX_DEBUG(_logger, "I_fire = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::I_gladapt;
	    LOG4CXX_DEBUG(_logger, "I_gladapt = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::I_pl;
	    LOG4CXX_DEBUG(_logger, "I_pl = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::I_radapt;
	    LOG4CXX_DEBUG(_logger, "I_radapt = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::I_rexp;
	    LOG4CXX_DEBUG(_logger, "I_rexp = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
        type = HICANN::neuron_parameter::V_exp;
	    LOG4CXX_DEBUG(_logger, "V_exp = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::V_syntci;
	    LOG4CXX_DEBUG(_logger, "V_syntci = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::V_syntcx;
	    LOG4CXX_DEBUG(_logger, "V_syntcx = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
		type = HICANN::neuron_parameter::V_t;
	    LOG4CXX_DEBUG(_logger, "V_t = " << HW_param.getParam(type) );
        fgc.setNeuron(nrn, type, HW_param.getParam(type));
	}
	return fgc[addr];
}

} //end namespace HMF
