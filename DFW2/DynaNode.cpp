#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"
#include "GraphCycle.h"
#include "DynaGeneratorMotion.h"
#include "BranchMeasures.h"

using namespace DFW2;

#define LOW_VOLTAGE 0.1
#define LOW_VOLTAGE_HYST LOW_VOLTAGE * 0.1

CDynaNodeBase::CDynaNodeBase() : CDevice()
{
}

// запоминает значение модуля напряжения с предыдущей итерации
// и рассчитываем напряжение в декартовых координатах
void CDynaNodeBase::UpdateVreVim()
{
	Vold = V;
	const cplx VreVim(std::polar((double)V, (double)Delta));
	FromComplex(Vre, Vim, VreVim);
}

void CDynaNodeBase::UpdateVDelta()
{
	const cplx VreVim(Vre, Vim);
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

// возвращает ток узла от нагрузки/генерации/шунтов
cplx CDynaNodeBase::GetSelfImbInotSuper(const double Vmin, double& Vsq)
{
	// рассчитываем модуль напряжения по составляющим,
	// так как модуль из уравнения может неточно соответствовать
	// сумме составляющих пока Ньютон не сошелся
	double V2{ Vre * Vre + Vim * Vim };
	Vsq = std::sqrt(V2);
	// рассчитываем нагрузку/генерацию по СХН по заданному модулю
	GetPnrQnr(Vsq);
	double Ire{ Iconst.real() }, Iim{ Iconst.imag() };

	if (!m_bInMetallicSC)
	{
		// если не в металлическом КЗ, обрабатываем нагрузку и генерацию, заданные в узле
 	    // если напряжение меньше 0.5*Uном*Uсхн_min переходим на шунт
		// чтобы исключить мощность из уравнений полностью
		// выбираем точку в 0.5 ниже чем Uсхн_min чтобы использовать вблизи
		// Uсхн_min стандартное cглаживание СХН

		if ((0.5 * Vmin - dLRCVicinity) > Vsq / V0)
		{
			Ire += dLRCShuntPartP * Vre + dLRCShuntPartQ * Vim;
			Iim -= dLRCShuntPartQ * Vre - dLRCShuntPartP * Vim;

#ifdef _DEBUG
			// проверка
			cplx S{ std::conj(cplx(Ire, Iim) - Iconst) * cplx(Vre, Vim) };
			cplx dS{ S - cplx(Pnr - Pgr,Qnr - Qgr) };
			if (std::abs(dS.real()) > 0.1 || std::abs(dS.imag()) > 0.1)
			{
				_ASSERTE(0);
				//GetPnrQnrSuper();
			}
#endif
			Pgr = Qgr = Pnr = Qnr = 0.0;
			// нагрузки и генерации в мощности больше нет, они перенесены в ток
		}
	}

	// добавляем токи собственной проводимости и токи ветвей
	Ire -= Yii.real() * Vre - Yii.imag() * Vim;
	Iim -= Yii.imag() * Vre + Yii.real() * Vim;

	if (!GetSuperNode()->m_bLowVoltage)
	{
		// добавляем токи от нагрузки (если напряжение не очень низкое)
		const double Pk{ Pnr - Pgr }, Qk{ Qnr - Qgr };
		Ire += (Pk * Vre + Qk * Vim) / V2;
		Iim += (Pk * Vim - Qk * Vre) / V2;
	}
#ifdef _DEBUG
	else
		_ASSERTE(std::abs(Pnr - Pgr) < DFW2_EPSILON && std::abs(Qnr - Qgr) < DFW2_EPSILON);
#endif
	return cplx(Ire, Iim);
}

cplx CDynaNodeBase::GetSelfImbISuper(const double Vmin, double& Vsq)
{
	// рассчитываем модуль напряжения по составляющим,
	// так как модуль из уравнения может неточно соответствовать
	// сумме составляющих пока Ньютон не сошелся
	double V2{ Vre * Vre + Vim * Vim };
	Vsq = std::sqrt(V2);
	// рассчитываем нагрузку/генерацию по СХН по заданному модулю
	GetPnrQnrSuper(Vsq);
	double Ire{ IconstSuper.real() }, Iim{ IconstSuper.imag() };

	if (!m_bInMetallicSC)
	{
		// если не в металлическом КЗ, обрабатываем нагрузку и генерацию, заданные в узле
		// если напряжение меньше 0.5*Uном*Uсхн_min переходим на шунт
		// чтобы исключить мощность из уравнений полностью
		// выбираем точку в 0.5 ниже чем Uсхн_min чтобы использовать вблизи
		// Uсхн_min стандартное cглаживание СХН

		if (AllLRCsInShuntPart(Vsq, Vmin))
		{
			Ire -= -dLRCShuntPartPSuper * Vre - dLRCShuntPartQSuper * Vim;
			Iim -=  dLRCShuntPartQSuper * Vre - dLRCShuntPartPSuper * Vim;

#ifdef _DEBUG
			// проверка
			cplx S{ std::conj(cplx(Ire, Iim) - IconstSuper) * cplx(Vre, Vim) };
			cplx dS{ S - cplx(Pnr - Pgr,Qnr - Qgr) };
			if (std::abs(dS.real()) > 0.1 || std::abs(dS.imag()) > 0.1)
			{
				_ASSERTE(0);
				double Vx = std::sqrt(Vre * Vre + Vim * Vim);
				GetPnrQnrSuper();
			}
#endif
			Pgr = Qgr = Pnr = Qnr = 0.0;
			// нагрузки и генерации в мощности больше нет, они перенесены в ток
		}
	}

	// добавляем токи собственной проводимости и токи ветвей
	Ire -= YiiSuper.real() * Vre - YiiSuper.imag() * Vim;
	Iim -= YiiSuper.imag() * Vre + YiiSuper.real() * Vim;

	if (!m_bLowVoltage)
	{
		// добавляем токи от нагрузки (если напряжение не очень низкое)
		const double Pk{ Pnr - Pgr }, Qk{ Qnr - Qgr };
		Ire += (Pk * Vre + Qk * Vim) / V2;
		Iim += (Pk * Vim - Qk * Vre) / V2;
	}
#ifdef _DEBUG
	else
		_ASSERTE(std::abs(Pnr - Pgr) < DFW2_EPSILON && std::abs(Qnr - Qgr) < DFW2_EPSILON);
#endif
	return cplx(Ire, Iim);
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
	Vmin *= 0.5;
	bool bRes = (Vmin - dLRCVicinity) > Vtest / V0;
	CLinkPtrCount *pLink = GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice) && bRes)
	{
		const auto& pSlaveNode{ static_cast<CDynaNodeBase*>(*ppDevice) };
		bRes = (Vmin - pSlaveNode->dLRCVicinity) > Vtest / pSlaveNode->V0;
	}
	return bRes;
}

// рассчитывает нагрузку узла с учетом СХН
// и суммирует все узлы, входящие в суперузел
// использует модуль напряжения из узла
void CDynaNodeBase::GetPnrQnrSuper()
{
	GetPnrQnrSuper(V);
}

