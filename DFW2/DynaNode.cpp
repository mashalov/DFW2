#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"

using namespace DFW2;

#define LOW_VOLTAGE 0.01
#define LOW_VOLTAGE_HYST LOW_VOLTAGE * 0.1

CDynaNodeBase::CDynaNodeBase() : CDevice()
{
}


CDynaNodeBase::~CDynaNodeBase() 
{
}

// запоминает значение модуля напряжения с предыдущей итерации
// и рассчитываем напряжение в декартовых координатах
void CDynaNodeBase::UpdateVreVim()
{
	Vold = V;
	cplx VreVim(polar(V, Delta));
	Vre = VreVim.real();
	Vim = VreVim.imag();
}

void CDynaNodeBase::UpdateVDelta()
{
	cplx VreVim(Vre, Vim);
	V = abs(VreVim);
	Delta = arg(VreVim);
}

void CDynaNodeBase::UpdateVreVimSuper()
{
	UpdateVreVim();
	CLinkPtrCount *pLink = GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice))
	{
		CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
		pSlaveNode->V = V;
		pSlaveNode->Delta = Delta;
		pSlaveNode->UpdateVreVim();
	}
}

void CDynaNodeBase::UpdateVDeltaSuper()
{
	UpdateVDelta();
	CLinkPtrCount *pLink = GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice))
	{
		CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
		pSlaveNode->Vre = Vre;
		pSlaveNode->Vim = Vim;
		pSlaveNode->UpdateVDelta();
	}
}

// Проверяет для всех узлов суперузла напряжения перехода с СХН на шунт
// если они меньше напряжения перехода минус окрестность сглаживания - возвращает true
bool CDynaNodeBase::AllLRCsInShuntPart(double Vtest, double Vmin)
{
	bool bRes = (Vmin - dLRCVicinity) > Vtest / Unom;
	CLinkPtrCount *pLink = GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice) && bRes)
	{
		CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
		bRes = (Vmin - pSlaveNode->dLRCVicinity) > Vtest / pSlaveNode->Unom;
	}
	return bRes;
}

// рассчитывает нагрузку узла с учетом СХН
// и суммирует все узлы, входящие в суперузел
void CDynaNodeBase::GetPnrQnrSuper()
{
	GetPnrQnr();
	CLinkPtrCount *pLink = GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice))
	{
		CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
		pSlaveNode->GetPnrQnr();
		Pnr += pSlaveNode->Pnr;
		Qnr += pSlaveNode->Qnr;
		Pgr += pSlaveNode->Pgr;
		Qgr += pSlaveNode->Qgr;
		dLRCPn += pSlaveNode->dLRCPn;
		dLRCQn += pSlaveNode->dLRCQn;
		dLRCPg += pSlaveNode->dLRCPg;
		dLRCQg += pSlaveNode->dLRCQg;
	}
}
// рассчитывает нагрузку узла с учетом СХН
void CDynaNodeBase::GetPnrQnr()
{
	// по умолчанию нагрузка равна заданной в УР
	Pnr = Pn;	Qnr = Qn;
	// генерация в узле также равна заданной в УР
	Pgr = Pg;	Qgr = Qg;
	double VdVnom = V / V0;

	dLRCPg = dLRCQg = dLRCPn = dLRCQn = 0.0;

	// если есть СХН нагрузки, рассчитываем
	// комплексную мощность и производные по напряжению

	_ASSERTE(m_pLRC);

	Pnr *= m_pLRC->GetPdP(VdVnom, dLRCPn, dLRCVicinity);
	Qnr *= m_pLRC->GetQdQ(VdVnom, dLRCQn, dLRCVicinity);
	dLRCPn *= Pn / V0;
	dLRCQn *= Qn / V0;

	// если есть СХН генерации (нет привязанных генераторов, но есть заданная в УР генерация)
	// рассчитываем расчетную генерацию
	if (m_pLRCGen)
	{
		Pgr *= m_pLRCGen->GetPdP(VdVnom, dLRCPg, dLRCVicinity); 
		Qgr *= m_pLRCGen->GetQdQ(VdVnom, dLRCQg, dLRCVicinity);
		dLRCPg *= Pg / V0;
		dLRCQg *= Qg / V0;
	}
}

