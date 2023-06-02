#pragma once
#include <stdlib.h>  
#ifdef _MSC_VER
#include <crtdbg.h>  
#endif

#define _USE_MATH_DEFINES
#include <cmath>

#ifdef _MSC_VER
#include "windows.h"
#define	LIBMODULE HMODULE
#else
#include <dlfcn.h>
#define LIBMODULE void*
typedef int (*FARPROC)();
#endif


#include "string"
#include "complex"
#include "vector"
#include "list"
#include "map"
#include "set"
#include "algorithm"
#include "dfw2exception.h"
#include "Messages.h"

#ifndef _MSC_VER
#include <assert.h>
#define _ASSERTE(a) assert((a))
#define __cdecl __attribute__((__cdecl__))
#endif

namespace DFW2
{
	using cplx = std::complex<double> ;

	class Consts
	{
	public:
		static constexpr double epsilon = 1e-10;
		static constexpr double atol_default = 1e-4;
		static constexpr double rtol_default = 1e-4;
		static inline const double sqrt3 = std::sqrt(3);
		static constexpr int dfwv_ersion = 1;
		static constexpr int dfw_result_file_version = 2;
		static inline bool Equal(const double& x, const double& y)
		{
			return std::abs(x - y) <= epsilon;
		}
	};

//#define USE_FMA_FULL
//#define USE_FMA

#if defined USE_FMA_FULL
#define USE_FMA
#endif 

#if defined USE_FMA || defined USE_FMA_FULL
#define FP_FAST_FMA 
#endif

#define _CheckNumber(a) _ASSERTE(!std::isnan((a)) && !std::isinf((a)));


}

