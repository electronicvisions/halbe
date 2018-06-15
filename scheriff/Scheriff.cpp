#include "ConfigurationStates.tpp"

namespace HMF {

struct Scheriff::Impl : public ConfigurationMachine
{
};

Scheriff::Scheriff() :
	impl(new Impl())
{}

Scheriff::~Scheriff()
{}

void Scheriff::start(bool throw_on_illegal_transition)
{
	impl->throw_on_no_transition = throw_on_illegal_transition;
	impl->get_state<Impl::Setup&>().throw_on_no_transition = false;
	impl->start(); // start fsm
}

int Scheriff::process_event(EventSystemStartup      e) { return impl->process_event(e); }
int Scheriff::process_event(EventResetCold          e) { return impl->process_event(e); }
int Scheriff::process_event(EventResetWarm          e) { return impl->process_event(e); }

int Scheriff::process_event(EventSetupAnalogReadout e) { return impl->process_event(e); }
int Scheriff::process_event(EventSetupSynapses      e) { return impl->process_event(e); }
int Scheriff::process_event(EventSetupL2            e) { return impl->process_event(e); }
int Scheriff::process_event(EventSetupFG            e) { return impl->process_event(e); }

int Scheriff::process_event(EventSetupL1            e) { return impl->process_event(e); }
int Scheriff::process_event(EventSetupL1BG          e) { return impl->process_event(e); }
int Scheriff::process_event(EventSetupL1DLL         e) { return impl->process_event(e); }

int Scheriff::process_event(EventStartExperiment    e) { return impl->process_event(e); }
int Scheriff::process_event(EventStopExperiment     e) { return impl->process_event(e); }

void Scheriff::log_f_name(const char * const fun_name)
{
	LOG4CXX_ERROR(_logger, "SCHERIFF was called by " <<  fun_name);
}

}