// рассчитывает нагрузку узла с учетом СХН
// и суммирует все узлы, входящие в суперузел
// использует заданный модуль напряжения
void CDynaNodeBase::GetPnrQnrSuper(double Vnode)
{
	GetPnrQnr(Vnode);
	CLinkPtrCount *pLink = GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice))
	{
		const auto& pSlaveNode{ static_cast<CDynaNodeBase*>(*ppDevice) };
		pSlaveNode->FromSuperNode();

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
// использует модуль напряжения из узла
void CDynaNodeBase::GetPnrQnr()
{
	GetPnrQnr(V);
}
// рассчитывает нагрузку узла с учетом СХН
// использует заданный модуль напряжения
void CDynaNodeBase::GetPnrQnr(double Vnode)
{
	// по умолчанию нагрузка равна заданной в УР
	Pnr = Pn;	Qnr = Qn;
	// генерация в узле также равна заданной в УР
	Pgr = Pg;	Qgr = Qg;
	double VdVnom = Vnode / V0;

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

	const double Vre2{ Vre * Vre };
	const double Vim2{ Vim * Vim };
	double V2{ Vre2 + Vim2 };
	const double V2sq{ std::sqrt(V2) };

	GetPnrQnrSuper(V2sq);

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
			dIredVre +=  dLRCShuntPartPSuper;
			dIredVim +=  dLRCShuntPartQSuper;
			dIimdVre += -dLRCShuntPartQSuper;
			dIimdVim +=  dLRCShuntPartPSuper;
			dLRCPg = dLRCQg = Pgr = Qgr = 0.0;
			dLRCPn = dLRCQn = Pnr = Qnr = 0.0;
		}
	}

	CLinkPtrCount *pGenLink = GetSuperLink(2);
	CDevice **ppGen(nullptr);
	
	double Pk{ Pnr - Pgr }, Qk{ Qnr - Qgr };

	double V4 = V2 * V2;
	double VreVim2 = 2.0 * Vre * Vim;
	double PgVre2 = Pk * Vre2;
	double PgVim2 = Pk * Vim2;
	double QgVre2 = Qk * Vre2;
	double QgVim2 = Qk * Vim2;
		
	if (pDynaModel->FillConstantElements())
	{
		// в этом блоке только постоянные элементы (чистые коэффициенты), которые можно не изменять,
		// если не изменились коэффициенты метода интегрирования и шаг

		// обходим генераторы и формируем производные от токов генераторов
		// если узел в металлическом КЗ производные равны нулю
		double dGenMatrixCoe = m_bInMetallicSC ? 0.0 :-1.0;
		while (pGenLink->InMatrix(ppGen))
		{
			// здесь нужно проверять находится ли генератор в матрице (другими словами включен ли он)
			// или строить суперссылку на генераторы по условию того, что они в матрице
			const auto& pGen{ static_cast<CDynaPowerInjector*>(*ppGen) };
			pDynaModel->SetElement(Vre, pGen->Ire, dGenMatrixCoe);
			pDynaModel->SetElement(Vim, pGen->Iim, dGenMatrixCoe);
		}

		if (m_bInMetallicSC)
		{
			// если в металлическом КЗ, то производные от токов ветвей равны нулю (в строке единицы только от Vre и Vim)
			for (VirtualBranch *pV = m_VirtualBranchBegin; pV < m_VirtualBranchEnd; pV++)
			{
				// dIre /dVre
				pDynaModel->SetElement(Vre, pV->pNode->Vre, 0.0);
				// dIre/dVim
				pDynaModel->SetElement(Vre, pV->pNode->Vim, 0.0);
				// dIim/dVre
				pDynaModel->SetElement(Vim, pV->pNode->Vre, 0.0);
				// dIim/dVim
				pDynaModel->SetElement(Vim, pV->pNode->Vim, 0.0);
			}
		}
		else
		{
			// если не в металлическом КЗ, то производные от токов ветвей формируем как положено
			for (VirtualBranch *pV = m_VirtualBranchBegin; pV < m_VirtualBranchEnd; pV++)
			{
				// dIre /dVre
				pDynaModel->SetElement(Vre, pV->pNode->Vre, -pV->Y.real());
				// dIre/dVim
				pDynaModel->SetElement(Vre, pV->pNode->Vim, pV->Y.imag());
				// dIim/dVre
				pDynaModel->SetElement(Vim, pV->pNode->Vre, -pV->Y.imag());
				// dIim/dVim
				pDynaModel->SetElement(Vim, pV->pNode->Vim, -pV->Y.real());
			}
		}

		// запоминаем позиции в строках матрицы, на которых заканчиваются постоянные элементы
		pDynaModel->CountConstElementsToSkip(Vre.Index);
		pDynaModel->CountConstElementsToSkip(Vim.Index);
	}
	else
	{
		// если постоянные элементы не надо обновлять, то пропускаем их и начинаем с непостоянных
		pDynaModel->SkipConstElements(Vre.Index);
		pDynaModel->SkipConstElements(Vim.Index);
	}

	// check low voltage
	if (m_bLowVoltage)
	{
		pDynaModel->SetElement(V, Vre, 0.0);
		pDynaModel->SetElement(V, Vim, 0.0);
		pDynaModel->SetElement(Vre, V, 0.0);
		pDynaModel->SetElement(Vim, V, 0.0);

#ifdef _DEBUG
		// ассерты могут работать на неподготовленные данные на эстимейте
		if (!pDynaModel->EstimateBuild())
		{
			_ASSERTE(std::abs(PgVre2) < DFW2_EPSILON && std::abs(PgVim2) < DFW2_EPSILON);
			_ASSERTE(std::abs(QgVre2) < DFW2_EPSILON && std::abs(QgVim2) < DFW2_EPSILON);
		}
#endif

	}
	else
	{
		pDynaModel->SetElement(V, Vre, -Vre / V2sq);
		pDynaModel->SetElement(V, Vim, -Vim / V2sq);

		const double Vn{ V2sq };
		const double Vn2{ Vn * Vn };

		//const double Vn{ V };
		//const double Vn2{ V * V };

		Pk /= Vn2;
		Qk /= Vn2;

		dIredVre += Pk;
		dIredVim += Qk;
		dIimdVre -= Qk;
		dIimdVim += Pk;

		dLRCPn -= dLRCPg;	dLRCQn -= dLRCQg;
		dLRCPn /= Vn;
		dLRCQn /= Vn;

		const double dPdV{ (dLRCPn - 2.0 * Pk) / Vn };
		const double dQdV{ (dLRCQn - 2.0 * Qk) / Vn };

		pDynaModel->SetElement(Vre, V, dPdV * Vre + dQdV * Vim);
		pDynaModel->SetElement(Vim, V, dPdV * Vim - dQdV * Vre);



		/*
		double d1 = (PgVre2 - PgVim2 + VreVim2 * Qk) / V4;
		double d2 = (QgVre2 - QgVim2 - VreVim2 * Pk) / V4;

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
		pDynaModel->SetElement(Vre, V, dLRCPn * VreV2 + dLRCQn * VimV2);
		pDynaModel->SetElement(Vim, V, dLRCPn * VimV2 - dLRCQn * VreV2);
		*/
	}

	pDynaModel->SetElement(Vre, Vre, dIredVre);
	pDynaModel->SetElement(Vre, Vim, dIredVim);
	pDynaModel->SetElement(Vim, Vre, dIimdVre);
	pDynaModel->SetElement(Vim, Vim, dIimdVim);

	return true;
}

void CDynaNodeBase::InitNordsiek(CDynaModel* pDynaModel)
{
	_ASSERTE(m_pContainer);
	struct RightVector* pRv = pDynaModel->GetRightVector(A(0));
	ptrdiff_t nEquationsCount = m_pContainer->EquationsCount();

	VariableIndexRefVec seed;
	for (auto&& var : GetVariables(seed))
	{
		pRv->pValue = &var.get().Value;
		_ASSERTE(pRv->pValue);
		pRv->pDevice = this;
		pRv++;
	}
}


bool CDynaNodeBase::BuildRightHand(CDynaModel *pDynaModel)
{
	//GetPnrQnrSuper();
	// в узле может быть уже известный постоянный ток
	double Ire(IconstSuper.real()), Iim(IconstSuper.imag()), dV(0.0);

	if (!m_bInMetallicSC)
	{
		// если не в металлическом КЗ, обрабатываем нагрузку и генерацию,
		// заданные в узле
		double V2sq{ 0.0 };
		FromComplex(Ire, Iim, GetSelfImbISuper(pDynaModel->GetLRCToShuntVmin(), V2sq));

		if (!m_bLowVoltage)
			dV = V - V2sq;

		// обходим генераторы
		CLinkPtrCount *pGenLink = GetSuperLink(2);
		CDevice **ppGen(nullptr);

		while (pGenLink->InMatrix(ppGen))
		{
			const auto& pGen{ static_cast<CDynaPowerInjector*>(*ppGen) };
			Ire -= pGen->Ire;
			Iim -= pGen->Iim;
		}

		for (VirtualBranch *pV = m_VirtualBranchBegin; pV < m_VirtualBranchEnd; pV++)
		{
			Ire -= pV->Y.real() * pV->pNode->Vre - pV->Y.imag() * pV->pNode->Vim;
			Iim -= pV->Y.imag() * pV->pNode->Vre + pV->Y.real() * pV->pNode->Vim;
		}
	}

	pDynaModel->SetFunction(V, dV);
	pDynaModel->SetFunction(Vre, Ire);
	pDynaModel->SetFunction(Vim, Iim);

	return true;
}

void CDynaNodeBase::NewtonUpdateEquation(CDynaModel* pDynaModel)
{
	dLRCVicinity = 5.0 * std::abs(Vold - V) / Unom;
	Vold = V;
	CLinkPtrCount *pLink = GetSuperLink(0);
	CDevice **ppDevice(nullptr);
	while (pLink->In(ppDevice))
		static_cast<CDynaNodeBase*>(*ppDevice)->FromSuperNode();
}