bool CDynaNodeBase::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	double Vre2 = Vre * Vre;
	double Vim2 = Vim * Vim;
	double V2 = Vre2 + Vim2;
	double V2sq = sqrt(V2);

	GetPnrQnrSuper();

	double dIredVre(1.0), dIredVim(0.0), dIimdVre(0.0), dIimdVim(1.0);

	if (!m_bInMetallicSC)
	{
		// если не в металлическом кз, считаем производные от нагрузки и генерации, заданных в узле
		dIredVre = -YiiSuper.real();
		dIredVim = YiiSuper.imag();
		dIimdVre = -YiiSuper.imag();
		dIimdVim = -YiiSuper.real();

		// если напряжение меньше 0.5*Uном*Uсхн_min переходим на шунт
		// чтобы исключить мощность из уравнений полностью
		// выбираем точку в 0.5 ниже чем Uсхн_min чтобы использовать вблизи
		// Uсхн_min стандартное cглаживание СХН

		if (AllLRCsInShuntPart(V2sq, pDynaModel->GetLRCToShuntVmin()))
		{
			_ASSERTE(m_pLRC);
			dIredVre +=  dLRCShuntPartP;
			dIredVim +=  dLRCShuntPartQ;
			dIimdVre += -dLRCShuntPartQ;
			dIimdVim +=  dLRCShuntPartP;
			dLRCPg = dLRCQg = Pgr = Qgr = 0.0;
			dLRCPn = dLRCQn = Pnr = Qnr = 0.0;
		}
	}

	CLinkPtrCount *pGenLink = GetSuperLink(2);
	CDevice **ppGen(nullptr);
	
	double Pgsum = Pnr - Pgr;
	double Qgsum = Qnr - Qgr;

	double V4 = V2 * V2;
	double VreVim2 = 2.0 * Vre * Vim;
	double PgVre2 = Pgsum * Vre2;
	double PgVim2 = Pgsum * Vim2;
	double QgVre2 = Qgsum * Vre2;
	double QgVim2 = Qgsum * Vim2;

	pDynaModel->SetElement(A(V_V), A(V_V), 1.0);
	pDynaModel->SetElement(A(V_DELTA), A(V_DELTA), 1.0);
		
	if (pDynaModel->FillConstantElements())
	{
		// в этом блоке только постоянные элементы (чистые коэффициенты), которые можно не изменять,
		// если не изменились коэффициенты метода интегрирования и шаг

		// обходим генераторы и формируем производные от токов генераторов
		// если узел в металлическом КЗ производные равны нулю
		double dGenMatrixCoe = m_bInMetallicSC ? 0.0 :-1.0;
		while (pGenLink->In(ppGen))
		{
			// здесь нужно проверять находится ли генератор в матрице (другими словами включен ли он)
			// или строить суперссылку на генераторы по условию того, что они в матрице
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			pDynaModel->SetElement(A(V_RE), pGen->A(CDynaPowerInjector::V_IRE), dGenMatrixCoe);
			pDynaModel->SetElement(A(V_IM), pGen->A(CDynaPowerInjector::V_IIM), dGenMatrixCoe);
		}

		if (m_bInMetallicSC)
		{
			// если в металлическом КЗ, то производные от токов ветвей равны нулю (в строке единицы только от Vre и Vim)
			for (VirtualBranch *pV = m_VirtualBranchBegin; pV < m_VirtualBranchEnd; pV++)
			{
				// dIre /dVre
				pDynaModel->SetElement(A(V_RE), pV->pNode->A(V_RE), 0.0);
				// dIre/dVim
				pDynaModel->SetElement(A(V_RE), pV->pNode->A(V_IM), 0.0);
				// dIim/dVre
				pDynaModel->SetElement(A(V_IM), pV->pNode->A(V_RE), 0.0);
				// dIim/dVim
				pDynaModel->SetElement(A(V_IM), pV->pNode->A(V_IM), 0,0);
			}
		}
		else
		{
			// если не в металлическом КЗ, то производные от токов ветвей формируем как положено
			for (VirtualBranch *pV = m_VirtualBranchBegin; pV < m_VirtualBranchEnd; pV++)
			{
				// dIre /dVre
				pDynaModel->SetElement(A(V_RE), pV->pNode->A(V_RE), -pV->Y.real());
				// dIre/dVim
				pDynaModel->SetElement(A(V_RE), pV->pNode->A(V_IM), pV->Y.imag());
				// dIim/dVre
				pDynaModel->SetElement(A(V_IM), pV->pNode->A(V_RE), -pV->Y.imag());
				// dIim/dVim
				pDynaModel->SetElement(A(V_IM), pV->pNode->A(V_IM), -pV->Y.real());
			}
		}

		// запоминаем позиции в строках матрицы, на которых заканчиваются постоянные элементы
		pDynaModel->CountConstElementsToSkip(A(V_RE));
		pDynaModel->CountConstElementsToSkip(A(V_IM));
	}
	else
	{
		// если постоянные элементы не надо обновлять, то пропускаем их и начинаем с непостоянных
		pDynaModel->SkipConstElements(A(V_RE));
		pDynaModel->SkipConstElements(A(V_IM));
	}

	// check low voltage
	if (m_bLowVoltage)
	{
		pDynaModel->SetElement(A(V_V), A(V_RE), 0.0);
		pDynaModel->SetElement(A(V_V), A(V_IM), 0.0);
		pDynaModel->SetElement(A(V_DELTA), A(V_RE), 0.0);
		pDynaModel->SetElement(A(V_DELTA), A(V_IM), 0.0);
		pDynaModel->SetElement(A(V_RE), A(V_V), 0.0);
		pDynaModel->SetElement(A(V_IM), A(V_V), 0.0);

		_ASSERTE(fabs(PgVre2) < DFW2_EPSILON && fabs(PgVim2) < DFW2_EPSILON);
		_ASSERTE(fabs(QgVre2) < DFW2_EPSILON && fabs(QgVim2) < DFW2_EPSILON);
	}
	else
	{
		pDynaModel->SetElement(A(V_V), A(V_RE), -Vre / V2sq);
		pDynaModel->SetElement(A(V_V), A(V_IM), -Vim / V2sq);
		pDynaModel->SetElement(A(V_DELTA), A(V_RE), Vim / V2);
		pDynaModel->SetElement(A(V_DELTA), A(V_IM), -Vre / V2);

		double d1 = (PgVre2 - PgVim2 + VreVim2 * Qgsum) / V4;
		double d2 = (QgVre2 - QgVim2 - VreVim2 * Pgsum) / V4;

		dIredVre -= d1;
		dIredVim += d2;
		dIimdVre += d2;
		dIimdVim += d1;

		// производные от СХН по напряжению считаем от модуля
		// через мощности
		V2 = V * V;
		double VreV2 = Vre / V2;
		double VimV2 = Vim / V2;
		dLRCPn -= dLRCPg;		dLRCQn -= dLRCQg;
		pDynaModel->SetElement(A(V_RE), A(V_V), dLRCPn * VreV2 + dLRCQn * VimV2);
		pDynaModel->SetElement(A(V_IM), A(V_V), dLRCPn * VimV2 - dLRCQn * VreV2);
	}

	pDynaModel->SetElement(A(V_RE), A(V_RE), dIredVre);
	pDynaModel->SetElement(A(V_RE), A(V_IM), dIredVim);
	pDynaModel->SetElement(A(V_IM), A(V_RE), dIimdVre);
	pDynaModel->SetElement(A(V_IM), A(V_IM), dIimdVim);

	return true;
}


