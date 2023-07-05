#include "stdafx.h"
#include "DynaBranch.h"
#include "DynaModel.h"
#include "GraphCycle.h"
#include "DynaGeneratorMotion.h"
#include "BranchMeasures.h"
#include <immintrin.h>

using namespace DFW2;

#define LOW_VOLTAGE 0.1	// может быть сделать в о.е. ? Что будет с узлами с низким Uном ?
#define LOW_VOLTAGE_HYST LOW_VOLTAGE * 0.1

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

// в узлах используется расширенная функция
// прогноза. После прогноза рассчитывается
// напряжение в декартовых координатах
// и сбрасываются значения по умолчанию перед
// итерационным процессом решения сети
void CDynaNode::Predict(const CDynaModel& DynaModel)
{
	VreVim = { Vre, Vim };
	LRCVicinity = 0.05;
	const double newDelta{ std::atan2(std::sin(Delta), std::cos(Delta)) };
	if (!Consts::Equal(newDelta, Delta))
	{
		RightVector* const pRvDelta{ GetModel()->GetRightVector(Delta.Index) };
		RightVector* const pRvLag{ GetModel()->GetRightVector(Lag.Index) };
		double dDL = Delta - Lag;
		Delta = newDelta;
		Lag = Delta - dDL;
		if (DynaModel.UseCOI())
			Lag += pSyncZone->Delta;
		pRvDelta->Nordsiek[0] = Delta;
		pRvLag->Nordsiek[0] = Lag;
	}

}

CDynaNode::CDynaNode() : CDynaNodeBase()
{
}

void CDynaNode::BuildDerivatives(CDynaModel *pDynaModel)
{
	double DiffDelta{ Delta };
	if (pDynaModel->UseCOI())
		DiffDelta += pSyncZone->Delta;
	pDynaModel->SetDerivative(Lag, (DiffDelta - Lag) / pDynaModel->GetFreqTimeConstant());
}


void CDynaNode::BuildEquations(CDynaModel* pDynaModel)
{
	CDynaNodeBase::BuildEquations(pDynaModel);

	// Копируем скольжение в слэйв-узлы суперузла
	// (можно совместить с CDynaNodeBase::FromSuperNode()
	// и сэкономить цикл
	const CLinkPtrCount* const pLink{ GetSuperLink(0) };
	LinkWalker<CDynaNode> pSlaveNode;
	while (pLink->In(pSlaveNode))
		pSlaveNode->S = S;

	const double T{ pDynaModel->GetFreqTimeConstant() }, w0{ pDynaModel->GetOmega0() };
	const double Vre2{ Vre * Vre }, Vim2{ Vim * Vim }, V2{ Vre2 + Vim2 };

	pDynaModel->SetElement(V, V, 1.0);
	pDynaModel->SetElement(Delta, Delta, 1.0);

	pDynaModel->SetElement(Lag, Delta, -1.0 / T);
	if(pDynaModel->UseCOI())
		pDynaModel->SetElement(Lag, pSyncZone->Delta, -1.0 / T);
	pDynaModel->SetElement(Lag, Lag, -1.0 / T);


	if (LowVoltage)
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
		if (pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->Delta, -1.0 / T / w0);
		pDynaModel->SetElement(S, Lag, 1.0 / T / w0);
		if(pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->S, -1.0);
		pDynaModel->SetElement(S, S, 1.0);
	}
	else
	{
		pDynaModel->SetElement(S, Delta, 0.0);
		if (pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->Delta, 0.0);
		pDynaModel->SetElement(S, Lag, 0.0);
		if(pDynaModel->UseCOI())
			pDynaModel->SetElement(S, pSyncZone->S, 0.0);
		pDynaModel->SetElement(S, S, 1.0);
	}
}