eDEVICEFUNCTIONSTATUS CDynaNodeBase::Init(CDynaModel* pDynaModel)
{
	UpdateVreVim();
	m_bLowVoltage = V < (LOW_VOLTAGE - LOW_VOLTAGE_HYST);
	PickV0();

	if (GetLink(1)->m_nCount > 0)					// если к узлу подключены генераторы, то СХН генераторов не нужна и мощности генерации 0
		Pg = Qg = Pgr = Qgr = 0.0;
	else if (Equal(Pg, 0.0) && Equal(Qg, 0.0))		// если генераторы не подключены, и мощность генерации равна нулю - СХН генераторов не нужна
		Pgr = Qgr = 0.0;
	else
	{
		m_pLRCGen = pDynaModel->GetLRCGen();		// если есть генерация но нет генераторов - нужна СХН генераторов
		if (m_pLRCGen->GetId() == -2) // если задана СХН Iconst - меняем СХН на постоянный ток в системе уравнений
		{
			// альтернативный вариант - генерация в узле 
			// представляется током
			// рассчитываем и сохраняем постоянный ток по мощности и напряжению
			Iconst = -std::conj(cplx(Pgr, Qgr) / cplx(Vre, Vim));
			// обнуляем генерацию в узле
			Pg = Qg = Pgr = Qgr = 0.0;;
			// и обнуляем СХН, так как она больше не нужна
			m_pLRCGen = nullptr;
		}
	}

	// если в узле нет СХН для динамики, подставляем СХН по умолчанию
	if (!m_pLRC)
		m_pLRC = pDynaModel->GetLRCDynamicDefault();

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
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
	if (std::abs(newDelta - Delta) > DFW2_EPSILON)
	{
		RightVector *pRvDelta = GetModel()->GetRightVector(Delta.Index);
		RightVector *pRvLag = GetModel()->GetRightVector(Lag.Index);
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
	pDynaModel->SetDerivative(Lag, (Delta - Lag) / T);
	return true;
}


bool CDynaNode::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = CDynaNodeBase::BuildEquations(pDynaModel);

	// Копируем скольжение в слэйв-узлы суперузла
	// (можно совместить с CDynaNodeBase::FromSuperNode()
	// и сэкономить цикл
	CLinkPtrCount* pLink = GetSuperLink(0);
	CDevice** ppDevice(nullptr);
	while (pLink->In(ppDevice))
	{
		const auto& pSlaveNode{ static_cast<CDynaNode*>(*ppDevice) };
		pSlaveNode->S = S;
	}

	double T = pDynaModel->GetFreqTimeConstant();
	double w0 = pDynaModel->GetOmega0();

	double Vre2 = Vre * Vre;
	double Vim2 = Vim * Vim;
	double V2 = Vre2 + Vim2;

	pDynaModel->SetElement(V, V, 1.0);
	pDynaModel->SetElement(Delta, Delta, 1.0);

	pDynaModel->SetElement(Lag, Delta, -1.0 / T);
	pDynaModel->SetElement(Lag, Lag, -1.0 / T);


	if (m_bLowVoltage)
	{
		pDynaModel->SetElement(Delta, Vre, 0.0);
		pDynaModel->SetElement(Delta, Vim, 0.0);
	}
	else
	{
		pDynaModel->SetElement(Delta, Vre, Vim / V2);
		pDynaModel->SetElement(Delta, Vim, -Vre / V2);
	}
	
	if (!pDynaModel->IsInDiscontinuityMode())
	{
		pDynaModel->SetElement(S, Delta, -1.0 / T / w0);
		pDynaModel->SetElement(S, Lag, 1.0 / T / w0);
		pDynaModel->SetElement(S, S, 1.0);
	}
	else
	{
		pDynaModel->SetElement(S, Delta, 0.0);
		pDynaModel->SetElement(S, Lag, 0.0);
		pDynaModel->SetElement(S, S, 1.0);
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

	double dDelta(0.0);

	if (pDynaModel->IsInDiscontinuityMode()) 
		dS = 0.0;

	if (!m_bLowVoltage)
		dDelta = Delta - atan2(Vim, Vre);
	
	pDynaModel->SetFunctionDiff(Lag, dLag);
	pDynaModel->SetFunction(S, dS);
	pDynaModel->SetFunction(Delta, dDelta);

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
	return &GetVariable(nVarIndex).Value;
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
	return &GetVariable(nVarIndex).Value;
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

void CDynaNodeContainer::CalcAdmittances(bool bFixNegativeZs)
{
	for (auto&& it : m_DevVec)
		static_cast<CDynaNodeBase*>(it)->CalcAdmittances(bFixNegativeZs);
}

// рассчитывает шунтовые части нагрузок узлов
void CDynaNodeContainer::CalculateShuntParts()
{
	// сначала считаем индивидуально для каждого узла
	for (auto&& it : m_DevVec)
		static_cast<CDynaNodeBase*>(it)->CalculateShuntParts();

	// затем суммируем шунтовые нагрузки в суперузлы
	for (auto&& node : m_DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
		// собираем проводимость
		pNode->YiiSuper = pNode->Yii;
		// собираем постоянный ток
		pNode->IconstSuper = pNode->Iconst;
		pNode->dLRCShuntPartPSuper = pNode->dLRCShuntPartP;
		pNode->dLRCShuntPartQSuper = pNode->dLRCShuntPartQ;
		CLinkPtrCount *pLink = m_SuperLinks[0].GetLink(node->m_nInContainerIndex);
		if (pLink->m_nCount)
		{
			CDevice **ppDevice(nullptr);
			// суммируем собственные проводимости и шунтовые части СХН нагрузки и генерации в узле
			while (pLink->In(ppDevice))
			{
				const auto& pSlaveNode{ static_cast<CDynaNodeBase*>(*ppDevice) };
				pNode->YiiSuper += pSlaveNode->Yii;
				pNode->IconstSuper += pSlaveNode->Iconst;
				pNode->dLRCShuntPartPSuper += pSlaveNode->dLRCShuntPartP;
				pNode->dLRCShuntPartQSuper += pSlaveNode->dLRCShuntPartQ;
			}
		}
	}
}

// рассчитывает нагрузки узла в виде шунта для V < Vmin СХН
void CDynaNodeBase::CalculateShuntParts()
{
	// TODO - надо разобраться с инициализацией V0 __до__ вызова этой функции
	double V02 = V0 * V0;
	if (m_pLRC)
	{
		// рассчитываем шунтовую часть СХН нагрузки в узле для низких напряжений
		dLRCShuntPartP = Pn * m_pLRC->P.begin()->a2;
		dLRCShuntPartQ = Qn * m_pLRC->Q.begin()->a2;
	}
	else
		dLRCShuntPartP = dLRCShuntPartQ = 0.0;

	if (m_pLRCGen)
	{
		// рассчитываем шунтовую часть СХН генерации в узле для низких напряжений
		dLRCShuntPartP -= Pg * m_pLRCGen->P.begin()->a2;
		dLRCShuntPartQ -= Qg * m_pLRCGen->Q.begin()->a2;
	}
	dLRCShuntPartP /= V02;
	dLRCShuntPartQ /= V02;
}

// Собирает проводимость узла на землю
// из заданной в узле, включенных реакторов и шунта КЗ

void CDynaNodeBase::GetGroundAdmittance(cplx& y)
{

	y.real(G + Gshunt);
	y.imag(B + Bshunt);
	for (const auto& reactor : reactors)
	{
		if (reactor->IsStateOn())
			y += cplx(reactor->g, reactor->b);
	}

	// добавляем к проводимости на землю
	// шунты Нортона от генераторов. Для тех генераторов
	// которые не работают в схеме Нортона его значение равно нулю
	// !!!!!! Также не следует рассчитываеть Нортона в генераторах
	// до расчета динамики. Для УР это неприемлемо !!!!!!
	CDevice** ppDevice{ nullptr };
	CLinkPtrCount* pLink{ GetLink(1) };
	while (pLink->In(ppDevice))
	{
		const auto& pGen{ static_cast<CDynaPowerInjector*>(*ppDevice) };
		if (pGen->IsStateOn())
			y += pGen->Ynorton();
	}

}

void CDynaNodeBase::CalcAdmittances(bool bFixNegativeZs)
{
	// собственную проводимость начинаем с проводимости на землю
	// с обратным знаком
	GetGroundAdmittance(Yii);
	Yii.real(-Yii.real());
	Yii.imag(-Yii.imag());

	m_bInMetallicSC = !(std::isfinite(Yii.real()) && std::isfinite(Yii.imag()));

	if (m_bInMetallicSC || !IsStateOn())
	{
		Vre = Vim = 0.0;
		V = Delta = 0.0;
	}
	else
	{
		CDevice **ppDevice(nullptr);
		CLinkPtrCount *pLink = GetLink(0);
		while (pLink->In(ppDevice))
		{
			const auto& pBranch{ static_cast<CDynaBranch*>(*ppDevice) };
			// проводимости ветви будут рассчитаны с учетом того,
			// что она могла быть отнесена внутрь суперузла
			pBranch->CalcAdmittances(bFixNegativeZs);
			// состояние ветви в данном случае не важно - слагаемые только к собственной
			// проводимости узла. Слагаемые сами по себе рассчитаны с учетом состояния ветви
			Yii -= (this == pBranch->m_pNodeIp) ? pBranch->Yips : pBranch->Yiqs;
		}
		
		if (V < DFW2_EPSILON)
		{
			Vre = V = Unom;
			Vim = Delta = 0.0;
		}

		_CheckNumber(Yii.real());
		_CheckNumber(Yii.imag());
	}
}

eDEVICEFUNCTIONSTATUS CDynaNode::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice)
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
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

CSynchroZone::CSynchroZone() : CDevice()
{
}


VariableIndexRefVec& CSynchroZone::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ S },ChildVec));
}

double* CSynchroZone::GetVariablePtr(ptrdiff_t nVarIndex)
{
	return &GetVariable(nVarIndex).Value;
}

