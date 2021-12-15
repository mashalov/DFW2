#include "stdafx.h"
#include "NodeMeasures.h"
#include "DynaModel.h"

using namespace DFW2;

double* CDynaNodeMeasure::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double* p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Pload.Value, V_PLOAD)
		MAP_VARIABLE(Qload.Value, V_QLOAD)
	}
	return p;
}


bool CDynaNodeMeasure::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes{ true };
	pDynaModel->SetElement(Pload, Pload, 1.0);
	pDynaModel->SetElement(Qload, Qload, 1.0);
	return true;
}


bool CDynaNodeMeasure::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(Pload, 0.0);
	pDynaModel->SetFunction(Qload, 0.0);
	return true;
}

eDEVICEFUNCTIONSTATUS CDynaNodeMeasure::Init(CDynaModel* pDynaModel)
{
	return ProcessDiscontinuity(pDynaModel);
}

VariableIndexRefVec& CDynaNodeMeasure::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Pload, Qload }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CDynaNodeMeasure::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	Pload = m_pNode->Pnr;
	Qload = m_pNode->Qnr;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

// описание переменных расчетных параметров узла
void CDynaNodeMeasure::DeviceProperties(CDeviceContainerProperties& props)
{
	// линковка делается только с узлами, поэтому описание
	// правил связывания не нужно
	props.SetType(DEVTYPE_NODEMEASURE);
	props.SetClassName(CDeviceContainerProperties::m_cszNameNodeMeasure, CDeviceContainerProperties::m_cszSysNameNodeMeasure);
	props.nEquationsCount = CDynaNodeMeasure::VARS::V_LAST;
	props.m_VarMap.insert({ m_cszPload,  CVarIndex(CDynaNodeMeasure::V_PLOAD, VARUNIT_MW) });
	props.m_VarMap.insert({ m_cszQload,  CVarIndex(CDynaNodeMeasure::V_QLOAD, VARUNIT_MVAR) });

	props.m_VarAliasMap.insert({ { "pn", m_cszPload }, { "qn", m_cszQload } });

	// измерения создаются индивидуально с узлом в конструкторе
	//props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaBranchMeasure>>();
}
