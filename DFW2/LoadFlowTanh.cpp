#include "stdafx.h"
#include "LoadFlow.h"
#include "DynaModel.h"

using namespace DFW2;

void CLoadFlow::NewtonTanh()
{
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningNewton);
	int it(0);	// количество итераций

	// вектор для указателей переключаемых узлов, с размерностью в половину уравнений матрицы

	double g0(0.0), g1(0.1), lambda(1.0);

	while (1)
	{
		//if (!CheckLF())
		//	throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		if (++it > m_Parameters.m_nMaxIterations)
			throw dfw2error(CDFW2Messages::m_cszLFNoConvergence);

		pNodes->m_IterationControl.Reset();

		// считаем небаланс по всем узлам кроме БУ
		_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get();
		for (; pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			if(pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ)
				pNode->Qgr = Qgtanh(pNode);
			GetNodeImb(pMatrixInfo);	// небаланс считается с учетом СХН
			pNodes->m_IterationControl.Update(pMatrixInfo);
		}
		// досчитываем небалансы в БУ
		for (pMatrixInfo = m_pMatrixInfoEnd; pMatrixInfo < m_pMatrixInfoSlackEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			GetNodeImb(pMatrixInfo);
			// для БУ небалансы только для результатов
			pNode->Pgr += pMatrixInfo->m_dImbP;
			pNode->Qgr += pMatrixInfo->m_dImbQ;
			// в контроле сходимости небаланс БУ всегда 0.0
			pMatrixInfo->m_dImbP = pMatrixInfo->m_dImbQ = 0.0;
			pNodes->m_IterationControl.Update(pMatrixInfo);
		}

		g1 = GetSquaredImb();

		pNodes->m_IterationControl.m_ImbRatio = g1;
		pNodes->DumpIterationControl();
		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;

		if (it > 100000 && g1 > g0)
		{
			double gs1v = -CDynaModel::gs1(klu, m_Rh, klu.B());
			// знак gs1v должен быть "-" ????
			lambda *= -0.5 * gs1v / (g1 - g0 - gs1v);
			RestoreVDelta();
			UpdateVDelta(lambda);
			continue;
		}

		lambda = 1.0;
		g0 = g1;

		BuildMatrixTanh();

		// сохраняем небаланс до итерации
		std::copy(klu.B(), klu.B() + klu.MatrixSize(), m_Rh.get());

		// сохраняем исходные значения переменных
		StoreVDelta();
		// сохраняем небаланс до итерации
		std::copy(klu.B(), klu.B() + klu.MatrixSize(), m_Rh.get());


		ptrdiff_t iMax(0);
		double maxb = klu.FindMaxB(iMax);
		CDynaNodeBase *pNode1(m_pMatrixInfo.get()[iMax / 2].pNode);
		
		SolveLinearSystem();

		double *bx = klu.B();

		maxb = klu.FindMaxB(iMax);
		CDynaNodeBase *pNode2(m_pMatrixInfo.get()[iMax / 2].pNode);

		for (pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
		{
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ)
			{
				double Vbackup = pNode->V;
				double *pb = klu.B() + pNode->A(1);
				pNode->V += *pb;
				double dQ = fabs(pNode->Qgr - Qgtanh(pNode));
				if (dQ > 0.9 * (pNode->LFQmax - pNode->LFQmin))
				{
					double newLambda = fabs(Vbackup - pNode->LFVref) / fabs(*pb);
					if (lambda > newLambda)
						lambda = newLambda;

				}
				pNode->V = Vbackup;
			}
		}



		

		// обновляем переменные
		UpdateVDelta(lambda);

	}

	// обновляем реактивную генерацию в суперузлах

	for (auto && supernode : m_SuperNodeParameters)
	{
		CDynaNodeBase *pNode = supernode.m_pNode;
		double Qrange = pNode->LFQmax - pNode->LFQmin;
		Qrange = (Qrange > 0.0) ? (pNode->Qgr - pNode->LFQmin) / Qrange : 0.0;
		CLinkPtrCount *pLink = pNode->GetSuperLink(0);
		CDevice **ppDevice(nullptr);
		while (pLink->In(ppDevice))
		{
			CDynaNodeBase *pSlaveNode = static_cast<CDynaNodeBase*>(*ppDevice);
			pSlaveNode->Qg = 0.0;
			if (pSlaveNode->IsStateOn())
				pSlaveNode->Qg = pSlaveNode->LFQmin + (pSlaveNode->LFQmax - pSlaveNode->LFQmin) * Qrange;
		}
		pNode->Qgr = supernode.LFQmin + (supernode.LFQmax - supernode.LFQmin) * Qrange;
		supernode.Restore();
	}

	for (auto&& it : pNodes->m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		if (!pNode->m_pSuperNodeParent)
		{
			pNode->Qg = pNode->IsStateOn() ? pNode->Qgr : 0.0;
			GetPnrQnr(pNode);
		}
	}
}