void CDynaNode::BuildRightHand(CDynaModel* pDynaModel)
{
	CDynaNodeBase::BuildRightHand(pDynaModel);

	const double T{ pDynaModel->GetFreqTimeConstant() };
	const double w0{ pDynaModel->GetOmega0() };

	double DiffDelta{ Delta };

	if (pDynaModel->UseCOI())
		DiffDelta += pSyncZone->Delta;

	const double dLag{ (DiffDelta - Lag) / T };
	double dS{ S - (DiffDelta - Lag) / T / w0 };

	double dDelta{ 0.0 };

	if (pDynaModel->IsInDiscontinuityMode()) 
		dS = 0.0;

	if (!LowVoltage)
		dDelta = Delta - std::atan2(Vim, Vre);
	
	pDynaModel->SetFunctionDiff(Lag, dLag);
	pDynaModel->SetFunction(S, dS);
	pDynaModel->SetFunction(Delta, dDelta);

	//DumpIntegrationStep(2021, 2031);
	//DumpIntegrationStep(2143, 2031);
	//DumpIntegrationStep(2141, 2031);
}

eDEVICEFUNCTIONSTATUS CDynaNode::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ CDynaNodeBase::Init(pDynaModel) };
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
	double* p{ nullptr };
	switch (nVarIndex)
	{
		MAP_VARIABLE(Bshunt, C_BSH)
		MAP_VARIABLE(Gshunt, C_GSH)
		MAP_VARIABLE(SyncDelta_, C_SYNCDELTA)
		MAP_VARIABLE(SyncSlip_, C_SYNCSLIP)
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

eDEVICEFUNCTIONSTATUS CDynaNode::SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice)
{
	eDEVICESTATE OldState{ GetState() };
	eDEVICEFUNCTIONSTATUS Status{ CDevice::SetState(eState, eStateCause) };

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
	if (pDynaModel->UseCOI())
		Lag += pSyncZone->Delta;

	SetLowVoltage(sqrt(Vre * Vre + Vim * Vim) < (LOW_VOLTAGE - LOW_VOLTAGE_HYST));
	//Delta = atan2(Vim, Vre);
	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

void CDynaNodeContainer::LULF()
{
	KLUWrapper<std::complex<double>> klu;
	size_t nNodeCount{ DevInMatrix.size() };
	size_t nBranchesCount{ pDynaModel_->Branches.Count() };
	// оценка количества ненулевых элементов
	size_t nNzCount{ nNodeCount + 2 * nBranchesCount };

	klu.SetSize(nNodeCount, nNzCount);
	double* const Ax{ klu.Ax() };
	double* const B{ klu.B() };
	ptrdiff_t* Ap{ klu.Ai() };
	ptrdiff_t* Ai{ klu.Ap() };

	// вектор указателей на диагональ матрицы
	auto pDiags{ std::make_unique<double* []>(nNodeCount) };
	double** ppDiags{ pDiags.get() };
	double* pB{ B };

	double* pAx{ Ax };
	ptrdiff_t* pAp{ Ap };
	ptrdiff_t* pAi{ Ai };

	std::ofstream fnode(GetModel()->Platform().ResultFile("nodes.csv"));
	std::ofstream fgen(GetModel()->Platform().ResultFile("gens.csv"));

	fnode << ";";

	for (auto&& it : DevInMatrix)
	{
		_ASSERTE(pAx < Ax + nNzCount * 2);
		_ASSERTE(pAi < Ai + nNzCount);
		_ASSERTE(pAp < Ap + nNodeCount);
		const auto& pNode{ static_cast<CDynaNodeBase*>(it) };
		pNode->LRCVicinity = 0.0;		// зона сглаживания СХН для начала нулевая - без сглаживания
		*pAp = (pAx - Ax) / 2;    pAp++;
		*pAi = pNode->A(0) / EquationsCount();		  pAi++;
		// первый элемент строки используем под диагональ
		// и запоминаем указатель на него
		*ppDiags = pAx;
		*pAx = 1.0; pAx++;
		*pAx = 0.0; pAx++;
		ppDiags++;

		if (!pNode->InMetallicSC)
		{
			// стартуем с плоского
			pNode->Vre = pNode->V = pNode->Unom;
			pNode->Vim = pNode->Delta = 0.0;
			// для всех узлов, которые не отключены и не находятся в металлическом КЗ (КЗ с нулевым шунтом)
			fnode << pNode->GetId() << ";";
			// Branches

			for (VirtualBranch *pV = pNode->pVirtualBranchBegin_; pV < pNode->pVirtualBranchEnd_; pV++)
			{
				*pAx = pV->Y.real();   pAx++;
				*pAx = pV->Y.imag();   pAx++;
				*pAi = pV->pNode->A(0) / EquationsCount(); pAi++;
			}
		}
	}

	nNzCount = (pAx - Ax) / 2;		// рассчитываем получившееся количество ненулевых элементов (делим на 2 потому что комплекс)
	Ap[nNodeCount] = nNzCount;

	size_t& nIteration{ IterationControl_.Number };
	for (nIteration = 0; nIteration < 200; nIteration++)
	{
		IterationControl_.Reset();
		ppDiags = pDiags.get();
		pB = B;

		fnode << std::endl << nIteration << ";";
		fgen  << std::endl << nIteration << ";";

		// проходим по узлам вне зависимости от их состояния, параллельно идем по диагонали матрицы
		for (auto&& it : DevInMatrix)
		{
			CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
			_ASSERTE(pB < B + nNodeCount * 2);

			/*
			if (pNode->GetId() == 1067 && m_pDynaModel->GetCurrentTime() > 0.53 && nIteration > 4)
				pNode->GetId();// pNode->BuildRightHand(m_pDynaModel);
				*/

			if (!pNode->InMetallicSC)
			{
				// для всех узлов которые включены и вне металлического КЗ

				cplx Unode(pNode->Vre, pNode->Vim);

				cplx I{ pNode->IconstSuper };
				cplx Y{ pNode->YiiSuper };		// диагональ матрицы по умолчанию собственная проводимость узла

				pNode->Vold = pNode->V;			// запоминаем напряжение до итерации для анализа сходимости

				pNode->GetPnrQnrSuper();

				// Generators

				const CLinkPtrCount* const pLink{ pNode->GetSuperLink(2) };
				LinkWalker<CDynaVoltageSource> pVsource;
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

					if (pVsource->IsKindOfType(DEVTYPE_GEN_INFPOWER))
					{
						const auto& pGen{ static_cast<CDynaGeneratorInfBus*>(static_cast<CDynaVoltageSource*>(pVsource)) };

						// в диагональ матрицы добавляем проводимость генератора
						// и вычитаем шунт Нортона, так как он уже добавлен в диагональ
						// матрицы. Это работает для генераторов у которых есть Нортон (он
						// обратен Zgen), и нет Нортона (он равен нулю)
						Y -= 1.0 / pGen->Zgen() - pGen->Ynorton();
						I -= pGen->Igen(nIteration);

					}

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

		klu.FactorSolve();

		// KLU может делать повторную факторизацию матрицы с начальным пивотингом
		// это быстро, но при изменении пивотов может вызывать численную неустойчивость.
		// У нас есть два варианта факторизации/рефакторизации на итерации LULF
		double* pB{ B };

		for (auto&& it: DevInMatrix)
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
				IterationControl_.MaxV.UpdateMaxAbs(pNode, CDevice::ZeroDivGuard(pNode->V - pNode->Vold, pNode->Vold));
		}

		DumpIterationControl(DFW2MessageStatus::DFW2LOG_DEBUG);

		if (std::abs(IterationControl_.MaxV.GetDiff()) < pDynaModel_->RtolLULF())
		{
			Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLULFConverged, IterationControl_.MaxV.GetDiff(), nIteration));
			break;
		}
	}
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
	if (VarName == CDynaNode::m_cszSz || VarName == CDynaNode::m_cszDz)
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
				GetModel()->GetCurrentTime(),
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
	props.bFinishStep = true;
	props.EquationsCount = CDynaNodeBase::VARS::V_LAST;
	props.bPredict = props.bNewtonUpdate = true;
	props.VarMap_.insert({ CDynaNodeBase::m_cszVre, CVarIndex(V_RE, VARUNIT_KVOLTS) });
	props.VarMap_.insert({ CDynaNodeBase::m_cszVim, CVarIndex(V_IM, VARUNIT_KVOLTS) });
	props.VarMap_.insert({ CDynaNodeBase::m_cszV, CVarIndex(V_V, VARUNIT_KVOLTS) });
	props.VarAliasMap_.insert({ "vras", { CDynaNodeBase::m_cszV }});
	props.ConstVarMap_.insert({ "SyncDelta", CConstVarIndex(CDynaNode::C_SYNCDELTA, VARUNIT_RADIANS, true, eDVT_CONSTSOURCE)});
	props.ConstVarMap_.insert({ "SyncSlip", CConstVarIndex(CDynaNode::C_SYNCSLIP, VARUNIT_UNITLESS, true, eDVT_CONSTSOURCE)});
}

