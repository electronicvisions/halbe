#pragma once

#include "Scheriff.h"

#include "defines.h"
#include "logger.h"
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/back/tools.hpp>

// nicer typeid output
#include "ztl/debug.h"

using namespace HMF;

namespace {

static log4cxx::LoggerPtr _logger = log4cxx::Logger::getLogger("scheriff");

template<typename Derived>
struct NoTransitionErrorMixin : public Derived {
	NoTransitionErrorMixin() :
		throw_on_no_transition(true)
	{}

	bool throw_on_no_transition;

	struct TaintedHardware {
		TaintedHardware() :
			invalid_state_transition(false)
		{}
		bool invalid_state_transition;
	} tainted_hardware;

	template<typename FSM, typename Event>
	void no_transition(Event const& e, FSM& fsm, int state) {
		typedef typename boost::msm::back::generate_state_set<typename FSM::stt>::type all_states;
		std::string name_of_state;
		boost::mpl::for_each<all_states, boost::msm::wrap<boost::mpl::placeholders::_1> >
			(boost::msm::back::get_state_name<typename FSM::stt>(name_of_state, state));
		std::stringstream ss;
		ss << "fsm " << ZTL::typestring<typename std::decay<FSM>::type>()
		   << " has no transition from state " << state
		   << " (" << ZTL::demangle(name_of_state.c_str())  << ") "
		   << " on event " << ZTL::typestring<typename std::decay<Event>::type>()
		   << std::endl;
		fsm.tainted_hardware.invalid_state_transition = true;
		if (throw_on_no_transition) {
			std::cerr << ss.str() << std::endl;
			Derived::no_transition(e, fsm, state);
			//throw std::runtime_error(ss.str());
		} else {
			LOG4CXX_ERROR(_logger, ss.str());
		}
	}
//	template <class Event,class FSM>
//		void on_entry(Event const& e, FSM& ) {std::cout << "entering: " << typeid(e).name() << std::endl;}
//	template <class Event,class FSM>
//		void on_exit(Event const& e, FSM& )  {std::cout << "leaving: " << typeid(e).name() << std::endl;}
};


// Configuration States and Transitions
struct ConfigurationMachine_ : public NoTransitionErrorMixin<boost::msm::front::state_machine_def<ConfigurationMachine_> > {

	// Down State
	struct SystemDown         : public boost::msm::front::state<> {
		typedef boost::mpl::vector1<FlagIdle> flag_list;
	};
	typedef SystemDown initial_state;

	// Configuration States
	struct SystemStartup      : public boost::msm::front::state<> {};
	struct ResetCold          : public boost::msm::front::state<> {};
	struct ResetWarm          : public boost::msm::front::state<> {};

	// and 3 more orthogonal states
	struct SetupAnalogReadout : public boost::msm::front::state<> {};
	struct SetupL2            : public boost::msm::front::state<> {};
	struct SetupSynapses      : public boost::msm::front::state<> {};

	// and finally...
	struct SystemRunning      : public boost::msm::front::state<> {};



	// composite L1 configuration state -- has some inner dependencies
	struct Setup_ : public NoTransitionErrorMixin<boost::msm::front::state_machine_def<Setup_> > {

		// substates
		struct SetupFG            : public boost::msm::front::state<> {};
		struct SetupL1            : public boost::msm::front::state<> {};
		struct SetupL1BG          : public boost::msm::front::state<> {};
		struct SetupL1DLL         : public boost::msm::front::state<> {};

		// Transitions depending on L1 setup ordering
		struct transition_table : boost::mpl::vector<
			_row < SetupFG                , EventSetupL1                   , SetupL1                                           >,
			_row < SetupL1                , EventSetupL1BG                 , SetupL1BG                                         >,
			_row < SetupL1BG              , EventSetupL1DLL                , SetupL1DLL                                        >,

			// jumping
			_row < SetupFG                , EventSetupL1BG                 , SetupL1BG                                         >,

			// identities
			_row < SetupFG                , EventSetupFG                   , SetupFG                                           >,
			_row < SetupL1                , EventSetupL1                   , SetupL1                                           >,
			_row < SetupL1BG              , EventSetupL1BG                 , SetupL1BG                                         >,
			_row < SetupL1DLL             , EventSetupL1DLL                , SetupL1DLL                                        >,

