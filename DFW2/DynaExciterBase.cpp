#include "stdafx.h"
#include "DynaExciterBase.h"
#include "DynaModel.h"

using namespace DFW2;

CDynaExciterBase::CDynaExciterBase() : CDevice(),
			//  выход лага идет на ограничитель, а не на выход
			ExcLag(*this, { EqeLag }, { Eqsum })	
{
	ExcLag.SetName(cszLag_);
}

eDEVICEFUNCTIONSTATUS CDynaExciterBase::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status(eDEVICEFUNCTIONSTATUS::DFS_OK);

	CDevice* pGen{ GetSingleLink(DEVTYPE_GEN_DQ) };
	if (!InitConstantVariable(Eqnom, pGen, CDynaGeneratorDQBase::cszEqnom_, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Eqe0, pGen, CDynaGeneratorDQBase::cszEqe_, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	if (!InitConstantVariable(Inom, pGen, CDynaGenerator1C::cszInom_, DEVTYPE_GEN_DQ))
		Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	if (CDevice::IsFunctionStatusOK(Status))
	{
		Ig0 = GetIg();
		Ug0 = ExtVg; 
		Eqe = Eqe0; 
		Eq0 = EqInput; 
		(VariableIndex&)ExcLag = Eqsum = Eqe0;
		Umin *= Eqnom;
		Umax *= Eqnom;
		ExtUf = 0.0;
		ExtUdec = 0.0;
	}
	return Status;
}

double* CDynaExciterBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Eqe.Value, V_EQE)
		MAP_VARIABLE(Eqsum.Value, V_EQSUM)
		MAP_VARIABLE(EqeV.Value, V_EQEV)
		MAP_VARIABLE(EqeLag.Value, V_EQELAG)
	}
	return p;
}

VariableIndexRefVec& CDynaExciterBase::GetVariables(VariableIndexRefVec& ChildVec)
{
	// ExcLag добавляется автоматически в JoinVariables
	return CDevice::GetVariables(JoinVariables({Eqe, Eqsum, EqeV, EqeLag}, ChildVec));
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
		MAP_VARIABLE(Uaux, C_UAUX)
	}
	return p;
}

eDEVICEFUNCTIONSTATUS CDynaExciterBase::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	CDevice *pGen = GetSingleLink(DEVTYPE_GEN_DQ);
	eDEVICEFUNCTIONSTATUS eRes = DeviceFunctionResult(InitExternalVariable(GenId, pGen, CDynaGeneratorDQBase::cszId_, DEVTYPE_GEN_DQ));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(GenIq, pGen, CDynaGeneratorDQBase::cszIq_, DEVTYPE_GEN_DQ));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtVg, pGen, CDynaNodeBase::cszV_, DEVTYPE_NODE));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(EqInput, pGen, CDynaGeneratorDQBase::cszEq_));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtUf, GetSingleLink(DEVTYPE_EXCCON), CDynaExciterBase::cszUf_, DEVTYPE_EXCCON_MUSTANG));
	eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtUdec, GetSingleLink(DEVTYPE_DEC), CDynaExciterBase::cszUdec_, DEVTYPE_DEC_MUSTANG));
	return eRes;
}

void CDynaExciterBase::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_EXCITER);
	props.SetType(DEVTYPE_EXCITER_MUSTANG);
	props.Aliases_.push_back(cszAliasExciter_);

	props.AddLinkFrom(DEVTYPE_GEN_DQ, DLM_SINGLE, DPD_MASTER);
	props.AddLinkTo(DEVTYPE_DEC, DLM_SINGLE, DPD_SLAVE, CDynaExciterBase::cszDECId_);
	props.AddLinkTo(DEVTYPE_EXCCON, DLM_SINGLE, DPD_SLAVE, CDynaExciterBase::cszExcConId_);
	
	props.EquationsCount = CDynaExciterBase::VARS::V_LAST;

	props.ConstVarMap_.insert(std::make_pair(CDynaExciterBase::cszExcConId_, CConstVarIndex(CDynaExciterBase::C_REGID, VARUNIT_PIECES, eDVT_CONSTSOURCE)));
	props.ConstVarMap_.insert(std::make_pair(CDynaExciterBase::cszDECId_, CConstVarIndex(CDynaExciterBase::C_DECID, VARUNIT_PIECES, eDVT_CONSTSOURCE)));
	props.ConstVarMap_.insert(std::make_pair(CDynaExciterBase::cszUaux_, CConstVarIndex(CDynaExciterBase::C_UAUX, VARUNIT_PU, eDVT_INTERNALCONST)));

	props.VarAliasMap_.insert({
		{ "Udop2", CDynaExciterBase::cszUaux_ }
		});
}
