#include "stdafx.h"
#include "DynaModel.h"
#include "klu.h"
#include "cs.h"
#include "stdio.h"
using namespace DFW2;

void CDynaModel::ReportKLUError()
{
	switch (Common.status)
	{
	case 0:
		Log(CDFW2Messages::DFW2LOG_INFO, CDFW2Messages::m_cszKLUOk);
		break;
	case 1:
		Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszKLUSingular);
		break;
	case -2:
		Log(CDFW2Messages::DFW2LOG_FATAL, CDFW2Messages::m_cszKLUOutOfMemory);
		break;
	case -3:
		Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszKLUInvalidInput);
		break;
	case -4:
		Log(CDFW2Messages::DFW2LOG_ERROR, CDFW2Messages::m_cszKLUIntOverflow);
		break;
	}
}

bool CDynaModel::Status()
{
	return m_bStatus;
}

void CDynaModel::Log(CDFW2Messages::DFW2MessageStatus Status,ptrdiff_t nDBIndex, const _TCHAR* cszMessage)
{
	Log(Status, cszMessage);
}

void CDynaModel::Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage, ...)
{
	const _TCHAR *cszCRLF = _T("\n");

	if (m_Parameters.m_bLogToConsole)
	{
		HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hCon, &csbi);

		switch (Status)
		{
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_FATAL:
			SetConsoleTextAttribute(hCon, BACKGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_ERROR:
			SetConsoleTextAttribute(hCon, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_WARNING:
			SetConsoleTextAttribute(hCon, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY );
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_MESSAGE:
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO:
			SetConsoleTextAttribute(hCon, FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			break;
		case CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG:
			SetConsoleTextAttribute(hCon, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		}
		_tcprintf(cszCRLF);
		va_list argList;
		va_start(argList, cszMessage);
		_vtprintf(cszMessage, argList);
		va_end(argList);
		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | FOREGROUND_RED);
	}

	if (m_pLogFile && m_Parameters.m_bLogToFile)
	{
		_ftprintf(m_pLogFile,cszCRLF);
		va_list argList;
		va_start(argList, cszMessage);
		_vftprintf(m_pLogFile,cszMessage, argList);
		va_end(argList);
	}
}


bool CDynaModel::ChangeOrder(ptrdiff_t Newq)
{
	bool bRes = true;

	if (sc.q != Newq)
		sc.RefactorMatrix();

	sc.q = Newq;

	switch (sc.q)
	{
	case 1:
		break;
	case 2:
		break;
	}
	return bRes;
}

struct RightVector* CDynaModel::GetRightVector(ptrdiff_t nRow)
{
	struct RightVector *pRv = NULL;
	if (nRow >= 0 && nRow < m_nMatrixSize)
		pRv = pRightVector + nRow;
	return pRv;
}




void CDynaModel::StopProcess()
{
	sc.m_bStopCommandReceived = true;
}

CDeviceContainer* CDynaModel::GetDeviceContainer(eDFW2DEVICETYPE Type)
{
	CDeviceContainer *pContainer = NULL;
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		if ((*it)->GetType() == Type)
		{
			pContainer = *it;
			break;
		}
	}
	return pContainer;
}

void CDynaModel::RebuildMatrix(bool bRebuild)
{
	if (!m_bRebuildMatrixFlag)
		m_bRebuildMatrixFlag = sc.m_bRefactorMatrix = bRebuild;
}

void CDynaModel::ProcessTopologyRequest()
{
	sc.m_bProcessTopology = true;
}

void CDynaModel::DiscontinuityRequest()
{
	sc.m_bDiscontinuityRequest = true;
}

double CDynaModel::GetWeightedNorm(double *pVector)
{
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + m_nMatrixSize;
	double *pb = pVector;
	double dSum = 0.0;
	for (; pVectorBegin < pVectorEnd; pVectorBegin++, pb++)
	{
		double weighted = pVectorBegin->GetWeightedError(*pb, *pVectorBegin->pValue);
		dSum += weighted * weighted;
	}
	return sqrt(dSum);
}

double CDynaModel::GetNorm(double *pVector)
{
	double dSum = 0.0;
	double *pb = pVector;
	double *pe = pVector + m_nMatrixSize;
	while (pb < pe)
	{
		dSum += *pb * *pb;
		pb++;
	}
	return sqrt(dSum);
}

bool CDynaModel::ServeDiscontinuityRequest()
{
	bool bRes = true;
	if (sc.m_bDiscontinuityRequest)
	{
		sc.m_bDiscontinuityRequest = false;
		bRes = EnterDiscontinuityMode() && bRes;
		bRes = ProcessDiscontinuity() && bRes;
	}
	return bRes;
}

bool CDynaModel::SetStateDiscontinuity(CDiscreteDelay *pDelayObject, double dDelay)
{
	return m_Discontinuities.SetStateDiscontinuity(pDelayObject, GetCurrentTime() + dDelay);
}

bool CDynaModel::RemoveStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	return m_Discontinuities.RemoveStateDiscontinuity(pDelayObject);
}

