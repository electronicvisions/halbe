/*! @file dispatch.h
 *
 *  @brief This file (together with macro_HALbe.h) contains the macro magic
 *  which is used by HALbe backend functions to select different "backends"
 *  (e.g. real hardware, remote calls, ESS, dump-to-disk, and maybe more).
 *
 * The design decision to provide a stateless API for hardware access resulted
 * in an API that uses handles as a first parameter. Getters return data types
 * (defined by HALbe) and potentially take parameters. The Setters have no
 * return type.  "Overloading" (in the sense of functionality) lead to a
 * dispatch-based implementation where the handle type is checked (via
 * dynamic_cast) and handle-type-specific code gets executed.
 *
 * Originally we had a lot of macros in the *backends to check for
 * "with-hardware-execution"/"dump-mode" (and later "ess-mode"). On 2013-04-19,
 * ECM introduced the HALBE_GETTER and HALBE_SETTER macros (using EVERYSECOND to
 * concatenate types and variable names for the function implementation).
 * A lot has happened since then (mostly by CK)... below are the beautiful guts.
 *
 * Have fun!
 */


#include <type_traits>
#include <typeinfo>

#include "hal/macro_HALbe.h"
#include "scheriff/Scheriff.h"


namespace
{
template <typename T, typename U>
struct CopyQualifiers
{	typedef U type;	  };

template <typename T, typename U>
struct CopyQualifiers<T &, U>
{	typedef U & type; };

template <typename T, typename U>
struct CopyQualifiers<const T &, U>
{	typedef const U & type; };
}

struct HICANN;
struct FPGA;

// Define struct to translate handle type:
// use as: 
// HANDLE_TO(Ess)
// typedef typename HandleToEss<FPGAEss>::type handle_type;
//  -> handle_type == FPGAEss
#define HANDLE_TO( suffix ) \
namespace HMF { namespace Handle { namespace { \
template <typename T> struct HandleTo##suffix##Impl { typedef T type; }; \
template<> struct HandleTo##suffix##Impl< ADC > { typedef ADC##suffix type; }; \
template<> struct HandleTo##suffix##Impl< HICANN > { typedef HICANN##suffix type; }; \
template<> struct HandleTo##suffix##Impl< FPGA >   { typedef FPGA##suffix type; }; \
template<> struct HandleTo##suffix##Impl< PMU >   { typedef PMU##suffix type; }; \
template <typename T> struct HandleTo##suffix { \
	typedef typename HandleTo##suffix##Impl< typename std::decay<T>::type >::type tmp;  \
	typedef typename CopyQualifiers<T, tmp >::type type; }; \
}}} // end ::HMF::Handle::<anonymous>



