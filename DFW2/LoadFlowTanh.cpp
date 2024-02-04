#include "stdafx.h"
#include "LoadFlow.h"
#include "DynaModel.h"

using namespace DFW2;

void CLoadFlow::NewtonTanh()
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningNewton);
	size_t& it{ pNodes->IterationControl().Number};

	// вектор для указателей переключаемых узлов, с размерностью в половину уравнений матрицы

	double g0(0.0), g1(0.1), lambda(1.0);

	TanhBeta = 150.0;
	NewtonStepRatio.Ratio_ = 0.0;

	while (1)
	{
		if (!CheckLF())
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		if (++it > Parameters.MaxIterations)
			throw dfw2error(CDFW2Messages::m_cszLFNoConvergence);

		pNodes->IterationControl().Reset();

		_MatrixInfo* pMaxV{ nullptr };
		// считаем небаланс по всем узлам кроме БУ
		_MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };
		for (; pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			if (pNode->eLFNodeType_ != CDynaNodeBase::eLFNodeType::LFNT_PQ)
				pNode->Qgr = Qgtanh(pNode);
			GetNodeImb(pMatrixInfo);	// небаланс считается с учетом СХН
			pNodes->IterationControl().Update(pMatrixInfo);
		}

		UpdateSlackBusesImbalance();

		g1 = GetSquaredImb();

		pNodes->IterationControl().ImbRatio = g1;
		if (g0 > 0.0)
			pNodes->IterationControl().ImbRatio = g1 / g0;

		DumpNewtonIterationControl();
		if (pNodes->IterationControl().Converged(Parameters.Imb))
			break;

		if (NewtonStepRatio.Ratio_ >= 1.0 && g1 > g0)
		{
			double gs1v = -CDynaModel::gs1(klu, pRh, klu.B());
			// знак gs1v должен быть "-" ????
			lambda *= -0.5 * gs1v / (g1 - g0 - gs1v);
			RestoreVDelta();
			UpdateVDelta(lambda);
			NewtonStepRatio.Ratio_ = lambda;
			NewtonStepRatio.eStepCause = LFNewtonStepRatio::eStepLimitCause::Backtrack;
			continue;
		}

		lambda = 1.0;
		g0 = g1;

		StoreVDelta();

		BuildMatrixTanh();

		// сохраняем небаланс до итерации
		std::copy(klu.B(), klu.B() + klu.MatrixSize(), pRh.get());
				
		ptrdiff_t iMax(0);
		double maxb = klu.FindMaxB(iMax);
		CDynaNodeBase *pNode1(pMatrixInfo_.get()[iMax / 2].pNode);
		
		SolveLinearSystem();

		maxb = klu.FindMaxB(iMax);
		CDynaNodeBase* pNode2(pMatrixInfo_.get()[iMax / 2].pNode);


		// обновляем переменные
		double MaxRatio = GetNewtonRatio(klu.B());
		UpdateVDelta(-MaxRatio);
		if (NewtonStepRatio.Ratio_ >= 1.0 && TanhBeta < 2500.0)
		{
			TanhBeta *= 1.0;
		}
	}

	// обновляем реактивную генерацию в суперузлах
	UpdateSupernodesPQ();
	for (auto&& it : *pNodes) static_cast<CDynaNodeBase*>(it)->StartLF(false, Parameters.Imb);
}

