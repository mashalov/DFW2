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
	return ProcessDiscontinuityImpl(pDynaModel);
}

VariableIndexRefVec& CDynaNodeMeasure::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ Pload, Qload }, ChildVec));
}

eDEVICEFUNCTIONSTATUS CDynaNodeMeasure::ProcessDiscontinuityImpl(CDynaModel* pDynaModel)
{
	m_pNode->GetPnrQnrSuper();
	Pload = m_pNode->Pnr;
	Qload = m_pNode->Qnr;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaNodeMeasure::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	_ASSERTE(CDevice::IsFunctionStatusOK(m_pNode->DiscontinuityProcessed()));
	return ProcessDiscontinuityImpl(pDynaModel);
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

double* CDynaNodeZeroLoadFlow::GetVariablePtr(ptrdiff_t nVarIndex)
{
	// все переменные уже собраны в вектор в конструкторе
	// поэтому просто возвращаем их по индексам
	if (nVarIndex < static_cast<ptrdiff_t>(m_Vars.size()))
		return &m_Vars[nVarIndex].get().Value;
	else
		throw dfw2error("CDynaNodeZeroLoadFlow::GetVariablePtr - variable index out of range");
}

VariableIndexRefVec& CDynaNodeZeroLoadFlow::GetVariables(VariableIndexRefVec& ChildVec)
{
	// вектор переменных уже собран в конструкторе
	return CDevice::GetVariables(JoinVariables(m_Vars, ChildVec));
}

bool CDynaNodeZeroLoadFlow::BuildEquations(CDynaModel* pDynaModel)
{
	MatrixRow* pRow{ m_MatrixRows.get() };
	const MatrixRow* pMatrixRowsEnd{ pRow + m_nSize };

	while (pRow < pMatrixRowsEnd)
	{
		pDynaModel->SetElement(pRow->Vre, pRow->Vre, 1.0);
		pDynaModel->SetElement(pRow->Vim, pRow->Vim, 1.0);
		pRow++;
	}
	return true;
}

eDEVICEFUNCTIONSTATUS CDynaNodeZeroLoadFlow::Init(CDynaModel* pDynaModel)
{
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaNodeZeroLoadFlow::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CDynaNodeZeroLoadFlow::BuildRightHand(CDynaModel* pDynaModel)
{
	MatrixRow* pRow{ m_MatrixRows.get() };
	const MatrixRow* pMatrixRowsEnd{ pRow + m_nSize };

	while (pRow < pMatrixRowsEnd)
	{
		pDynaModel->SetFunction(pRow->Vre, 0.0);
		pDynaModel->SetFunction(pRow->Vim, 0.0);
		pRow++;
	}

	return true;
}

void CDynaNodeZeroLoadFlow::UpdateSuperNodeSet(const NodeSet& ZeroLFNodes)
{
	// рассчитываем количество переменных (индикаторов напряжения)
	// необходимых для формирования системы YU
	m_nSize = 0;
	// задан сет суперузлов
	for (const auto& ZeroSuperNode : ZeroLFNodes)
	{
		const auto& ZeroSuperNodeData{ ZeroSuperNode->ZeroLF.ZeroSupeNode };
		// считаем количество переменных по количеству узлов в суперузлах
		m_nSize += ZeroSuperNodeData->LFMatrix.size();
	}
	// значения переменных храним в векторе

	if (!m_nSize)
		return;

	// размерность вектора выбираем на все узлы, чтобы не менять адреса переменных
	// после обновления сета суперузлов
	if(!m_MatrixRows)
		m_MatrixRows = std::make_unique<MatrixRow[]>((*ZeroLFNodes.begin())->GetContainer()->Count());

	// для доступа к переменным снаружи собираем их в стандартный вектор ссылок
	m_Vars.reserve(2 * m_nSize);

	MatrixRow* pRow{ m_MatrixRows.get() };
	for (const auto& ZeroSuperNode : ZeroLFNodes)
	{
		const auto& ZeroSuperNodeData{ ZeroSuperNode->ZeroLF.ZeroSupeNode };
		for (const auto& ZeroNode : ZeroSuperNodeData->LFMatrix)
		{
			pRow->pNode = ZeroNode.pNode;
			m_Vars.push_back(pRow->Vre);
			m_Vars.push_back(pRow->Vim);
			pRow++;
		}
	}
}

void CDynaNodeZeroLoadFlow::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_ZEROLOADFLOW);
	props.SetClassName(CDeviceContainerProperties::m_cszNameZeroLoadFlow, CDeviceContainerProperties::m_cszSysNameZeroLoadFlow);
	props.nEquationsCount = 0;
	props.bVolatile = true;
}