bool CSynchroZone::BuildEquations(CDynaModel* pDynaModel)
{
	bool bRes = true;
	if (m_bInfPower)
	{
		pDynaModel->SetElement(S, S, 1.0);
	}
	else
	{
		pDynaModel->SetElement(S, S, 1.0);
		for (auto&& it : m_LinkedGenerators)
		{
			if(it->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				const auto& pGenMj{ static_cast<CDynaGeneratorMotion*>(it) };
				pDynaModel->SetElement(S, pGenMj->s, -pGenMj->Mj / Mj);
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
		pDynaModel->SetFunction(S, 0.0);
	}
	else
	{
		for (auto&& it : m_LinkedGenerators)
		{
			if (it->IsKindOfType(DEVTYPE_GEN_MOTION))
			{
				const auto& pGenMj{ static_cast<CDynaGeneratorMotion*>(it) };
				dS -= pGenMj->Mj * pGenMj->s / Mj;
			}
		}
		pDynaModel->SetFunction(S, dS);
	}
	return true;
}


eDEVICEFUNCTIONSTATUS CSynchroZone::Init(CDynaModel* pDynaModel)
{
	S = 0.0;
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

bool CDynaNodeContainer::LULF()
{
	bool bRes = true;

	KLUWrapper<std::complex<double>> klu;
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
	auto pDiags = std::make_unique<double*[]>(nNodeCount);
	double **ppDiags = pDiags.get();
	double *pB = B;

	double *pAx = Ax;
	ptrdiff_t *pAp = Ap;
	ptrdiff_t *pAi = Ai;

	std::ofstream fnode(GetModel()->Platform().ResultFile("nodes.csv"));
	std::ofstream fgen(GetModel()->Platform().ResultFile("gens.csv"));

	fnode << ";";

	for (auto&& it : m_DevInMatrix)
	{
		_ASSERTE(pAx < Ax + nNzCount * 2);
		_ASSERTE(pAi < Ai + nNzCount);
		_ASSERTE(pAp < Ap + nNodeCount);
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		pNode->dLRCVicinity = 0.0;		// зона сглаживания СХН для начала нулевая - без сглаживания
		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / EquationsCount();		  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		if (!pNode->m_bInMetallicSC)
		{
			// стартуем с плоского
			pNode->Vre = pNode->V = pNode->Unom;
			pNode->Vim = pNode->Delta = 0.0;
			// для всех узлов, которые не отключены и не находятся в металлическом КЗ (КЗ с нулевым шунтом)
			fnode << pNode->GetId() << ";";
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

	ptrdiff_t& nIteration = m_IterationControl.Number;
	for (nIteration = 0; nIteration < 200; nIteration++)
	{
		m_IterationControl.Reset();
		ppDiags = pDiags.get();
		pB = B;

		fnode << std::endl << nIteration << ";";
		fgen  << std::endl << nIteration << ";";

		// проходим по узлам вне зависимости от их состояния, параллельно идем по диагонали матрицы
		for (auto&& it : m_DevInMatrix)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
			_ASSERTE(pB < B + nNodeCount * 2);

			/*
			if (pNode->GetId() == 1067 && m_pDynaModel->GetCurrentTime() > 0.53 && nIteration > 4)
				pNode->GetId();// pNode->BuildRightHand(m_pDynaModel);
				*/

			if (!pNode->m_bInMetallicSC)
			{
				// для всех узлов которые включены и вне металлического КЗ

				cplx Unode(pNode->Vre, pNode->Vim);

				cplx I{ pNode->IconstSuper };
				cplx Y{ pNode->YiiSuper };		// диагональ матрицы по умолчанию собственная проводимость узла

				pNode->Vold = pNode->V;			// запоминаем напряжение до итерации для анализа сходимости

				pNode->GetPnrQnrSuper();

				// Generators

				CDevice **ppDeivce(nullptr);
				CLinkPtrCount *pLink = pNode->GetSuperLink(2);

				// проходим по генераторам
				while (pLink->InMatrix(ppDeivce))
				{
					const auto& pVsource{ static_cast<CDynaVoltageSource*>(*ppDeivce) };
					// если в узле есть хотя бы один генератор, то обнуляем мощность генерации узла
					// если в узле нет генераторов но есть мощность генерации - то она будет учитываться
					// задающим током
					// если генераторм выключен то просто пропускаем, его мощность и ток будут равны нулю
					if (!pVsource->IsStateOn())
						continue;

					pVsource->CalculatePower();

					const auto& pGen{ static_cast<CDynaGeneratorInfBus*>(pVsource) };
					// в диагональ матрицы добавляем проводимость генератора
					// и вычитаем шунт Нортона, так как он уже добавлен в диагональ
					// матрицы. Это работает для генераторов у которых есть Нортон (он
					// обратен Zgen), и нет Нортона (он равен нулю)
					Y -= 1.0 / pGen->Zgen() - pGen->Ynorton();
					I -= pGen->Igen(nIteration);

					_CheckNumber(I.real());
					_CheckNumber(I.imag());

					fgen << pVsource->P << ";";
				}

				// рассчитываем задающий ток узла от нагрузки
				// можно посчитать ток, а можно посчитать добавку в диагональ
				//I += conj(cplx(Pnr - pNode->Pg, Qnr - pNode->Qg) / pNode->VreVim);
				if (pNode->V > 0.0)
					Y += std::conj(cplx(pNode->Pgr - pNode->Pnr, pNode->Qgr - pNode->Qnr) / pNode->V.Value / pNode->V.Value);
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

				fnode << pNode->V / pNode->V0 << ";";
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
			const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
			// напряжение после решения системы в векторе задающий токов
			pNode->Vre = *pB;		pB++;
			pNode->Vim = *pB;		pB++;

			// считаем напряжение узла в полярных координатах
			pNode->UpdateVDeltaSuper();
			// рассчитываем зону сглаживания СХН (также как для Ньютона)
			/*if (pNode->m_pLRC)
				pNode->dLRCVicinity = 30.0 * std::abs(pNode->Vold - pNode->V) / pNode->Unom;
			*/
			// считаем изменение напряжения узла между итерациями и находим
			// самый изменяющийся узел
			if (!pNode->m_bInMetallicSC)
				m_IterationControl.m_MaxV.UpdateMaxAbs(pNode, CDevice::ZeroDivGuard(pNode->V - pNode->Vold, pNode->Vold));
		}

		DumpIterationControl(DFW2MessageStatus::DFW2LOG_DEBUG);

		if (std::abs(m_IterationControl.m_MaxV.GetDiff()) < m_pDynaModel->GetRtolLULF())
		{
			Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLULFConverged, m_IterationControl.m_MaxV.GetDiff(), nIteration));
			break;
		}
	}
	return bRes;
}

// Для перехода от расчета динамики к расчету УР
// меняем местами нагрузку и расчетную нагрузку узла, СХН и 
// номинальное напряжение с расчетным
// Для возврата к динамике делаем еще один обмен
// Возможно придется отказаться от этих обменов: просто
// забирать из исходных данных номинальную нагрузку и считать УР
// самостоятельно

void CDynaNodeContainer::SwitchLRCs(bool bSwitchToDynamicLRC)
{
	if (bSwitchToDynamicLRC != m_bDynamicLRC)
	{
		m_bDynamicLRC = bSwitchToDynamicLRC;
		for (auto&& node : m_DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
			// меняем местами СХН УР и динамики
			std::swap(pNode->m_pLRCLF, pNode->m_pLRC);
			// меняем местами расчетную и номинальную 
			// нагрузки
			std::swap(pNode->Pn, pNode->Pnr);
			std::swap(pNode->Qn, pNode->Qnr);
			// если СХН для УР - номинальное напряжение СХН
			// равно номинальному напряжению узла
			// если СХН для динамики - номинальное напряжение СХН
			// равно расчетному
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

VariableIndexExternal CDynaNodeBase::GetExternalVariable(std::string_view VarName)
{
	if (VarName == CDynaNode::m_cszSz)
	{
		VariableIndexExternal ExtVar = { -1, nullptr };

		if (m_pSyncZone)
			ExtVar = m_pSyncZone->GetExternalVariable(CDynaNode::m_cszS);
		return ExtVar;
	}
	else
		return CDevice::GetExternalVariable(VarName);
}


void CDynaNodeBase::StoreStates()
{
	m_bSavedLowVoltage = m_bLowVoltage;
}

void CDynaNodeBase::RestoreStates()
{
	m_bLowVoltage = m_bSavedLowVoltage;
}

void CDynaNodeBase::FromSuperNode()
{
	_ASSERTE(m_pSuperNodeParent);
	V = m_pSuperNodeParent->V;
	Delta = m_pSuperNodeParent->Delta;
	Vre = m_pSuperNodeParent->Vre;
	Vim = m_pSuperNodeParent->Vim;
	dLRCVicinity = m_pSuperNodeParent->dLRCVicinity;
	m_bLowVoltage = m_pSuperNodeParent->m_bLowVoltage;
}

void CDynaNodeBase::SetLowVoltage(bool bLowVoltage)
{
	if (m_bLowVoltage)
	{
		if (!bLowVoltage)
		{
			m_bLowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Напряжение {} в узле {} выше порогового", V.Value, GetVerbalName()));
		}
	}
	else
	{
		if (bLowVoltage)
		{
			m_bLowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Напряжение {} в узле {} ниже порогового", V.Value, GetVerbalName()));
		}
	}
}

double CDynaNodeBase::FindVoltageZC(CDynaModel *pDynaModel, RightVector *pRvre, RightVector *pRvim, double Hyst, bool bCheckForLow)
{
	double rH = 1.0;

	// выбираем границу сравгнения с гистерезисом - на снижение -, на повышение +
	const double Border = LOW_VOLTAGE + (bCheckForLow ? -Hyst : Hyst );
	const double  h = pDynaModel->GetH();
	const double *lm = pDynaModel->Methodl[DET_ALGEBRAIC * 2 + pDynaModel->GetOrder() - 1];


	// рассчитываем текущие напряжения по Нордсику (итоговые еще не рассчитаны в итерации)
#ifdef USE_FMA
	double Vre1 = std::fma(pRvre->Error, lm[0], pRvre->Nordsiek[0]);
	double Vim1 = std::fma(pRvim->Error, lm[0], pRvim->Nordsiek[0]);
#else
	double Vre1 = pRvre->Nordsiek[0] + pRvre->Error * lm[0];
	double Vim1 = pRvim->Nordsiek[0] + pRvim->Error * lm[0];
#endif

	// текущий модуль напряжения
	double Vcheck = sqrt(Vre1 * Vre1 + Vim1 * Vim1);
	// коэффициенты первого порядка
	double dVre1 = (pRvre->Nordsiek[1] + pRvre->Error * lm[1]) / h;
	double dVim1 = (pRvim->Nordsiek[1] + pRvim->Error * lm[1]) / h;
	// коэффициенты второго порядка
	double dVre2 = (pRvre->Nordsiek[2] + pRvre->Error * lm[2]) / h / h;
	double dVim2 = (pRvim->Nordsiek[2] + pRvim->Error * lm[2]) / h / h;

	// функция значения переменной от шага
	// Vre(h) = Vre1 + h * dVre1 + h^2 * dVre2
	// Vim(h) = Vim1 + h * dVim1 + h^2 * dVim2

	// (Vre1 + h * dVre1 + h^2 * dVre2)^2 + (Vim1 + h * dVim1 + h^2 * dVim2)^2 - Boder^2 = 0

	// определяем разность границы и текущего напряжения и взвешиваем разность по выражению контроля погрешности
	double derr = std::abs(pRvre->GetWeightedError(std::abs(Vcheck - Border), Border));
	const ptrdiff_t q(pDynaModel->GetOrder());

	if (derr < pDynaModel->GetZeroCrossingTolerance())
	{
		// если погрешность меньше заданной в параметрах
		// ставим заданную фиксацию напряжения 
		SetLowVoltage(bCheckForLow);
		pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
	}
	else
	{
		// если погрешность больше - определяем коэффициент шага, на котором
		// нужно проверить разность снова
		if ( q == 1)
		{
			// для первого порядка решаем квадратное уравнение
			// (Vre1 + h * dVre1)^2 + (Vim1 + h * dVim1)^2 - Boder^2 = 0
			// Vre1^2 + 2 * Vre1 * h * dVre1 + dVre1^2 * h^2 + Vim1^2 + 2 * Vim1 * h * dVim1 + dVim1^2 * h^2 - Border^2 = 0

			double a = dVre1 * dVre1 + dVim1 * dVim1;
			double b = 2.0 * (Vre1 * dVre1 + Vim1 * dVim1);
			double c = Vre1 * Vre1 + Vim1 * Vim1 - Border * Border;
			rH = CDynaPrimitive::GetZCStepRatio(pDynaModel, a, b, c);

			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetLowVoltage(bCheckForLow);
				pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
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

				// здесь была проверка диапазона t, но при ее использовании возможно неправильное
				// определение доли шага, так как на первых итерациях значение может значительно выходить
				// за диапазон. Но можно попробовать контролировать диапазон не на первой, а на последующих итерациях

				if (std::abs(dt) < DFW2_EPSILON * 10.0)
					break;

				/*
				if (t > 0.0 || t < -h)
				{
					t = FLT_MAX;
					break;
				}
				*/ 
			}
			rH = (h + t) / h;
			if (pDynaModel->ZeroCrossingStepReached(rH))
			{
				SetLowVoltage(bCheckForLow);
				pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
			}
		}

		if (rH < 0.0)
		{

			const double h0 = h * rH - h;
			if (q == 1)
				dVre2 = dVim2 = 0.0;
			const double checkVre = Vre1 + dVre1 * h0 + dVre2 * h0 * h0;
			const double checkVim = Vim1 + dVim1 * h0 + dVim2 * h0 * h0;
			const double Vh0 = std::sqrt(checkVre * checkVre + checkVim * checkVim);

			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("Negative ZC ratio rH={} at node \"{}\" at t={} order={}, V={} <- V={} Border={}",
				rH,
				GetVerbalName(),
				GetModel()->GetCurrentTime(),
				q,
				Vh0,
				Vcheck,
				Border
			));

			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("Nordsieck Vre=[{};{};{}]->[{};{};{}]",
				pRvre->Nordsiek[0],
				pRvre->Nordsiek[1],
				pRvre->Nordsiek[2],
				Vre1,
				dVre1 * h,
				dVre2 * h * h
			));

			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("Nordsieck Vim=[{};{};{}]->[{};{};{}]",
				pRvim->Nordsiek[0],
				pRvim->Nordsiek[1],
				pRvim->Nordsiek[2],
				Vim1,
				dVim1 * h,
				dVim2 * h * h
			));
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
	RightVector *pRvre = pDynaModel->GetRightVector(Vre.Index);
	RightVector *pRvim = pDynaModel->GetRightVector(Vim.Index);
	const double *lm = pDynaModel->Methodl[DET_ALGEBRAIC * 2 + pDynaModel->GetOrder() - 1];

#ifdef USE_FMA
	double Vre1 = std::fma(pRvre->Error, lm[0], pRvre->Nordsiek[0]);
	double Vim1 = std::fma(pRvim->Error, lm[0], pRvim->Nordsiek[0]);
#else
	double Vre1 = pRvre->Nordsiek[0] + pRvre->Error * lm[0];
	double Vim1 = pRvim->Nordsiek[0] + pRvim->Error * lm[0];
#endif

	double Vcheck = sqrt(Vre1 * Vre1 + Vim1 * Vim1);

	/*
	if (GetId() == 61112076 && GetModel()->GetCurrentTime() > 2.7)
	{
		const double Vold(std::sqrt(pRvre->Nordsiek[0] * pRvre->Nordsiek[0] + pRvim->Nordsiek[0] * pRvim->Nordsiek[0]));
		Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("{}->{}", Vold, Vcheck));
	}
	*/

	if (m_bLowVoltage)
	{
		double Border = LOW_VOLTAGE + Hyst;

		if (Vcheck > Border)
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, false);
	}
	else
	{
		double Border = LOW_VOLTAGE - Hyst;

		if (Vcheck < Border)
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, true);
	}

	return rH;
}


void CDynaNodeBase::ProcessTopologyRequest()
{
	if (m_pContainer)
		static_cast<CDynaNodeContainer*>(m_pContainer)->ProcessTopologyRequest();
}

// добавляет ветвь в список ветвей с нулевым сопротивлением суперузла
VirtualZeroBranch* CDynaNodeBase::AddZeroBranch(CDynaBranch* pBranch)
{

	// На вход могут приходить и ветви с сопротивлением больше порогового, но они не будут добавлены в список
	// ветвей с нулевым сопротивлением. Потоки по таким ветвями будут рассчитаны как обычно и будут нулями, если ветвь
	// находится в суперузле (все напряжения одинаковые)

	if (pBranch->IsZeroImpedance())
	{
		if (m_VirtualZeroBranchEnd >= static_cast<CDynaNodeContainer*>(m_pContainer)->GetZeroBranchesEnd())
			throw dfw2error("CDynaNodeBase::AddZeroBranch VirtualZeroBranches overrun");

		// если ветвь имеет сопротивление ниже минимального 
		bool bAdd(true);
		// проверяем 1) - не добавлена ли она уже; 2) - нет ли параллельной ветви
		VirtualZeroBranch *pParallelFound(nullptr);
		for (VirtualZeroBranch *pVb = m_VirtualZeroBranchBegin; pVb < m_VirtualZeroBranchEnd; pVb++)
		{
			if (pVb->pBranch == pBranch)
			{
				// если уже добавлена, выходим и не добавляем
				bAdd = false;
				break;
			}

			// для того чтобы привязать параллельную цепь нужна основная цепь у которой pParallel == nullptr
			if (!pVb->pParallelTo)
			{
				// проверяем есть ли параллельная впрямую
				bool bParr = pVb->pBranch->m_pNodeIp == pBranch->m_pNodeIp && pVb->pBranch->m_pNodeIq == pBranch->m_pNodeIq;
				// и если нет - проверяем параллельную в обратную
				bParr = bParr ? bParr : pVb->pBranch->m_pNodeIq == pBranch->m_pNodeIp && pVb->pBranch->m_pNodeIp == pBranch->m_pNodeIq;
				if (bParr)
					// если есть параллельная, ставим в добавляемую ветвь ссылку на найденную параллельную ветвь
					pParallelFound = pVb;
			}
		}
		if (bAdd)
		{
			// если ветвь добавляем
			// то сначала проверяем есть ли место
			// сдвигаем указатель на конец списка ветвей с нулевым сопротивлением для данного узла
			m_VirtualZeroBranchEnd->pBranch = pBranch;
			if (pParallelFound)
			{
				m_VirtualZeroBranchEnd->pParallelTo = pParallelFound->pBranch;
				pParallelFound->nParallelCount++;
			}
			// а в основной ветви увеличиваем счетчик параллельных
			m_VirtualZeroBranchEnd++;
		}
	}

	// возвращаем конец списка ветвей с нулевым сопротивлением
	return m_VirtualZeroBranchEnd;
}

void CDynaNodeBase::TidyZeroBranches()
{
	// сортируем нулевые ветви так, чтобы вначале были основные, в конце параллельные основным
	std::sort(m_VirtualZeroBranchBegin, m_VirtualZeroBranchEnd, [](const VirtualZeroBranch& lhs, const VirtualZeroBranch& rhs)->bool { return lhs.pParallelTo < rhs.pParallelTo; });
	m_VirtualZeroBranchParallelsBegin = m_VirtualZeroBranchBegin;
	m_VirtualZeroBranchParallelsBegin = m_VirtualZeroBranchBegin;
	// находим начало параллельных цепей
	while (m_VirtualZeroBranchParallelsBegin < m_VirtualZeroBranchEnd)
	{
		if (!m_VirtualZeroBranchParallelsBegin->pParallelTo)
			m_VirtualZeroBranchParallelsBegin++;
		else
			break;
	}
	//список нулевых ветвей такой
	// [m_VirtualZeroBranchBegin; m_VirtualZeroBranchParrallelsBegin) - основные
	// [m_VirtualZeroBranchParrallelsBegin; m_VirtualZeroBranchEnd) - параллельные

}

// Проверяет оторван ли узел от связей
bool CDynaNodeBase::IsDangling()
{
	CLinkPtrCount* pLink = GetLink(0);
	CDevice** ppDevice(nullptr);
	// ищем хотя бы одну включенную ветвь
	while (pLink->In(ppDevice))
		if (static_cast<CDynaBranch*>(*ppDevice)->BranchAndNodeConnected(this))
			break; // включенная ветвь есть

	return !ppDevice;
}

void CDynaNodeBase::CreateZeroLoadFlowData()
{
	// Мы хотим сделать матрицу Y для расчета потокораспределения внутри
	// суперузла

	// Для обычных (не супер) узлов не выполняем
	if (m_pSuperNodeParent)
		return;

	const CLinkPtrCount* pSuperNodeLink{ GetSuperLink(0) };
	CDevice** ppNodeEnd{ pSuperNodeLink->m_pPointer + pSuperNodeLink->m_nCount };

	// не выполняем также и для суперзулов, в которых нет узлов
	if (pSuperNodeLink->m_pPointer == ppNodeEnd)
		return;

	// создаем данные суперузла
	ZeroLF.ZeroSupeNode = std::make_unique<CDynaNodeBase::ZeroLFData::ZeroSuperNodeData>();

	CDynaNodeBase::ZeroLFData::ZeroSuperNodeData *pZeroSuperNode{ ZeroLF.ZeroSupeNode.get() };

	// создаем данные для построения Y : сначала создаем вектор строк матрицы
	pZeroSuperNode->LFMatrix.reserve(ppNodeEnd - pSuperNodeLink->m_pPointer);

	// отыщем узел с максимальным количеством связей
	// поиск начинаем с узла-представителя суперзула
	std::pair<CDynaNodeBase*, size_t>  pMaxRankNode{ this, GetLink(0)->m_nCount };

	// находим узел с максимальным числом связей
	for (CDevice** ppSlaveDev = pSuperNodeLink->m_pPointer; ppSlaveDev < ppNodeEnd; ppSlaveDev++)
	{
		const auto& pSlaveNode{ static_cast<CDynaNodeBase*>(*ppSlaveDev) };
		if (size_t nLinkCount{ pSlaveNode->GetLink(0)->m_nCount }; pMaxRankNode.second < nLinkCount)
			pMaxRankNode = { pSlaveNode, nLinkCount };
	}

	// узел с максимальным количеством связей не входит в Y
	// его индекс в Y делаем отрицательным
	pMaxRankNode.first->ZeroLF.m_nSuperNodeLFIndex = -1;
	// а индикатор напряжения ставим единичным
	pMaxRankNode.first->ZeroLF.vRe = 1.0;
	pMaxRankNode.first->ZeroLF.vIm = 0.0;

	// добавляем в Y узлы кроме узла с максимальным количеством связей и нумеруем их
	ptrdiff_t nRowIndex{ 0 };

	const auto AddAndIndexNode = [this, &pMaxRankNode, &nRowIndex](CDynaNodeBase* pNode)
	{
		if (pMaxRankNode.first != pNode)
		{
			ZeroLF.ZeroSupeNode->LFMatrix.push_back({ pNode });
			pNode->ZeroLF.m_nSuperNodeLFIndex = nRowIndex++;
		}
	};

	AddAndIndexNode(this);
	for (CDevice** ppSlaveDev = pSuperNodeLink->m_pPointer; ppSlaveDev < ppNodeEnd; ppSlaveDev++)
		AddAndIndexNode(static_cast<CDynaNodeBase*>(*ppSlaveDev));

	// считаем общее количество ветвей из данного суперузла
	// включая параллельные (с запасом)
	size_t nBranchesCount{ 0 };

	for (auto&& node : pZeroSuperNode->LFMatrix)
	{
		CLinkPtrCount* pBranchLink = node->GetLink(0);
		CDevice** ppDevice(nullptr);
		// ищем ветви соединяющие разные суперузлы
		while (pBranchLink->In(ppDevice))
		{
			const auto& pBranch{ static_cast<CDynaBranch*>(*ppDevice) };
			if (!pBranch->IsZeroImpedance())
				nBranchesCount++;
		}
	}

	// здесь имеем общее количество ветвей суперзула в другие суперузлы
	// реальное их количество может быть меньше за счет эквивалентирования
	// параллельных ветвей
	// используем простой общий массив из-за того, что его аллокация/деаллокация
	// быстрее чем а/д для контейнеров для каждого узла. Кроме того, ветви
	// внутри общего массива разделены парами указателей. Для вектора пришлось
	// бы использовать неинициализированные итераторы
	pZeroSuperNode->m_VirtualBranches = std::make_unique<VirtualBranch[]>(nBranchesCount);
	// вектор списков узлов, связанных связями с нулевыми сопротивлениями
	pZeroSuperNode->m_VirtualZeroBranches = std::make_unique<VirtualBranch[]>(2 * (m_VirtualZeroBranchParallelsBegin - m_VirtualZeroBranchBegin));

	VirtualBranch* pHead{ pZeroSuperNode->m_VirtualBranches.get() };
	VirtualBranch* pZeroHead{ pZeroSuperNode->m_VirtualZeroBranches.get() };
	

	const auto FindParallel = [](VirtualBranch* pBegin, VirtualBranch* pEnd, const CDynaNodeBase* pNode) -> VirtualBranch* 
	{
		for (VirtualBranch* vb = pBegin; vb < pEnd; vb++)
			if (vb->pNode == pNode)
				return vb;

		return nullptr;
	};

	// начинаем считать количество ненулевых элементов матрицы
	pZeroSuperNode->nZcount = pZeroSuperNode->LFMatrix.size();
	// заполняем списки ветвей для каждого из узлов суперузла
	for (auto&& node : pZeroSuperNode->LFMatrix)
	{
		CLinkPtrCount* pBranchLink = node->GetLink(0);
		CDevice** ppDevice(nullptr);

		auto& ZeroLF{ node->ZeroLF };

		// между этими указателями список виртуальных ветей данного узла
		ZeroLF.pVirtualBranchesBegin = ZeroLF.pVirtualBranchesEnd = pHead;
		ZeroLF.pVirtualZeroBranchesBegin = ZeroLF.pVirtualZeroBranchesEnd = pZeroHead;
		ZeroLF.SlackInjection = ZeroLF.Yii = 0.0;

		// ищем ветви, соединяющие текущий узел с другими суперузлами
		while (pBranchLink->In(ppDevice))
		{
			const auto& pBranch{ static_cast<CDynaBranch*>(*ppDevice) };

			// интересуют только включенные ветви - так или иначе связывающие узлы 
			if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON)
			{
				const auto& pOppNode{ pBranch->GetOppositeNode(node) };

				if (!pBranch->IsZeroImpedance())
				{
					// если ветвь не в данном суперузле, добавляем ее в список виртуальных ветвей 
					const auto& Ykm = pBranch->OppositeY(node);
					// нам нужны связи между суперузлами, так как только они индексированы в матрице
					// поэтому все связи эквивалентируем к связям между суперузлами
					const auto& pOppSuperNode{ pOppNode->GetSuperNode() };
					// ищем не было ли уже добавлено виртуальной ветви на этот узел от выбранного суперзула
					VirtualBranch* pDup{ FindParallel(ZeroLF.pVirtualBranchesBegin, ZeroLF.pVirtualBranchesEnd, pOppSuperNode) };
					// если ветвь на такой суперузел уже была добавлена, добавляем проводимость к существующей
					if (pDup)
						pDup->Y += Ykm;
					else
					{
						// если ветви не было добавлено - добавляем новую
						ZeroLF.pVirtualBranchesEnd->pNode = pOppSuperNode;
						ZeroLF.pVirtualBranchesEnd->Y = Ykm;
						ZeroLF.pVirtualBranchesEnd++;
					}
				}
				else
				{
					// ветвь внутри суперузла
					// учитываем ветвь в собственной проводимости

					ZeroLF.Yii += 1.0;

					// если ветвь приходит от базисного узла
					if (pOppNode == pMaxRankNode.first)
					{
						// учитываем инъекцию от базисного узла
						ZeroLF.SlackInjection += 1.0;
					}
					else
					{
						// если ветвь в суперузле но не от базисного узла, добавляем ее в список нулевых виртуальных ветвей
						VirtualBranch* pDup{ FindParallel(ZeroLF.pVirtualZeroBranchesBegin, ZeroLF.pVirtualZeroBranchesEnd, pOppNode) };
						// учитываем, если ветвь параллельная
						if (pDup)
							pDup->Y += 1.0;
						else
						{
							ZeroLF.pVirtualZeroBranchesEnd->pNode = pOppNode;
							ZeroLF.pVirtualZeroBranchesEnd->Y = 1.0;
							ZeroLF.pVirtualZeroBranchesEnd++;
							// и учитываем в количестве ненулевых элементов
							pZeroSuperNode->nZcount++;
						}
					}
				}
			}
		}
		pHead = ZeroLF.pVirtualBranchesEnd;
		pZeroHead = ZeroLF.pVirtualZeroBranchesEnd;
	}
}

void CDynaNodeBase::RequireSuperNodeLF()
{
	if (m_pSuperNodeParent)
		throw dfw2error(fmt::format("CDynaNodeBase::RequireSuperNodeLF - node {} is not super node", GetVerbalName()));
	static_cast<CDynaNodeContainer*>(m_pContainer)->RequireSuperNodeLF(this);
}

void CDynaNodeBase::SuperNodeLoadFlowYU(CDynaModel* pDynaModel)
{
	
	CreateZeroLoadFlowData();
	if (!ZeroLF.ZeroSupeNode)
		return;

	const double Vmin{ pDynaModel->GetLRCToShuntVmin() };
	const auto& Matrix{ ZeroLF.ZeroSupeNode->LFMatrix };
	const ptrdiff_t nBlockSize{ static_cast<ptrdiff_t>(Matrix.size()) };

	KLUWrapper<std::complex<double>> klu;
	klu.SetSize(nBlockSize, ZeroLF.ZeroSupeNode->nZcount);

	ptrdiff_t* pAi = klu.Ai();	// строки
	ptrdiff_t* pAp = klu.Ap();	// столбцы
	double* pAx = klu.Ax();
	double* pB = klu.B();

	const ptrdiff_t* cpAi{ pAi };
	const ptrdiff_t* cpAp{ pAp };
	const double* cpAx{ pAx };
	const double* cpB{ pB };

	*pAi = 0;
	pAi++;
	for (auto&& node : Matrix)
	{
		const auto& ZeroLF{ node->ZeroLF };

		*pAx = ZeroLF.Yii;	pAx += 2;
		*pAp = ZeroLF.m_nSuperNodeLFIndex;	pAp++;
		for (const VirtualBranch* pV = ZeroLF.pVirtualZeroBranchesBegin; pV < ZeroLF.pVirtualZeroBranchesEnd; pV++)
		{
			*pAx = -pV->Y.real();	pAx += 2;
			*pAp = pV->pNode->ZeroLF.m_nSuperNodeLFIndex;	pAp++;
		}
		*pAi = *(pAi - 1) + (ZeroLF.pVirtualZeroBranchesEnd - ZeroLF.pVirtualZeroBranchesBegin) + 1;
		pAi++;
	}
	
	pB = klu.B();

	double Vsq{ 0.0 };
	for (const auto& node : Matrix)
	{
		cplx Is{ node->GetSelfImbInotSuper(Vmin, Vsq) };

		// добавляем предварительно рассчитанную 
		// инъекцию от связей с базисным узлом
		Is -= node->ZeroLF.SlackInjection;
				
		// собираем сумму перетоков по виртуальным ветвям узла
		// ветви находятся в общем для всего узла векторе и разделены
		// вилками указателей pBranchesBegin <  pBranchesEnd
		for (const VirtualBranch* vb = node->ZeroLF.pVirtualBranchesBegin; vb < node->ZeroLF.pVirtualBranchesEnd; vb++)
			Is -= vb->Y * cplx(vb->pNode->Vre, vb->pNode->Vim);

		CDevice** ppGen{ nullptr };
		CLinkPtrCount* pGenLink{ node->GetLink(1) };
		while (pGenLink->InMatrix(ppGen))
		{
			const auto& pGen{ static_cast<CDynaPowerInjector*>(*ppGen) };
			Is -= cplx(pGen->Ire, pGen->Iim);
		}



		*pB = -Is.real();	pB++;
		*pB = -Is.imag();	pB++;
	}

	pB = klu.B();
	klu.Solve();

	// результат решения потокораспределения
	// вводим в узлы суперузла

	for (const auto& node : Matrix)
	{
		node->ZeroLF.vRe = *pB;		pB++;
		node->ZeroLF.vIm = *pB;		pB++;
	}

	// рассчитываем потоки в ветвях
	// 
	// комплекс напряжения в суперузле - просто напряжение одного из узлов
	// используется для расчета токов в шунтах и мощностей
	cplx Vs{ Vre, Vim };
	for (const VirtualZeroBranch* pZb = m_VirtualZeroBranchBegin; pZb < m_VirtualZeroBranchEnd; pZb++)
	{
		const auto& pBranch{ pZb->pBranch };
		const auto& pNode1{ pBranch->m_pNodeIp->ZeroLF };
		const auto& pNode2{ pBranch->m_pNodeIq->ZeroLF };

		// рассчитываем ток ветви по индикаторам
		pBranch->Se = pBranch->Sb = cplx(pNode2.vRe - pNode1.vRe, pNode2.vIm - pNode1.vIm);
		// добавляем токи от шунтов с учетом односторонних отключений
		pBranch->Sb += (pBranch->Yip  - pBranch->Yips) * Vs;
		pBranch->Se += (pBranch->Yiqs - pBranch->Yiq ) * Vs;
		// рассчитываем мощности в начале и в конце
		pBranch->Sb = std::conj(pBranch->Sb) * Vs;
		pBranch->Se = std::conj(pBranch->Se) * Vs;
	}
}

void CDynaNodeBase::CalculateZeroLFBranches()
{
	if (m_pSuperNodeParent)
		return;

	// для ветвей с нулевым сопротивлением, параллельных основным ветвям копируем потоки основных,
	// так как потоки основных рассчитаны как доля параллельных потоков. Все потоки по паралелльным цепям
	// с сопротивлениями ниже минимальных будут одинаковы

	for (VirtualZeroBranch* pZb = m_VirtualZeroBranchParallelsBegin; pZb < m_VirtualZeroBranchEnd; pZb++)
	{
		const auto& pBranch{ pZb->pBranch };
		pBranch->Sb = pBranch->Se = 0.0;
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON)
			pBranch->Sb = pBranch->Se = pZb->pParallelTo->Sb;
	}

	// Далее добавляем потоки от шунтов для всех ветвей, включая параллельные
	for (VirtualZeroBranch* pZb = m_VirtualZeroBranchBegin; pZb < m_VirtualZeroBranchEnd; pZb++)
	{
		const auto& pBranch{ pZb->pBranch };
		if (pBranch->m_BranchState == CDynaBranch::BranchState::BRANCH_ON)
		{
			pZb->pBranch->Sb -= cplx(pBranch->m_pNodeIp->V * pBranch->m_pNodeIp->V * cplx(pBranch->GIp, -pBranch->BIp));
			pZb->pBranch->Se += cplx(pBranch->m_pNodeIq->V * pBranch->m_pNodeIq->V * cplx(pBranch->GIq, -pBranch->BIq));
		}
	}
}