void CLoadFlow::SeidellTanh()
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningSeidell);

	MATRIXINFO SeidellOrder;
	BuildSeidellOrder(SeidellOrder);

	_ASSERTE(SeidellOrder.size() == pMatrixInfoSlackEnd - pMatrixInfo_.get());

	// TODO !!!! рассчитываем проводимости узлов с устранением отрицательных сопротивлений
	//pNodes->CalcAdmittances(true);
	double dPreviousImb = -1.0;
	for (size_t SeidellIterations = 0; SeidellIterations < Parameters.SeidellIterations; SeidellIterations++)
	{
		// множитель для ускорения Зейделя
		double dStep = Parameters.SeidellStep;

		if (SeidellIterations > 2)
		{
			// если сделали более 2-х итераций начинаем анализировать небалансы
			if (dPreviousImb < 0.0)
			{
				// первый небаланс, если еще не рассчитывали
				dPreviousImb = ImbNorm(pNodes->IterationControl().MaxImbP.GetDiff(), pNodes->IterationControl().MaxImbQ.GetDiff());
			}
			else
			{
				// если есть предыдущий небаланс, рассчитываем отношение текущего и предыдущего
				double dCurrentImb{ ImbNorm(pNodes->IterationControl().MaxImbP.GetDiff(), pNodes->IterationControl().MaxImbQ.GetDiff())};
				double dImbRatio{ dCurrentImb / dPreviousImb };
				/*
				dPreviousImb = dCurrentImb;
				if (dImbRatio < 1.0)
					dStep = 1.0;
				else
				{
					dStep = 1.0 + exp((1 - dImbRatio) * 10.0);
				}
				*/
			}
		}

		// сбрасываем статистику итерации
		pNodes->IterationControl().Reset();
		// определяем можно ли выполнять переключение типов узлов (по количеству итераций)
		bool bPVPQSwitchEnabled = SeidellIterations >= Parameters.EnableSwitchIteration;

		// для всех узлов в порядке обработки Зейделя
		for (auto&& it : SeidellOrder)
		{
			_MatrixInfo* pMatrixInfo{ it };
			CDynaNodeBase* pNode = pMatrixInfo->pNode;
			// рассчитываем нагрузку по СХН
			double& Pe{ pMatrixInfo->ImbP }, &Qe{ pMatrixInfo->ImbQ };
			// рассчитываем небалансы
			GetNodeImb(pMatrixInfo);
			cplx Unode(pNode->Vre, pNode->Vim);

			// для всех узлов кроме БУ обновляем статистику итерации
			pNodes->IterationControl().Update(pMatrixInfo);

			double Q = Qe + pNode->Qgr;	// расчетная генерация в узле

			cplx I1 = dStep / conj(Unode) / pNode->YiiSuper;

			switch (pNode->eLFNodeType_)
			{
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMAX:
			case CDynaNodeBase::eLFNodeType::LFNT_PVQMIN:
			case CDynaNodeBase::eLFNodeType::LFNT_PV:
			case CDynaNodeBase::eLFNodeType::LFNT_PQ:
			{
				cplx dU = I1 * cplx(Pe, -Qe);
				pNode->Vre += dU.real();
				pNode->Vim += dU.imag();
			}
			break;
			case CDynaNodeBase::eLFNodeType::LFNT_BASE:
				pNode->Pgr += Pe;
				pNode->Qgr += Qe;
				Pe = Qe = 0.0;
				break;
			}

			pNode->UpdateVDeltaSuper();
			// для всех узлов кроме статистику итерации
			pNodes->IterationControl().Update(pMatrixInfo);
		}

		if (!CheckLF())
			// если итерация привела не недопустимому режиму - выходим
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		pNodes->DumpIterationControl();

		// если достигли заданного небаланса - выходим
		if (pNodes->IterationControl().Converged(Parameters.Imb))
			break;
	}

	// TODO !!!!! пересчитываем проводимости узлов без устранения отрицательных сопротивлений
	//pNodes->CalcAdmittances(false);
}

double CLoadFlow::Qgtanh(CDynaNodeBase *pNode)
{
	double Qs = (pNode->LFQmax - pNode->LFQmin) / 2.0;
	double Qm = (pNode->LFQmax + pNode->LFQmin) / 2.0;
	double k = 1.0 + 0.0001;
	double Qg0 = (std::min)(pNode->LFQmax - Parameters.Imb, (std::max)(0.0, pNode->LFQmin + Parameters.Imb));
	double ofs = atanh((Qg0 - Qm) / Qs) / TanhBeta * 0.0;
	return Qs * tanh(TanhBeta * (pNode->LFVref - pNode->V + ofs) / pNode->Unom) + Qm;
	_CheckNumber(pNode->Qgr);
}


