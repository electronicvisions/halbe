#pragma once

#include <iostream>
#include <boost/scoped_ptr.hpp>

namespace HMF {

// Events
struct EventSystemStartup      {};
struct EventResetCold          {};
struct EventResetWarm          {};

struct EventSetupAnalogReadout {};
struct EventSetupSynapses      {};
struct EventSetupL2            {};

struct EventSetupFG            {};
struct EventSetupL1            {};
struct EventSetupL1BG          {};
struct EventSetupL1DLL         {};

struct EventStartExperiment    {};
struct EventStopExperiment     {};


// Flags
struct FlagIdle;

// let's call him scheriff!
struct Scheriff
{
	Scheriff();
	~Scheriff();

	void start(bool throw_on_illegal_transition=false);

    int process_event(EventSystemStartup     );
    int process_event(EventResetCold         );
    int process_event(EventResetWarm         );

    int process_event(EventSetupAnalogReadout);
    int process_event(EventSetupSynapses     );
    int process_event(EventSetupL2           );
    int process_event(EventSetupFG           );

    int process_event(EventSetupL1           );
    int process_event(EventSetupL1BG         );
    int process_event(EventSetupL1DLL        );

    int process_event(EventStartExperiment   );
    int process_event(EventStopExperiment    );

	static void log_f_name(const char * const fun_name);

private:
	struct Impl;
	boost::scoped_ptr<Impl> impl;
};

} // namespace HMF