bool CDynaNodeBase::BuildRightHand(CDynaModel *pDynaModel)
{
	GetPnrQnrSuper();

	double Ire(0.0), Iim(0.0);
	double dV(0.0), dDelta(0.0);

	if (!m_bInMetallicSC)
	{
		// если не в металлическом КЗ, обрабатываем нагрузку и генерацию,
		// заданные в узле

		double V2 = Vre * Vre + Vim * Vim;
		double V2sq = sqrt(V2);

		// если напряжение меньше 0.5*Uном*Uсхн_min переходим на шунт
		// чтобы исключить мощность из уравнений полностью
		// выбираем точку в 0.5 ниже чем Uсхн_min чтобы использовать вблизи
		// Uсхн_min стандартное cглаживание СХН

		if (AllLRCsInShuntPart(V2sq, pDynaModel->GetLRCToShuntVmin()))
		{
			Ire -= -dLRCShuntPartP * Vre - dLRCShuntPartQ * Vim;
			Iim -=  dLRCShuntPartQ * Vre - dLRCShuntPartP * Vim;
			Pgr = Qgr = 0.0;
			Pnr = Qnr = 0.0;
			// нагрузки и генерации в мощности больше нет, они перенесены в ток
		}

		// обходим генераторы
		CLinkPtrCount *pBranchLink = GetSuperLink(1);
		CLinkPtrCount *pGenLink = GetSuperLink(2);
		CDevice **ppBranch(nullptr), **ppGen(nullptr);
		ResetVisited();
		while (pGenLink->In(ppGen))
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*ppGen);
			Ire -= pGen->Ire;
			Iim -= pGen->Iim;
		}

		// добавляем токи собственной проводимости и токи ветвей
		Ire -= YiiSuper.real() * Vre - YiiSuper.imag() * Vim;
		Iim -= YiiSuper.imag() * Vre + YiiSuper.real() * Vim;
		for (VirtualBranch *pV = m_VirtualBranchBegin; pV < m_VirtualBranchEnd; pV++)
		{
			Ire -= pV->Y.real() * pV->pNode->Vre - pV->Y.imag() * pV->pNode->Vim;
			Iim -= pV->Y.imag() * pV->pNode->Vre + pV->Y.real() * pV->pNode->Vim;
		}


		if (!m_bLowVoltage)
		{
			// добавляем токи от нагрузки (если напряжение не очень низкое)
			// считаем невязки модуля и угла, иначе оставляем нулями
			double Pk = Pnr - Pgr;
			double Qk = Qnr - Qgr;
			Ire += (Pk * Vre + Qk * Vim) / V2;
			Iim += (Pk * Vim - Qk * Vre) / V2;
			dV = V - V2sq;
			dDelta = Delta - atan2(Vim, Vre);

		}
#ifdef _DEBUG
		else
			_ASSERTE(fabs(Pnr - Pgr) < DFW2_EPSILON && fabs(Qnr - Qgr) < DFW2_EPSILON);
#endif
	}

	pDynaModel->SetFunction(A(V_V), dV);
	pDynaModel->SetFunction(A(V_DELTA), dDelta);
	pDynaModel->SetFunction(A(V_RE), Ire);
	pDynaModel->SetFunction(A(V_IM), Iim);

	return true;
}

void CDynaNodeBase::NewtonUpdateEquation(CDynaModel* pDynaModel)
{
	dLRCVicinity = 5.0 * fabs(Vold - V) / Unom;
	Vold = V;
}

eDEVICEFUNCTIONSTATUS CDynaNodeBase::Init(CDynaModel* pDynaModel)
{
	UpdateVreVim();
	m_bLowVoltage = V < (LOW_VOLTAGE - LOW_VOLTAGE_HYST);
	V0 = (V > 0) ? V : Unom;

	if (GetLink(1)->m_nCount > 0)					// если к узлу подключены генераторы, то СХН генераторов не нужна и мощности генерации 0
		Pg = Qg = Pgr = Qgr = 0.0;
	else if (Equal(Pg, 0.0) && Equal(Qg, 0.0))		// если генераторы не подключены, и мощность генерации равна нулю - СХН генераторов не нужна
		Pgr = Qgr = 0.0;
	else
		m_pLRCGen = pDynaModel->GetLRCGen();		// если есть генерация но нет генераторов - нужна СХН генераторов

	// если в узле нет СХН для динамики, подставляем СХН по умолчанию
	if (!m_pLRC)
		m_pLRC = pDynaModel->GetLRCDynamicDefault();

	return DFS_OK;
}

// в узлах используется расширенная функция
// прогноза. После прогноза рассчитывается
// напряжение в декартовых координатах
// и сбрасываются значения по умолчанию перед
// итерационным процессом решения сети
void CDynaNode::Predict()
{
	dLRCVicinity = 0.0;
	double newDelta = atan2(sin(Delta), cos(Delta));
	if (fabs(newDelta - Delta) > DFW2_EPSILON)
	{
		RightVector *pRvDelta = GetModel()->GetRightVector(A(V_DELTA));
		RightVector *pRvLag = GetModel()->GetRightVector(A(V_LAG));
		double dDL = Delta - Lag;
		Delta = newDelta;
		Lag = Delta - dDL;
		pRvDelta->Nordsiek[0] = Delta;
		pRvLag->Nordsiek[0] = Lag;
	}

}

