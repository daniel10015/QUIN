#pragma once


#ifdef QN_PLATFORM_WINDOWS
	#ifdef QN_BUILD_DLL
		#define QUIN_API __declspec(dllexport)
	
	#else
		#define QUIN_API __declspec(dllimport)
	#endif
#else
	#error Quin only supports windows!
#endif

// clever macro to assign bits to categories
#define BIT(x) (1<<x)

// asserts (use custom logging instead of actual asserts)
#ifdef QN_ENABLE_ASSERTS
	#define QN_ASSERT(v, msg) if(!v) { QN_ERROR("ASSERTION FAILED {0}", msg); __debugbreak(); }
	#define QN_CORE_ASSERT(v, msg) if(!v) { QN_CORE_ERROR("ASSERTION FAILED {0}", msg); __debugbreak(); }
#else
	#define QN_ASSERT(v, msg) v;
	#define QN_CORE_ASSERT(v, msg) v;
#endif