#include "stdafx.h"
#include "DynaExciterBase.h"
#include "DynaModel.h"

using namespace DFW2;

CDynaExciterBase::CDynaExciterBase() : CDevice(),
									   PvEqsum(V_EQSUM,Eqsum),
									   ExcLag(this, &EqeV, V_EQEV, &PvEqsum)
{
	ExtUf.Value(&Uexc);
	ExtUdec.Value(&Udec);
	ExtVg.Value(&Ug0);
}

eDEVICEFUNCTIONSTATUS CDynaExciterBase::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;

	double Eqnom;
	CDevice *pGen = GetSingleLink(DEVTYPE_GEN_1C);
	if (!InitConstantVariable(Eqnom, pGen, CDynaGenerator1C::m_cszEqnom, DEVTYPE_GEN_1C))
		Status = DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pGen, CDynaGenerator1C::m_cszEqe, DEVTYPE_GEN_1C))
		Status = DFS_FAILED;

	if (CDevice::IsFunctionStatusOK(Status))
	{
		bool bRes = true;
		Ig0 = GetIg();
		Ug0 = ExtVg.Value(); 
		Eqe = Eqe0; 
		Eq0 = EqInput.Value(); 
		EqeV = Eqsum = Eqe0;
		Umin *= Eqnom;
		Umax *= Eqnom;
		Uexc = 0.0;
		Udec = 0.0;
		Status = bRes ? DFS_OK : DFS_FAILED;
	}
	return Status;
}

double* CDynaExciterBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;
	switch (nVarIndex)
	{
		MAP_VARIABLE(Eqe, V_EQE)
		MAP_VARIABLE(EqeV, V_EQEV)
		MAP_VARIABLE(Eqsum, V_EQSUM)
	}
	return p;
}

double CDynaExciterBase::GetIg()
{
	return sqrt(GenId.Value() * GenId.Value() + GenIq.Value() * GenIq.Value());
}

void CDynaExciterBase::SetLagTimeConstantRatio(double TexcNew)
{
	ExcLag.ChangeTimeConstant(Texc * TexcNew);
}


double* CDynaExciterBase::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;
	switch (nVarIndex)
	{
		MAP_VARIABLE(RegId, C_REGID)
		MAP_VARIABLE(DECId, C_DECID)
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaExciterBase::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	CDevice *pGen = GetSingleLink(DEVTYPE_GEN_1C);
	eDEVICEFUNCTIONSTATUS eRes = DeviceFunctionResult(InitExternalVariable(GenId, pGen, CDynaGenerator1C::m_cszId, DEVTYPE_GEN_1C));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(GenIq, pGen, CDynaGenerator1C::m_cszIq, DEVTYPE_GEN_1C));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtVg, pGen, CDynaNodeBase::m_cszV, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(EqInput, pGen, CDynaGenerator1C::m_cszEq));
	CDevice *pReg = GetSingleLink(DEVTYPE_EXCCON);
	CDevice *pDEC = GetSingleLink(DEVTYPE_DEC);
	if (pReg)
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtUf, pReg, CDynaExciterBase::m_cszUf, DEVTYPE_EXCCON_MUSTANG));
	if (pDEC)
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtUdec, pDEC, CDynaExciterBase::m_cszUdec, DEVTYPE_DEC_MUSTANG));
	return eRes;
}

const CDeviceContainerProperties CDynaExciterBase::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_EXCITER);
	props.SetType(DEVTYPE_EXCITER_MUSTANG);

	props.AddLinkFrom(DEVTYPE_GEN_1C, DLM_SINGLE, DPD_MASTER);
	props.AddLinkTo(DEVTYPE_DEC, DLM_SINGLE, DPD_SLAVE, CDynaExciterBase::m_cszDECId);
	props.AddLinkTo(DEVTYPE_EXCCON, DLM_SINGLE, DPD_SLAVE, CDynaExciterBase::m_cszExcConId);
	
	props.nEquationsCount = CDynaExciterBase::VARS::V_LAST;

	props.m_ConstVarMap.insert(make_pair(CDynaExciterBase::m_cszExcConId, CConstVarIndex(CDynaExciterBase::C_REGID, eDVT_CONSTSOURCE)));
	props.m_ConstVarMap.insert(make_pair(CDynaExciterBase::m_cszDECId, CConstVarIndex(CDynaExciterBase::C_DECID, eDVT_CONSTSOURCE)));

	return props;
}

const _TCHAR *CDynaExciterBase::m_cszUf			= _T("Uf");
const _TCHAR *CDynaExciterBase::m_cszUdec		= _T("Vdec");
const _TCHAR *CDynaExciterBase::m_cszExcConId	= _T("ExcControlId");
const _TCHAR *CDynaExciterBase::m_cszDECId		= _T("ForcerId");
