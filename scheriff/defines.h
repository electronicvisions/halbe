#pragma once

// ECM: currently helps only up to 50...
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS


#if !defined(BOOST_MPL_LIMIT_VECTOR_SIZE)
#	define BOOST_MPL_LIMIT_VECTOR_SIZE 50
#else
#	if BOOST_MPL_LIMIT_VECTOR_SIZE < 50
		// undef isn't feasible as it might produce linker errors,
		// let's inform the user and let him fix the build
#		error "BOOST_MPL_LIMIT_VECTOR_SIZE too small"
#	endif
#endif


#if !defined( BOOST_MPL_LIMIT_MAP_SIZE)
	#	define BOOST_MPL_LIMIT_MAP_SIZE 50
#else
#	if BOOST_MPL_LIMIT_MAP_SIZE < 50
#		error "BOOST_MPL_LIMIT_MAP_SIZE too small"
#	endif
#endif


#if !defined( FUSION_MAX_VECTOR_SIZE)
#	define FUSION_MAX_VECTOR_SIZE 20
#else
#	if FUSION_MAX_VECTOR_SIZE < 22
#		error "FUSION_MAX_VECTOR_SIZE too small"
#	endif
#endif


#if !defined( FUSION_MAX_MAP_SIZE)
#	define FUSION_MAX_MAP_SIZE 20
#else
#	if FUSION_MAX_MAP_SIZE < 20
#		error "FUSION_MAX_MAP_SIZE too small"
#	endif
#endif