			// "parallel" transitions
			_row < SetupFG                , EventSetupAnalogReadout        , SetupFG                                           >,
			_row < SetupFG                , EventSetupL2                   , SetupFG                                           >,
			_row < SetupFG                , EventSetupSynapses             , SetupFG                                           >,
			_row < SetupL1                , EventSetupAnalogReadout        , SetupL1                                           >,
			_row < SetupL1                , EventSetupL2                   , SetupL1                                           >,
			_row < SetupL1                , EventSetupSynapses             , SetupL1                                           >,
			_row < SetupL1BG              , EventSetupAnalogReadout        , SetupL1BG                                         >,
			_row < SetupL1BG              , EventSetupL2                   , SetupL1BG                                         >,
			_row < SetupL1BG              , EventSetupSynapses             , SetupL1BG                                         >,
			_row < SetupL1DLL             , EventSetupAnalogReadout        , SetupL1DLL                                        >,
			_row < SetupL1DLL             , EventSetupL2                   , SetupL1DLL                                        >,
			_row < SetupL1DLL             , EventSetupSynapses             , SetupL1DLL                                        >
		> {};
		typedef SetupFG initial_state;

	};
	typedef boost::msm::back::state_machine<Setup_> Setup;


	// Transitions
	struct transition_table : boost::mpl::vector<

		/*******************************************************************************************************************
		 * Full experiment transitions                                                                                     *
		 *                                                                                                                 *
		 * StHAL can use this flow like this to call the corresponding config/action functions                             *
		 *     SystemDown, EventConfigureFullExperiment, SystemStartup, ActionThatDoesSystemStartup                        *
		 *******************************************************************************************************************/
		_row < SystemDown             , EventSystemStartup             , SystemStartup                                     >,
		_row < SystemStartup          , EventResetCold                 , ResetCold                                         >,
		_row < ResetCold              , EventResetWarm                 , ResetWarm                                         >,


		// Setup is a composite state
		_row < ResetWarm              , EventSetupFG                   , Setup                                             >,
		_row < ResetWarm              , EventSetupL1                   , Setup                                             >,
		_row < ResetWarm              , EventSetupL1BG                 , Setup                                             >,
		// the parallel ones:
		_row < ResetWarm              , EventSetupAnalogReadout        , Setup                                             >,
		_row < ResetWarm              , EventSetupL2                   , Setup                                             >,
		_row < ResetWarm              , EventSetupSynapses             , Setup                                             >,


		_row < Setup                  , EventStartExperiment           , SystemRunning                                     >,
		_row < SystemRunning          , EventStopExperiment            , SystemDown                                        >, // closing the loop


		/******************************************************************************************************************* 
		 * Allowed "jumping" transitions                                                                           * 
		 *******************************************************************************************************************/
		_row < ResetCold              , EventStopExperiment            , SystemDown                                        >,

		_row < ResetWarm              , EventSystemStartup             , SystemStartup                                     >,
		_row < ResetWarm              , EventStartExperiment           , SystemRunning                                     >,
		_row < ResetWarm              , EventStopExperiment            , SystemDown                                        >,

		_row < Setup                  , EventResetWarm                 , ResetWarm                                         >,
		_row < Setup                  , EventSystemStartup             , SystemStartup                                     >,
		_row < Setup                  , EventStopExperiment            , SystemDown                                        >,

		_row < SystemRunning          , EventSystemStartup             , SystemStartup                                     >,
		_row < SystemRunning          , EventResetWarm                 , ResetWarm                                         >,
		// changing analog readout stuff is safe?
		_row < SystemRunning          , EventSetupAnalogReadout        , SystemRunning                                     >,

		/******************************************************************************************************************* 
		 * Identities -- transitions to same state                                                                         * 
		 *******************************************************************************************************************/
		_row < SystemStartup          , EventSystemStartup             , SystemStartup                                     >,
		_row < ResetWarm              , EventResetWarm                 , ResetWarm                                         >

	> {};

}; // struct ConfigurationMachine_
typedef boost::msm::back::state_machine<ConfigurationMachine_> ConfigurationMachine;

} // end namespace