//// Dump
#include "hal/Handle/ADCDump.h"
#include "hal/Handle/FPGADump.h"
#include "hal/Handle/PMUDump.h"
HANDLE_TO(Dump)
#define USE_DUMP(name, HandleType, handle, ...) \
	{ \
		/* For the unknown reader: we try to dynamic_cast to a specific handle
		 * type (e.g. of dump-type) and call specific code if it works.
		 * The HandleToXXX macro generates a struct providing easier translation
		 * of type names.
		 */ \
		typedef typename ::HMF::Handle::HandleToDump< HandleType >::type handle_type; \
		if(auto * handle##local = dynamic_cast<typename std::remove_reference<handle_type>::type*>(&handle)) { \
			/* we do not RETURN here, so that other stuff still happens (e.g.
			 * the real access to hardware).
			 */ \
			handle##local -> dump(#name, EVERYSECOND(_, *handle##local, __VA_ARGS__)); }; \
	}


//// ESS
#ifdef HAVE_ESS
#include "hal/Handle/ADCEss.h"
#include "hal/Handle/FPGAEss.h"
#include "hal/Handle/HICANNEss.h"
#include "hal/Handle/PMUEss.h"
#include "ESS/halbe_to_ess.h"
HANDLE_TO(Ess)

#define USE_ESS(return, name, HandleType, handle, ...) \
	{ \
		typedef typename ::HMF::Handle::HandleToEss< HandleType >::type handle_type; \
		if(auto * handle##local = dynamic_cast<typename std::remove_reference<handle_type>::type*>(&handle)) { \
			/* we DO RETURN here, so that nothing else happens after this (i.e.
			 * NO access to real hardware).
			 */ \
			return handle##local -> ess() . name ( EVERYSECOND(_, *handle##local, __VA_ARGS__)); }; \
	}
#else
#define USE_ESS(name, type, handle, ...)
#endif

// ECM: RPC-based calls are not genereally supported, see ADCBackend for an application
#define USE_REMOTE_HW(optionalReturnKeyword, func_name, HandleType, handle, ...)

//// Hardware

// #ifdef HAVE_HARDWARE
#include "hal/Handle/ADCHw.h"
#include "hal/Handle/FPGAHw.h"
#include "hal/Handle/HICANNHw.h"
#include "hal/Handle/PMUHw.h"
HANDLE_TO(Hw)

#define USE_HARDWARE(return, name, HandleType, handle, ...) \
	{ \
		typedef typename ::HMF::Handle::HandleToHw< HandleType >::type handle_type; \
		if(auto * handle##local = dynamic_cast<typename std::remove_reference<handle_type>::type*>(&handle)) { \
			return name##IMPL(EVERYSECOND(_, * handle##local, __VA_ARGS__)); } \
	}
// #else
// #define USE_HARDWARE(...)
// #endif

#define CALL_SCHERIFF(event, fun_name, handle) \
	if ( handle.useScheriff()) { \
		if (handle.get_scheriff().process_event( ::HMF:: event () ) == 0) \
			Scheriff::log_f_name(#fun_name); \
	}


//// Dispatch macros
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/is_empty.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>

#define IMPL_FUNC_PROTO(ReturnType, name, HandleType, ...) \
	ReturnType name ## IMPL(EVERYTWO(typename ::HMF::Handle::HandleToHw< HandleType >::type, __VA_ARGS__))

// FIXME: ugly, but late
#define IMPL_BODY_DISPATCH(ExceptionType, ret, ReturnType, name, ...) \
		USE_DUMP(name, __VA_ARGS__, _) \
		USE_ESS(ret, name, __VA_ARGS__, _) \
		USE_REMOTE_HW(ret, name, __VA_ARGS__, _) \
		BOOST_PP_IF(BOOST_PP_IS_EMPTY(ExceptionType), BOOST_PP_EMPTY(), try {) \
		USE_HARDWARE(ret, name, __VA_ARGS__, _) \
		BOOST_PP_IF(BOOST_PP_IS_EMPTY(ExceptionType), BOOST_PP_EMPTY(), } catch(ExceptionType & e) { throw std::runtime_error(std::string(e.what()) + " at: " + e.where()); }) \
		BOOST_PP_IF(BOOST_PP_IS_EMPTY(ret), BOOST_PP_EMPTY(), return ReturnType();)

#define IMPL_FUNC(ExceptionType, ret, ReturnType, name, ...) \
	IMPL_FUNC_PROTO(ReturnType, name, __VA_ARGS__); \
	ReturnType name (EVERYTWO(__VA_ARGS__)) { \
		IMPL_BODY_DISPATCH(ExceptionType, ret, ReturnType, name, __VA_ARGS__) \
	} \
	IMPL_FUNC_PROTO(ReturnType, name, __VA_ARGS__)

#define IMPL_FUNC_WITH_SHERIFF(ExceptionType, ret, ReturnType, event, name, ...) \
	IMPL_FUNC_PROTO(ReturnType, name, __VA_ARGS__); \
	ReturnType name (EVERYTWO(__VA_ARGS__)) { \
		CALL_SCHERIFF(event, name, MYSECOND(__VA_ARGS__)) \
		IMPL_BODY_DISPATCH(ExceptionType, ret, ReturnType, name, __VA_ARGS__) \
	} \
	IMPL_FUNC_PROTO(ReturnType, name, __VA_ARGS__)



#define HALBE_SETTER(name, ...) \
	IMPL_FUNC(BOOST_PP_EMPTY(), BOOST_PP_EMPTY(), void, name, __VA_ARGS__)

#define HALBE_SETTER_WITH_EXCEPTION_TRANSLATION(ExceptionType, name, ...) \
	IMPL_FUNC(ExceptionType, BOOST_PP_EMPTY(), void, name, __VA_ARGS__)

#define HALBE_SETTER_GUARDED(event, name, ...) \
	IMPL_FUNC_WITH_SHERIFF(BOOST_PP_EMPTY(), BOOST_PP_EMPTY(), void, event, name, __VA_ARGS__)

#define HALBE_SETTER_GUARDED_WITH_EXCEPTION_TRANSLATION(ExceptionType, event, name, ...) \
	IMPL_FUNC_WITH_SHERIFF(ExceptionType, BOOST_PP_EMPTY(), void, event, name, __VA_ARGS__)

#define HALBE_GETTER(ReturnType, name, ...) \
	IMPL_FUNC(BOOST_PP_EMPTY(), return, ReturnType, name, __VA_ARGS__)

// SETTER_GUARDED_RETURNS and GETTER_GUARDED are equivalent here and only differ
// in the semantics of the function
#define HALBE_SETTER_GUARDED_RETURNS(ReturnType, event, name, ...) \
	IMPL_FUNC_WITH_SHERIFF(BOOST_PP_EMPTY(), return, ReturnType, event, name, __VA_ARGS__)

#define HALBE_GETTER_GUARDED(...) \
	HALBE_SETTER_GUARDED_RETURNS(__VA_ARGS__)


#define HALBE_GETTER_WITH_EXCEPTION_TRANSLATION(ExceptionType, ReturnType, name, ...) \
	IMPL_FUNC(ExceptionType, return, ReturnType, name, __VA_ARGS__)