CDynaNode::CDynaNode() : CDynaNodeBase()
{

}

bool CDynaNode::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double T = pDynaModel->GetFreqTimeConstant();
	pDynaModel->SetDerivative(A(V_LAG), (Delta - Lag) / T);
	return true;
}


bool CDynaNode::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = CDynaNodeBase::BuildEquations(pDynaModel);

	double T = pDynaModel->GetFreqTimeConstant();
	double w0 = pDynaModel->GetOmega0();



	pDynaModel->SetElement(A(V_LAG), A(V_DELTA), -1.0 / T);
	//pDynaModel->SetElement(A(V_LAG), A(V_LAG), 1.0 + hb0 / T);
	pDynaModel->SetElement(A(V_LAG), A(V_LAG), -1.0 / T);
	
	if (!pDynaModel->IsInDiscontinuityMode())
	{
		pDynaModel->SetElement(A(V_S), A(V_DELTA), -1.0 / T / w0);
		pDynaModel->SetElement(A(V_S), A(V_LAG), 1.0 / T / w0);
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
	}
	else
	{
		pDynaModel->SetElement(A(V_S), A(V_DELTA), 0.0);
		pDynaModel->SetElement(A(V_S), A(V_LAG), 0.0);
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
	}
	return true;
}

bool CDynaNode::BuildRightHand(CDynaModel* pDynaModel)
{
	CDynaNodeBase::BuildRightHand(pDynaModel);

	double T = pDynaModel->GetFreqTimeConstant();
	double w0 = pDynaModel->GetOmega0();
	double dLag = (Delta - Lag) / T;
	double dS = S - (Delta - Lag) / T / w0;

	if (pDynaModel->IsInDiscontinuityMode()) 
		dS = 0.0;
	
	pDynaModel->SetFunctionDiff(A(V_LAG), dLag);
	pDynaModel->SetFunction(A(V_S), dS);

	//DumpIntegrationStep(2021, 2031);
	//DumpIntegrationStep(2143, 2031);
	//DumpIntegrationStep(2141, 2031);

	return true;
}

eDEVICEFUNCTIONSTATUS CDynaNode::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = CDynaNodeBase::Init(pDynaModel);
	if (CDevice::IsFunctionStatusOK(Status))
	{
		S = 0.0;
		Lag = Delta;
	}
	return Status;
}

// переменные базового узла - модуль и угол
double* CDynaNodeBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Delta,V_DELTA)
		MAP_VARIABLE(V, V_V)
		MAP_VARIABLE(Vre, V_RE)
		MAP_VARIABLE(Vim, V_IM)
	}
	return p;
}

// константы узла - проводимость шунта
double* CDynaNodeBase::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);
	switch (nVarIndex)
	{
		MAP_VARIABLE(Bshunt, C_BSH)
		MAP_VARIABLE(Gshunt, C_GSH)
	}
	return p;
}

// переменные узла - переменные базового узла + скольжение и лаг скольжения
double* CDynaNode::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = CDynaNodeBase::GetVariablePtr(nVarIndex);
	if (!p)
	{
		switch (nVarIndex)
		{
			MAP_VARIABLE(Lag, V_LAG)
			MAP_VARIABLE(S, V_S)
				/*
			MAP_VARIABLE(Sv, V_SV)
			MAP_VARIABLE(Sip, V_SIP)
			MAP_VARIABLE(Cop, V_COP)
			*/
		}
	}
	return p;
}

CDynaNodeContainer::CDynaNodeContainer(CDynaModel *pDynaModel) : 
									   CDeviceContainer(pDynaModel)
{
	// в контейнере требуем особой функции прогноза и обновления после
	// ньютоновской итерации
	m_ContainerProps.bNewtonUpdate = m_ContainerProps.bPredict = true;
}

CDynaNodeContainer::~CDynaNodeContainer()
{
	ClearSuperLinks();
}

void CDynaNodeContainer::CalcAdmittances(bool bSeidell)
{
	for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
		pNode->CalcAdmittances(bSeidell);
	}
}

void CDynaNodeBase::CalcAdmittances(bool bSeidell)
{
	Yii = -cplx(G + Gshunt, B + Bshunt);

	dLRCShuntPartP = dLRCShuntPartQ	= 0.0;
	double V02 = V0 * V0;

	if (m_pLRC)
	{
		// рассчитываем шунтовую часть СХН нагрузки в узле для низких напряжений
		dLRCShuntPartP = Pn * m_pLRC->P->a2;
		dLRCShuntPartQ = Qn * m_pLRC->Q->a2;
	}

	if (m_pLRCGen)
	{
		// рассчитываем шунтовую часть СХН генерации в узле для низких напряжений
		dLRCShuntPartP -= Pg * m_pLRCGen->P->a2;
		dLRCShuntPartP -= Qg * m_pLRCGen->Q->a2;
	}

	dLRCShuntPartP /= V02;
	dLRCShuntPartQ /= V02;

	m_bInMetallicSC = !(_finite(Yii.real()) && _finite(Yii.imag()));

	if (m_bInMetallicSC || !IsStateOn())
	{
		Vre = Vim = 0.0;
		V = Delta = 0.0;
	}
	else
	{
		CDevice **ppBranch(nullptr);
		CLinkPtrCount *pLink = GetLink(0);
		ResetVisited();
		while (pLink->In(ppBranch))
		{
			CDynaBranch *pBranch = static_cast<CDynaBranch*>(*ppBranch);
			pBranch->CalcAdmittances(bSeidell);

			// TODO Single-end trip

			switch (pBranch->m_BranchState)
			{
			case CDynaBranch::BRANCH_OFF:
				break;
			case CDynaBranch::BRANCH_ON:

				if (this == pBranch->m_pNodeIp)
					Yii -= (pBranch->Yips);
				else
					Yii -= (pBranch->Yiqs);
				break;

			case CDynaBranch::BRANCH_TRIPIP:
				break;
			case CDynaBranch::BRANCH_TRIPIQ:
				break;
			}
		}

		if (V < DFW2_EPSILON)
		{
			Vre = V = Unom;
			Vim = Delta = 0.0;
		}
	}
}