void CDynaNode::FinishStep(const CDynaModel& DynaModel)
{
	SyncDelta_ = pSyncZone->Delta + Delta;
	SyncDelta_ = std::atan2(std::sin(SyncDelta_), std::cos(SyncDelta_));
	SyncSlip_  = pSyncZone->S;
}

void CDynaNode::DeviceProperties(CDeviceContainerProperties& props)
{
	CDynaNodeBase::DeviceProperties(props);
	props.SetClassName(CDeviceContainerProperties::m_cszNameNode, CDeviceContainerProperties::m_cszSysNameNode);
	props.EquationsCount = CDynaNode::VARS::V_LAST;
	props.VarMap_.insert({ CDynaNode::m_cszS, CVarIndex(V_S, VARUNIT_PU) });
	props.VarMap_.insert({ CDynaNodeBase::m_cszDelta, CVarIndex(V_DELTA, VARUNIT_RADIANS) });

	/*
	props.m_VarMap.insert(make_pair("Sip", CVarIndex(V_SIP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair("Cop", CVarIndex(V_COP, VARUNIT_PU)));
	props.m_VarMap.insert(make_pair("Sv", CVarIndex(V_SV, VARUNIT_PU)));
	*/

	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaNode>>();

	props.Aliases_.push_back(CDeviceContainerProperties::m_cszAliasNode);
	props.ConstVarMap_.insert({ CDynaNode::m_cszGsh, CConstVarIndex(CDynaNode::C_GSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });
	props.ConstVarMap_.insert({ CDynaNode::m_cszBsh, CConstVarIndex(CDynaNode::C_BSH, VARUNIT_SIEMENS, eDVT_INTERNALCONST) });
}

void CDynaNodeBase::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty(CDevice::m_cszname, TypedSerializedValue::eValueType::VT_NAME);
	AddStateProperty(Serializer);
	Serializer->AddEnumProperty("tip", new CSerializerAdapterEnum<CDynaNodeBase::eLFNodeType>(eLFNodeType_, CDynaNodeBase::m_cszLFNodeTypeNames));
	Serializer->AddProperty("ny", TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddProperty("vras", V, eVARUNITS::VARUNIT_KVOLTS);
	Serializer->AddProperty("delta", Delta, eVARUNITS::VARUNIT_DEGREES);
	Serializer->AddProperty("pnr", Pn, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty("qnr", Qn, eVARUNITS::VARUNIT_MVAR);
	Serializer->AddProperty(CDynaNodeBase::m_cszPload, Pnr, eVARUNITS::VARUNIT_MW);
	Serializer->AddProperty(CDynaNodeBase::m_cszQload, Qnr, eVARUNITS::VARUNIT_MVAR);
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

