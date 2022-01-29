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


void CDynaNodeMeasure::BuildEquations(CDynaModel* pDynaModel)
{
	pDynaModel->SetElement(Pload, Pload, 1.0);
	pDynaModel->SetElement(Qload, Qload, 1.0);
}


void CDynaNodeMeasure::BuildRightHand(CDynaModel* pDynaModel)
{
	pDynaModel->SetFunction(Pload, 0.0);
	pDynaModel->SetFunction(Qload, 0.0);
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

void CDynaNodeZeroLoadFlow::BuildEquations(CDynaModel* pDynaModel)
{
	CDynaNodeBase **ppNode{ m_MatrixRows.get() },  **ppNodeEnd{ m_MatrixRows.get() + m_nSize };
	const double Vmin{ pDynaModel->GetLRCToShuntVmin() };

	while (ppNode < ppNodeEnd)
	{
		const auto& pNode{ *ppNode };
		const auto& ZeroNode{ pNode->ZeroLF };
		const auto& vRe{ ZeroNode.vRe }, vIm{ ZeroNode.vIm };
		const auto& pSuperNode{ pNode->GetSuperNode() };

		// вариант с напряжением из модуля суперузла
		const double Vsq{ pSuperNode->V };
		const double V2{ Vsq * Vsq };
		// вариант с напряжением из составляющих суперузла
		//const double V2{ pSuperNode->Vre * pSuperNode->Vre + pSuperNode->Vim * pSuperNode->Vim };
		//const double Vsq{ std::sqrt(V2) };

		double dIredVre(1.0), dIredVim(0.0), dIimdVre(0.0), dIimdVim(1.0);

		// добавляем токи собственной проводимости и токи ветвей
		dIredVre = -pNode->Yii.real();
		dIredVim = pNode->Yii.imag();
		dIimdVre = -pNode->Yii.imag();
		dIimdVim = -pNode->Yii.real();

		

		pNode->GetPnrQnr();

		double dIredV{ 0.0 }, dIimdV{ 0.0 };

		if (!pNode->m_bLowVoltage)
		{
			double Pk{ pNode->Pnr - pNode->Pgr }, Qk{ pNode->Qnr - pNode->Qgr };

			Pk /= V2;
			Qk /= V2;

			dIredVre += Pk;
			dIredVim += Qk;
			dIimdVre -= Qk;
			dIimdVim += Pk;

			pNode->dLRCPn -= pNode->dLRCPg;	
			pNode->dLRCQn -= pNode->dLRCQg;
			pNode->dLRCPn /= Vsq;
			pNode->dLRCQn /= Vsq;

			const double dPdV{ (pNode->dLRCPn - 2.0 * Pk) / Vsq };
			const double dQdV{ (pNode->dLRCQn - 2.0 * Qk) / Vsq };

			dIredV = dPdV * pSuperNode->Vre + dQdV * pSuperNode->Vim;
			dIimdV = dPdV * pSuperNode->Vim - dQdV * pSuperNode->Vre;

			dIredVre += Pk;
			dIredVim += Qk;
			dIimdVre += Pk;
			dIimdVim -= Qk;
		}

		pDynaModel->SetElement(vRe, pSuperNode->V, dIredV);
		pDynaModel->SetElement(vIm, pSuperNode->V, dIimdV);

		if ((0.5 * Vmin - pNode->dLRCVicinity) > Vsq / pNode->V0)
		{
			_ASSERTE(pNode->m_pLRC);
			dIredVre += pNode->dLRCShuntPartP;
			dIredVim += pNode->dLRCShuntPartQ;
			dIimdVre += -pNode->dLRCShuntPartQ;
			dIimdVim += pNode->dLRCShuntPartP;
			pNode->dLRCPg = pNode->dLRCQg = pNode->Pgr = pNode->Qgr = 0.0;
			pNode->dLRCPn = pNode->dLRCQn = pNode->Pnr = pNode->Qnr = 0.0;
		}

		// токи от генераторов
		const CLinkPtrCount* const pGenLink{ pNode->GetLink(1) };
		CDevice** ppGen{ nullptr };
		double dGenMatrixCoe{ pNode->m_bInMetallicSC ? 0.0 : -1.0 };
		while (pGenLink->InMatrix(ppGen))
		{
			const auto& pGen{ static_cast<CDynaPowerInjector*>(*ppGen) };
			pDynaModel->SetElement(vRe, pGen->Ire, dGenMatrixCoe);
			pDynaModel->SetElement(vIm, pGen->Iim, dGenMatrixCoe);
		}
		
		// потоки от ветвей между суперузлами
		for (const VirtualBranch* vb = ZeroNode.pVirtualBranchesBegin; vb < ZeroNode.pVirtualBranchesEnd; vb++)
		{
			//Re -= vb->Y.real() * vb->pNode->Vre - vb->Y.imag() * vb->pNode->Vim;
			//Im -= vb->Y.imag() * vb->pNode->Vre + vb->Y.real() * vb->pNode->Vim;
			const auto& OppNode{ vb->pNode };
			pDynaModel->SetElement(vRe, OppNode->Vre, -vb->Y.real());
			pDynaModel->SetElement(vRe, OppNode->Vim, vb->Y.imag());
			pDynaModel->SetElement(vIm, OppNode->Vre, -vb->Y.imag());
			pDynaModel->SetElement(vIm, OppNode->Vim, -vb->Y.real());
		}

		// потоки от ветвей внутри суперузла
		for (const VirtualBranch* vb = ZeroNode.pVirtualZeroBranchesBegin; vb < ZeroNode.pVirtualZeroBranchesEnd; vb++)
		{
			//Re -= vb->Y.real() * vb->pNode->ZeroLF.vRe;
			//Im -= vb->Y.real() * vb->pNode->ZeroLF.vIm;

			pDynaModel->SetElement(vRe, vb->pNode->ZeroLF.vRe, -vb->Y.real());
			pDynaModel->SetElement(vIm, vb->pNode->ZeroLF.vIm, -vb->Y.real());
		}

		pDynaModel->SetElement(vRe, pSuperNode->Vre, dIredVre);
		pDynaModel->SetElement(vRe, pSuperNode->Vim, dIredVim);
		pDynaModel->SetElement(vIm, pSuperNode->Vre, dIimdVre);
		pDynaModel->SetElement(vIm, pSuperNode->Vim, dIimdVim);
		// собственная проводимость от индикаторов
		pDynaModel->SetElement(vRe, ZeroNode.vRe, ZeroNode.Yii);
		pDynaModel->SetElement(vIm, ZeroNode.vIm, ZeroNode.Yii);

		ppNode++;
	}
}

eDEVICEFUNCTIONSTATUS CDynaNodeZeroLoadFlow::Init(CDynaModel* pDynaModel)
{
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CDynaNodeZeroLoadFlow::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	for (auto&& node : m_ZeroSuperNodes)
		node->SuperNodeLoadFlowYU(pDynaModel);
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

void CDynaNodeZeroLoadFlow::BuildRightHand(CDynaModel* pDynaModel)
{
	CDynaNodeBase** ppNode{ m_MatrixRows.get() }, ** ppNodeEnd{ m_MatrixRows.get() + m_nSize };

	while (ppNode < ppNodeEnd)
	{

		const auto& pNode{ *ppNode };
		auto& ZeroNode{ pNode->ZeroLF };
		double Vsq{ 0.0 };
		// получаем ток узла от настоящего шунта, инъекции и тока
		// генератора УР
		cplx Is{ pNode->GetSelfImbInotSuper(Vsq) };
		//double Re{ Is.real() }, Im{ Is.imag() };

		// добавляем инъекцию тока от базисного узла
		Is -= ZeroNode.SlackInjection;
		//Re -= ZeroNode.SlackInjection;


		// добавляем токи от генераторов
		const CLinkPtrCount* const pGenLink{ pNode->GetLink(1) };
		CDevice** ppGen{ nullptr };
		while (pGenLink->InMatrix(ppGen))
		{
			const auto& pGen{ static_cast<CDynaPowerInjector*>(*ppGen) };
			Is -= cplx(pGen->Ire, pGen->Iim);
			//Re -= pGen->Ire;
			//Im -= pGen->Iim;
		}

		// перетоки по настоящим связям от внешних суперузлов
		for (const VirtualBranch* vb = ZeroNode.pVirtualBranchesBegin; vb < ZeroNode.pVirtualBranchesEnd; vb++)
		{
			Is -= vb->Y * cplx(vb->pNode->Vre, vb->pNode->Vim);
#ifdef USE_FMA
			//Re = std::fma(-vb->Y.real(), vb->pNode->Vre, std::fma( vb->Y.imag(), vb->pNode->Vim, Re));
			//Im = std::fma(-vb->Y.imag(), vb->pNode->Vre, std::fma(-vb->Y.real(), vb->pNode->Vim, Im));
#else
			//Re -= vb->Y.real() * vb->pNode->Vre - vb->Y.imag() * vb->pNode->Vim;
			//Im -= vb->Y.imag() * vb->pNode->Vre + vb->Y.real() * vb->pNode->Vim;
#endif
		}

		// инъекция от "шунта" индикатора
		Is += pNode->ZeroLF.Yii * cplx(pNode->ZeroLF.vRe, pNode->ZeroLF.vIm);
#ifdef USE_FMA
		Re = std::fma(pNode->ZeroLF.vRe, pNode->ZeroLF.Yii, Re);
		Im = std::fma(pNode->ZeroLF.vIm, pNode->ZeroLF.Yii, Im);
#else
		//Re += pNode->ZeroLF.vRe * pNode->ZeroLF.Yii;
		//Im += pNode->ZeroLF.vIm * pNode->ZeroLF.Yii;
#endif

		// далее добавляем "токи" от индикаторов напряжения
		for (const VirtualBranch* vb = ZeroNode.pVirtualZeroBranchesBegin; vb < ZeroNode.pVirtualZeroBranchesEnd; vb++)
		{
			Is -= vb->Y * cplx(vb->pNode->ZeroLF.vRe, vb->pNode->ZeroLF.vIm);
#ifdef USE_FMA
			Re = std::fma(-vb->Y.real(), vb->pNode->ZeroLF.vRe, Re);
			Im = std::fma(-vb->Y.real(), vb->pNode->ZeroLF.vIm, Im);
#else
			//Re -= vb->Y.real() * vb->pNode->ZeroLF.vRe;
			//Im -= vb->Y.real() * vb->pNode->ZeroLF.vIm;
#endif
		}

		//_ASSERTE(std::abs(Re - Is.real()) < DFW2_EPSILON && std::abs(Im - Is.imag()) < DFW2_EPSILON);

		pDynaModel->SetFunction(ZeroNode.vRe, Is.real());
		pDynaModel->SetFunction(ZeroNode.vIm, Is.imag());

		ppNode++;
	}
}

void CDynaNodeZeroLoadFlow::UpdateSuperNodeSet()
{
	// рассчитываем количество переменных (индикаторов напряжения)
	// необходимых для формирования системы YU
	m_nSize = 0;
	// задан сет суперузлов
	for (const auto& ZeroSuperNode : m_ZeroSuperNodes)
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
		m_MatrixRows = std::make_unique<CDynaNodeBase*[]>((*m_ZeroSuperNodes.begin())->GetContainer()->Count());

	// для доступа к переменным снаружи собираем их в стандартный вектор ссылок
	m_Vars.clear();
	m_Vars.reserve(2 * m_nSize);

	CDynaNodeBase** pNode{ m_MatrixRows.get() };
	for (const auto& ZeroSuperNode : m_ZeroSuperNodes)
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