eDEVICEFUNCTIONSTATUS CDynaNode::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause)
{
	eDEVICESTATE OldState = GetState();
	eDEVICEFUNCTIONSTATUS Status = CDevice::SetState(eState, eStateCause);

	if (OldState != eState)
	{
		ProcessTopologyRequest();

		switch (eState)
		{
		case eDEVICESTATE::DS_OFF:
			V = Delta = Lag = S = /*Sip = Cop = Sv = */ 0.0;
			break;
		case eDEVICESTATE::DS_ON:
			V = Unom;
			Delta = 0;
			Lag = S = /*Sv = */0.0;
			Vre = V;
			Vim = 0;
			/*
			Sip = 0;
			Cop = 0;
			*/
			CalcAdmittances(false);
			break;
		}
	}
	return Status;
}

eDEVICEFUNCTIONSTATUS CDynaNode::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	Lag = Delta - S * pDynaModel->GetFreqTimeConstant() * pDynaModel->GetOmega0();
	SetLowVoltage(sqrt(Vre * Vre + Vim * Vim) < (LOW_VOLTAGE - LOW_VOLTAGE_HYST));
	//Delta = atan2(Vim, Vre);
	return DFS_OK;
}

CSynchroZone::CSynchroZone() : CDevice()
{
}

double* CSynchroZone::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);

	switch (nVarIndex)
	{
		MAP_VARIABLE(S, V_S)
	}
	return p;
}

bool CSynchroZone::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;
	if (m_bInfPower)
	{
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
	}
	else
	{
		pDynaModel->SetElement(A(V_S), A(V_S), 1.0);
		for (auto&& it : m_LinkedGenerators)
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(it);
			if(pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				CDynaGeneratorMotion *pGenMj = static_cast<CDynaGeneratorMotion*>(pGen);
				pDynaModel->SetElement(A(V_S), pGen->A(CDynaGeneratorMotion::V_S), -pGenMj->Mj / Mj);
			}
		}
	}
	return true;
}


bool CSynchroZone::BuildRightHand(CDynaModel* pDynaModel)
{
	double dS = S;
	if (m_bInfPower)
	{
		pDynaModel->SetFunction(A(V_S), 0.0);
	}
	else
	{
		for (DEVICEVECTORITR it = m_LinkedGenerators.begin(); it != m_LinkedGenerators.end(); it++)
		{
			CDynaPowerInjector *pGen = static_cast<CDynaPowerInjector*>(*it);
			if (pGen->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				CDynaGeneratorMotion *pGenMj = static_cast<CDynaGeneratorMotion*>(pGen);
				dS -= pGenMj->Mj * pGenMj->s / Mj;
			}
		}
		pDynaModel->SetFunction(A(V_S), dS);
	}
	return true;
}


eDEVICEFUNCTIONSTATUS CSynchroZone::Init(CDynaModel* pDynaModel)
{
	return DFS_OK;
}

