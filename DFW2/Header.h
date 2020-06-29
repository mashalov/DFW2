#pragma once
#include <stdlib.h>  
#include <crtdbg.h>  

#define _USE_MATH_DEFINES
#include <cmath>

#include "windows.h"
#include "string"
#include "complex"
#include "vector"
#include "list"
#include "map"
#include "set"
#include "algorithm"
#include "cex.h"
#include "dfw2exception.h"
#include "Messages.h"

namespace DFW2
{
	typedef std::complex<double> cplx;

#define DFW2_EPSILON 1E-10
#define DFW2_ATOL_DEFAULT 1E-7
#define DFW2_RTOL_DEFAULT 1E-4
#define M_SQRT3 1.7320508075688772935274463415059
#define DFW2_VERSION 1
#define DFW2_RESULTFILE_VERSION 1

#define _CheckNumber(a) _ASSERTE(!_isnan((a)) && !isinf((a)));


#define Equal(x,y) (fabs((x)-(y)) < DFW2_EPSILON)

}