void CLoadFlow::BuildMatrixTanh()
{
	double *pb = klu.B();
	double *pAx = klu.Ax();

	// обходим только узлы в матрице
	for (_MatrixInfo *pMatrixInfo = pMatrixInfo_.get(); pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
	{

		// здесь считаем, что нагрузка СХН в Node::pnr/Node::qnr уже в расчете и анализе небалансов
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		GetPnrQnrSuper(pNode);
		cplx Sneb;
		if (pNode->eLFNodeType_ != CDynaNodeBase::eLFNodeType::LFNT_PQ)
			pNode->Qgr = Qgtanh(pNode);
		double dPdDelta(0.0), dQdDelta(0.0);
		double dPdV = pNode->GetSelfdPdV();
		double dQdV = pNode->GetSelfdQdV();
		if (pNode->eLFNodeType_ != CDynaNodeBase::eLFNodeType::LFNT_PQ)
		{

			double Qs = (pNode->LFQmax - pNode->LFQmin) / 2.0;
			double Qm = (pNode->LFQmax + pNode->LFQmin) / 2.0;
			double Qg0 = (std::min)(pNode->LFQmax - Parameters.Imb, (std::max)(0.0, pNode->LFQmin + Parameters.Imb));
			double ofs = atanh((Qg0 - Qm) / Qs) / TanhBeta * 0.0;
			double co = cosh(TanhBeta * (pNode->LFVref - pNode->V + ofs) / pNode->Unom);
			double ddQ = TanhBeta * Qs / co / co / pNode->Unom;
			dQdV += ddQ;
		}

		double *pAxSelf = pAx;
		pAx += 2;
		cplx UnodeConj(pNode->Vre, -pNode->Vim);
		for (VirtualBranch *pBranch = pMatrixInfo->pNode->pVirtualBranchBegin_; pBranch < pMatrixInfo->pNode->pVirtualBranchEnd_; pBranch++)
		{
			CDynaNodeBase *pOppNode = pBranch->pNode;
			cplx neb = cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
			cplx mult = UnodeConj * neb;
			Sneb -= neb;

			dPdDelta -= mult.imag();
			dPdV += -CDevice::ZeroDivGuard(mult.real(), pNode->V);
			dQdDelta += -mult.real();
			dQdV += CDevice::ZeroDivGuard(mult.imag(), pNode->V);

			if (NodeInMatrix(pOppNode))
			{
				double dPdDeltaM = mult.imag();
				double dPdVM = -CDevice::ZeroDivGuard(mult.real(), pOppNode->V);
				double dQdDeltaM = mult.real();
				double dQdVM = CDevice::ZeroDivGuard(mult.imag(), pOppNode->V);
				*pAx = dPdDeltaM;
				*(pAx + pMatrixInfo->nRowCount) = dQdDeltaM;
				pAx++;
				*pAx = dPdVM;
				*(pAx + pMatrixInfo->nRowCount) = dQdVM;
				pAx++;
			}
		}
		*pAxSelf = dPdDelta;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdDelta;
		pAxSelf++;
		*pAxSelf = dPdV;
		*(pAxSelf + pMatrixInfo->nRowCount) = dQdV;

		pAx += pMatrixInfo->nRowCount;


		Sneb -= std::conj(UnodeConj) * pNode->YiiSuper;
		Sneb = std::conj(Sneb) * std::conj(UnodeConj) + cplx(pNode->Pnr - pNode->Pgr - pMatrixInfo->UncontrolledP, pNode->Qnr - pNode->Qgr - pMatrixInfo->UncontrolledQ);

		*pb = Sneb.real(); pb++;
		*pb = Sneb.imag(); pb++;
	}
}

#ifdef _DEBUG
void CLoadFlow::CompareWithRastr()
{
	FILE *s;
	fopen_s(&s, "c:\\tmp\\rastrnodes.csv", "w+");

	CDynaNodeBase *pNodeMaxV(nullptr);
	CDynaNodeBase *pNodeMaxDelta(nullptr);
	CDynaNodeBase *pNodeMaxQg(nullptr);
	CDynaNodeBase *pNodeMaxPnr(nullptr);
	CDynaNodeBase *pNodeMaxQnr(nullptr);

	for (auto&& it : pNodes->DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		if (pNode->IsStateOn())
		{
			double mx = std::abs(pNode->V - pNode->Vrastr);
			if (pNodeMaxV)
			{
				if (mx > std::abs(pNodeMaxV->V - pNodeMaxV->Vrastr))
					pNodeMaxV = pNode;
			}
			else
				pNodeMaxV = pNode;

			mx = std::abs(pNode->Delta - pNode->Deltarastr);
			if (pNodeMaxDelta)
			{
				if (mx > std::abs(pNodeMaxDelta->V - pNodeMaxDelta->Vrastr))
					pNodeMaxDelta = pNode;
			}
			else
				pNodeMaxDelta = pNode;


			mx = std::abs(pNode->Qg - pNode->Qgrastr);
			if (pNodeMaxQg)
			{
				if (mx > std::abs(pNodeMaxQg->Qg - pNodeMaxQg->Qgrastr))
					pNodeMaxQg = pNode;
			}
			else
				pNodeMaxQg = pNode;

			mx = std::abs(pNode->Qnr - pNode->Qnrrastr);
			if (pNodeMaxQnr)
			{
				if (mx > std::abs(pNodeMaxQnr->Qnr - pNodeMaxQnr->Qnrrastr))
					pNodeMaxQnr = pNode;
			}
			else
				pNodeMaxQnr = pNode;

			mx = std::abs(pNode->Pnr - pNode->Pnrrastr);
			if (pNodeMaxPnr)
			{
				if (mx > std::abs(pNodeMaxPnr->Pnr - pNodeMaxQnr->Pnrrastr))
					pNodeMaxPnr = pNode;
			}
			else
				pNodeMaxPnr = pNode;
		}
		//ATLTRACE("\n %20f %20f %20f %20f %20f %20f", pNode->V, pNode->Delta * 180 / M_PI, pNode->Pg, pNode->Qg, pNode->Pnr, pNode->Qnr);
		fprintf(s, "%td;%20g;%20g\n", pNode->GetId(), pNode->V.Value, pNode->Delta.Value * 180 / M_PI);
	}
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, "Rastr differences");
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("V     {} {}", pNodeMaxV->V - pNodeMaxV->Vrastr, pNodeMaxV->GetVerbalName()));
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Delta {} {}", pNodeMaxDelta->Delta - pNodeMaxDelta->Deltarastr, pNodeMaxDelta->GetVerbalName()));
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Qg    {} {}", pNodeMaxQg->Qg - pNodeMaxQg->Qgrastr, pNodeMaxQg->GetVerbalName()));
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Pnr   {} {}", pNodeMaxPnr->Pnr - pNodeMaxPnr->Pnrrastr, pNodeMaxPnr->GetVerbalName()));
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Qnr   {} {}", pNodeMaxQnr->Qnr - pNodeMaxQnr->Qnrrastr, pNodeMaxQnr->GetVerbalName()));
	fclose(s);
}
#endif