bool CDynaNodeContainer::LULF()
{
	bool bRes = true;

	KLUWrapper<complex<double>> klu;
	size_t nNodeCount = m_DevInMatrix.size();
	size_t nBranchesCount = m_pDynaModel->Branches.Count();
	// оценка количества ненулевых элементов
	size_t nNzCount = nNodeCount + 2 * nBranchesCount;

	klu.SetSize(nNodeCount, nNzCount);
	double *Ax = klu.Ax();
	double *B  = klu.B();
	ptrdiff_t *Ap = klu.Ai();
	ptrdiff_t *Ai = klu.Ap();

	// вектор указателей на диагональ матрицы
	auto pDiags = make_unique<double*[]>(nNodeCount);
	double **ppDiags = pDiags.get();
	double *pB = B;

	double *pAx = Ax;
	ptrdiff_t *pAp = Ap;
	ptrdiff_t *pAi = Ai;

	FILE *fnode(nullptr), *fgen(nullptr);
	_tfopen_s(&fnode, _T("c:\\tmp\\nodes.csv"), _T("w+, ccs=UTF-8"));
	_tfopen_s(&fgen, _T("c:\\tmp\\gens.csv"), _T("w+, ccs=UTF-8"));
	_ftprintf(fnode, _T(";"));

	CalcAdmittances(false);
	for (auto&& it : m_DevInMatrix)
	{
		_ASSERTE(pAx < Ax + nNzCount * 2);
		_ASSERTE(pAi < Ai + nNzCount);
		_ASSERTE(pAp < Ap + nNodeCount);
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		pNode->dLRCVicinity = 0.0;		// зона сглаживания СХН для начала нулевая - без сглаживания
		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / EquationsCount();		  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		// если узел отключен, в матрице он все равно будет но с единичным диагональным элементом
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		if (!pNode->m_bInMetallicSC)
		{
			// стартуем с плоского
			pNode->Vre = pNode->V = pNode->Unom;
			pNode->Vim = pNode->Delta = 0.0;
			// для всех узлов, которые не отключены и не находятся в металлическом КЗ (КЗ с нулевым шунтом)
			_ftprintf(fnode, _T("%td;"), pNode->GetId());
			// Branches

			for (VirtualBranch *pV = pNode->m_VirtualBranchBegin; pV < pNode->m_VirtualBranchEnd; pV++)
			{
				*pAx = pV->Y.real();   pAx++;
				*pAx = pV->Y.imag();   pAx++;
				*pAi = pV->pNode->A(0) / EquationsCount(); pAi++;
			}
		}
	}

	nNzCount = (pAx - Ax) / 2;		// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[nNodeCount] = nNzCount;
	for (ptrdiff_t nIteration = 0; nIteration < 200; nIteration++)
	{
		m_IterationControl.Reset();
		ppDiags = pDiags.get();
		pB = B;

		_ftprintf(fnode, _T("\n%td;"), nIteration);
		_ftprintf(fgen, _T("\n%td;"), nIteration);
		// проходим по узлам вне зависимости от их состояния, параллельно идем по диагонали матрицы
		for (auto&& it : m_DevInMatrix)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
			_ASSERTE(pB < B + nNodeCount * 2);

			if (!pNode->m_bInMetallicSC)
			{
				// для всех узлов которые включены и вне металлического КЗ

				cplx Unode(pNode->Vre, pNode->Vim);

				cplx I = 0.0;					// обнуляем узел
				cplx Y = pNode->YiiSuper;		// диагональ матрицы по умолчанию собственная проводимость узла

				pNode->Vold = pNode->V;			// запоминаем напряжение до итерации для анализа сходимости

				pNode->GetPnrQnrSuper();

				// Generators

				CDevice **ppDeivce(nullptr);
				CLinkPtrCount *pLink = pNode->GetSuperLink(2);
				pNode->ResetVisited();
				// проходим по генераторам
				while (pLink->In(ppDeivce))
				{
					CDynaVoltageSource *pVsource = static_cast<CDynaVoltageSource*>(*ppDeivce);
					// если в узле есть хотя бы один генератор, то обнуляем мощность генерации узла
					// если в узле нет генераторов но есть мощность генерации - то она будет учитываться
					// задающим током
					// если генераторм выключен то просто пропускаем, его мощность и ток будут равны нулю
					if (!pVsource->IsStateOn())
						continue;

					pVsource->CalculatePower();

					if (0)
					{
						//Альтернативный вариант с расчетом подключения к сети через мощность. Что-то нестабильный
						cplx Sg(pVsource->P, pVsource->Q);
						cplx E = pVsource->GetEMF();
						cplx Yg = conj(Sg / Unode) / (E - Unode);
						I -= E * Yg;
						Y -= Yg;
					}
					else
					{
						CDynaGeneratorInfBus *pGen = static_cast<CDynaGeneratorInfBus*>(pVsource);
						// в диагональ матрицы добавляем проводимость генератора
						Y -= 1.0 / cplx(0.0, pGen->Xgen());
						I -= pGen->Igen(nIteration);
					}


					_CheckNumber(I.real());
					_CheckNumber(I.imag());

					_ftprintf(fgen, _T("%g;"), pVsource->P);
				}

				// рассчитываем задающий ток узла от нагрузки
				// можно посчитать ток, а можно посчитать добавку в диагональ
				//I += conj(cplx(Pnr - pNode->Pg, Qnr - pNode->Qg) / pNode->VreVim);
				if (pNode->V > 0.0)
					Y += conj(cplx(pNode->Pgr - pNode->Pnr, pNode->Qgr - pNode->Qnr) / pNode->V / pNode->V);
				//Y -= conj(cplx(Pnr, Qnr) / pNode->V / pNode->V);

				_CheckNumber(I.real());
				_CheckNumber(I.imag());
				_CheckNumber(Y.real());
				_CheckNumber(Y.imag());

				// и заполняем вектор комплексных токов
				*pB = I.real(); pB++;
				*pB = I.imag(); pB++;
				// диагональ матрицы формируем по Y узла
				**ppDiags = Y.real();
				*(*ppDiags + 1) = Y.imag();
				_ftprintf(fnode, _T("%g;"), pNode->V / pNode->V0);
			}
			else
			{
				// если узел в металлическом КЗ, задающий ток для него равен нулю
				*pB = 0.0; pB++;
				*pB = 0.0; pB++;
				_ASSERTE(pNode->Vre <= 0.0 && pNode->Vim <= 0.0 && pNode->V <= 0.0);
			}
			ppDiags++;
		}
		ptrdiff_t refactorOK = 1;

		klu.FactorSolve();

		// KLU может делать повторную факторизацию матрицы с начальным пивотингом
		// это быстро, но при изменении пивотов может вызывать численную неустойчивость.
		// У нас есть два варианта факторизации/рефакторизации на итерации LULF
		double *pB = B;

		for (auto&& it: m_DevInMatrix)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
			// напряжение после решения системы в векторе задающий токов
			pNode->Vre = *pB;		pB++;
			pNode->Vim = *pB;		pB++;

			// считаем напряжение узла в полярных координатах
			pNode->UpdateVDelta();
			// рассчитываем зону сглаживания СХН (также как для Ньютона)
			/*if (pNode->m_pLRC)
				pNode->dLRCVicinity = 30.0 * fabs(pNode->Vold - pNode->V) / pNode->Unom;
			*/
			// считаем изменение напряжения узла между итерациями и находим
			// самый изменяющийся узел
			if (!pNode->m_bInMetallicSC)
				m_IterationControl.m_MaxV.UpdateMaxAbs(pNode, CDevice::ZeroDivGuard(pNode->V - pNode->Vold, pNode->Vold));
		}

		for (auto && it : m_DevInMatrix)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
			if (pNode->m_pSuperNodeParent)
			{
				pNode->Vre = pNode->m_pSuperNodeParent->Vre;
				pNode->Vim = pNode->m_pSuperNodeParent->Vim;
				pNode->UpdateVDelta();
			}
		}

		DumpIterationControl();

		if (fabs(m_IterationControl.m_MaxV.GetDiff()) < m_pDynaModel->GetRtolLULF())
		{
			Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(CDFW2Messages::m_cszLULFConverged, m_IterationControl.m_MaxV.GetDiff(), nIteration));
			break;
		}
	}
	fclose(fnode);
	fclose(fgen);
	return bRes;
}

