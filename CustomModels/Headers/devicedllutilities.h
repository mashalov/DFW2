#define UNICODE
#include "stddef.h"
#include "tchar.h"
#include "string.h"
#include "windows.h"
#include "DLLStructs.h"

#define _USE_MATH_DEFINES
#include "math.h"

errno_t _tcscpy_s(_TCHAR* szDest, size_t nMaxSize, const _TCHAR* cszSource);
void SetConstantVarInfo(ConstVarsInfo *pVarsInfo, const _TCHAR *cszName, double dMin, double dMax, double dDefaultValue, int eDeviceType, int VarFlags);
void SetLink(LinkType *pLink, enum eLINKTYPE eLinkType, int eDeviceType, int eLinkMode, int eDependency, const _TCHAR *cszLinkField);
void SetInputVarsInfo(InputVarsInfo *pVarsInfo, const _TCHAR *cszName, int eDeviceType, const _TCHAR *cszKey);
void SetVarInfo(VarsInfo *pVarsInfo, long nIndex, const _TCHAR *cszName, int eVarType, int bOutput);
void SetPinInfo(BLOCKPININDEX* pBlockPin, long nIndex, enum eVARIABLELOCATION eVarLocation);
void SetBlockDescription(BlockDescriptions * pBlockDescription, enum PrimitiveBlockType eType, long nId, const _TCHAR* cszName);
BOOL __declspec(dllexport) DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved);