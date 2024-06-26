﻿#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"
#include "GraphCycle.h"
#include "DynaGeneratorMotion.h"
#include "BranchMeasures.h"
#include <immintrin.h>

using namespace DFW2;

#define LOW_VOLTAGE 0.1	// может быть сделать в о.е. ? Что будет с узлами с низким Uном ?
#define LOW_VOLTAGE_HYST LOW_VOLTAGE * 0.1

// константы узла - проводимость шунта
double* CDynaNodeBase::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double* p{ nullptr };
	switch (nVarIndex)
	{
		MAP_VARIABLE(Bshunt, C_BSH)
		MAP_VARIABLE(Gshunt, C_GSH)
		MAP_VARIABLE(SyncDelta_, C_SYNCDELTA)
		MAP_VARIABLE(Pn, C_PLOAD0)
		MAP_VARIABLE(Qn, C_QLOAD0)
	}
	return p;
}

double* CDynaNodeBase::GetVariablePtr(ptrdiff_t nVarIndex)
{
	return &GetVariable(nVarIndex).Value;
}

VariableIndexRefVec& CDynaNodeBase::GetVariables(VariableIndexRefVec& ChildVec)
{
	return CDevice::GetVariables(JoinVariables({ V, Vre, Vim }, ChildVec));
}

// запоминает значение модуля напряжения с предыдущей итерации
// и рассчитываем напряжение в декартовых координатах
void CDynaNodeBase::UpdateVreVim()
{
	Vold = V;
	VreVim = std::polar(static_cast<double>(V), static_cast<double>(Delta));
	FromComplex(Vre, Vim, VreVim);
}

void CDynaNodeBase::UpdateVDelta()
{
	VreVim = { Vre, Vim };
	V = std::abs(VreVim);
	Delta = std::arg(VreVim);
}

void CDynaNodeBase::UpdateVreVimSuper()
{
	UpdateVreVim();
	const CLinkPtrCount* const pLink{ GetSuperLink(0) };
	LinkWalker<CDynaNodeBase> pSlaveNode;
	while (pLink->In(pSlaveNode))
	{
		pSlaveNode->V = V;
		pSlaveNode->Delta = Delta;
		pSlaveNode->UpdateVreVim();
	}
}

// возвращает ток узла от нагрузки/генерации/шунтов
cplx CDynaNodeBase::GetSelfImbInotSuper(double& Vsq)
{
	// рассчитываем модуль напряжения по составляющим,
	// так как модуль из уравнения может неточно соответствовать
	// сумме составляющих пока Ньютон не сошелся
	double V2{ Vre * Vre + Vim * Vim };
	Vsq = std::sqrt(V2);
	cplx  cI{ Iconst };
	
	//double Ire{ Iconst.real() }, Iim{ Iconst.imag() };

	if (!InMetallicSC)
	{
		// если не в металлическом КЗ, обрабатываем нагрузку и генерацию, заданные в узле
 	    // если напряжение меньше VshuntPartBelow переходим на шунт и обнуляем
		// расчетные нагрузку и генерацию чтобы исключить мощность из уравнений полностью
		// Проверяем напряжение с учетом радиуса сглаживания,
		// радиус сглаживания выражаем в именованных  единицах относительно
		// номинального напряжения СХН
		if ((Vsq + LRCVicinity * V0) < VshuntPartBelow)
		{
			cI += std::conj(LRCShuntPart) * VreVim;
			//Ire += dLRCShuntPartP * Vre + dLRCShuntPartQ * Vim;
			//Iim -= dLRCShuntPartQ * Vre - dLRCShuntPartP * Vim;

#ifdef _DEBUG
			// проверка
			GetPnrQnr(Vsq);
			const auto& Atol{ GetModel()->Parameters().m_dAtol };
			cplx S{ std::conj(cI - Iconst) * VreVim };
			cplx dS{ S - cplx(Pnr - Pgr,Qnr - Qgr) };
			if (std::abs(dS.real()) > Atol || std::abs(dS.imag()) > Atol)
			{
				_ASSERTE(0);
				GetPnrQnrSuper();
			}
#endif
			Pgr = Qgr = Pnr = Qnr = 0.0;
			// нагрузки и генерации в мощности больше нет, они перенесены в ток
		}
		else
			// если напряжение выше чем переход на шунт
			// рассчитываем нагрузку/генерацию по СХН по заданному модулю
			GetPnrQnr(Vsq);
	}

	// добавляем токи собственной проводимости и токи ветвей
	cI -= Yii * VreVim;
	//Ire -= Yii.real() * Vre - Yii.imag() * Vim;
	//Iim -= Yii.imag() * Vre + Yii.real() * Vim;

	if (!GetSuperNode()->LowVoltage)
	{
		// добавляем токи от нагрузки (если напряжение не очень низкое)
		//const double Pk{ Pnr - Pgr }, Qk{ Qnr - Qgr };
		//Ire += (Pk * Vre + Qk * Vim) / V2;
		//Iim += (Pk * Vim - Qk * Vre) / V2;
		cplx cS{ Pnr - Pgr, Qgr - Qnr };
		cS /= V2;
		cI += cS * VreVim;
	}
#ifdef _DEBUG
	else
		_ASSERTE(std::abs(Pnr - Pgr) < Consts::epsilon && std::abs(Qnr - Qgr) < Consts::epsilon);
#endif
	//_ASSERTE(Equal(Ire, cI.real()) && Equal(Iim, cI.imag()));
	return cI;
}

cplx CDynaNodeBase::GetSelfImbISuper(double& Vsq)
{
	// рассчитываем модуль напряжения по составляющим,
	// так как модуль из уравнения может неточно соответствовать
	// сумме составляющих пока Ньютон не сошелся
	double V2{ Vre * Vre + Vim * Vim };
	Vsq = std::sqrt(V2);
	cplx  cI{ IconstSuper };

	//double Ire{ IconstSuper.real() }, Iim{ IconstSuper.imag() };

	if (!InMetallicSC)
	{
		// если не в металлическом КЗ, обрабатываем нагрузку и генерацию, заданные в узле
		// если напряжение меньше VshuntPartBelow переходим на шунт и обнуляем
		// расчетные нагрузку и генерацию чтобы исключить мощность из уравнений полностью
		// Проверяем напряжение с учетом радиуса сглаживания,
		// радиус сглаживания выражаем в именованных  единицах относительно
		// номинального напряжения СХН
		
		if ((Vsq + LRCVicinity * V0Super) < VshuntPartBelowSuper)
		{
			cI += std::conj(LRCShuntPartSuper) * VreVim;
			//Ire -= -dLRCShuntPartPSuper * Vre - dLRCShuntPartQSuper * Vim;
			//Iim -=  dLRCShuntPartQSuper * Vre - dLRCShuntPartPSuper * Vim;

#ifdef _DEBUG
			// проверка
			GetPnrQnrSuper(Vsq);
			cplx S{ std::conj(cI - IconstSuper) * VreVim };
			cplx dS{ S - cplx(Pnr - Pgr, Qnr - Qgr) };
			const auto& Atol{ GetModel()->Parameters().m_dAtol };
			if (std::abs(dS.real()) > Atol || std::abs(dS.imag()) > Atol)
			{
				_ASSERTE(0);
				double Vx = std::sqrt(Vre * Vre + Vim * Vim);
				GetPnrQnrSuper(Vsq);
			}
#endif
			Pgr = Qgr = Pnr = Qnr = 0.0;
			// нагрузки и генерации в мощности больше нет, они перенесены в ток
		}
		else // рассчитываем нагрузку/генерацию по СХН по заданному модулю
			GetPnrQnrSuper(Vsq);
	}

	// добавляем токи собственной проводимости и токи ветвей
	cI -= YiiSuper * VreVim;
	//Ire -= YiiSuper.real() * Vre - YiiSuper.imag() * Vim;
	//Iim -= YiiSuper.imag() * Vre + YiiSuper.real() * Vim;

	if (!LowVoltage)
	{
		// добавляем токи от нагрузки (если напряжение не очень низкое)
		cplx cS{ Pnr - Pgr, Qgr - Qnr };
		cS /= V2;
		cI += cS * VreVim;
		//const double Pk{ Pnr - Pgr }, Qk{ Qnr - Qgr };
		//Ire += (Pk * Vre + Qk * Vim) / V2;
		//Iim += (Pk * Vim - Qk * Vre) / V2;
	}
#ifdef _DEBUG
	else
		_ASSERTE(std::abs(Pnr - Pgr) < Consts::epsilon && std::abs(Qnr - Qgr) < Consts::epsilon);
#endif
	//_ASSERTE(Equal(Ire, cI.real()) && Equal(Iim, cI.imag()));
	return cI;
}

void CDynaNodeBase::UpdateVDeltaSuper()
{
	UpdateVDelta();
	const CLinkPtrCount* const pLink{ GetSuperLink(0) };
	LinkWalker<CDynaNodeBase> pSlaveNode;
	while (pLink->In(pSlaveNode))
	{
		pSlaveNode->Vre = Vre;
		pSlaveNode->Vim = Vim;
		pSlaveNode->UpdateVDelta();
	}
}

// Проверяет для всех узлов суперузла напряжения перехода с СХН на шунт
// если они меньше напряжения перехода минус окрестность сглаживания - возвращает true
bool CDynaNodeBase::AllLRCsInShuntPart(double Vtest)
{
	bool bRes{ (Vtest + LRCVicinity * V0) < VshuntPartBelow };
	const CLinkPtrCount* const pLink{ GetSuperLink(0) };
	LinkWalker<CDynaNodeBase> pSlaveNode;

	while (pLink->In(pSlaveNode) && bRes)
		bRes = (Vtest + pSlaveNode->LRCVicinity * pSlaveNode->V0 ) < pSlaveNode->VshuntPartBelow;

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
	const CLinkPtrCount* const pLink{ GetSuperLink(0) };

	if (pLink->Count())
	{
		__m128d load = _mm_load_pd(&Pnr);
		__m128d gen = _mm_load_pd(&Pgr);
		__m128d lrcload = _mm_load_pd(reinterpret_cast<double(&)[2]>(dLRCLoad));
		__m128d lrcgen = _mm_load_pd(reinterpret_cast<double(&)[2]>(dLRCGen));

		LinkWalker<CDynaNodeBase> pSlaveNode;
		while (pLink->In(pSlaveNode))
		{
			pSlaveNode->FromSuperNode();
			pSlaveNode->GetPnrQnr(Vnode);

			// Pnr += pSlaveNode->Pnr;
			// Qnr += pSlaveNode->Qnr;
			load = _mm_add_pd(load, _mm_load_pd(&pSlaveNode->Pnr));
			// Pgr += pSlaveNode->Pgr;
			// Qgr += pSlaveNode->Qgr;
			gen = _mm_add_pd(gen, _mm_load_pd(&pSlaveNode->Pgr));
			// dLRCLoad += pSlaveNode->dLRCLoad;
			// dLRCGen += pSlaveNode->dLRCGen;
			lrcload = _mm_add_pd(lrcload, _mm_load_pd(reinterpret_cast<double(&)[2]>(pSlaveNode->dLRCLoad)));
			lrcgen = _mm_add_pd(lrcgen, _mm_load_pd(reinterpret_cast<double(&)[2]>(pSlaveNode->dLRCGen)));
		}

		_mm_store_pd(&Pnr, load);
		_mm_store_pd(&Pgr, gen);
		_mm_store_pd(reinterpret_cast<double(&)[2]>(dLRCLoad), lrcload);
		_mm_store_pd(reinterpret_cast<double(&)[2]>(dLRCGen), lrcgen);
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
	const double VdVnom{ Vnode / V0 };
	
	dLRCLoad = dLRCGen = 0.0;

	// если есть СХН нагрузки, рассчитываем
	// комплексную мощность и производные по напряжению

	_ASSERTE(pLRC);

	// принимаем производные СХН в re/im для активной и реактивной мощности
	double& re{ reinterpret_cast<double(&)[2]>(dLRCLoad)[0] };
	double& im{ reinterpret_cast<double(&)[2]>(dLRCLoad)[1] };

	Pnr *= pLRC->P()->GetBoth(VdVnom, re, LRCVicinity);
	Qnr *= pLRC->Q()->GetBoth(VdVnom, im, LRCVicinity);

	// производные масштабируем в номинальным мощностям и напряжению
	re *= Pn;
	im *= Qn;
	dLRCLoad /= V0;

	// если есть СХН генерации (нет привязанных генераторов, но есть заданная в УР генерация)
	// рассчитываем расчетную генерацию
	if (pLRCGen)
	{
		double& re{ reinterpret_cast<double(&)[2]>(dLRCGen)[0] };
		double& im{ reinterpret_cast<double(&)[2]>(dLRCGen)[1] };


		Pgr *= pLRCGen->P()->GetBoth(VdVnom, re, LRCVicinity); 
		re *= Pg;
		Qgr *= pLRCGen->Q()->GetBoth(VdVnom, im, LRCVicinity);
		im *= Qg;
		dLRCGen /= V0;
	}
}

void CDynaNodeBase::BuildEquations(CDynaModel *pDynaModel)
{
	const double Vre2{ Vre * Vre };
	const double Vim2{ Vim * Vim };
	double V2{ Vre2 + Vim2 };
	const double V2sq{ std::sqrt(V2) };

	double dIredVre(1.0), dIredVim(0.0), dIimdVre(0.0), dIimdVim(1.0);

	if (!InMetallicSC)
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

		if ((V2sq + LRCVicinity * V0Super) < VshuntPartBelowSuper)
		{
			_ASSERTE(pLRC);
			dIredVre +=  LRCShuntPartSuper.real();
			dIredVim +=  LRCShuntPartSuper.imag();
			dIimdVre += -LRCShuntPartSuper.imag();
			dIimdVim +=  LRCShuntPartSuper.real();
			Pgr = Qgr = Pnr = Qnr = 0.0;
			dLRCLoad = dLRCGen = 0.0;
		}
		else
			GetPnrQnrSuper(V2sq);
	}

	const CLinkPtrCount* const pGenLink = GetSuperLink(2);
	LinkWalker<CDynaPowerInjector> pGen;

	CDevice** ppGen{ nullptr };
	
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
		double dGenMatrixCoe{ InMetallicSC ? 0.0 : -1.0 };
		while (pGenLink->InMatrix(pGen))
		{
			// здесь нужно проверять находится ли генератор в матрице (другими словами включен ли он)
			// или строить суперссылку на генераторы по условию того, что они в матрице
			pDynaModel->SetElement(Vre, pGen->Ire, dGenMatrixCoe);
			pDynaModel->SetElement(Vim, pGen->Iim, dGenMatrixCoe);
		}

		if (InMetallicSC)
		{
			// если в металлическом КЗ, то производные от токов ветвей равны нулю (в строке единицы только от Vre и Vim)
			for (VirtualBranch *pV = pVirtualBranchBegin_; pV < pVirtualBranchEnd_; pV++)
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
			for (VirtualBranch *pV = pVirtualBranchBegin_; pV < pVirtualBranchEnd_; pV++)
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
		pDynaModel->CountConstElementsToSkip(Vre, Vim);
	}
	else
		// если постоянные элементы не надо обновлять, то пропускаем их и начинаем с непостоянных
		pDynaModel->SkipConstElements(Vre, Vim);

	// check low voltage
	if (LowVoltage)
	{
		pDynaModel->SetElement(V, Vre, 0.0);
		pDynaModel->SetElement(V, Vim, 0.0);
		pDynaModel->SetElement(Vre, V, 0.0);
		pDynaModel->SetElement(Vim, V, 0.0);

#ifdef _DEBUG
		// ассерты могут работать на неподготовленные данные на эстимейте
		if (!pDynaModel->EstimateBuild())
		{
			_ASSERTE(std::abs(PgVre2) < Consts::epsilon && std::abs(PgVim2) < Consts::epsilon);
			_ASSERTE(std::abs(QgVre2) < Consts::epsilon && std::abs(QgVim2) < Consts::epsilon);
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

		dLRCLoad -= dLRCGen;
		dLRCLoad /= Vn;

		const double dPdV{ (dLRCLoad.real() - 2.0 * Pk) / Vn };
		const double dQdV{ (dLRCLoad.imag() - 2.0 * Qk) / Vn };
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
}

void CDynaNodeBase::BuildRightHand(CDynaModel *pDynaModel)
{
	// в узле может быть уже известный постоянный ток
	//double Ire(IconstSuper.real()), Iim(IconstSuper.imag()), 
	double dV{0.0};

/*	if (Id_ == 6400 && (pDynaModel->GetIntegrationStepNumber() >= 114 && pDynaModel->GetIntegrationStepNumber() <= 115))
		pDynaModel->DebugDump(*this, Vre, Vim);*/

	alignas(32) cplx cI{ IconstSuper };

	if (!InMetallicSC)
	{
		// если не в металлическом КЗ, обрабатываем нагрузку и генерацию,
		// заданные в узле
		double V2sq{ 0.0 };

		cI = GetSelfImbISuper(V2sq);
		__m128d sI = _mm_load_pd(reinterpret_cast<double(&)[2]>(cI));

		//FromComplex(Ire, Iim, cI);

		if (!LowVoltage)
			dV = V - V2sq;

		// обходим генераторы
		const CLinkPtrCount* const pGenLink{ GetSuperLink(2) };
		LinkWalker<CDynaPowerInjector> pGen;

		while (pGenLink->InMatrix(pGen))
		{
			sI = _mm_sub_pd(sI, _mm_set_pd(pGen->Iim, pGen->Ire));
			//cI -= cplx(pGen->Ire, pGen->Iim);
			//Ire -= pGen->Ire;
			//Iim -= pGen->Iim;
		}


		__m128d neg = _mm_setr_pd(0.0, -0.0);

		for (VirtualBranch *pV = pVirtualBranchBegin_; pV < pVirtualBranchEnd_; pV++)
		{
			__m128d yb = _mm_load_pd(reinterpret_cast<double(&)[2]>(pV->Y));
			__m128d ov = _mm_load_pd(reinterpret_cast<double(&)[2]>(pV->pNode->VreVim));

			/*if (Id_ == 6400 && (pDynaModel->GetIntegrationStepNumber() >= 114 && pDynaModel->GetIntegrationStepNumber() <= 115))
				pDynaModel->DebugDump(*this, pV->pNode->Vre, pV->pNode->Vim);*/

			// v1 = a + jb ; v2 = c +jd; v1*v2 = (a*c - b*d) + j(a*d + b*c)

			__m128d v3 = _mm_mul_pd(yb, ov);	// multiply v1 * v2	 |a*c|b*d|
			yb = _mm_permute_pd(yb, 0x5);		// shuffle v1 |b|a|
			ov = _mm_xor_pd(ov, neg);			// conjugate v2 |c|-d|
			__m128d v4 = _mm_mul_pd(yb, ov);	// multiply modified v1 and v2 |b*c|-a*d|
			yb = _mm_hsub_pd(v3, v4);			// horizontally subtract the elements in v3 and v4 |a*c-b*d|b*c+a*d|

			sI = _mm_sub_pd(sI, yb);
			
			//cI -= pV->Y * cplx(pV->pNode->Vre, pV->pNode->Vim);
			//cI -= pV->Y * pV->pNode->VreVim;
			//Ire -= pV->Y.real() * pV->pNode->Vre - pV->Y.imag() * pV->pNode->Vim;
			//Iim -= pV->Y.imag() * pV->pNode->Vre + pV->Y.real() * pV->pNode->Vim;
		}

		_mm_store_pd(reinterpret_cast<double(&)[2]>(cI), sI);
	}

	//_ASSERTE(Equal(Ire, cI.real()) && Equal(Iim, cI.imag()));

	pDynaModel->SetFunction(V, dV);
	pDynaModel->SetFunction(Vre, cI.real());
	pDynaModel->SetFunction(Vim, cI.imag());
}

void CDynaNodeBase::NewtonUpdateEquation(CDynaModel* pDynaModel)
{
	//dLRCVicinity = 5.0 * std::abs(Vold - V) / Unom;
	Vold = V;
	VreVim = { Vre, Vim };
	const CLinkPtrCount* const pLink{ GetSuperLink(0) };
	LinkWalker<CDynaNodeBase> pSlaveNode;
	while (pLink->In(pSlaveNode))
		pSlaveNode->FromSuperNode();
}

eDEVICEFUNCTIONSTATUS CDynaNodeBase::Init(CDynaModel* pDynaModel)
{
	UpdateVreVim();
	LowVoltage = V < (LOW_VOLTAGE - LOW_VOLTAGE_HYST);
	PickV0();

	if (GetLink(1)->Count() > 0)						// если к узлу подключены генераторы, то СХН генераторов не нужна и мощности генерации 0
		Pg = Qg = Pgr = Qgr = 0.0;
	else if (Consts::Equal(Pg, 0.0) && Consts::Equal(Qg, 0.0))		// если генераторы не подключены, и мощность генерации равна нулю - СХН генераторов не нужна
		Pgr = Qgr = 0.0;
	else
	{
		pLRCGen = pDynaModel->GetLRCGen();		// если есть генерация но нет генераторов - нужна СХН генераторов
		if (Pg < 0.1)
			pLRCGen = pDynaModel->GetLRCYconst();

		Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszNodeGenerationLRCSelected, 
			GetVerbalName(),
			cplx(Pg,Qg),
			pLRCGen->GetVerbalName()));
	}

	// если в узле нет СХН для динамики, подставляем СХН по умолчанию
	if (!pLRC)
		pLRC = pDynaModel->GetLRCDynamicDefault();

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

CDynaNodeContainer::CDynaNodeContainer(CDynaModel *pDynaModel) : 
									   CDeviceContainer(pDynaModel)
{
	// в контейнере требуем особой функции прогноза и обновления после
	// ньютоновской итерации
	ContainerProps_.bNewtonUpdate = ContainerProps_.bPredict = true;
}

CDynaNodeContainer::~CDynaNodeContainer()
{
	ClearSuperLinks();
}

void CDynaNodeContainer::CalcAdmittances(bool bFixNegativeZs)
{
	for (auto&& it : DevVec)
		static_cast<CDynaNodeBase*>(it)->CalcAdmittances(bFixNegativeZs);
}

// рассчитывает шунтовые части нагрузок узлов
void CDynaNodeContainer::CalculateShuntParts()
{
	// сначала считаем индивидуально для каждого узла
	for (auto&& it : DevVec)
		static_cast<CDynaNodeBase*>(it)->CalculateShuntParts();

	// затем суммируем шунтовые нагрузки в суперузлы
	for (auto&& node : DevVec)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
		// собираем проводимость
		pNode->YiiSuper = pNode->Yii;
		// собираем постоянный ток
		pNode->IconstSuper = pNode->Iconst;
		pNode->LRCShuntPartSuper = pNode->LRCShuntPart;
		pNode->VshuntPartBelowSuper = pNode->VshuntPartBelow;
		pNode->V0Super = pNode->V0;
		const CLinkPtrCount* const pLink{ SuperLinks[0].GetLink(node->InContainerIndex()) };
		LinkWalker<CDynaNodeBase> pSlaveNode;
		// суммируем собственные проводимости и шунтовые части СХН нагрузки и генерации в узле
		while (pLink->In(pSlaveNode))
		{
			pNode->YiiSuper += pSlaveNode->Yii;
			pNode->IconstSuper += pSlaveNode->Iconst;
			pNode->LRCShuntPartSuper += pSlaveNode->LRCShuntPart;
			// напряжение шунта выбираем как минимальное от всех узлов суперузла
			pNode->VshuntPartBelowSuper = (std::min)(pNode->VshuntPartBelowSuper, pSlaveNode->VshuntPartBelow);
			// номинальное напряжение СХН выбираем как максимальное от всех узлов суперузла,
			// чтобы рассчитать границу dLRCVicinity c перекрытием
			pNode->V0Super = (std::max)(pNode->V0Super, pSlaveNode->V0);
		}
	}
}

// рассчитывает нагрузки узла в виде шунта для V < Vmin СХН
void CDynaNodeBase::CalculateShuntParts()
{
	// TODO - надо разобраться с инициализацией V0 __до__ вызова этой функции
	double V02{ V0 * V0 };

	LRCShuntPart = 0.0;

	if (pLRC)
	{
		// рассчитываем шунтовую часть СХН нагрузки в узле для низких напряжений
		LRCShuntPart = { Pn * pLRC->P()->P.begin()->a2, Qn * pLRC->Q()->P.begin()->a2 };
		VshuntPartBelow = pLRC->VshuntBelow();
	}

	if (pLRCGen)
	{
		// рассчитываем шунтовую часть СХН генерации в узле для низких напряжений
		LRCShuntPart -= cplx(Pg * pLRCGen->P()->P.begin()->a2, Qg * pLRCGen->Q()->P.begin()->a2);
		VshuntPartBelow = (std::min)(pLRCGen->VshuntBelow(), VshuntPartBelow);
	}

	VshuntPartBelow *= V0;
	LRCShuntPart /= V02;
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
	const CLinkPtrCount* const pLink{ GetLink(1) };
	LinkWalker<CDynaPowerInjector> pGen;
	while (pLink->In(pGen))
	{
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

	InMetallicSC = !(std::isfinite(Yii.real()) && std::isfinite(Yii.imag()));

	if (InMetallicSC || !IsStateOn())
	{
		Vre = Vim = 0.0;
		V = Delta = 0.0;
	}
	else
	{
		const CLinkPtrCount* const pLink{ GetLink(0) };
		LinkWalker<CDynaBranch> pBranch;
		while (pLink->In(pBranch))
		{
			// проводимости ветви будут рассчитаны с учетом того,
			// что она могла быть отнесена внутрь суперузла
			pBranch->CalcAdmittances(bFixNegativeZs);
			// состояние ветви в данном случае не важно - слагаемые только к собственной
			// проводимости узла. Слагаемые сами по себе рассчитаны с учетом состояния ветви
			Yii -= (this == pBranch->pNodeIp_) ? pBranch->Yips : pBranch->Yiqs;
		}
		
		if (V < Consts::epsilon)
		{
			Vre = V = Unom;
			Vim = Delta = 0.0;
		}

		_CheckNumber(Yii.real());
		_CheckNumber(Yii.imag());
	}
}

std::pair<cplx, cplx> CDynaNodeBase::GetYI(ptrdiff_t Iteration)
{
	cplx I{ IconstSuper };
	cplx Y{ YiiSuper };		// диагональ матрицы по умолчанию собственная проводимость узла

	if (!InMetallicSC)
	{
		// для всех узлов которые включены и вне металлического КЗ

		const cplx Unode{ Vre, Vim };
		GetPnrQnrSuper();

		// Generators

		const CLinkPtrCount* const pLink{ GetSuperLink(2) };
		LinkWalker<CDynaPowerInjector> pVsource;
		// проходим по генераторам
		while (pLink->InMatrix(pVsource))
		{
			// если в узле есть хотя бы один генератор, то обнуляем мощность генерации узла
			// если в узле нет генераторов но есть мощность генерации - то она будет учитываться
			// задающим током
			// если генераторм выключен то просто пропускаем, его мощность и ток будут равны нулю
			if (!pVsource->IsStateOn())
				continue;

			pVsource->CalculatePower();

			if (pVsource->IsKindOfType(DEVTYPE_POWER_INJECTOR))
			{
				const auto& pGen{ static_cast<CDynaPowerInjector*>(pVsource) };

				// в диагональ матрицы добавляем проводимость генератора
				// и вычитаем шунт Нортона, так как он уже добавлен в диагональ
				// матрицы. Это работает для генераторов у которых есть Нортон (он
				// обратен Zgen), и нет Нортона (он равен нулю)
				Y -= pGen->Ygen() - pGen->Ynorton();
				I -= pGen->Igen(Iteration);

			}
		}

		// рассчитываем задающий ток узла от нагрузки
		// можно посчитать ток, а можно посчитать добавку в диагональ
		//I += conj(cplx(Pnr - pNode->Pg, Qnr - pNode->Qg) / pNode->VreVim);
		if (V > 0.0)
			Y += std::conj(cplx(Pgr - Pnr, Qgr - Qnr) / V.Value / V.Value);

		_CheckNumber(I.real());
		_CheckNumber(I.imag());
		_CheckNumber(Y.real());
		_CheckNumber(Y.imag());
	}
	else
	{
		Y = 1.0;
		I = 0.0;
	}

	return { Y, I };
}

void CDynaNodeContainer::LULF()
{
	CLULF lulf(*this);
	lulf.Solve();
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
	if (bSwitchToDynamicLRC != DynamicLRC)
	{
		DynamicLRC = bSwitchToDynamicLRC;
		for (auto&& node : DevVec)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(node) };
			// меняем местами СХН УР и динамики
			std::swap(pNode->pLRCLF, pNode->pLRC);
			// меняем местами расчетную и номинальную 
			// нагрузки
			std::swap(pNode->Pn, pNode->Pnr);
			std::swap(pNode->Qn, pNode->Qnr);
			// если СХН для УР - номинальное напряжение СХН
			// равно номинальному напряжению узла
			// если СХН для динамики - номинальное напряжение СХН
			// равно расчетному
			if (!DynamicLRC)
				pNode->V0 = pNode->Unom;
			else
				pNode->V0 = pNode->V;
		}
	}
}

VariableIndexExternal CDynaNodeBase::GetExternalVariable(std::string_view VarName)
{
	if (VarName == CDynaNodeBase::cszSz_ || VarName == CDynaNodeBase::cszSyncDelta_)
	{
		VariableIndexExternal ExtVar = { -1, nullptr };

		if (pSyncZone)
			ExtVar = pSyncZone->GetExternalVariable(VarName);
		return ExtVar;
	}
	else
		return CDevice::GetExternalVariable(VarName);
}


void CDynaNodeBase::StoreStates()
{
	SavedLowVoltage = LowVoltage;
}

void CDynaNodeBase::RestoreStates()
{
	LowVoltage = SavedLowVoltage;
}

void CDynaNodeBase::FromSuperNode()
{
	_ASSERTE(pSuperNodeParent);
	V = pSuperNodeParent->V;
	Delta = pSuperNodeParent->Delta;
	Vre = pSuperNodeParent->Vre;
	Vim = pSuperNodeParent->Vim;
	VreVim = pSuperNodeParent->VreVim;
	LRCVicinity = pSuperNodeParent->LRCVicinity;
	LowVoltage = pSuperNodeParent->LowVoltage;
}

void CDynaNodeBase::SetLowVoltage(bool bLowVoltage)
{
	if (LowVoltage)
	{
		if (!bLowVoltage)
		{
			LowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Напряжение {} в узле {} выше порогового", V.Value, GetVerbalName()));
		}
	}
	else
	{
		if (bLowVoltage)
		{
			LowVoltage = bLowVoltage;
			if (IsStateOn())
				Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Напряжение {} в узле {} ниже порогового", V.Value, GetVerbalName()));
		}
	}
}

double CDynaNodeBase::FindVoltageZC(CDynaModel *pDynaModel, const RightVector *pRvre, const RightVector *pRvim, double Hyst, bool bCheckForLow)
{
	double rH{ 1.0 };
	const double Vre1{ pDynaModel->NextStepValue(pRvre) };
	const double Vim1{ pDynaModel->NextStepValue(pRvim) };
	const double Vcheck{ sqrt(Vre1 * Vre1 + Vim1 * Vim1) };
	// выбираем границу сравгнения с гистерезисом - на снижение -, на повышение +
	const double Border{ LOW_VOLTAGE + (bCheckForLow ? -Hyst : Hyst) };
	const double derr{ std::abs(pRvre->GetWeightedError(std::abs(Vcheck - Border), Border)) };

	if (derr < pDynaModel->ZeroCrossingTolerance())
	{
		// если погрешность меньше заданной в параметрах
		// ставим заданную фиксацию напряжения 
		SetLowVoltage(bCheckForLow);
		pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
	}
	else
	{
		rH = pDynaModel->FindZeroCrossingOfModule(pRvre, pRvim, Border, bCheckForLow);
		if (pDynaModel->ZeroCrossingStepReached(rH))
		{
			SetLowVoltage(bCheckForLow);
			pDynaModel->DiscontinuityRequest(*this, DiscontinuityLevel::Light);
		} else if(rH < 0.0)
		{
			Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("Negative ZC ratio rH={} at node \"{}\" at t={} V={} Border={}",
				rH,
				GetVerbalName(),
				GetModel()->GetCurrentIntegrationTime(),
				Vcheck,
				Border
			));
		}
	}
	return rH;
}
// узел не должен быть в матрице, если он отключен или входит в суперузел
bool CDynaNodeBase::InMatrix() const noexcept
{
	if (pSuperNodeParent || !IsStateOn())
		return false;
	else
		return true;
}

double CDynaNodeBase::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH{ 1.0 };
	const double Hyst{ LOW_VOLTAGE_HYST };
	const RightVector* pRvre{ pDynaModel->GetRightVector(Vre.Index) };
	const RightVector* pRvim{ pDynaModel->GetRightVector(Vim.Index) };
	const double Vre1{ pDynaModel->NextStepValue(pRvre)};
	const double Vim1{ pDynaModel->NextStepValue(pRvim) };
	const double Vcheck{ std::sqrt(Vre1 * Vre1 + Vim1 * Vim1) };

	/*if (GetId() == 2005 && GetModel()->GetIntegrationStepNumber() >= 6900)
	{
		const double Vold(std::sqrt(pRvre->Nordsiek[0] * pRvre->Nordsiek[0] + pRvim->Nordsiek[0] * pRvim->Nordsiek[0]));
		Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("{}->{}", Vold, Vcheck));
	}*/

	if (LowVoltage)
	{
		const double Border{ LOW_VOLTAGE + Hyst };
		if (Vcheck > Border)
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, false);
	}
	else
	{
		const double Border{ LOW_VOLTAGE - Hyst };
		if (Vcheck < Border)
			rH = FindVoltageZC(pDynaModel, pRvre, pRvim, Hyst, true);
	}

	return rH;
}

// добавляет ветвь в список ветвей с нулевым сопротивлением суперузла
VirtualZeroBranch* CDynaNodeBase::AddZeroBranch(CDynaBranch* pBranch)
{

	// На вход могут приходить и ветви с сопротивлением больше порогового, но они не будут добавлены в список
	// ветвей с нулевым сопротивлением. Потоки по таким ветвями будут рассчитаны как обычно и будут нулями, если ветвь
	// находится в суперузле (все напряжения одинаковые)

	if (pBranch->IsZeroImpedance())
	{
		if (pVirtualZeroBranchEnd_>= static_cast<CDynaNodeContainer*>(pContainer_)->GetZeroBranchesEnd())
			throw dfw2error("CDynaNodeBase::AddZeroBranch VirtualZeroBranches overrun");

		// если ветвь имеет сопротивление ниже минимального 
		bool bAdd{ true };
		// проверяем 1) - не добавлена ли она уже; 2) - нет ли параллельной ветви
		VirtualZeroBranch* pParallelFound{ nullptr };
		for (VirtualZeroBranch *pVb = pVirtualZeroBranchBegin_; pVb < pVirtualZeroBranchEnd_; pVb++)
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
				bool bParr{ pVb->pBranch->pNodeIp_ == pBranch->pNodeIp_ && pVb->pBranch->pNodeIq_ == pBranch->pNodeIq_ };
				// и если нет - проверяем параллельную в обратную
				bParr = bParr ? bParr : pVb->pBranch->pNodeIq_ == pBranch->pNodeIp_ && pVb->pBranch->pNodeIp_ == pBranch->pNodeIq_;
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
			pVirtualZeroBranchEnd_->pBranch = pBranch;
			if (pParallelFound)
			{
				pVirtualZeroBranchEnd_->pParallelTo = pParallelFound->pBranch;
				pParallelFound->nParallelCount++;
			}
			// а в основной ветви увеличиваем счетчик параллельных
			pVirtualZeroBranchEnd_++;
		}
	}

	// возвращаем конец списка ветвей с нулевым сопротивлением
	return pVirtualZeroBranchEnd_;
}

void CDynaNodeBase::TidyZeroBranches()
{
	// сортируем нулевые ветви так, чтобы вначале были основные, в конце параллельные основным
	std::sort(pVirtualZeroBranchBegin_, pVirtualZeroBranchEnd_, [](const VirtualZeroBranch& lhs, const VirtualZeroBranch& rhs)->bool 
		{ 
			return lhs.pParallelTo < rhs.pParallelTo; 
		});
	pVirtualZeroBranchParallelsBegin_ = pVirtualZeroBranchBegin_;

	// находим начало параллельных цепей
	while (pVirtualZeroBranchParallelsBegin_ < pVirtualZeroBranchEnd_)
	{
		if (!pVirtualZeroBranchParallelsBegin_->pParallelTo)
			pVirtualZeroBranchParallelsBegin_++;
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
	/*

	CDevice** ppDevice{ nullptr };
	// ищем хотя бы одну включенную ветвь
	while (pLink->In(ppDevice))
		if (static_cast<CDynaBranch*>(*ppDevice)->BranchAndNodeConnected(this))
			break; // включенная ветвь есть
	*/

	const CLinkPtrCount* const pLink{ GetLink(0) };
	LinkWalker<CDynaBranch> pBranch;
	while (pLink->In(pBranch))
		if (pBranch->BranchAndNodeConnected(this))
			break;

	return pBranch.empty();
}

void CDynaNodeBase::CreateZeroLoadFlowData()
{
	// Мы хотим сделать матрицу Y для расчета потокораспределения внутри
	// суперузла

	// Для обычных (не супер) узлов не выполняем
	if (pSuperNodeParent)
		return;

	const CLinkPtrCount* const pSuperNodeLink{ GetSuperLink(0) };
	const auto ppNodeEnd{ pSuperNodeLink->end() };

	// не выполняем также и для суперзулов, в которых нет узлов
	if (!pSuperNodeLink->Count())
		return;

	// создаем данные суперузла
	ZeroLF.ZeroSupeNode = std::make_unique<CDynaNodeBase::ZeroLFData::ZeroSuperNodeData>();

	CDynaNodeBase::ZeroLFData::ZeroSuperNodeData *pZeroSuperNode{ ZeroLF.ZeroSupeNode.get() };

	// создаем данные для построения Y : сначала создаем вектор строк матрицы
	pZeroSuperNode->LFMatrix.reserve(ppNodeEnd - pSuperNodeLink->begin());

	// отыщем узел с максимальным количеством связей
	// поиск начинаем с узла-представителя суперзула
	std::pair<CDynaNodeBase*, size_t>  pMaxRankNode{ this, GetLink(0)->Count() };

	// находим узел с максимальным числом связей
	for (CDevice* const* ppSlaveDev = pSuperNodeLink->begin(); ppSlaveDev < ppNodeEnd; ppSlaveDev++)
	{
		const auto& pSlaveNode{ static_cast<CDynaNodeBase*>(*ppSlaveDev) };
		if (size_t nLinkCount{ pSlaveNode->GetLink(0)->Count() }; pMaxRankNode.second < nLinkCount)
			pMaxRankNode = { pSlaveNode, nLinkCount };
	}

	// узел с максимальным количеством связей не входит в Y
	// его индекс в Y делаем отрицательным
	pMaxRankNode.first->ZeroLF.SuperNodeLFIndex = -1;
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
			pNode->ZeroLF.SuperNodeLFIndex = nRowIndex++;
		}
	};

	AddAndIndexNode(this);
	for (CDevice* const* ppSlaveDev = pSuperNodeLink->begin(); ppSlaveDev < ppNodeEnd; ppSlaveDev++)
		AddAndIndexNode(static_cast<CDynaNodeBase*>(*ppSlaveDev));

	// считаем общее количество ветвей из данного суперузла
	// включая параллельные (с запасом)
	size_t nBranchesCount{ 0 };

	for (auto&& node : pZeroSuperNode->LFMatrix)
	{
		const CLinkPtrCount* const pBranchLink{ node->GetLink(0) };
		LinkWalker<CDynaBranch> pBranch;
		// ищем ветви соединяющие разные суперузлы
		while (pBranchLink->In(pBranch))
		{
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
	pZeroSuperNode->pVirtualBranches = std::make_unique<VirtualBranch[]>(nBranchesCount);
	// вектор списков узлов, связанных связями с нулевыми сопротивлениями
	pZeroSuperNode->pVirtualZeroBranches = std::make_unique<VirtualBranch[]>(2 * (pVirtualZeroBranchParallelsBegin_ - pVirtualZeroBranchBegin_));

	VirtualBranch* pHead{ pZeroSuperNode->pVirtualBranches.get() };
	VirtualBranch* pZeroHead{ pZeroSuperNode->pVirtualZeroBranches.get() };
	

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
		const CLinkPtrCount* const pBranchLink{ node->GetLink(0) };
		LinkWalker<CDynaBranch> pBranch;

		auto& ZeroLF{ node->ZeroLF };

		// между этими указателями список виртуальных ветей данного узла
		ZeroLF.pVirtualBranchesBegin = ZeroLF.pVirtualBranchesEnd = pHead;
		ZeroLF.pVirtualZeroBranchesBegin = ZeroLF.pVirtualZeroBranchesEnd = pZeroHead;
		ZeroLF.SlackInjection = ZeroLF.Yii = 0.0;

		// ищем ветви, соединяющие текущий узел с другими суперузлами
		while (pBranchLink->In(pBranch))
		{
			// интересуют только включенные ветви - так или иначе связывающие узлы 
			if (pBranch->BranchState_ == CDynaBranch::BranchState::BRANCH_ON)
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
	if (pSuperNodeParent)
		throw dfw2error(fmt::format("CDynaNodeBase::RequireSuperNodeLF - node {} is not super node", GetVerbalName()));
	static_cast<CDynaNodeContainer*>(pContainer_)->RequireSuperNodeLF(this);
}

void CDynaNodeBase::SuperNodeLoadFlowYU(CDynaModel* pDynaModel)
{
	
	CreateZeroLoadFlowData();
	if (!ZeroLF.ZeroSupeNode)
		return;

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
		*pAp = ZeroLF.SuperNodeLFIndex;	pAp++;
		for (const VirtualBranch* pV = ZeroLF.pVirtualZeroBranchesBegin; pV < ZeroLF.pVirtualZeroBranchesEnd; pV++)
		{
			*pAx = -pV->Y.real();	pAx += 2;
			*pAp = pV->pNode->ZeroLF.SuperNodeLFIndex;	pAp++;
		}
		*pAi = *(pAi - 1) + (ZeroLF.pVirtualZeroBranchesEnd - ZeroLF.pVirtualZeroBranchesBegin) + 1;
		pAi++;
	}
	
	pB = klu.B();

	double Vsq{ 0.0 };
	for (const auto& node : Matrix)
	{
		cplx Is{ node->GetSelfImbInotSuper(Vsq) };

		// добавляем предварительно рассчитанную 
		// инъекцию от связей с базисным узлом
		Is -= node->ZeroLF.SlackInjection;
				
		// собираем сумму перетоков по виртуальным ветвям узла
		// ветви находятся в общем для всего узла векторе и разделены
		// вилками указателей pBranchesBegin <  pBranchesEnd
		for (const VirtualBranch* vb = node->ZeroLF.pVirtualBranchesBegin; vb < node->ZeroLF.pVirtualBranchesEnd; vb++)
			Is -= vb->Y * cplx(vb->pNode->Vre, vb->pNode->Vim);

		const CLinkPtrCount* const pGenLink{ node->GetLink(1) };
		LinkWalker<CDynaPowerInjector> pGen;

		while (pGenLink->InMatrix(pGen))
			Is -= cplx(pGen->Ire, pGen->Iim);

		CDevice::FromComplex(pB, -Is);
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
	for (const VirtualZeroBranch* pZb = pVirtualZeroBranchBegin_; pZb < pVirtualZeroBranchEnd_; pZb++)
	{
		const auto& pBranch{ pZb->pBranch };
		const auto& pNode1{ pBranch->pNodeIp_->ZeroLF };
		const auto& pNode2{ pBranch->pNodeIq_->ZeroLF };

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
	if (pSuperNodeParent)
		return;

	// для ветвей с нулевым сопротивлением, параллельных основным ветвям копируем потоки основных,
	// так как потоки основных рассчитаны как доля параллельных потоков. Все потоки по паралелльным цепям
	// с сопротивлениями ниже минимальных будут одинаковы

	for (VirtualZeroBranch* pZb = pVirtualZeroBranchParallelsBegin_; pZb < pVirtualZeroBranchEnd_; pZb++)
	{
		const auto& pBranch{ pZb->pBranch };
		pBranch->Sb = pBranch->Se = 0.0;
		if (pBranch->BranchState_ == CDynaBranch::BranchState::BRANCH_ON)
			pBranch->Sb = pBranch->Se = pZb->pParallelTo->Sb;
	}

	// Далее добавляем потоки от шунтов для всех ветвей, включая параллельные
	for (VirtualZeroBranch* pZb = pVirtualZeroBranchBegin_; pZb < pVirtualZeroBranchEnd_; pZb++)
	{
		const auto& pBranch{ pZb->pBranch };
		if (pBranch->BranchState_ == CDynaBranch::BranchState::BRANCH_ON)
		{
			pZb->pBranch->Sb -= cplx(pBranch->pNodeIp_->V * pBranch->pNodeIp_->V * cplx(pBranch->GIp, -pBranch->BIp));
			pZb->pBranch->Se += cplx(pBranch->pNodeIq_->V * pBranch->pNodeIq_->V * cplx(pBranch->GIq, -pBranch->BIq));
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

	if (pSuperNodeParent)
		return; // это не суперузел

	const CLinkPtrCount* const pSuperNodeLink{ GetSuperLink(0) };
	if (!pSuperNodeLink->Count())
		return; // это суперузел, но у него нет связей

	// Создаем граф с узлами ребрами от типов расчетных узлов и ветвей
	using GraphType = GraphCycle<CDynaNodeBase*, VirtualZeroBranch*>;
	using NodeType = GraphType::GraphNodeBase;
	using EdgeType = GraphType::GraphEdgeBase;
	// Создаем вектор внутренних узлов суперузла, включая узел представитель
	std::unique_ptr<NodeType[]> pGraphNodes = std::make_unique<NodeType[]>(pSuperNodeLink->Count() + 1);
	// Создаем вектор ребер за исключением параллельных ветвей
	std::unique_ptr<EdgeType[]> pGraphEdges = std::make_unique<EdgeType[]>(pVirtualZeroBranchParallelsBegin_ - pVirtualZeroBranchBegin_);
	const auto ppNodeEnd{ pSuperNodeLink->end() };
	NodeType *pNode = pGraphNodes.get();
	GraphType gc;
	// Вводим узлы в граф. В качестве идентификатора узла используем адрес объекта
	for (CDevice* const* ppDev = pSuperNodeLink->begin(); ppDev < ppNodeEnd; ppDev++, pNode++)
		gc.AddNode(pNode->SetId(static_cast<CDynaNodeBase*>(*ppDev)));
	// добавляем узел представитель
	gc.AddNode(pNode->SetId(this));

	// Вводим в граф ребра
	EdgeType *pEdge = pGraphEdges.get();
	for (VirtualZeroBranch *pZb = pVirtualZeroBranchBegin_; pZb < pVirtualZeroBranchParallelsBegin_; pZb++, pEdge++)
		gc.AddEdge(pEdge->SetIds(pZb->pBranch->pNodeIp_, pZb->pBranch->pNodeIq_, pZb));

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

		const CLinkPtrCount* const pBranchLink{ pInSuperNode->GetLink(0) };
		LinkWalker<CDynaBranch> pBranch;
		// рассчитываем сумму потоков по инцидентным ветвям
		cplx I;
		while (pBranchLink->In(pBranch))
			I += pBranch->CurrentFrom(pInSuperNode);

		S -= std::conj(I) * Unode;
		CDevice::FromComplex(pB, S);
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
		CDevice::FromComplex(pB, 0.0);
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
	props.EquationsCount = CDynaNodeBase::VARS::V_LAST;
	props.bPredict = props.bNewtonUpdate = true;
	props.VarMap_.insert({ CDynaNodeBase::cszVre_, CVarIndex(V_RE, VARUNIT_KVOLTS) });
	props.VarMap_.insert({ CDynaNodeBase::cszVim_, CVarIndex(V_IM, VARUNIT_KVOLTS) });
	props.VarMap_.insert({ CDynaNodeBase::cszV_, CVarIndex(V_V, VARUNIT_KVOLTS) });
	props.VarAliasMap_.insert({ CDynaNodeBase::cszAliasV_ , { CDynaNodeBase::cszV_ }});
	props.DeviceFactory = std::make_unique <CDeviceFactory<CDynaNodeBase>>();

	props.ConstVarMap_.insert({ CDynaNode::cszGsh_, CConstVarIndex(CDynaNode::C_GSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaNode::cszBsh_, CConstVarIndex(CDynaNode::C_BSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaNode::cszPload0_, CConstVarIndex(CDynaNode::C_PLOAD0, VARUNIT_MW, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaNode::cszQload0_, CConstVarIndex(CDynaNode::C_QLOAD0, VARUNIT_MVAR, eDVT_INTERNALCONST) });
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

void CDynaNodeContainer::AddShortCircuitNode(CDynaNodeBase* pNode, const ShortCircuitInfo& ShortCircuitInfo)
{
	auto itsc{ ShortCircuitNodes_.find(pNode) };
	if(itsc == ShortCircuitNodes_.end())
		ShortCircuitNodes_.emplace(pNode, ShortCircuitInfo);
	else
	{
		if(ShortCircuitInfo.Usc.has_value())
		if (itsc->second.Usc.has_value())
			Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszShortCircuitNodeAlreadyAdded, pNode->GetVerbalName()));
		else 
			itsc->second.Usc = ShortCircuitInfo.Usc;

		if (ShortCircuitInfo.RXratio.has_value())
			Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszShortCircuitNodeAlreadyAdded, pNode->GetVerbalName()));
		else if (ShortCircuitInfo.RXratio.has_value())
			itsc->second.RXratio = ShortCircuitInfo.RXratio;
	}
}

void CDynaNodeContainer::LinkToLRCs(CDeviceContainer& containerLRC)
{
	static_cast<CDynaLRCContainer*>(&containerLRC)->CreateFromSerialized();

	for (auto&& dev : DevVec)
	{
		const auto& pNode = static_cast<CDynaNode*>(dev);
		if (pNode->LRCLoadFlowId > 0)
		{
			pNode->pLRCLF = static_cast<CDynaLRC*>(containerLRC.GetDevice(pNode->LRCLoadFlowId));
			if (!pNode->pLRCLF)
				Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLRCIdNotFound, pNode->LRCLoadFlowId, pNode->GetVerbalName()));
		}

		pNode->pLRC = static_cast<CDynaLRC*>(containerLRC.GetDevice(pNode->LRCTransientId));
		if (!pNode->pLRC)
			Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLRCIdNotFound, pNode->LRCTransientId, pNode->GetVerbalName()));
			
	}
}

void CDynaNodeContainer::RequireSuperNodeLF(CDynaNodeBase *pSuperNode)
{
	if (pSuperNode->pSuperNodeParent)
		throw dfw2error(fmt::format("CDynaNodeBase::RequireSuperNodeLF - node {} is not super node", pSuperNode->GetVerbalName()));
	ZeroLFSet.insert(pSuperNode);
}

void CDynaNodeBase::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty(CDevice::cszname_, TypedSerializedValue::eValueType::VT_NAME);
	AddStateProperty(Serializer);
	Serializer->AddEnumProperty("tip", new CSerializerAdapterEnum<CDynaNodeBase::eLFNodeType>(eLFNodeType_, CDynaNodeBase::cszLFNodeTypeNames_));
	Serializer->AddProperty("ny", TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty(CDynaNodeBase::cszAliasV_, V, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty("delta", Delta, eVARUNITS::VARUNIT_DEGREES);
	Serializer->AddProperty("pnr", Pn, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty("qnr", Qn, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty(CDynaNodeBase::cszPload_, Pnr, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty(CDynaNodeBase::cszQload_, Qnr, eVARUNITS::VARUNIT_MVAR);
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
	Serializer->AddState("LRCShuntPart", LRCShuntPart, eVARUNITS::VARUNIT_MW);
	Serializer->AddState("LRCShuntPartSuper", LRCShuntPartSuper, eVARUNITS::VARUNIT_MW);
	Serializer->AddState("Gshunt", Gshunt, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("Bshunt", Bshunt, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("InMetallicSC", InMetallicSC);
	Serializer->AddState("InLowVoltage", LowVoltage);
	Serializer->AddState("SavedInLowVoltage", SavedLowVoltage);
	Serializer->AddState("LRCVicinity", LRCVicinity);
	Serializer->AddState("dLRCLoad", dLRCLoad);
	Serializer->AddState("dLRCGen", dLRCGen);
	Serializer->AddState("Vold", Vold, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddState("Yii", Yii, eVARUNITS::VARUNIT_SIEMENS);
	Serializer->AddState("YiiSuper", YiiSuper, eVARUNITS::VARUNIT_SIEMENS);
}

void CLULF::SolveEM()
{
	klu_.SetSize(Nodes_.DevInMatrix.size(), Nodes_.DevInMatrix.size() + 2 * Nodes_.GetModel()->Branches.Count());
	double* const Ax{ klu_.Ax() };
	ptrdiff_t* Ap{ klu_.Ai() };
	ptrdiff_t* Ai{ klu_.Ap() };
	double* pAx{ Ax };
	ptrdiff_t* pAp{ Ap };
	ptrdiff_t* pAi{ Ai };

	CheckShortCircuitNodes();
	BuildNodeOrder();

	double** ppDiags{ pDiags_.get() };
	double* pB{ klu_.B() };

	for (auto&& it : NodeOrder_)
	{
		_ASSERTE(pAx < Ax + klu_.NonZeroCount() * 2);
		_ASSERTE(pAi < Ai + klu_.NonZeroCount());
		_ASSERTE(pAp < Ap + klu_.MatrixSize());
		const auto& pNode{ it.pNode };
		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / Nodes_.EquationsCount();		  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		if (pNode->InMetallicSC || it.SCinfo != nullptr)
			continue;
		// Branches
		for (const auto& pv : pNode->VirtualBranches())
		{
			*pAx = pv.Y.real();   pAx++;
			*pAx = pv.Y.imag();   pAx++;
			*pAi = pv.pNode->A(0) / Nodes_.EquationsCount(); pAi++;
		}
	}

	// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[klu_.MatrixSize()] = (pAx - Ax) / 2;

	size_t& nIteration{ Nodes_.IterationControl().Number };
	for (nIteration = 0; nIteration < 200; nIteration++)
	{
		Nodes_.IterationControl().Reset();
		ppDiags = pDiags_.get();
		pB = klu_.B();

		fnode_ << std::endl << nIteration << ";";
		fgen_ << std::endl << nIteration << ";";

		for (auto&& it : NodeOrder_)
		{
			CDynaNodeBase* pNode{ static_cast<CDynaNodeBase*>(it.pNode) };
			_ASSERTE(pB < klu_.B() + klu_.MatrixSize() * 2);
			// если для узла задано Uост рассчитываем шунт и фиксируем на нем напряжение
			pNode->Vold = pNode->V;
			auto [Y, I] { pNode->GetYI(nIteration) };
			if (it.SCinfo != nullptr)
			{
				// рассчитываем ток от ветвей
				for (const auto& pv : pNode->VirtualBranches())
					I -= pv.pNode->VreVim * pv.Y;
				// решаем уравнение шунта bs
				const double a{ -(it.SCinfo->RXratio.value() * it.SCinfo->RXratio.value() + 1.0)};
				const double b{ 2.0 * (Y.imag() - it.SCinfo->RXratio.value() * Y.real()) };
				const double Usc{ it.SCinfo->Usc.value() * pNode->Unom };
				const double c{ std::norm(I) / Usc / Usc - Y.real() * Y.real() - Y.imag() * Y.imag() };
				double bs1{ 0.0 }, bs2{ 0.0 };
				// получаем 0-2 корней
				switch (MathUtils::CSquareSolver::Roots(a, b, c, bs1, bs2))
				{
				case 0:	// если корней нет, используем фикцию
					bs1 = -pNode->Bshunt;
					if (pNode->V > Usc)
						bs1 *= 0.8;
					else
						bs1 *= 1.2;
					break;
				case 2:
				{
					// если корней 2 - рассчитываем 2 шунта и 2 напряжения
					const cplx v1{ I / cplx(Y.real() + it.SCinfo->RXratio.value() * bs1, Y.imag() - bs1) };
					const cplx v2{ I / cplx(Y.real() + it.SCinfo->RXratio.value() * bs2, Y.imag() - bs2) };
					// проверяем что модуль напряжения получим тот, который задан
					_ASSERTE(std::abs(std::abs(v1) - Usc) < 1e-3);
					_ASSERTE(std::abs(std::abs(v2) - Usc) < 1e-3);
					// выбираем корень по минимальному отклонению от текущего напряжения узла
					if (std::norm(v1 - pNode->VreVim) > std::norm(v2 - pNode->VreVim))
					{
						pNode->VreVim = v2;
						bs1 = bs2;
					}
					else
						pNode->VreVim = v1;
				}
				break;
				}
				const cplx Ysc{ it.SCinfo->RXratio.value() * bs1, -bs1 };
				// формируем уравнение для этого узла 1.0*V=V
				I = pNode->VreVim;
				Y = 1.0;
				// шунт от напряжения задаем _поверх_ существующего шунта. Он сидит в YiiSuper
				// но тут проблема будет если шунт придет _после_ шунта напряжения !
				CDevice::FromComplex(pNode->Gshunt, pNode->Bshunt, Ysc);
				// и заполняем вектор комплексных токов
			}
			// и заполняем вектор комплексных токов
			CDevice::FromComplex(pB, I);
			// диагональ матрицы формируем по Y узла
			**ppDiags = Y.real();
			*(*ppDiags + 1) = Y.imag();
			fnode_ << pNode->V / pNode->V0 << ";";
			ppDiags++;
		}

		klu_.FactorSolve();

		pB = klu_.B();

		// после решения системы обновляем данные во всех узлах
		for (auto&& it : Nodes_.DevInMatrix)
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
			if (!pNode->InMetallicSC)
				Nodes_.IterationControl().MaxV.UpdateMaxAbs(pNode, CDevice::ZeroDivGuard(pNode->V - pNode->Vold, pNode->Vold));
		}

		Nodes_.DumpIterationControl(DFW2MessageStatus::DFW2LOG_DEBUG);

		if (std::abs(Nodes_.IterationControl().MaxV.GetDiff()) < Nodes_.GetModel()->RtolLULF())
		{
			Nodes_.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLULFConverged, 
				Nodes_.IterationControl().MaxV.GetDiff(),
				nIteration));
			break;
		}
	}

	// для всех узлов с Uост
	for (auto&& scnode : Nodes_.ShortCircuitNodes_)
	{
		// рассчитываем z-шунт
		const cplx Ysc{ scnode.first->Gshunt, scnode.first->Bshunt };
		const cplx Zsc{ -1.0 / Ysc };
		// добавляем его к собстсвенной проводимости узла
		scnode.first->YiiSuper += Ysc;

		// даем репорт и пишем медленные переменные по z-шунту
		const auto Description{ fmt::format(CDFW2Messages::m_cszShortCircuitShunt,
			scnode.second.Usc.value(),
			scnode.second.RXratio.value(),
			scnode.first->GetVerbalName()) };

		Nodes_.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszShortCircuitShuntCalculated,
			scnode.first->GetVerbalName(),
			scnode.second.Usc.value(),
			scnode.second.RXratio.value(),
			Zsc,
			scnode.first->V,
			scnode.first->V / scnode.first->Unom,
			SCMethodName()));

		Nodes_.GetModel()->WriteSlowVariable(scnode.first->GetType(),
			{ scnode.first->GetId() },
			CDynaNodeBase::cszR_,
			Zsc.real(),
			0.0,
			Description);

		Nodes_.GetModel()->WriteSlowVariable(scnode.first->GetType(),
			{ scnode.first->GetId() },
			CDynaNodeBase::cszX_,
			Zsc.imag(),
			0.0,
			Description);
	}

	// после того как шунты для Uост рассчитаны
	// сбрасываем их список до следующего расчета с Uост
	Nodes_.ShortCircuitNodes_.clear();
}

void CLULF::SolveVG()
{
	klu_.SetSize(Nodes_.DevInMatrix.size(), Nodes_.DevInMatrix.size() + 2 * Nodes_.GetModel()->Branches.Count());
	double* const Ax{ klu_.Ax() };
	ptrdiff_t* Ap{ klu_.Ai() };
	ptrdiff_t* Ai{ klu_.Ap() };
	double* pAx{ Ax };
	ptrdiff_t* pAp{ Ap };
	ptrdiff_t* pAi{ Ai };


	if (Nodes_.ShortCircuitNodes_.size() > 1)
		throw dfw2error(fmt::format(CDFW2Messages::m_cszShortCircutShuntMethodCanCalculateJustOne, SCMethodName(), Nodes_.ShortCircuitNodes_.size()));

	CheckShortCircuitNodes();
	BuildNodeOrder();

	double** ppDiags{ pDiags_.get() };
	double* pB{ klu_.B() };

	for (auto&& it : NodeOrder_)
	{
		_ASSERTE(pAx < Ax + klu_.NonZeroCount() * 2);
		_ASSERTE(pAi < Ai + klu_.NonZeroCount());
		_ASSERTE(pAp < Ap + klu_.MatrixSize());
		const auto& pNode{ it.pNode };
		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / Nodes_.EquationsCount();		  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		if (pNode->InMetallicSC)
			continue;

		// Branches
		for (const auto& pv : pNode->VirtualBranches())
		{
			*pAx = pv.Y.real();   pAx++;
			*pAx = pv.Y.imag();   pAx++;
			*pAi = pv.pNode->A(0) / Nodes_.EquationsCount(); pAi++;
		}
	}

	// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[klu_.MatrixSize()] = (pAx - Ax) / 2;

	size_t& nIteration{ Nodes_.IterationControl().Number };
	for (nIteration = 0; nIteration < 200; nIteration++)
	{
		Nodes_.IterationControl().Reset();
		ppDiags = pDiags_.get();
		pB = klu_.B();

		fnode_ << std::endl << nIteration << ";";
		fgen_ << std::endl << nIteration << ";";

		for (auto&& it : NodeOrder_)
		{
			CDynaNodeBase* pNode{ static_cast<CDynaNodeBase*>(it.pNode) };
			_ASSERTE(pB < klu_.B() + klu_.MatrixSize() * 2);
			// если для узла задано Uост рассчитываем шунт и фиксируем на нем напряжение
			pNode->Vold = pNode->V;
			auto [Y, I] { pNode->GetYI(nIteration) };
			// и заполняем вектор комплексных токов
			CDevice::FromComplex(pB, I);
			// диагональ матрицы формируем по Y узла
			**ppDiags = Y.real();
			*(*ppDiags + 1) = Y.imag();
			fnode_ << pNode->V / pNode->V0 << ";";
			ppDiags++;
		}

		klu_.FactorSolve();
		// получили напряжения без шунта Uост
		auto U0{ std::make_unique<double[]>(2 * klu_.MatrixSize()) };
		std::copy(klu_.B(), klu_.B() + 2 * klu_.MatrixSize(), U0.get());
		std::fill(klu_.B(), klu_.B() + 2 * klu_.MatrixSize(), 0.0);

		for (const auto& scit : Nodes_.ShortCircuitNodes_)
		{
			klu_.B()[2 * scit.first->A(0) / Nodes_.EquationsCount()] = scit.second.RXratio.value();
			klu_.B()[2 * scit.first->A(0) / Nodes_.EquationsCount() + 1] = -1.0;
		}

		klu_.FactorSolve();

		pB = klu_.B();

		double t{ 0.0 };
		cplx uka;

		if (!Nodes_.ShortCircuitNodes_.empty())
		{
			auto scit{ Nodes_.ShortCircuitNodes_.begin() };
			ptrdiff_t zkindex{ 2 * scit->first->A(0) / Nodes_.EquationsCount() };
			const cplx zk{ cplx(klu_.B()[zkindex], klu_.B()[zkindex + 1]) };
			const double uk{ scit->second.Usc.value() * scit->first->Unom };
			const cplx u0{ cplx(U0[zkindex], U0[zkindex + 1]) };
			const double a{ std::norm(zk) };
			const double b{ 2.0 * klu_.B()[zkindex] };
			const double c{ 1.0 - std::norm(u0) / uk / uk };
			double bs1{ 0.0 }, bs2{ 0.0 };
			// получаем 0-2 корней
			switch (MathUtils::CSquareSolver::Roots(a, b, c, bs1, bs2))
			{
			case 0:
				break;
			case 1:
			{
				const double deltak{ std::arg(u0) - std::atan2(bs1 * zk.imag(), bs1 * zk.real() + 1.0) };
			}
			break;
			case 2:
			{
				const double deltak1{ std::arg(u0) - std::atan2(bs1 * zk.imag(), bs1 * zk.real() + 1.0) };
				const double deltak2{ std::arg(u0) - std::atan2(bs2 * zk.imag(), bs2 * zk.real() + 1.0) };
				cplx uka1{ std::polar(uk, deltak1) };
				cplx uka2{ std::polar(uk, deltak2) };
				const auto t1{ (u0 / uka1 - 1.0) / zk };
				const auto t2{ (u0 / uka2 - 1.0) / zk };

				t = bs2;
				uka = uka2;
				if (std::norm(u0 - uka1) < std::norm(u0 - uka2))
				{
					t = bs1;
					uka = uka1;
				}
				scit->first->Gshunt = t * scit->second.RXratio.value();
				scit->first->Bshunt = -t;
			}
			break;
			}
		}

		// после решения системы обновляем данные во всех узлах
		for (auto&& it : Nodes_.DevInMatrix)
		{
			const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
			ptrdiff_t zkindex{ 2 * pNode->A(0) / Nodes_.EquationsCount() };
			const cplx u0{ cplx(U0[zkindex], U0[zkindex + 1]) };
			const cplx zk{ cplx(klu_.B()[zkindex], klu_.B()[zkindex + 1]) };
			const cplx u{ Nodes_.ShortCircuitNodes_.empty() ? u0 : u0 - t * zk * uka };
			CDevice::FromComplex(pNode->Vre, pNode->Vim, u);
			// считаем напряжение узла в полярных координатах
			pNode->UpdateVDeltaSuper();
			// считаем изменение напряжения узла между итерациями и находим
			// самый изменяющийся узел
			if (!pNode->InMetallicSC)
				Nodes_.IterationControl().MaxV.UpdateMaxAbs(pNode, CDevice::ZeroDivGuard(pNode->V - pNode->Vold, pNode->Vold));
		}

		Nodes_.DumpIterationControl(DFW2MessageStatus::DFW2LOG_DEBUG);

		if (std::abs(Nodes_.IterationControl().MaxV.GetDiff()) < Nodes_.GetModel()->RtolLULF())
		{
			Nodes_.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLULFConverged,
				Nodes_.IterationControl().MaxV.GetDiff(),
				nIteration));
			break;
		}
	}

	// для всех узлов с Uост
	for (auto&& scnode : Nodes_.ShortCircuitNodes_)
	{
		// рассчитываем z-шунт
		const cplx Ysc{ scnode.first->Gshunt, scnode.first->Bshunt };
		const cplx Zsc{ -1.0 / Ysc };
		// добавляем его к собстсвенной проводимости узла
		scnode.first->YiiSuper += Ysc;

		// даем репорт и пишем медленные переменные по z-шунту
		const auto Description{ fmt::format(CDFW2Messages::m_cszShortCircuitShunt,
			scnode.second.Usc.value(),
			scnode.second.RXratio.value(),
			scnode.first->GetVerbalName()) };


		Nodes_.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszShortCircuitShuntCalculated,
			scnode.first->GetVerbalName(),
			scnode.second.Usc.value(),
			scnode.second.RXratio.value(),
			Zsc,
			scnode.first->V,
			scnode.first->V / scnode.first->Unom,
			SCMethodName()));

		Nodes_.GetModel()->WriteSlowVariable(scnode.first->GetType(),
			{ scnode.first->GetId() },
			CDynaNodeBase::cszR_,
			Zsc.real(),
			0.0,
			Description);

		Nodes_.GetModel()->WriteSlowVariable(scnode.first->GetType(),
			{ scnode.first->GetId() },
			CDynaNodeBase::cszX_,
			Zsc.imag(),
			0.0,
			Description);
	}

	// после того как шунты для Uост рассчитаны
	// сбрасываем их список до следующего расчета с Uост
	Nodes_.ShortCircuitNodes_.clear();
}

void CLULF::CheckShortCircuitNodes()
{
	// проверяем нет ли узлов с незаданным Uост или Uост <= 0.0
	auto scnode{ Nodes_.ShortCircuitNodes_.begin() };
	while (scnode != Nodes_.ShortCircuitNodes_.end())
	{
		if (!scnode->second.Usc.has_value())
		{
			if (scnode->second.RXratio.has_value())
			{
				Nodes_.Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszShortCircuitVoltageNotSetButRX,
					scnode->first->GetVerbalName(),
					scnode->second.RXratio.value()));
			}
			scnode = Nodes_.ShortCircuitNodes_.erase(scnode);
		} else if (scnode->second.Usc.value() < Consts::epsilon) // если есть - удаляем их из списка Uост и устраиваем металлическое КЗ
		{
			Nodes_.Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszShortCircuitVoltageTooLow,
				scnode->first->GetVerbalName(),
				scnode->second.Usc.value()));
			scnode->first->InMetallicSC = true;
			scnode = Nodes_.ShortCircuitNodes_.erase(scnode);
		} else if (!scnode->second.RXratio.has_value()) // если не задано R/X - используем 0
		{
			Nodes_.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszShortCircuitVoltageRXZeroed,
				scnode->first->GetVerbalName(),
				scnode->second.Usc.value()));
			scnode->second.RXratio = 0.0;
			scnode++;
		}
		else
			scnode++;
	}

}

