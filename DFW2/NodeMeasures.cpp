#include "stdafx.h"
#include "NodeMeasures.h"
#include "DynaModel.h"
#include "DynaGeneratorInfBus.h"

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
	CDynaNodeBase **ppNode{ m_MatrixRows.get() },  **ppNodeEnd{ m_MatrixRows.get() + m_nSize };

	while (ppNode < ppNodeEnd)
	{
		const auto& pNode{ *ppNode };
		const auto& ZeroNode{ pNode->ZeroLF };
		pDynaModel->SetElement(ZeroNode.vRe, ZeroNode.vRe, 1.0);
		pDynaModel->SetElement(ZeroNode.vIm, ZeroNode.vIm, 1.0);


		for (const VirtualBranch* vb = ZeroNode.pVirtualBranchesBegin; vb < ZeroNode.pVirtualBranchesEnd; vb++)
		{
			//Is += vb->Y * cplx(vb->pNode->Vre, vb->pNode->Vim);
			//pDynaModel->SetElement(pNode->Vre, vb->pNode->Vre, 1.0);
		}

		for (const VirtualBranch* vb = ZeroNode.pVirtualZeroBranchesBegin; vb < ZeroNode.pVirtualZeroBranchesEnd; vb++)
		{
			//Is += vb->Y * cplx(vb->pNode->ZeroLF.vRe, vb->pNode->ZeroLF.vIm);
		}
		

		ppNode++;
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
	CDynaNodeBase** ppNode{ m_MatrixRows.get() }, ** ppNodeEnd{ m_MatrixRows.get() + m_nSize };

	while (ppNode < ppNodeEnd)
	{

		const auto& pNode{ *ppNode };
		auto& ZeroNode{ pNode->ZeroLF };

		// сначала рассчитываем составляющую в настоящей мощности
		// нагрузка и проводимость на землю
		pNode->GetPnrQnr();

		// получаем ток узла от настоящего шунта, инъекции и тока
		// генератора УР
		cplx Is{ -pNode->GetSelfImbInotSuper() };
		// добавляем инъекцию тока от базисного узла
		Is += ZeroNode.SlackInjection;


		// добавляем токи от генераторов
		CDevice** ppGen(nullptr);
		CLinkPtrCount* pGenLink = pNode->GetLink(1);
		while (pGenLink->InMatrix(ppGen))
		{
			const auto& pGen{ static_cast<CDynaPowerInjector*>(*ppGen) };
			Is += cplx(pGen->Ire, pGen->Iim);
		}

		double Re{ Is.real() }, Im{ Is.imag() };

		// перетоки по настоящим связям от внешних суперузлов
		for (const VirtualBranch* vb = ZeroNode.pVirtualBranchesBegin; vb < ZeroNode.pVirtualBranchesEnd; vb++)
		{
			Is += vb->Y * cplx(vb->pNode->Vre, vb->pNode->Vim);
			Re += vb->Y.real() * vb->pNode->Vre - vb->Y.imag() * vb->pNode->Vim;
			Im += vb->Y.imag() * vb->pNode->Vre + vb->Y.real() * vb->pNode->Vim;
		}

		// инъекция от "шунта" индикатора
		Is -= pNode->ZeroLF.Yii * cplx(pNode->ZeroLF.vRe, pNode->ZeroLF.vIm);
		// далее добавляем "токи" от индикаторов напряжения


		Re += -pNode->ZeroLF.vRe * pNode->ZeroLF.Yii;
		Im += -pNode->ZeroLF.vIm * pNode->ZeroLF.Yii;

		for (const VirtualBranch* vb = ZeroNode.pVirtualZeroBranchesBegin; vb < ZeroNode.pVirtualZeroBranchesEnd; vb++)
		{
			Is += vb->Y * cplx(vb->pNode->ZeroLF.vRe, vb->pNode->ZeroLF.vIm);
			Re += vb->Y.real() * vb->pNode->ZeroLF.vRe;
			Im += vb->Y.real() * vb->pNode->ZeroLF.vIm;
		}


		//_ASSERTE(std::abs(Re) < DFW2_EPSILON && std::abs(Im) < DFW2_EPSILON);
		_ASSERTE(std::abs(Re - Is.real()) < DFW2_EPSILON && std::abs(Im - Is.imag()) < DFW2_EPSILON);

		pDynaModel->SetFunction(ZeroNode.vRe, 0.0);
		pDynaModel->SetFunction(ZeroNode.vIm, 0.0);

		ppNode++;
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
		auto& ZeroSuperNodeData{ ZeroSuperNode->ZeroLF.ZeroSupeNode };
		// считаем количество переменных по количеству узлов в суперузлах
		m_nSize += ZeroSuperNodeData->LFMatrix.size();
	}
	// значения переменных храним в векторе

	if (!m_nSize)
		return;

	// размерность вектора выбираем на все узлы, чтобы не менять адреса переменных
	// после обновления сета суперузлов
	if(!m_MatrixRows)
		m_MatrixRows = std::make_unique<CDynaNodeBase*[]>((*ZeroLFNodes.begin())->GetContainer()->Count());

	// для доступа к переменным снаружи собираем их в стандартный вектор ссылок
	m_Vars.clear();
	m_Vars.reserve(2 * m_nSize);

	CDynaNodeBase** pNode{ m_MatrixRows.get() };
	for (const auto& ZeroSuperNode : ZeroLFNodes)
	{
		const auto& ZeroSuperNodeData{ ZeroSuperNode->ZeroLF.ZeroSupeNode };
		for (const auto& ZeroNode : ZeroSuperNodeData->LFMatrix)
		{
			*pNode = ZeroNode;
			auto& node{ ZeroNode->ZeroLF };
			m_Vars.push_back(node.vRe);
			m_Vars.push_back(node.vIm);
			pNode++;
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
