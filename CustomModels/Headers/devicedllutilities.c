#include "DeviceDLLUtilities.h"

void SetConstantVarInfo(ConstVarsInfo *pVarsInfo, const _TCHAR *cszName, double dMin, double dMax, double dDefaultValue, int eDeviceType, int VarFlags)
{
	_tcscpy_s(pVarsInfo->VarInfo.Name, CUSTOMDEVICEDLL_MAXNAME, cszName);
	pVarsInfo->dMin = dMin;
	pVarsInfo->dMax = dMax;
	pVarsInfo->dDefault = dDefaultValue;
	pVarsInfo->eDeviceType = eDeviceType;
	pVarsInfo->VarFlags = VarFlags;
}

void SetLink(LinkType *pLink, enum eLINKTYPE eLinkType, int eDeviceType, int eLinkMode, int eDependency, const _TCHAR *cszLinkField)
{
	pLink->eLinkType = eLinkType;
	pLink->eDeviceType = eDeviceType;
	pLink->eLinkMode = eLinkMode;
	pLink->eDependency = eDependency;
	_tcscpy_s(pLink->LinkField, CUSTOMDEVICEDLL_MAXNAME, cszLinkField);
}

void SetInputVarsInfo(InputVarsInfo *pVarsInfo, const _TCHAR *cszName, int eDeviceType, const _TCHAR *cszKey)
{
	pVarsInfo->eDeviceType = eDeviceType;
	_tcscpy_s(pVarsInfo->VarInfo.Name, CUSTOMDEVICEDLL_MAXNAME, cszName);
	_tcscpy_s(pVarsInfo->Key, CUSTOMDEVICEDLL_MAXNAME, cszKey);
}

void SetVarInfo(VarsInfo *pVarsInfo, long nIndex, const _TCHAR *cszName, int eVarType, int bOutput)
{
	_tcscpy_s(pVarsInfo->Name, CUSTOMDEVICEDLL_MAXNAME, cszName);
	pVarsInfo->nIndex = nIndex;
	pVarsInfo->eUnits = eVarType;
	pVarsInfo->bOutput = bOutput;
}

void SetPinInfo(BLOCKPININDEX* pBlockPin, long nIndex, enum eVARIABLELOCATION eVarLocation)
{
	pBlockPin->Location = eVarLocation;
	pBlockPin->nIndex = nIndex;
}

void SetBlockDescription(BlockDescriptions * pBlockDescription, enum PrimitiveBlockType eType, long nId, const _TCHAR* cszName)
{
	pBlockDescription->eType = eType;
	pBlockDescription->UserId = nId;
	_tcscpy_s(pBlockDescription->Name, CUSTOMDEVICEDLL_MAXNAME, cszName);
}


#ifndef _MSC_VER


errno_t _tcscpy_s(_TCHAR* szDest, size_t nMaxSize, const _TCHAR* cszSource)
{
	_tcsncpy(szDest, cszSource, nMaxSize);
	return 0;
}

#endif


BOOL __declspec(dllexport) DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved) 
{
	return TRUE;
}