void CLoadFlow::ContinuousNewton()
{
	pDynaModel->Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningContinuousNewton);
	size_t& it{ pNodes->IterationControl().Number };
	//Limits limits(*this);

	// квадраты небалансов до и после итерации
	double g0(0.0), g1(0.1);
	lambda_ = 1.0;
	NodeTypeSwitchesDone = 1;
	it = 0;

	const double hmin{ 0.2 }, hmax{3.0};
	double h{ hmin };
	const double eps{ 0.01 };
	
	while (1)
	{
		if (!CheckLF())
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		/*if (it > Parameters.MaxIterations)
			throw dfw2error(CDFW2Messages::m_cszLFNoConvergence);*/

		pNodes->IterationControl().Reset();
		//limits.Clear();

		const auto Imb = [this]()
		{
			_MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };
			for (; pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
			{
				const auto& pNode{ pMatrixInfo->pNode };
				GetNodeImb(pMatrixInfo);
			}
		};

		const auto addvecs = [](double* x, double mult, double const* y, size_t size, bool clear = false)
		{
			const double* end{ x + size };
			for (; x < end; x++, y++)
			{
				if (clear)
					*x = 0;
				*x += *y * mult;
			}
		};

		const auto periodAnlge = [](double* x, size_t size)
		{
			const double* end{ x + size };
			for (; x < end; x += 2)
				*x = MathUtils::AngleRoutines::WrapPosNegPI(*x);
		};


		// считаем небаланс по всем узлам кроме БУ
		_MatrixInfo* pMatrixInfo{ pMatrixInfo_.get() };
		for (; pMatrixInfo < pMatrixInfoEnd; pMatrixInfo++)
		{
			const auto& pNode{ pMatrixInfo->pNode };
			GetNodeImb(pMatrixInfo);
			if (pNode->eLFNodeType_ == CDynaNodeBase::eLFNodeType::LFNT_PV)
				pNode->Qgr = pNode->Qgr + pMatrixInfo->ImbQ;
			pNodes->IterationControl().Update(pMatrixInfo);
		}

		// общее количество узлов с нарушенными ограничениями и подлежащих переключению
		//pNodes->IterationControl().QviolatedCount = limits.ViolatedCount();

		UpdateSlackBusesImbalance();

		g1 = GetSquaredImb();

		NewtonStepRatio.Ratio_ = h;

		// отношение квадратов невязок
		pNodes->IterationControl().ImbRatio = g0 > 0.0 ? g1 / g0 : 0.0;
		DumpNewtonIterationControl();
		it++;

		if (pNodes->IterationControl().Converged(1000.0))
			break;

		if (pNodes->IterationControl().ImbRatio > 1)
		{
			RestoreVDelta();
			NewtonStepRatio.eStepCause = LFNewtonStepRatio::eStepLimitCause::Backtrack;
			h *= 0.5;
		}

		// сохраняем исходные значения переменных
		StoreVDelta();

		//limits.Apply();

		lambda_ = 1.0;
		g0 = g1;

		BuildMatrixCurrent();
		SolveLinearSystem();

		auto k1{ std::make_unique<double[]>(klu.MatrixSize()) };
		std::copy(klu.B(), klu.B() + klu.MatrixSize(), k1.get());

		int Method{ 1 };

		if (Method == 0)
		{
			// RK4

			RestoreUpdateVDelta(k1.get(), -h * 0.5);
			BuildMatrixCurrent();
			SolveLinearSystem();
			auto k2{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k2.get());

			RestoreUpdateVDelta(k2.get(), -h * 0.5);
			BuildMatrixCurrent();
			SolveLinearSystem();
			auto k3{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k3.get());

			RestoreUpdateVDelta(k3.get(), -h);
			BuildMatrixCurrent();
			SolveLinearSystem();
			auto k4{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k4.get());
			UpdateVDelta(k1.get(), -h / 6);
			UpdateVDelta(k2.get(), -2.0 * h / 6);
			UpdateVDelta(k3.get(), -2.0 * h / 6);
			UpdateVDelta(k4.get(), h / 6);
		}
		else if (Method == 1)
		{
			// RKF

			auto d{ std::make_unique<double[]>(klu.MatrixSize()) };
						
			(addvecs)(d.get(), -h / 3.0, k1.get(), klu.MatrixSize(), true);
			
		
			RestoreUpdateVDelta(d.get(), 1.0);
			BuildMatrixCurrent();
			SolveLinearSystem();

			auto k2{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k2.get());

			(addvecs)(d.get(), -h / 6.0, k1.get(), klu.MatrixSize(), true);
			(addvecs)(d.get(), -h / 6.0, k2.get(), klu.MatrixSize(), false);


			(periodAnlge)(d.get(), klu.MatrixSize());
			RestoreUpdateVDelta(d.get(), 1.0);
			//RestoreUpdateVDelta(k1.get(), -h / 6.0);
			//UpdateVDelta(k2.get(), -h / 6.0);
			BuildMatrixCurrent();
			SolveLinearSystem();

			auto k3{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k3.get());

			(addvecs)(d.get(), -h / 8.0, k1.get(), klu.MatrixSize(), true);
			(addvecs)(d.get(), -h * 3.0 / 8.0, k3.get(), klu.MatrixSize(), false);

			(periodAnlge)(d.get(), klu.MatrixSize());
			RestoreUpdateVDelta(d.get(), 1.0);

			//RestoreUpdateVDelta(k1.get(), -h / 8.0);
			//UpdateVDelta(k3.get(), -h * 3.0 / 8.0);
			BuildMatrixCurrent();
			SolveLinearSystem();

			auto k4{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k4.get());

			(addvecs)(d.get(), -h / 2.0, k1.get(), klu.MatrixSize(), true);
			(addvecs)(d.get(), h * 3.0 / 2.0, k3.get(), klu.MatrixSize(), false);
			(addvecs)(d.get(), -h * 2.0, k4.get(), klu.MatrixSize(), false);
			(periodAnlge)(d.get(), klu.MatrixSize());
			RestoreUpdateVDelta(d.get(), 1.0);

			//RestoreUpdateVDelta(k1.get(), -h / 2.0);
			//UpdateVDelta(k3.get(), h * 3.0 / 2.0);
			//UpdateVDelta(k4.get(), -h * 2.0);
			BuildMatrixCurrent();
			SolveLinearSystem();

			auto k5{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k5.get());

			RestoreVDelta();

			auto a1{ std::make_unique<double[]>(klu.MatrixSize()) };
			auto a2{ std::make_unique<double[]>(klu.MatrixSize()) };

			pMatrixInfo = pMatrixInfo_.get();

			double r{ 0.0 };
			double rms{ 0.0 };

			double* pk1{ k1.get() };
			double* pk2{ k2.get() };
			double* pk3{ k3.get() };
			double* pk4{ k4.get() };
			double* pk5{ k5.get() };
			double* pa1{ a1.get() };
			double* pa2{ a2.get() };

			for(double *pd {d.get()} ; pd < d.get() + klu.MatrixSize() ; pd++)
			{

				*pa1 = - h * (*pk1 * 0.5 - *pk3 * 3.0 / 2.0 + *pk4 * 2.0);
				*pa2 = - h * (*pk1 / 6.0 + *pk4 * 2.0 / 3.0 + *pk5 / 6.0);

				double er{ std::abs(*pa1 - *pa2) };
				if (r < er)
					r = er;

				rms += er * er;

				pk1++; pk2++; pk3++; pk4++; pk5++; pa1++; pa2++;
			}


			for (_MatrixInfo* pMatrix = pMatrixInfo_.get(); pMatrix < pMatrixInfoEnd; pMatrix++)
			{
				const auto& pNode{ pMatrix->pNode };

				double* pk1{ k1.get() + pNode->A(0) };
				double* pk2{ k2.get() + pNode->A(0) };
				double* pk3{ k3.get() + pNode->A(0) };
				double* pk4{ k4.get() + pNode->A(0) };
				double* pk5{ k5.get() + pNode->A(0) };
				double* pa1{ a1.get() + pNode->A(0) };
				double* pa2{ a2.get() + pNode->A(0) };

				for (int i = 0; i < 2; i++)
				{
					*pa1 = -h * (*pk1 * 0.5 - *pk3 * 3.0 / 2.0 + *pk4 * 2.0);
					*pa2 = -h * (*pk1 / 6.0 + *pk4 * 2.0 / 3.0 + *pk5 / 6.0);

					double er{ std::abs(* pa1 - *pa2)};
					if (i == 0)
						er = std::abs(MathUtils::AngleRoutines::WrapPosNegPI(er));
					else
						er /= pNode->Unom;

					if (r < er)
						r = er;

					rms += er * er;

					pk1++; pk2++; pk3++; pk4++; pk5++; pa1++; pa2++;
					
				}
			}


			(addvecs)(d.get(), 1.0, a2.get(), klu.MatrixSize(), true);
			(addvecs)(d.get(),  1.0 / 5.0, a1.get(), klu.MatrixSize(), false);
			(addvecs)(d.get(), -1.0 / 5.0, a2.get(), klu.MatrixSize(), false);
			(periodAnlge)(d.get(), klu.MatrixSize());

			RestoreVDelta();
			if (double ratio{ GetNewtonRatio(d.get()) }; ratio < 1.0)
			{
				h *= ratio;
				continue;
			}

			UpdateVDelta(d.get());
		
			rms = std::sqrt(rms / klu.MatrixSize() );
			r = rms;

			r /= 5 * h;
			double hk{ 0.9 * std::pow(eps / r, 0.25) };
			h *= hk;
			h = (std::min)((std::max)(h, hmin), hmax);
			NewtonStepRatio.eStepCause = LFNewtonStepRatio::eStepLimitCause::None;

			/*
			if (hk < 0.1)
			{
				RestoreVDelta();
				NewtonStepRatio.eStepCause = LFNewtonStepRatio::eStepLimitCause::Backtrack;
			}
			*/
		}
		else if (Method == 2)
		{
			//RK2 Ralston

			RestoreUpdateVDelta(k1.get(), -h * 3.0 / 4.0);
			BuildMatrixCurrent();
			SolveLinearSystem();
			auto k2{ std::make_unique<double[]>(klu.MatrixSize()) };
			std::copy(klu.B(), klu.B() + klu.MatrixSize(), k2.get());

			double r{ 0.0 };
			double ksi{ 0.0 };
			const double sigma1{0.95}, sigma2{1.05}, tmin{0.1}, tmax{1.0};
			pMatrixInfo = pMatrixInfo_.get();

			auto d{ std::make_unique<double[]>(klu.MatrixSize()) };
			double* pk1{ k1.get() }, * pk2{ k2.get() };

			
			bool bDelta{ true };
			for (double* pd{ d.get() }; pd < d.get() + klu.MatrixSize(); pd++, pk1++, pk2++)
			{
				*pd = -h / 3.0 * (*pk1 + *pk2 * 2.0);
				auto val{ std::abs(*pk1) };
				if (bDelta)
					val = MathUtils::AngleRoutines::WrapPosNegPI(val) / 2 / M_PI;
				else
					val /= 100;

				bDelta = !bDelta;

				if (ksi < val)
					ksi = val;
			}

			RestoreVDelta();
			if (double ratio{ GetNewtonRatio(d.get()) }; ratio < 1.0)
			{
				h *= ratio;
				continue;
			}

			UpdateVDelta(d.get());

			if (ksi / 2.0 > 0.3)
				h = (std::max)(h * sigma1, tmin);
			else
				h = (std::min)(h * sigma2, tmax);

		}
	}

	// обновляем реактивную генерацию в суперузлах
	//UpdateSupernodesPQ();
}