bool CDynaModel::CheckStateDiscontinuity(CDiscreteDelay *pDelayObject)
{
	return m_Discontinuities.CheckStateDiscontinuity(pDelayObject);
}

void CDynaModel::NotifyRelayDelay(const CRelayDelayLogic* pRelayDelay)
{
	m_Automatic.NotifyRelayDelay(pRelayDelay);
}

bool CDynaModel::PushVarSearchStack(CDevice*pDevice)
{
	if (pDevice)
	{
		if (m_setVisitedDevices.insert(pDevice).second)
		{
			if (m_ppVarSearchStackTop < m_ppVarSearchStackBase + m_Parameters.nVarSearchStackDepth)
			{
				*m_ppVarSearchStackTop = pDevice;
				m_ppVarSearchStackTop++;
				return true;
			}
			else
			{
				Log(CDFW2Messages::DFW2LOG_FATAL, Cex(CDFW2Messages::m_cszVarSearchStackDepthNotEnough, m_Parameters.nVarSearchStackDepth));
				return false;
			}
		}
	}
	return true;
}
bool CDynaModel::PopVarSearchStack(CDevice* &pDevice)
{
	if (m_ppVarSearchStackTop <= m_ppVarSearchStackBase)
		return false;
	m_ppVarSearchStackTop--;
	pDevice = *m_ppVarSearchStackTop;
	return true;
}

void CDynaModel::ResetStack()
{
	if (!m_ppVarSearchStackBase)
		m_ppVarSearchStackBase = new CDevice*[m_Parameters.nVarSearchStackDepth];
	m_ppVarSearchStackTop = m_ppVarSearchStackBase;
	m_setVisitedDevices.clear();
}

bool CDynaModel::InitExternalVariables()
{
	bool bRes = true;
	m_cszDampingName = (GetFreqDampingType() == APDT_ISLAND) ? CDynaNode::m_cszSz : CDynaNode::m_cszS;
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
			bRes = (*dit)->InitExternalVariables(this) && bRes;
	}

	bRes = UpdateExternalVariables() && bRes;

	return bRes;
}

bool CDynaModel::UpdateExternalVariables()
{
	bool bRes = true;
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
		{
			switch((*dit)->UpdateExternalVariables(this))
			{
			case DFS_DONTNEED:
				break;
			case DFS_FAILED:
				bRes = false;
			default:
				continue;
			}
			break;
		}
	}
	return bRes;
}


CDeviceContainer* CDynaModel::GetContainerByAlias(const _TCHAR* cszAlias)
{
	CDeviceContainer *pContainer(NULL);

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		if ((*it)->HasAlias(cszAlias))
		{
			pContainer = *it;
			break;
		}
	}

	return pContainer;
}

bool CDynaModel::ApproveContainerToWriteResults(CDeviceContainer *pDevCon)
{
	return pDevCon->Count() && pDevCon->GetType() != DEVTYPE_SYNCZONE && pDevCon->GetType() != DEVTYPE_BRANCH;
}


int ErrorCompare(void *pContext, const void *pValue1, const void *pValue2)
{
	RightVector **pV1 = (RightVector**)pValue1;
	RightVector **pV2 = (RightVector**)pValue2;
	return static_cast<int>((*pV2)->nErrorHits - (*pV1)->nErrorHits);
}

