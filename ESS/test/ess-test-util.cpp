#include "ess-test-util.h"
#include "hal/HICANN/FGControl.h"

namespace HMF
{

using namespace geometry;

ESSTest::ESSTest() :
	fpga_c{Coordinate::FPGAGlobal{Coordinate::Enum{5}}},
	hicann_c{createHICANNs()},
	ess{new Handle::Ess(fpga_c.toWafer())},
	fpga{fpga_c, ess, hicann_c},
	h(*fpga.get(Coordinate::HICANNGlobal{hicann_c[0]}))
{
	ess->enableSpikeDebugging(true);
}

std::vector<Coordinate::HICANNOnWafer> ESSTest::createHICANNs()
{
	std::vector<Coordinate::HICANNOnWafer> hicanns;
	Coordinate::HICANNOnWafer coord{Coordinate::Enum{0}};
	hicanns.push_back(coord);
	return hicanns;
}

ESSTest::~ESSTest() {}

void load_pattern_naud(HICANN::FGControl &fgc, size_t neuron, std::string const& pattern)
{
    int x = -1;
    std::map<std::string, int> lut;
    lut.insert(std::pair<std::string, int>("custom",0));
    lut.insert(std::pair<std::string, int>("tonic",1));
    lut.insert(std::pair<std::string, int>("adaptation",2));
    lut.insert(std::pair<std::string, int>("initial_burst",3));
    lut.insert(std::pair<std::string, int>("tonic_burst",4));
    lut.insert(std::pair<std::string, int>("phasic",5));
    lut.insert(std::pair<std::string, int>("chaos",6));

    auto it = lut.find(pattern);
    if(it != lut.end() )
        x = it->second;

    Coordinate::NeuronOnHICANN nrn{Enum(neuron)};
    Coordinate::FGBlockOnHICANN shrd = nrn.toSharedFGBlockOnHICANN();
    switch(x)
    {
        case(0):    //custom
        {
            std::cout << "Firing Pattern: Custom" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 169);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 227);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 430);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 23);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 348);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 0);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 700);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 973);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 195);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 786);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 786);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 500);
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 169);
            break;
        }
        case(1):    //tonic
        {
            std::cout << "Firing Pattern: Tonic" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 331);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 0);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 0);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 22);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 287);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 22);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 1023);   // not defined in model -> set to average_val
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 973);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 205);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 450);     // strange effect for V_t <= 502, sim does not terminate TODO look into this...
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 355);
            break;
        }
        case(2):    //adaptation
        {
            std::cout << "Firing Pattern: Adaptation" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 285);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 511);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 511);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 921);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 227);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 22);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 511);    // not defined in model -> set to average_val
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 80);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 205);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 503);     // strange effect for V_t <= 502, sim does not terminate TODO look into this...
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 355);
        break;
        }
        case(3):    //initial_burst
        {
            std::cout << "Firing Pattern: Initial Burst" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 355);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 511);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 511);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 83);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 46);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 1023);    // not defined in model -> set to min val for bursting
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 124);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 205);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 503);     // strange effect for V_t <= 502, sim does not terminate TODO look into this...
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 401);
            break;
        }
        case(4):    //tonic_burst
        {
            std::cout << "Firing Pattern: Tonic Burst" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 355);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 511);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 511);  // has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 287);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 22);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 1023);   // not defined in model -> set to min val for bursting
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 151);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 205);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 786);// has no influence
            fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 503);     // strange effect for V_t <= 502, sim does not terminate TODO look into this...
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 424);
            break;
        }
        case(5):    //phasic
        {
            std::cout << "Phasic-Spiking parameters not in settable Range for Naud" << std::endl;
            break;
        }
        default:
        {  
            std::cout << pattern << "is no valid firing pattern" << std::endl;
            break;
        }
    }
}

void load_pattern(HICANN::FGControl &fgc, size_t neuron, std::string const& pattern)
{
    int x = -1;
    std::map<std::string, int> lut;
    lut.insert(std::pair<std::string, int>("custom",0));
    lut.insert(std::pair<std::string, int>("tonic",1));
    lut.insert(std::pair<std::string, int>("adaptation",2));
    lut.insert(std::pair<std::string, int>("initial_burst",3));
    lut.insert(std::pair<std::string, int>("tonic_burst",4));
    lut.insert(std::pair<std::string, int>("phasic",5));
    lut.insert(std::pair<std::string, int>("chaos",6));

    auto it = lut.find(pattern);
    if(it != lut.end() )
        x = it->second;

    Coordinate::NeuronOnHICANN nrn{Enum(neuron)};
    Coordinate::FGBlockOnHICANN shrd = nrn.toSharedFGBlockOnHICANN();
    //setting the non bifurcation params for all pattern
    fgc.setNeuron(nrn, HICANN::neuron_parameter::E_l, 285);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::E_syni, 511);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::E_synx, 511);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gl, 121);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::I_pl, 1023);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::I_rexp, 250);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::V_exp, 205);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntci, 800);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::V_syntcx, 800);
    fgc.setNeuron(nrn, HICANN::neuron_parameter::V_t, 512);
    switch(x)
    {
        case(0):    //custom
        {
            std::cout << "Use Naud for custom settings!" << std::endl;
            break;
        }
        case(1):    //tonic
        {
            std::cout << "Firing Pattern: Tonic" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 22);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 0);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 1023);
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 444);
            break;
        }
        case(2):    //adaptation
        {
            std::cout << "Firing Pattern: Adaptation" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 500);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 80);
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 343);
        break;
        }
        case(3):    //initial_burst
        {
            std::cout << "Firing Pattern: Initial Burst" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 1023);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 500);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 181);
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 418);
            break;
        }
        case(4):    //tonic_burst name??? binh calls this phasic bursting, does it really correspond?
        {
            std::cout << "Firing Pattern: Tonic Burst" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 1000);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 500);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 197);
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 432);
            break;
        }
        case(5):    //phasic
        {
            std::cout << "Firing Pattern:  Phasic Spiking" << std::endl;
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_fire, 800);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_gladapt, 500);
            fgc.setNeuron(nrn, HICANN::neuron_parameter::I_radapt, 181);
            fgc.setShared(shrd, HICANN::shared_parameter::V_reset, 343);
            break;
        }
        default:
        {  
            std::cout << pattern << "is no valid firing pattern" << std::endl;
            break;
        }
    }
}

}//end namespace HMF