void CDynaNodeContainer::SwitchLRCs(bool bSwitchToDynamicLRC)
{
	if (bSwitchToDynamicLRC != m_bDynamicLRC)
	{
		m_bDynamicLRC = bSwitchToDynamicLRC;
		for (DEVICEVECTORITR it = m_DevVec.begin(); it != m_DevVec.end(); it++)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(*it);
			std::swap(pNode->m_pLRCLF, pNode->m_pLRC);
			std::swap(pNode->Pn, pNode->Pnr);
			std::swap(pNode->Qn, pNode->Qnr);
			if (!m_bDynamicLRC)
				pNode->V0 = pNode->Unom;
			else
				pNode->V0 = pNode->V;
		}
	}
}

bool CDynaNodeContainer::Seidell()
{
	return LULF();
}

ExternalVariable CDynaNodeBase::GetExternalVariable(const _TCHAR* cszVarName)
{
	if (!_tcscmp(cszVarName, CDynaNode::m_cszSz))
	{
		ExternalVariable ExtVar = { nullptr, -1 };

		if (m_pSyncZone)
			ExtVar = m_pSyncZone->GetExternalVariable(CDynaNode::m_cszS);
		return ExtVar;
	}
	else
		return CDevice::GetExternalVariable(cszVarName);
}


void CDynaNodeBase::StoreStates()
{
	m_bSavedLowVoltage = m_bLowVoltage;
}

void CDynaNodeBase::RestoreStates()
{
	m_bLowVoltage = m_bSavedLowVoltage;
}

void CDynaNodeBase::SetLowVoltage(bool bLowVoltage)
{
	if (m_bLowVoltage)
	{
		if (!bLowVoltage)
		{
			m_bLowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Напряжение %g в узле %s выше порогового"), V, GetVerbalName()));
		}
	}
	else
	{
		if (bLowVoltage)
		{
			m_bLowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(_T("Напряжение %g в узле %s ниже порогового"), V, GetVerbalName()));
		}
	}
}

double CDynaNodeBase::FindVoltageZC(CDynaModel *pDynaModel, RightVector *pRvre, RightVector *pRvim, double Hyst, bool bCheckForLow)
{
	double rH = 1.0;

	double Border = LOW_VOLTAGE + (bCheckForLow ? -Hyst : Hyst );

	double  h = pDynaModel->GetH();
	const double *lm = pDynaModel->Methodl[DET_ALGEBRAIC * 2 + pDynaModel->GetOrder() - 1];
#ifdef USE_FMA
	double Vre1 = CDynaModel::FMA(pRvre->Error, lm[0], pRvre->Nordsiek[0]);
	double Vim1 = CDynaModel::FMA(pRvim->Error, lm[0], pRvim->Nordsiek[0]);
#else
	double Vre1 = pRvre->Nordsiek[0] + pRvre->Error * lm[0];
	double Vim1 = pRvim->Nordsiek[0] + pRvim->Error * lm[0];
#endif

	double Vcheck = sqrt(Vre1 * Vre1 + Vim1 * Vim1);
	double dVre1 = (pRvre->Nordsiek[1] + pRvre->Error * lm[1]) / h;
	double dVim1 = (pRvim->Nordsiek[1] + pRvim->Error * lm[1]) / h;
	double dVre2 = (pRvre->Nordsiek[2] + pRvre->Error * lm[2]) / h / h;
	double dVim2 = (pRvim->Nordsiek[2] + pRvim->Error * lm[2]) / h / h;
	double derr = fabs(pRvre->GetWeightedError(fabs(Vcheck - Border), Border));

	if (derr < pDynaModel->GetZeroCrossingTolerance())
	{
		SetLowVoltage(bCheckForLow);
		pDynaModel->DiscontinuityRequest();
	}
	else
	{
		if (pDynaModel->GetOrder() == 1)
		{
			double a = dVre1 * dVre1 + dVim1 * dVim1;
			double b = 2.0 * (Vre1 * dVre1 + Vim1 * dVim1);
			double c = Vre1 * Vre1 + Vim1 * Vim1 - Border * Border;
			rH = CDynaPrimitive::GetZCStepRatio(pDynaModel, a, b, c);
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetLowVoltage(bCheckForLow);
				pDynaModel->DiscontinuityRequest();
			}
		}
		else
		{
			double a = dVre2 * dVre2 + dVim2 * dVim2;														// (xs2^2 + ys2^2)*t^4 
			double b = 2.0 * (dVre1 * dVre2 + dVim1 * dVim1);												// (2*xs1*xs2 + 2*ys1*ys2)*t^3 
			double c = (dVre1 * dVre1 + dVim1 * dVim1 + 2.0 * Vre1 * dVre2 + 2.0 * Vim1 * dVim2);			// (xs1^2 + ys1^2 + 2*x1*xs2 + 2*y1*ys2)*t^2 
			double d = (2.0 * Vre1 * dVre1 + 2.0 * Vim1 * dVim1);											// (2*x1*xs1 + 2*y1*ys1)*t 
			double e = Vre1 * Vre1 + Vim1 * Vim1 - Border * Border;											// x1^2 + y1^2 - e
			double t = -0.5 * h;

			for (int i = 0; i < 5; i++)
			{
				double dt = (a*t*t*t*t + b * t*t*t + c * t*t + d * t + e) / (4.0*a*t*t*t + 3.0*b*t*t + 2.0*c*t + d);
				t = t - dt;
				if (t > 0.0 || t < -h)
				{
					t = FLT_MAX;
					break;
				}
				else if (fabs(dt) < DFW2_EPSILON * 10.0)
					break;
			}
			rH = (h + t) / h;
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetLowVoltage(bCheckForLow);
				pDynaModel->DiscontinuityRequest();
			}
		}
	}

	return rH;
}
// узел не должен быть в матрице, если он отключен или входит в суперузел
bool CDynaNodeBase::InMatrix()
{
	if (m_pSuperNodeParent || !IsStateOn())
		return false;
	else
		return true;
}