void CLULF::BuildNodeOrder()
{
	NodeOrder_.reserve(klu_.MatrixSize());
	for (auto&& it : Nodes_.DevInMatrix)
	{
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		pNode->LRCVicinity = 0.0;		// зона сглаживания СХН для начала нулевая - без сглаживания
		if (!pNode->InMetallicSC)
		{
			// стартуем с плоского
			pNode->Vre = pNode->V = pNode->Unom;
			pNode->Vim = pNode->Delta = 0.0;
			fnode_ << pNode->GetId() << ";";
		}
		auto scit{ Nodes_.ShortCircuitNodes_.find(pNode) };
		NodeOrder_.emplace_back(NodeOrderT{pNode, scit == Nodes_.ShortCircuitNodes_.end() ? nullptr : &scit->second});
	}
	pDiags_ = std::make_unique<double* []>(klu_.MatrixSize());
	fnode_.open(Nodes_.GetModel()->Platform().ResultFile("nodes.csv"));
	fgen_.open(Nodes_.GetModel()->Platform().ResultFile("gens.csv"));
	fnode_ << ";";

}

std::string CLULF::SCMethodName() const
{
	std::string ret{ "???" };
	if (auto Serializer{ Nodes_.GetModel()->GetParametersSerializer() }; Serializer)
		if (auto ShuntMethodParameter{ Serializer->at(CDynaModel::Parameters::cszShortCircuitShuntMethod_) }; ShuntMethodParameter)
			if (ShuntMethodParameter->Adapter)
				ret = ShuntMethodParameter->Adapter->GetString();
	return ret;
}


void CLULF::Solve()
{
	switch (Nodes_.GetModel()->Parameters().ShortCircuitShuntMethod_)
	{
	case CDynaModel::eShortCircuitShuntMethod::EM:
		SolveEM();
		break;
	case CDynaModel::eShortCircuitShuntMethod::VG:
		SolveVG();
		break;
	default:
		SolveEM();
		break;
	}
	
}