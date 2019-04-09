#include "stdafx.h"
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
	wcsncpy(szDest, cszSource, nMaxSize);
	return 0;
}

#endif

double init_relay(double Input, double Tol, double Delay)
{
	return 0.0;
}


double init_limit(double Input, double dMin, double dMax)
{
	return max(min(Input,dMin),dMax);
}

double init_bit(double Input)
{
	return (Input > 0.0) ? 1.0 : 0.0;
}


double init_and(double Input1, double Input2)
{
	if (Input1 > 0 && Input2 > 0)
		return 1.0;
	else
		return 0.0;
}

double init_or(double Input1, double Input2)
{
	if (Input1 > 0 || Input2 > 0)
		return 1.0;
	else
		return 0.0;
}

double init_not(double Input)
{
	if (Input > 0)
		return 0.0;
	else
		return 1.0;
}

double init_abs(double Input)
{
	return fabs(Input);
}

double init_expand(double Input, double dTime)
{
	double dOut = 0.0;

	if (Input > 0)
		dOut = 1.0;

	return dOut;
}

double init_shrink(double Input, double dTime)
{
	double dOut = 0.0;

	if (Input > 0.0)
		dOut = 1.0;

	if (dTime <= 0.0)
		dOut = 0.0;

	return dOut;
}

double init_deadband(double Input, double dDeadBand)
{
	double dOut = 0.0;
	if (Input > dDeadBand)
		dOut = Input - dDeadBand;
	else
		if (Input < -dDeadBand)
			dOut = Input + dDeadBand;

	return dOut;
}