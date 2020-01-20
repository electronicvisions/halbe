#include "ESS/halbe_to_ess_formatter.h"
#include "halco/hicann/v2/l1.h"
#include "halco/hicann/v2/synapse.h"

namespace HMF
{

size_t revert_vbus(halco::hicann::v2::VLineOnHICANN const& vline)
{
    if(vline < 128)
        return vline.value();
    else
        return (255 - vline.value())*2 + vline.value()%128 + 1;
}

size_t revert_vbus(size_t const& vline)
{
    if(vline > 255)
        throw std::out_of_range("valid range for vlines is 0...255");
    else if(vline < 128)
        return vline;
    else
        return (255 - vline)*2 + vline%128 + 1;
}


size_t revert_hbus(halco::hicann::v2::HLineOnHICANN const& hline)
{
    return 63 - hline.value();
}

size_t revert_hbus(size_t const& hline)
{
    if(hline > 63)
        throw std::out_of_range("valid range for hlines is 0...255");
    else   
        return 63 - hline;
}


void format_synswitch(halco::hicann::v2::SynapseSwitchRowOnHICANN const& s, size_t &synbl, size_t &addr)
{
	if(s.line() < 112)		//top half
	{
		addr = 111 - s.line();
		if(s.toSideHorizontal() == 1)	//right block
			synbl = 3;
		else				//left block
			synbl = 1;
	}
	else					//bottom half
	{
		addr = s.line() - 112;
		if(s.toSideHorizontal() == 1)	//right block
			synbl = 2;
		else
			synbl = 0;		//left block
	}
}


size_t format_synapse_row(halco::hicann::v2::SynapseRowOnHICANN const& s)
{
   if(s.value() < 224)
       return 223 - s.value();
   else
       return s.value();
}


size_t to_synblock(halco::hicann::v2::SynapseDriverOnHICANN const& coord)
{
    size_t synbl = 0;
    if (coord.toSideHorizontal() == halco::common::left) {
        synbl = 0;
    } else if (coord.toSideHorizontal() == halco::common::right) {
        synbl = 1;
    }
    return synbl;
}

size_t to_drvaddr(halco::hicann::v2::SynapseDriverOnHICANN const& coord)
{
    if(coord.line() < 112)  //top -> invert coordinates
    {
        return 55 - coord.line()/2;
    }
    else                    //bot -> dont invert coordinates
    {
        return coord.line()/2;
    }
}

//returns the block of the repeater in ESS Coordinates
ESS::RepeaterLocation to_repblock(halco::hicann::v2::RepeaterBlockOnHICANN const& block)
{
    ESS::RepeaterLocation rep_block = ESS::RepeaterLocation::REP_UL;

    size_t val = static_cast<size_t>(block.toEnum());
    switch(val)
    {
        case 0 : {rep_block = ESS::RepeaterLocation::REP_UL ; break;}
        case 1 : {rep_block = ESS::RepeaterLocation::REP_UR ; break;}
        case 2 : {rep_block = ESS::RepeaterLocation::REP_L ; break;}
        case 3 : {rep_block = ESS::RepeaterLocation::REP_R ; break;}
        case 4 : {rep_block = ESS::RepeaterLocation::REP_DL ; break;}
        case 5 : {rep_block = ESS::RepeaterLocation::REP_DR ; break;}
    }
    return rep_block;
}

//returns the addres of the repeater inside the block
size_t to_repaddr(halco::hicann::v2::RepeaterBlockOnHICANN const& block, size_t val)
{
    size_t addr = 0;
    size_t rblock = static_cast<size_t>(block.toEnum());
    switch(rblock)
    {
        case 0 :                        //top left repeater block: considering line swapping 
            {                           //1->1, 3->2, 5->3, ... , 123->62, 125->63, 127->0
                if(val != 127)
                    addr = (val+1)/2; 
                else
                    addr = 0;
                break;
            }
        case 1 :                        // top right repeater block: considering line swapping  
            {                           //128->0, 130->63, 132->62, ... , 250->3, 252->2, 254->1
                if(val != 128)
                    addr = (256 - val)/2;
                else
                    addr = 0;
                break;
            }
        case 2 :                        //even line => center left block    
            {                           //0->31, 2->30, ... , 60->1, 62->0
                addr = (62 - val)/2;
                break;
            }
        case 3 :                        //odd line => center right block       
            {                           //1->1, 3->2 ... 59->30, 61->31, 63->0
                // if( val != 63 )
                //     addr = (val +1 )/2;
                // else
                //     addr = 0;
                // break;

                // PM: HACK FIXME hacking around: // 1->30 3->29 61->0 63->31
                assert (val % 2);
                assert (val < 64);
                if (val == 63 )
                    addr = 31;
                else
                    addr = (61 - val) / 2;
                break;
            }
        case 4 :                        //bottom left repeater block                        
            {                           //0->0, 2->1, 4->2, ... , 122->61, 124->62, 126->63
                addr = val/2; 
                break;
            }
        case 5 : 
            { 
                addr = (255 - val)/2; 
                break;
            }
    }
    return addr;
}

//translate_neuron_merger from HICANNBackendHelper. include not possible due to incompatability of systemc and hicann_source....
size_t format_merger(size_t const merger)
{
	assert(merger<15);
	static std::array<size_t, 15> const _lut = {{
		 7,  6,  5,  4,  3,  2,  1,  0, // background merger
		11, 10,  9,  8, 13, 12, 14      // other neuron merger
	}};
	return _lut[merger];
}

} //end namespace HMF
