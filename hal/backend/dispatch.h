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
#include <utility>

#include "hate/iterator_traits.h"

#include "hal/macro_HALbe.h"


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

template<typename TO_TYPE, typename T>
auto handles_conversion(T handles)
{
	/* TODO: Instead of hard-coding support for std::vector we could deduce
	 * the container type here.
	 */
	static_assert(hate::is_specialization_of<T, std::vector>::value,
		"only vector-handles are supported for now");
	std::vector<boost::shared_ptr<TO_TYPE>> ret;
	for (auto h: handles) {
		boost::shared_ptr<TO_TYPE> tmp = boost::dynamic_pointer_cast<TO_TYPE>(h);
		if (tmp) {
			ret.push_back(tmp);
		}
	}
	return ret;
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
template<> struct HandleTo##suffix##Impl< std::vector< boost::shared_ptr<ADC> > > { typedef std::vector< boost::shared_ptr< ADC##suffix > > type; }; \
template<> struct HandleTo##suffix##Impl< HICANN > { typedef HICANN##suffix type; }; \
template<> struct HandleTo##suffix##Impl< std::vector< boost::shared_ptr<HICANN> > > { typedef std::vector< boost::shared_ptr< HICANN##suffix > > type; }; \
template<> struct HandleTo##suffix##Impl< FPGA >   { typedef FPGA##suffix type; }; \
template<> struct HandleTo##suffix##Impl< std::vector< boost::shared_ptr<FPGA> > > { typedef std::vector< boost::shared_ptr< FPGA##suffix > > type; }; \
template<> struct HandleTo##suffix##Impl< PMU >   { typedef PMU##suffix type; }; \
template<> struct HandleTo##suffix##Impl< std::vector< boost::shared_ptr<PMU> > > { typedef std::vector< boost::shared_ptr< PMU##suffix > > type; }; \
template <typename T> struct HandleTo##suffix { \
	typedef typename HandleTo##suffix##Impl< typename std::decay<T>::type >::type tmp;  \
	typedef typename CopyQualifiers<T, tmp >::type type; }; \
}}} // end ::HMF::Handle::<anonymous>


//// Dump
#include "hal/Handle/ADCDump.h"
#include "hal/Handle/FPGADump.h"
#include "hal/Handle/PMUDump.h"
HANDLE_TO(Dump)

template<typename HandleType, typename... Args>
auto do_use_dump(char const* fooname, HandleType& handle, Args... args)
{
	/* For the unknown reader: we try to dynamic_cast to a specific handle
	 * type (e.g. of dump-type) and call specific code if it works.
	 * The HandleToXXX macro generates a struct providing easier translation
	 * of type names.
	 */
	if constexpr(! hate::has_iterator<HandleType>::value) {
		typedef typename ::HMF::Handle::HandleToDump<HandleType>::type dumphandle_type;
		if(auto* h = dynamic_cast<typename std::remove_reference<dumphandle_type>::type*>(&handle)) {
			h->dump(fooname, *h, args...);
		}
	} else {
		typedef typename ::HMF::Handle::HandleToDump<typename HandleType::value_type::element_type>::type handle_type;
		auto handles = handles_conversion<handle_type>(handle);
		for (auto h: handles) {
			h->dump(fooname, *h, args...);
		}
	}
}

