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
	typedef std::complex<double> cplx;

#define DFW2_EPSILON 1E-10
#define DFW2_ATOL_DEFAULT 1E-4
#define DFW2_RTOL_DEFAULT 1E-4
#define M_SQRT3 1.7320508075688772935274463415059
#define DFW2_VERSION 1
#define DFW2_RESULTFILE_VERSION 2

//#define USE_FMA_FULL
//#define USE_FMA

#if defined USE_FMA_FULL
#define USE_FMA
#endif 

#if defined USE_FMA || defined USE_FMA_FULL
#define FP_FAST_FMA 
#endif

#define _CheckNumber(a) _ASSERTE(!std::isnan((a)) && !std::isinf((a)));

	inline bool Equal(const double x, const double y)
	{
		return std::abs(x - y) < DFW2_EPSILON;
	}
}

