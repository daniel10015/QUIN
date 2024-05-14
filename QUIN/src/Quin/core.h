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