#define USE_DUMP(name, HandleType, handle, ...) \
	{ \
		do_use_dump(BOOST_PP_STRINGIZE(name##IMPL), EVERYSECOND(HandleType, handle,  __VA_ARGS__)); \
	}


//// ESS
#ifdef HAVE_ESS
#include "hal/Handle/ADCEss.h"
#include "hal/Handle/FPGAEss.h"
#include "hal/Handle/HICANNEss.h"
#include "hal/Handle/PMUEss.h"
#include "ESS/halbe_to_ess.h"
HANDLE_TO(Ess)


#define CREATE_ESS_DISPATCHER(rreturn, name, HandleType, handle, ...) \
template<typename T> \
auto __ess_dispatch_##name(EVERYTWO(T, handle, __VA_ARGS__), typename std::enable_if<!hate::has_iterator<T>::value>::type* = 0) \
{ \
	typedef typename ::HMF::Handle::HandleToEss<typename std::remove_reference<decltype(handle)>::type>::type __handle_type; \
	__handle_type* __h = dynamic_cast<typename std::remove_pointer<typename std::remove_reference<__handle_type>>::type::type*>(&handle); \
	typedef decltype(__h->ess(). name (EVERYSECOND(__handle_type, *__h, __VA_ARGS__))) __return_type; \
	if constexpr(std::is_same<__return_type, void>::value) { \
		if (__h) { \
			__h->ess(). name (EVERYSECOND(__handle_type, *__h, __VA_ARGS__)); \
		} \
		return std::make_pair(false, nullptr); \
	} else { \
		auto ret = std::make_pair(false, __return_type()); \
		if (__h) { \
			ret = std::make_pair(true, __h->ess(). name (EVERYSECOND(__handle_type, *__h, __VA_ARGS__))); \
		} \
		return ret; \
	} \
} \
template<typename T> \
auto __ess_dispatch_##name(EVERYTWO(T, handle, __VA_ARGS__), typename std::enable_if<hate::has_iterator<T>::value>::type* = 0) \
{ \
	typedef typename decltype(handle) ::value_type __value_type; \
	typedef typename ::HMF::Handle::HandleToEss<typename __value_type::element_type>::type __handle_type; \
	auto __handles = handles_conversion<__handle_type>(handle); \
	auto ret = std::make_pair(false, nullptr); \
	if (! (__handles.empty() || handle.empty())) { \
		/* just use one handle (HAL2ESS doesn't really match the hardware backend) to pass the vector */ \
		__handles[0]->ess(). name (EVERYSECOND(__handle_type, handle, __VA_ARGS__)); \
		ret = std::make_pair(true, nullptr); \
	} \
	return ret; \
}


#define USE_ESS(return, name, HandleType, handle, ...) \
	{ \
		/* IMPORTANT:
		 * We DO RETURN here, so that nothing else happens after this (i.e.
		 * NO access to real hardware).
		 */ \
		auto ret = __ess_dispatch_##name<HandleType>(EVERYSECOND(HandleType, handle, __VA_ARGS__)); \
		if (std::get<0>(ret)) { \
			return std::get<1>(ret); \
		} else { \
			/* continue to hardware as no ESS handle was found */ \
		} \
	}
#else
#define CREATE_ESS_DISPATCHER(return, name, HandleType, handle, ...)
#define USE_ESS(name, type, handle, ...)
#endif


//// Hardware

// ECM: RPC-based calls are not genereally supported, see ADCBackend for an application
#define USE_REMOTE_HW(optionalReturnKeyword, func_name, HandleType, handle, ...)

// #ifdef HAVE_HARDWARE
#include "hal/Handle/ADCHw.h"
#include "hal/Handle/FPGAHw.h"
#include "hal/Handle/HICANNHw.h"
#include "hal/Handle/PMUHw.h"
HANDLE_TO(Hw)


template<typename F, typename HandleType, typename... Args>
auto do_use_hardware(F foo, HandleType& handle, Args... args)
{
	if constexpr(! hate::has_iterator<HandleType>::value) {
		typedef typename ::HMF::Handle::HandleToHw<HandleType>::type handle_type;
		if(auto * h = dynamic_cast<typename std::remove_reference<handle_type>::type*>(&handle)) {
			return foo(*h, args...);
		} else {
			// get rid of "non-void function missing return" warnings...
			typedef decltype(foo(*h, args...)) return_type;
			if constexpr(! std::is_same<return_type, void>::value) {
				return return_type{};
			}
		}
	} else {
		typedef typename ::HMF::Handle::HandleToHw<typename HandleType::value_type::element_type>::type handle_type;
		auto handles = handles_conversion<handle_type>(handle);
		return foo(handles, args...);
	}
}

#define USE_HARDWARE(ReturnType, return, name, HandleType, handle, ...) \
	{ \
		typedef ::HMF::Handle::HandleToHw<HandleType>::type typehw; \
		return do_use_hardware( \
			static_cast<ReturnType (*) ( EVERYSECOND_DROP_LAST(handle, typehw, _, __VA_ARGS__) )>( & name##IMPL ), \
			EVERYSECOND(HandleType, handle,  __VA_ARGS__) \
		); \
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
		USE_HARDWARE(ReturnType, ret, name, __VA_ARGS__, _) \
		BOOST_PP_IF(BOOST_PP_IS_EMPTY(ExceptionType), BOOST_PP_EMPTY(), } catch(ExceptionType & e) { throw std::runtime_error(std::string(e.what()) + " at: " + e.where()); }) \
		BOOST_PP_IF(BOOST_PP_IS_EMPTY(ret), BOOST_PP_EMPTY(), return ReturnType();)

#define IMPL_FUNC(ExceptionType, ret, ReturnType, name, ...) \
	IMPL_FUNC_PROTO(ReturnType, name, __VA_ARGS__); \
	CREATE_ESS_DISPATCHER(ret, name, __VA_ARGS__, _) \
	ReturnType name (EVERYTWO(__VA_ARGS__)) { \
		IMPL_BODY_DISPATCH(ExceptionType, ret, ReturnType, name, __VA_ARGS__) \
	} \
	IMPL_FUNC_PROTO(ReturnType, name, __VA_ARGS__)

#define HALBE_SETTER(name, ...) \
	IMPL_FUNC(BOOST_PP_EMPTY(), BOOST_PP_EMPTY(), void, name, __VA_ARGS__)

#define HALBE_SETTER_WITH_EXCEPTION_TRANSLATION(ExceptionType, name, ...) \
	IMPL_FUNC(ExceptionType, BOOST_PP_EMPTY(), void, name, __VA_ARGS__)

#define HALBE_GETTER(ReturnType, name, ...) \
	IMPL_FUNC(BOOST_PP_EMPTY(), return, ReturnType, name, __VA_ARGS__)

#define HALBE_GETTER_WITH_EXCEPTION_TRANSLATION(ExceptionType, ReturnType, name, ...) \
	IMPL_FUNC(ExceptionType, return, ReturnType, name, __VA_ARGS__)