void CLoadFlow::SeidellTanh()
{
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszLFRunningSeidell);

	MATRIXINFO SeidellOrder;
	SeidellOrder.reserve(m_pMatrixInfoSlackEnd - m_pMatrixInfo.get());

	_MatrixInfo* pMatrixInfo = m_pMatrixInfoSlackEnd - 1;

	// в начало добавляем БУ
	for (; pMatrixInfo >= m_pMatrixInfoEnd; pMatrixInfo--)
	{
		SeidellOrder.push_back(pMatrixInfo);
		pMatrixInfo->bVisited = true;
	}

	// затем PV узлы
	for (; pMatrixInfo >= m_pMatrixInfo.get(); pMatrixInfo--)
	{
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ)
		{
			SeidellOrder.push_back(pMatrixInfo);
			pMatrixInfo->bVisited = true;
		}
	}

	// сортируем PV узлы по убыванию диапазона реактивной мощности
	sort(SeidellOrder.begin() + (m_pMatrixInfoSlackEnd - m_pMatrixInfoEnd), SeidellOrder.end(), SortPV);

	// добавляем узлы в порядок обработки Зейделем с помощью очереди
	// очередь строим от базисных и PV-узлов по связям. Порядок очереди 
	// определяем по мере удаления от узлов базисных и PV-узлов 
	QUEUE queue;
	for (auto&& it : SeidellOrder)
		AddToQueue(it, queue);


	// пока в очереди есть узлы
	while (!queue.empty())
	{
		// достаем узел из очереди
		pMatrixInfo = queue.front();
		queue.pop_front();
		// добавляем узел в очередь Зейделя
		SeidellOrder.push_back(pMatrixInfo);
		// и добавляем оппозитные узлы добавленного узла
		AddToQueue(pMatrixInfo, queue);
	}

	_ASSERTE(SeidellOrder.size() == m_pMatrixInfoSlackEnd - m_pMatrixInfo.get());

	// рассчитываем проводимости узлов с устранением отрицательных сопротивлений
	pNodes->CalcAdmittances(true);
	double dPreviousImb = -1.0;
	for (int nSeidellIterations = 0; nSeidellIterations < m_Parameters.m_nSeidellIterations; nSeidellIterations++)
	{
		// множитель для ускорения Зейделя
		double dStep = m_Parameters.m_dSeidellStep;

		if (nSeidellIterations > 2)
		{
			// если сделали более 2-х итераций начинаем анализировать небалансы
			if (dPreviousImb < 0.0)
			{
				// первый небаланс, если еще не рассчитывали
				dPreviousImb = ImbNorm(pNodes->m_IterationControl.m_MaxImbP.GetDiff(), pNodes->m_IterationControl.m_MaxImbQ.GetDiff());
			}
			else
			{
				// если есть предыдущий небаланс, рассчитываем отношение текущего и предыдущего
				double dCurrentImb = ImbNorm(pNodes->m_IterationControl.m_MaxImbP.GetDiff(), pNodes->m_IterationControl.m_MaxImbQ.GetDiff());
				double dImbRatio = dCurrentImb / dPreviousImb;
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
		pNodes->m_IterationControl.Reset();
		// определяем можно ли выполнять переключение типов узлов (по количеству итераций)
		bool bPVPQSwitchEnabled = nSeidellIterations >= m_Parameters.m_nEnableSwitchIteration;

		// для всех узлов в порядке обработки Зейделя
		for (auto&& it : SeidellOrder)
		{
			pMatrixInfo = it;
			CDynaNodeBase *pNode = pMatrixInfo->pNode;
			// рассчитываем нагрузку по СХН
			GetPnrQnrSuper(pNode);
			pNode->Qgr = Qgtanh(pNode);
			double& Pe = pMatrixInfo->m_dImbP;
			double& Qe = pMatrixInfo->m_dImbQ;
			// рассчитываем небалансы
			Pe = pNode->GetSelfImbP();
			Qe = pNode->GetSelfImbQ();

			cplx Unode(pNode->Vre, pNode->Vim);
			for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
			{
				CDynaNodeBase *pOppNode = pBranch->pNode;
				cplx mult = conj(Unode) * cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
				Pe -= mult.real();
				Qe += mult.imag();
			}

			double Q = Qe + pNode->Qgr;	// расчетная генерация в узле

			cplx I1 = dStep / conj(Unode) / pNode->YiiSuper;

			switch (pNode->m_eLFNodeType)
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
			pNodes->m_IterationControl.Update(pMatrixInfo);
		}

		if (!CheckLF())
			// если итерация привела не недопустимому режиму - выходим
			throw dfw2error(CDFW2Messages::m_cszUnacceptableLF);

		pNodes->DumpIterationControl();

		// если достигли заданного небаланса - выходим
		if (pNodes->m_IterationControl.Converged(m_Parameters.m_Imb))
			break;
	}

	// пересчитываем проводимости узлов без устранения отрицательных сопротивлений
	pNodes->CalcAdmittances(false);
}

double CLoadFlow::Qgtanh(CDynaNodeBase *pNode)
{
	double Qs = (pNode->LFQmax - pNode->LFQmin) / 2.0;
	double Qm = (pNode->LFQmax + pNode->LFQmin) / 2.0;
	double k = 1.0 + 0.0001;
	double Qg0 = min(pNode->LFQmax - m_Parameters.m_Imb, max(0.0, pNode->LFQmin + m_Parameters.m_Imb));
	double ofs = atanh((Qg0 - Qm) / Qs) / m_dTanhBeta * 0.0;
	return Qs * tanh(m_dTanhBeta * (pNode->LFVref - pNode->V + ofs) / pNode->Unom) + Qm;
	_CheckNumber(pNode->Qgr);
}


void CLoadFlow::BuildMatrixTanh()
{
	double *pb = klu.B();
	double *pAx = klu.Ax();

	// обходим только узлы в матрице
	for (_MatrixInfo *pMatrixInfo = m_pMatrixInfo.get(); pMatrixInfo < m_pMatrixInfoEnd; pMatrixInfo++)
	{

		ptrdiff_t k = pb - klu.B();
		// здесь считаем, что нагрузка СХН в Node::pnr/Node::qnr уже в расчете и анализе небалансов
		CDynaNodeBase *pNode = pMatrixInfo->pNode;
		GetPnrQnrSuper(pNode);
		double Pe = pNode->GetSelfImbP();
		if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ)
			pNode->Qgr = Qgtanh(pNode);
		double Qe = pNode->GetSelfImbQ();
		_CheckNumber(Qe);
		double dPdDelta(0.0), dQdDelta(0.0);
		double dPdV = pNode->GetSelfdPdV();
		double dQdV = pNode->GetSelfdQdV();
		if (pNode->m_eLFNodeType != CDynaNodeBase::eLFNodeType::LFNT_PQ)
		{

			double Qs = (pNode->LFQmax - pNode->LFQmin) / 2.0;
			double Qm = (pNode->LFQmax + pNode->LFQmin) / 2.0;
			double Qg0 = min(pNode->LFQmax - m_Parameters.m_Imb, max(0.0, pNode->LFQmin + m_Parameters.m_Imb));
			double ofs = atanh((Qg0 - Qm) / Qs) / m_dTanhBeta * 0.0;
			double co = cosh(m_dTanhBeta * (pNode->LFVref - pNode->V + ofs) / pNode->Unom);
			double ddQ = m_dTanhBeta * Qs / co / co / pNode->Unom;
			dQdV += ddQ;
		}

		double *pAxSelf = pAx;
		pAx += 2;
		cplx Unode(pNode->Vre, pNode->Vim);
		for (VirtualBranch *pBranch = pMatrixInfo->pNode->m_VirtualBranchBegin; pBranch < pMatrixInfo->pNode->m_VirtualBranchEnd; pBranch++)
		{
			CDynaNodeBase *pOppNode = pBranch->pNode;
			cplx mult = conj(Unode);
			mult *= cplx(pOppNode->Vre, pOppNode->Vim) * pBranch->Y;
			Pe -= mult.real();
			Qe += mult.imag();

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



		*pb = Pe; pb++;
		*pb = Qe; pb++;
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

	for (auto&& it : pNodes->m_DevVec)
	{
		CDynaNodeBase *pNode = static_cast<CDynaNodeBase*>(it);
		if (pNode->IsStateOn())
		{
			double mx = fabs(pNode->V - pNode->Vrastr);
			if (pNodeMaxV)
			{
				if (mx > fabs(pNodeMaxV->V - pNodeMaxV->Vrastr))
					pNodeMaxV = pNode;
			}
			else
				pNodeMaxV = pNode;

			mx = fabs(pNode->Delta - pNode->Deltarastr);
			if (pNodeMaxDelta)
			{
				if (mx > fabs(pNodeMaxDelta->V - pNodeMaxDelta->Vrastr))
					pNodeMaxDelta = pNode;
			}
			else
				pNodeMaxDelta = pNode;


			mx = fabs(pNode->Qg - pNode->Qgrastr);
			if (pNodeMaxQg)
			{
				if (mx > fabs(pNodeMaxQg->Qg - pNodeMaxQg->Qgrastr))
					pNodeMaxQg = pNode;
			}
			else
				pNodeMaxQg = pNode;

			mx = fabs(pNode->Qnr - pNode->Qnrrastr);
			if (pNodeMaxQnr)
			{
				if (mx > fabs(pNodeMaxQnr->Qnr - pNodeMaxQnr->Qnrrastr))
					pNodeMaxQnr = pNode;
			}
			else
				pNodeMaxQnr = pNode;

			mx = fabs(pNode->Pnr - pNode->Pnrrastr);
			if (pNodeMaxPnr)
			{
				if (mx > fabs(pNodeMaxPnr->Pnr - pNodeMaxQnr->Pnrrastr))
					pNodeMaxPnr = pNode;
			}
			else
				pNodeMaxPnr = pNode;
		}
		//ATLTRACE("\n %20f %20f %20f %20f %20f %20f", pNode->V, pNode->Delta * 180 / M_PI, pNode->Pg, pNode->Qg, pNode->Pnr, pNode->Qnr);
		fprintf(s, "%d;%20g;%20g\n", pNode->GetId(), pNode->V, pNode->Delta * 180 / M_PI);
	}
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Rastr differences"));
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("V     %g %s"), pNodeMaxV->V - pNodeMaxV->Vrastr, pNodeMaxV->GetVerbalName());
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Delta %g %s"), pNodeMaxDelta->Delta - pNodeMaxDelta->Deltarastr, pNodeMaxDelta->GetVerbalName());
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Qg    %g %s"), pNodeMaxQg->Qg - pNodeMaxQg->Qgrastr, pNodeMaxQg->GetVerbalName());
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Pnr   %g %s"), pNodeMaxPnr->Pnr - pNodeMaxPnr->Pnrrastr, pNodeMaxPnr->GetVerbalName());
	m_pDynaModel->Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Qnr   %g %s"), pNodeMaxQnr->Qnr - pNodeMaxQnr->Qnrrastr, pNodeMaxQnr->GetVerbalName());
	fclose(s);
}
#endif