double CDynaNodeBase::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	double Hyst = LOW_VOLTAGE_HYST;
	RightVector *pRvre = pDynaModel->GetRightVector(A(V_RE));
	RightVector *pRvim = pDynaModel->GetRightVector(A(V_IM));
	const double *lm = pDynaModel->Methodl[DET_ALGEBRAIC * 2 + pDynaModel->GetOrder() - 1];

#ifdef USE_FMA
	double Vre1 = CDynaModel::FMA(pRvre->Error, lm[0], pRvre->Nordsiek[0]);
	double Vim1 = CDynaModel::FMA(pRvim->Error, lm[0], pRvim->Nordsiek[0]);
#else
	double Vre1 = pRvre->Nordsiek[0] + pRvre->Error * lm[0];
	double Vim1 = pRvim->Nordsiek[0] + pRvim->Error * lm[0];
#endif

	double Vcheck = sqrt(Vre1 * Vre1 + Vim1 * Vim1);

	if (m_bLowVoltage)
	{
		double Border = LOW_VOLTAGE + Hyst;
		if (Vcheck > Border)
		{
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, false);
		}
	}
	else
	{
		double Border = LOW_VOLTAGE - Hyst;
		if (Vcheck < Border)
		{
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, true);
		}
	}

	return rH;
}


void CDynaNodeBase::ProcessTopologyRequest()
{
	if (m_pContainer)
		static_cast<CDynaNodeContainer*>(m_pContainer)->ProcessTopologyRequest();
}

const CDeviceContainerProperties CDynaNodeBase::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.eDeviceType = DEVTYPE_NODE;
	props.SetType(DEVTYPE_NODE);

	// к узлу могут быть прилинкованы ветви много к одному
	props.AddLinkFrom(DEVTYPE_BRANCH, DLM_MULTI, DPD_SLAVE);
	// и инжекторы много к одному. Для них узел выступает ведущим
	props.AddLinkFrom(DEVTYPE_POWER_INJECTOR, DLM_MULTI, DPD_SLAVE);

	props.nEquationsCount = CDynaNodeBase::VARS::V_LAST;
	props.bPredict = props.bNewtonUpdate = true;

	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszDelta, CVarIndex(V_DELTA,VARUNIT_RADIANS)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszV, CVarIndex(V_V, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszVre, CVarIndex(V_RE, VARUNIT_KVOLTS)));
	props.m_VarMap.insert(make_pair(CDynaNodeBase::m_cszVim, CVarIndex(V_IM, VARUNIT_KVOLTS)));
	return props;
}

const CDeviceContainerProperties CDynaNode::DeviceProperties()
{
	CDeviceContainerProperties props = CDynaNodeBase::DeviceProperties();
	props.m_strClassName = CDeviceContainerProperties::m_cszNameNode;
	props.nEquationsCount = CDynaNode::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(CDynaNode::m_cszS, CVarIndex(V_S, VARUNIT_PU)));

	/*
	props.m_VarMap.insert(make_pair(_T("Sip"), CVarIndex(V_SIP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Cop"), CVarIndex(V_COP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair(_T("Sv"), CVarIndex(V_SV, VARUNIT_PU)));
	*/


	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasNode);
	props.m_ConstVarMap.insert(make_pair(CDynaNode::m_cszGsh, CConstVarIndex(CDynaNode::C_GSH, eDVT_INTERNALCONST)));
	props.m_ConstVarMap.insert(make_pair(CDynaNode::m_cszBsh, CConstVarIndex(CDynaNode::C_BSH, eDVT_INTERNALCONST)));
	return props;
}

const CDeviceContainerProperties CSynchroZone::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.bVolatile = true;
	props.eDeviceType = DEVTYPE_SYNCZONE;
	props.nEquationsCount = CSynchroZone::VARS::V_LAST;
	props.m_VarMap.insert(make_pair(CDynaNode::m_cszS, CVarIndex(0,VARUNIT_PU)));
	return props;
}


const _TCHAR *CDynaNodeBase::m_cszV = _T("V");
const _TCHAR *CDynaNodeBase::m_cszDelta = _T("Delta");
const _TCHAR *CDynaNodeBase::m_cszVre = _T("Vre");
const _TCHAR *CDynaNodeBase::m_cszVim = _T("Vim");
const _TCHAR *CDynaNodeBase::m_cszGsh = _T("gsh");
const _TCHAR *CDynaNodeBase::m_cszBsh = _T("bsh");

const _TCHAR *CDynaNode::m_cszS = _T("S");
const _TCHAR *CDynaNode::m_cszSz = _T("Sz");
