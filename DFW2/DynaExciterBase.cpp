#include "stdafx.h"
#include "DynaExciterBase.h"
#include "DynaModel.h"

using namespace DFW2;

CDynaExciterBase::CDynaExciterBase() : CDevice(),
			ExcLag(*this, { EqeV }, { Eqsum })
{
}

eDEVICEFUNCTIONSTATUS CDynaExciterBase::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status(eDEVICEFUNCTIONSTATUS::DFS_OK);

	double Eqnom;
	CDevice *pGen = GetSingleLink(DEVTYPE_GEN_1C);
	if (!InitConstantVariable(Eqnom, pGen, CDynaGenerator1C::m_cszEqnom, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pGen, CDynaGenerator1C::m_cszEqe, DEVTYPE_GEN_1C))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	if (CDevice::IsFunctionStatusOK(Status))
	{
		bool bRes = true;
		Ig0 = GetIg();
		Ug0 = ExtVg; 
		Eqe = Eqe0; 
		Eq0 = EqInput; 
		(VariableIndex&)ExcLag = Eqsum = Eqe0;
		Umin *= Eqnom;
		Umax *= Eqnom;
		ExtUf = 0.0;
		ExtUdec = 0.0;
		Status = bRes ? eDEVICEFUNCTIONSTATUS::DFS_OK : eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	}
	return Status;
}

double* CDynaExciterBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Eqe.Value, V_EQE)
		MAP_VARIABLE(EqeV.Value, V_EQEV)
		MAP_VARIABLE(Eqsum.Value, V_EQSUM)
	}
	return p;
}

VariableIndexRefVec& CDynaExciterBase::GetVariables(VariableIndexRefVec& ChildVec)
{
	// ExcLag добавляется автоматически в JoinVariables
	return CDevice::GetVariables(JoinVariables({Eqe, Eqsum, EqeV}, ChildVec));
}

double CDynaExciterBase::GetIg()
{
	return sqrt(GenId * GenId + GenIq * GenIq);
}

void CDynaExciterBase::SetLagTimeConstantRatio(double TexcNew)
{
	ExcLag.ChangeTimeConstant(Texc * TexcNew);
}


double* CDynaExciterBase::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
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
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtUf, GetSingleLink(DEVTYPE_EXCCON), CDynaExciterBase::m_cszUf, DEVTYPE_EXCCON_MUSTANG));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtUdec, GetSingleLink(DEVTYPE_DEC), CDynaExciterBase::m_cszUdec, DEVTYPE_DEC_MUSTANG));
	return eRes;
}

void CDynaExciterBase::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_EXCITER);
	props.SetType(DEVTYPE_EXCITER_MUSTANG);

	props.AddLinkFrom(DEVTYPE_GEN_1C, DLM_SINGLE, DPD_MASTER);
	props.AddLinkTo(DEVTYPE_DEC, DLM_SINGLE, DPD_SLAVE, CDynaExciterBase::m_cszDECId);
	props.AddLinkTo(DEVTYPE_EXCCON, DLM_SINGLE, DPD_SLAVE, CDynaExciterBase::m_cszExcConId);
	
	props.nEquationsCount = CDynaExciterBase::VARS::V_LAST;

	props.m_ConstVarMap.insert(std::make_pair(CDynaExciterBase::m_cszExcConId, CConstVarIndex(CDynaExciterBase::C_REGID, eDVT_CONSTSOURCE)));
	props.m_ConstVarMap.insert(std::make_pair(CDynaExciterBase::m_cszDECId, CConstVarIndex(CDynaExciterBase::C_DECID, eDVT_CONSTSOURCE)));

}

const char* CDynaExciterBase::m_cszUf		= "Uf";
const char* CDynaExciterBase::m_cszUdec		= "Vdec";
const char* CDynaExciterBase::m_cszExcConId	= "ExcControlId";
const char* CDynaExciterBase::m_cszDECId	= "ForcerId";