void CDynaModel::GetWorstEquations(ptrdiff_t nCount)
{
	RightVector **pSortOrder = new RightVector*[m_nMatrixSize];
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + m_nMatrixSize;
	RightVector **ppSortOrder = pSortOrder;

	while (pVectorBegin < pVectorEnd)
	{
		*ppSortOrder = pVectorBegin;
		pVectorBegin++;
		ppSortOrder++;
	}

	qsort_s(pSortOrder, m_nMatrixSize, sizeof(RightVector*), ErrorCompare, nullptr);

	if (nCount > m_nMatrixSize)
		nCount = m_nMatrixSize;

	ppSortOrder = pSortOrder;
	while (nCount)
	{
		pVectorBegin = *ppSortOrder;
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("%-6d %s %s Rtol %g Atol %g"), pVectorBegin->nErrorHits,
								   pVectorBegin->pDevice->GetVerbalName(),
								   pVectorBegin->pDevice->VariableNameByPtr(pVectorBegin->pValue),
								   pVectorBegin->Rtol,
								   pVectorBegin->Atol);
		ppSortOrder++;
		nCount--;
	}

	delete pSortOrder;

}

// ограничивает частоту изменения шага до минимального, просчитанного на серии шагов
// возвращает true, если изменение шага может быть разрешено
bool CDynaModel::StepControl::FilterStep(double dStep)
{
	// определяем минимальный шаг в серии шагов, длина которой
	// nStepsToStepChange

	if (dFilteredStepInner > dStep)
		dFilteredStepInner = dStep;

	dFilteredStep = dFilteredStepInner;

	// если не достигли шага, на котором ограничение изменения заканчивается
	if (nStepsToEndRateGrow >= nStepsCount)
	{
		// и шаг превышает заданный коэффициент ограничения, ограничиваем шаг до этого коэффициента
		if (dRateGrowLimit < dFilteredStep)
			dFilteredStep = dRateGrowLimit;
	}
	// возвращаем true если серия шагов ограничения закончилась и 
	// отфильтрованный шаг должен увеличиться
	return (--nStepsToStepChange <= 0) && dFilteredStep > 1.0;
}

// ограничивает частоту изменения порядка, просчитанного на серии шагов
// возвращает true, если порядок может быть увеличиен после контроля
// на серии шагов

bool CDynaModel::StepControl::FilterOrder(double dStep)
{

	if (dFilteredOrderInner > dStep)
		dFilteredOrderInner = dStep;

	dFilteredOrder = dFilteredOrderInner;

	if (nStepsToEndRateGrow >= nStepsCount)
	{
		if (dRateGrowLimit < dFilteredOrder)
			dFilteredOrder = dRateGrowLimit;
	}
	// возвращаем true, если отфильтрованный порядок на серии шагов
	// должен увеличиться
	return (--nStepsToOrderChange <= 0) && dFilteredOrder > 1.0;
}

csi cs_gatxpy(const cs *A, const double *x, double *y)
{
	csi p, j, n, *Ap, *Ai;
	double *Ax;
	if (!CS_CSC(A) || !x || !y) return (0);       /* check inputs */
	n = A->n; Ap = A->p; Ai = A->i; Ax = A->x;
	for (j = 0; j < n; j++)
	{
		for (p = Ap[j]; p < Ap[j + 1]; p++)
		{
			//y[Ai[p]] += Ax[p] * x[j];

			y[j] += Ax[p] * x[Ai[p]];

			_ASSERTE(Ai[p] < n);
		}
	}
	return (1);
}

//									   l0			l1			l2			Cq
const double CDynaModel::l[4][4] = { { 1.0,			1.0,		0.0,		2.0 },				//  BDF-1
									 { 2.0 / 3.0,	1.0,		1.0 /3.0,   4.5 },				//  BDF-2
									 { 1.0,			1.0,		0.0,		2.0 },				//  ADAMS-1
									 { 0.5,			1.0,		0.5,		12.0 } };			//  ADAMS-2

