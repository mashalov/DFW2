#ifdef _MSC_VER
#include "..\DFW2\DLLStructs.h"
#else
#define UNICODE
typedef long ptrdiff_t;
#include "DLLStructs.h"
#endif

#include "tchar.h"
#include "string.h"


#define _USE_MATH_DEFINES

#ifdef _MSC_VER
#include "cmath"
#else
#include "math.h"
errno_t _tcscpy_s(_TCHAR* szDest, size_t nMaxSize, const _TCHAR* cszSource);
#endif

double init_relay(double Input, double Tol, double Delay);
double init_limit(double Input, double dMin, double dMax);
double init_bit(double Input);
double init_and(double Input1, double Input2);
double init_or(double Input1, double Input2);
double init_not(double Input);
double init_abs(double Input);
double init_deadband(double Input, double dDeadBand);
double init_expand(double Input, double dTime);
double init_shrink(double Input, double dTime);

void SetConstantVarInfo(ConstVarsInfo *pVarsInfo, const _TCHAR *cszName, double dMin, double dMax, double dDefaultValue, int eDeviceType, int VarFlags);
void SetLink(LinkType *pLink, enum eLINKTYPE eLinkType, int eDeviceType, int eLinkMode, int eDependency, const _TCHAR *cszLinkField);
void SetInputVarsInfo(InputVarsInfo *pVarsInfo, const _TCHAR *cszName, int eDeviceType, const _TCHAR *cszKey);
void SetVarInfo(VarsInfo *pVarsInfo, long nIndex, const _TCHAR *cszName, int eVarType, int bOutput);
void SetPinInfo(BLOCKPININDEX* pBlockPin, long nIndex, enum eVARIABLELOCATION eVarLocation);
void SetBlockDescription(BlockDescriptions * pBlockDescription, enum PrimitiveBlockType eType, long nId, const _TCHAR* cszName);