void CDynaNodeBase::SuperNodeLoadFlow(CDynaModel *pDynaModel)
{
	//if (m_Id == 1101)
	{
		SuperNodeLoadFlowYU(pDynaModel);
		return;
	}

	if (m_pSuperNodeParent)
		return; // это не суперузел

	CLinkPtrCount *pSuperNodeLink = GetSuperLink(0);
	if (!pSuperNodeLink->m_nCount)
		return; // это суперузел, но у него нет связей

	// Создаем граф с узлами ребрами от типов расчетных узлов и ветвей
	using GraphType = GraphCycle<CDynaNodeBase*, VirtualZeroBranch*>;
	using NodeType = GraphType::GraphNodeBase;
	using EdgeType = GraphType::GraphEdgeBase;
	// Создаем вектор внутренних узлов суперузла, включая узел представитель
	std::unique_ptr<NodeType[]> pGraphNodes = std::make_unique<NodeType[]>(pSuperNodeLink->m_nCount + 1);
	// Создаем вектор ребер за исключением параллельных ветвей
	std::unique_ptr<EdgeType[]> pGraphEdges = std::make_unique<EdgeType[]>(m_VirtualZeroBranchParallelsBegin - m_VirtualZeroBranchBegin);
	CDevice** ppNodeEnd{ pSuperNodeLink->m_pPointer + pSuperNodeLink->m_nCount };
	NodeType *pNode = pGraphNodes.get();
	GraphType gc;
	// Вводим узлы в граф. В качестве идентификатора узла используем адрес объекта
	for (CDevice **ppDev = pSuperNodeLink->m_pPointer; ppDev < ppNodeEnd; ppDev++, pNode++)
		gc.AddNode(pNode->SetId(static_cast<CDynaNodeBase*>(*ppDev)));
	// добавляем узел представитель
	gc.AddNode(pNode->SetId(this));

	// Вводим в граф ребра
	EdgeType *pEdge = pGraphEdges.get();
	for (VirtualZeroBranch *pZb = m_VirtualZeroBranchBegin; pZb < m_VirtualZeroBranchParallelsBegin; pZb++, pEdge++)
		gc.AddEdge(pEdge->SetIds(pZb->pBranch->m_pNodeIp, pZb->pBranch->m_pNodeIq, pZb));

	GraphType::CyclesType Cycles;
	// Определяем циклы
	gc.GenerateCycles(Cycles);
	if (!Cycles.empty())
	{
			
		/*pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, "Cycles");
		pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex("%s", GetVerbalName()));
		for (CDevice **ppDev = pSuperNodeLink->m_pPointer; ppDev < ppNodeEnd; ppDev++, pNode++)
			pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex("%s", (*ppDev)->GetVerbalName()));

		for (VirtualZeroBranch *pZb = m_VirtualZeroBranchBegin; pZb < m_VirtualZeroBranchParallelsBegin; pZb++, pEdge++)
			pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex("%s", pZb->pBranch->GetVerbalName()));

		for (auto&& cycle : Cycles)
		{
			pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, "Cycle");
			for (auto&& vb : cycle)
			{
				pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, Cex("%s %s", vb.m_bDirect ? "+" : "-", vb.m_pEdge->m_IdBranch->pBranch->GetVerbalName()));
			}
		}
		*/
	}

	// Формируем матрицу для расчета токов в ветвях
	// Количество уравнений равно количеству узлов - 1 + количество контуров = количество ветвей внутри суперузла

	// исключаем из списка узлов узел с наибольшим числом связей (можно и первый попавшийся, KLU справится, но найти
	// такой узел легко
	const GraphType::GraphNodeBase* pMaxRangeNode{ gc.GetMaxRankNode() };

	// количество ненулевых элементов равно количеству ребер * 2 - количество связей исключенного узла + количество ребер в циклах
	ptrdiff_t nNz(gc.Edges().size() * 2 - pMaxRangeNode->Rank());
	for (auto&& cycle : Cycles)
		nNz += cycle.size();

	KLUWrapper<std::complex<double>> klu;
	klu.SetSize(gc.Edges().size(), nNz);

	ptrdiff_t* pAi = klu.Ai();
	ptrdiff_t* pAp = klu.Ap();
	double* pAx = klu.Ax();
	double* pB = klu.B();

	ptrdiff_t* cpAp = pAp;
	ptrdiff_t* cpAi = pAi;
	double* cpB = pB;
	double* cpAx = pAx;

		
	*pAi = 0;	// первая строка начинается с индекса 0

	// мнимую часть коэффициентов матрицы обнуляем
	// вещественная будет -1, 0 или +1
	double* pz = pAx + nNz * 2 - 1;
	while (pz > pAx)
	{
		*pz = 0.0;
		pz -= 2;
	}

	// уравнения по I зК для узлов
	for (auto&& node : gc.Nodes())
	{
		// исключаем узел с максимальным числом связей
		if (node == pMaxRangeNode)
			continue;
						
		ptrdiff_t nCurrentRow = *pAi;		// запоминаем начало текущей строки
		pAi++;								// передвигаем указатель на следующую строку
		for (EdgeType** edge = node->m_ppEdgesBegin; edge < node->m_ppEdgesEnd; edge++)
		{
			*pAp = (*edge)->m_nIndex;							// формируем номер столбца для ребра узла
			*pAx = (*edge)->m_pBegin == node ? 1.0 : -1.0;;		// формируем вещественный коэффициент в матрице по направлению ребра к узлу
			pAx += 2;											// переходим к следущему вещественному коэффициенту
			pAp++;
		}
		// считаем сколько ненулевых элементов в строке и формируем индекс следующей строки
		*pAi = nCurrentRow + node->m_ppEdgesEnd - node->m_ppEdgesBegin;

		// рассчитываем инъекцию в узле
		// нагрузка по СХН
		const auto& pInSuperNode{ static_cast<CDynaNodeBase*>(node->m_Id) };
		pInSuperNode->GetPnrQnr();
		const cplx Unode{ pInSuperNode->Vre, pInSuperNode->Vim };
		const double Vs2{ pInSuperNode->V * pInSuperNode->V };
		cplx S{ pInSuperNode->Pnr - pInSuperNode->Pgr - Vs2 * pInSuperNode->Yii.real(),
				pInSuperNode->Qnr - pInSuperNode->Qgr + Vs2 * pInSuperNode->Yii.imag() };

		CLinkPtrCount* pBranchLink = pInSuperNode->GetLink(0);
		CDevice** ppDevice(nullptr);
		// рассчитываем сумму потоков по инцидентным ветвям
		cplx I;
		while (pBranchLink->In(ppDevice))
		{
			const auto& pBranch{ static_cast<CDynaBranch*>(*ppDevice) };
			I += pBranch->CurrentFrom(pInSuperNode);
		}
		S -= std::conj(I) * Unode;

		*pB = S.real();			pB++;
		*pB = S.imag();			pB++;
	}

	// уравнения для контуров
	for (auto&& cycle : Cycles)
	{
		ptrdiff_t nCurrentRow = *pAi;
		pAi++;
		for (auto& edge : cycle)
		{
			*pAp = edge.m_pEdge->m_nIndex;				// формируем номер столбца для ребра в контуре
			*pAx = edge.m_bDirect ? 1.0 : -1.0;			// формируем вещественный коэффициент в матрице по направлению ребра в контуре
			pAx += 2;									// переходим к следующему вещественому коэффициенту
			pAp++;
		}
		*pAi = nCurrentRow + cycle.size();				// следующая строка матрицы начинается через количество элементов равное количеству ребер контура
		*pB = 0.0;			pB++;
		*pB = 0.0;			pB++;
	}

	pB = klu.B();

	klu.Solve();


	// вводим в ветви с нулевым сопротивлением поток мощности, рассчитанный
	// по контурным уравнениям

	// !!!!!!!!!!!!!!!!!!!!!! Надо что-то придумать для односторонних отключений !!!!!!!!!!!!!!!!!!!!!!!

	for (auto&& edge : gc.Edges())
	{
		VirtualZeroBranch* pVirtualBranch(static_cast<VirtualZeroBranch*>(edge->m_IdBranch));
		const auto& pBranch{ pVirtualBranch->pBranch };
		_ASSERTE(pBranch->IsZeroImpedance());
		// учитываем, что у ветвей могут быть параллельные. Поток будет разделен по параллельным
		// ветвям. Для ветвей с ненулевым сопротивлением внутри суперузлов
		// обычный расчет потока по напряжениям даст ноль, так как напряжения узлов одинаковые
		cplx sb(*pB / pVirtualBranch->nParallelCount);  pB++;
		sb.imag(*pB / pVirtualBranch->nParallelCount);	pB++;
		// считаем потоки в конце и в начале с учетом без учета шунтов
		pBranch->Sb = pBranch->Se = sb;
	}

	CalculateZeroLFBranches();
}

void CDynaNodeBase::DeviceProperties(CDeviceContainerProperties& props)
{
	props.eDeviceType = DEVTYPE_NODE;
	props.SetType(DEVTYPE_NODE);

	// к узлу могут быть прилинкованы ветви много к одному
	props.AddLinkFrom(DEVTYPE_BRANCH, DLM_MULTI, DPD_SLAVE);
	// и инжекторы много к одному. Для них узел выступает ведущим
	props.AddLinkFrom(DEVTYPE_POWER_INJECTOR, DLM_MULTI, DPD_SLAVE);

	props.nEquationsCount = CDynaNodeBase::VARS::V_LAST;
	props.bPredict = props.bNewtonUpdate = true;
	props.m_VarMap.insert({ CDynaNodeBase::m_cszVre, CVarIndex(V_RE, VARUNIT_KVOLTS) });
	props.m_VarMap.insert({ CDynaNodeBase::m_cszVim, CVarIndex(V_IM, VARUNIT_KVOLTS) });
	props.m_VarMap.insert({ CDynaNodeBase::m_cszV, CVarIndex(V_V, VARUNIT_KVOLTS) });
	props.m_VarAliasMap.insert({ "vras", { CDynaNodeBase::m_cszV }});
}

void CDynaNode::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaNodeBase::DeviceProperties(props);
	props.SetClassName(CDeviceContainerProperties::m_cszNameNode, CDeviceContainerProperties::m_cszSysNameNode);
	props.nEquationsCount = CDynaNode::VARS::V_LAST;
	props.m_VarMap.insert({ CDynaNode::m_cszS, CVarIndex(V_S, VARUNIT_PU) });
	props.m_VarMap.insert({ CDynaNodeBase::m_cszDelta, CVarIndex(V_DELTA, VARUNIT_RADIANS) });

	/*
	props.m_VarMap.insert(make_pair("Sip", CVarIndex(V_SIP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair("Cop", CVarIndex(V_COP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair("Sv", CVarIndex(V_SV, VARUNIT_PU)));
	*/

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaNode>>();

	props.m_lstAliases.push_back(CDeviceContainerProperties::m_cszAliasNode);
	props.m_ConstVarMap.insert({ CDynaNode::m_cszGsh, CConstVarIndex(CDynaNode::C_GSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });
	props.m_ConstVarMap.insert({ CDynaNode::m_cszBsh, CConstVarIndex(CDynaNode::C_BSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });
}

void CSynchroZone::DeviceProperties(CDeviceContainerProperties& props)
{
	props.bVolatile = true;
	props.eDeviceType = DEVTYPE_SYNCZONE;
	props.nEquationsCount = CSynchroZone::VARS::V_LAST;
	props.m_VarMap.insert(std::make_pair(CDynaNode::m_cszS, CVarIndex(0,VARUNIT_PU)));
	props.DeviceFactory = std::make_unique<CDeviceFactory<CSynchroZone>>();
}


void CDynaNodeBase::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty(CDevice::m_cszname, TypedSerializedValue::eValueType::VT_NAME);
	AddStateProperty(Serializer);
	Serializer->AddEnumProperty("tip", new CSerializerAdapterEnum(m_eLFNodeType, m_cszLFNodeTypeNames));
	Serializer->AddProperty("ny", TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty("vras", V, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty("delta", Delta, eVARUNITS::VARUNIT_DEGREES);
	Serializer->AddProperty("pnr", Pn, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty("qnr", Qn, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("pn", Pnr, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty("qn", Qnr, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("pg", Pg, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty("qg", Qg, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("gsh", G, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty("grk", Gr0, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddProperty("nrk", Nr, eVARUNITS::VARUNIT_PIECES);
	Serializer->AddProperty("vzd", LFVref, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty("qmin", LFQmin, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("qmax", LFQmax, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty("uhom", Unom, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty("bsh", B, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("brk", Br0, eVARUNITS::VARUNIT_SIEMENS, -1.0);
	Serializer->AddProperty("LRCLFId", LRCLoadFlowId);
	Serializer->AddProperty("LRCTransId", LRCTransientId);
	Serializer->AddState("Vre", Vre, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Vim", Vim, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("pgr", Vim, eVARUNITS::VARUNIT_MW);
	Serializer->AddState("qgr", Vim, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState("LRCShuntPartP", dLRCShuntPartP, eVARUNITS::VARUNIT_MW);
	Serializer->AddState("LRCShuntPartQ", dLRCShuntPartQ, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState("LRCShuntPartPSuper", dLRCShuntPartPSuper, eVARUNITS::VARUNIT_MW);
	Serializer->AddState("LRCShuntPartQSuper", dLRCShuntPartQSuper, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddState("Gshunt", Gshunt, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("Bshunt", Bshunt, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("InMetallicSC", m_bInMetallicSC);
	Serializer->AddState("InLowVoltage", m_bLowVoltage);
	Serializer->AddState("SavedInLowVoltage", m_bSavedLowVoltage);
	Serializer->AddState("LRCVicinity", dLRCVicinity);
	Serializer->AddState("dLRCPn", dLRCPn);
	Serializer->AddState("dLRCQn", dLRCQn);
	Serializer->AddState("dLRCPg", dLRCPg);
	Serializer->AddState("dLRCQg", dLRCQg);
	Serializer->AddState("Vold", Vold, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Yii", Yii, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("YiiSuper", YiiSuper, eVARUNITS::VARUNIT_SIEMENS);
}

void CDynaNode::UpdateSerializer(CSerializerBase* Serializer)
{
	CDynaNodeBase::UpdateSerializer(Serializer);
	Serializer->AddState("SLag", Lag);
	Serializer->AddState("S", S);
}

VariableIndexRefVec& CDynaNodeBase::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ V, Vre, Vim }, ChildVec));
}

VariableIndexRefVec& CDynaNode::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDynaNodeBase::GetVariables(JoinVariables({ Delta, Lag, S }, ChildVec));
}


void CDynaNodeContainer::LinkToReactors(CDeviceContainer& containerReactors)
{
	CDynaBranchContainer* pBranchContainer = static_cast<CDynaBranchContainer*>(GetModel()->GetDeviceContainer(eDFW2DEVICETYPE::DEVTYPE_BRANCH));

	for (const auto& reactor : containerReactors)
	{
		const auto& pReactor{ static_cast<const CDynaReactor*>(reactor) };

		// отбираем шинные реакторы
		if (pReactor->Type == 0)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(GetDevice(pReactor->HeadNode)) };
			if (pNode)
				pNode->reactors.push_back(pReactor);
			else
			{
				Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszNodeNotFoundForReactor,
					pReactor->HeadNode,
					reactor->GetVerbalName()));
			}
		}
		else if (pReactor->Placement == 0) // а также добавляем реакторы с ветвей, подключенные до выключателя
		{
			// проверяем наличие ветви, заданной в реакторе
			const auto& pBranch{ pBranchContainer->GetByKey({ pReactor->HeadNode, pReactor->TailNode, pReactor->ParrBranch }) };
			// если ветвь найдена
			if (pBranch)
			{
				// добавляем реактор к узлу начала или конца в зависимости от типа реактора
				const auto& pNode{ static_cast<CDynaNodeBase*>(GetDevice(pReactor->Type == 1 ? pReactor->HeadNode : pReactor->TailNode)) };
				if (pNode)
					pNode->reactors.push_back(pReactor);
			}
			else
			{
				Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszBranchNotFoundForReactor,
					pReactor->HeadNode,
					pReactor->TailNode,
					pReactor->ParrBranch,
					reactor->GetVerbalName()));
			}
		}
	}
}

void CDynaNodeContainer::LinkToLRCs(CDeviceContainer& containerLRC)
{
	static_cast<CDynaLRCContainer*>(&containerLRC)->CreateFromSerialized();

	for (auto&& dev : m_DevVec)
	{
		const auto& pNode = static_cast<CDynaNode*>(dev);
		if (pNode->LRCLoadFlowId > 0)
		{
			pNode->m_pLRCLF = static_cast<CDynaLRC*>(containerLRC.GetDevice(pNode->LRCLoadFlowId));
			if (!pNode->m_pLRCLF)
				Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLRCIdNotFound, pNode->LRCLoadFlowId, pNode->GetVerbalName()));
		}

		pNode->m_pLRC = static_cast<CDynaLRC*>(containerLRC.GetDevice(pNode->LRCTransientId));
		if (!pNode->m_pLRC)
			Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLRCIdNotFound, pNode->LRCTransientId, pNode->GetVerbalName()));
			
	}
}

void CDynaNodeContainer::RequireSuperNodeLF(CDynaNodeBase *pSuperNode)
{
	if (pSuperNode->m_pSuperNodeParent)
		throw dfw2error(fmt::format("CDynaNodeBase::RequireSuperNodeLF - node {} is not super node", pSuperNode->GetVerbalName()));
	m_ZeroLFSet.insert(pSuperNode);
}
