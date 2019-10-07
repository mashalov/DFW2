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
		DumpMatrix();
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
		//SetConsoleOutputCP(65001);
		_tcprintf(cszCRLF);
			   
		va_list argList;
		size_t len;
		TCHAR * buffer;
		va_start(argList, cszMessage);
		len = _vsctprintf(cszMessage, argList) + sizeof(_TCHAR);
		buffer = new _TCHAR[len];
		_vstprintf_s(buffer, len, cszMessage, argList);
		_tcprintf(buffer);
		delete(buffer);
		va_end(argList);

		SetConsoleTextAttribute(hCon, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | FOREGROUND_RED);
	}

	if (m_pLogFile && m_Parameters.m_bLogToFile)
	{
		_ftprintf(m_pLogFile,cszCRLF);
		va_list argList;
		va_start(argList, cszMessage);
		_vftprintf_s(m_pLogFile,cszMessage, argList);
		va_end(argList);
	}
}


bool CDynaModel::ChangeOrder(ptrdiff_t Newq)
{
	bool bRes = true;

	if (sc.q != Newq)
	{
		sc.RefactorMatrix();
		sc.UpdateConstElements();
	}

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
	if (nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::GetRightVector matrix size overrun Row %d MatrixSize %d"), nRow, m_nEstimatedMatrixSize));
	return pRightVector + nRow;
}




void CDynaModel::StopProcess()
{
	sc.m_bStopCommandReceived = true;
}

CDeviceContainer* CDynaModel::GetDeviceContainer(eDFW2DEVICETYPE Type)
{
	auto& it = find_if(m_DeviceContainers.begin(), m_DeviceContainers.end(), [&Type](const auto& Cont) { return Cont->GetType() == Type; });

	if (it == m_DeviceContainers.end())
		return nullptr;
	else
		return *it;
}

void CDynaModel::RebuildMatrix(bool bRebuild)
{
	if (!m_bRebuildMatrixFlag)
		m_bRebuildMatrixFlag = sc.m_bRefactorMatrix = bRebuild;
}

void CDynaModel::ProcessTopologyRequest()
{
	sc.m_bProcessTopology = true;
	sc.UpdateConstElements();
}

void CDynaModel::DiscontinuityRequest()
{
	sc.m_bDiscontinuityRequest = true;
}

double CDynaModel::GetWeightedNorm(double *pVector)
{
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + klu.MatrixSize();
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
	double *pe = pVector + klu.MatrixSize();
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
	CDeviceContainer *pContainer(nullptr);

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
	RightVector **pSortOrder = new RightVector*[klu.MatrixSize()];
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + klu.MatrixSize();
	RightVector **ppSortOrder = pSortOrder;

	while (pVectorBegin < pVectorEnd)
	{
		*ppSortOrder = pVectorBegin;
		pVectorBegin++;
		ppSortOrder++;
	}

	qsort_s(pSortOrder, klu.MatrixSize(), sizeof(RightVector*), ErrorCompare, nullptr);

	if (nCount > klu.MatrixSize())
		nCount = klu.MatrixSize();

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

void CDynaModel::DumpMatrix(bool bAnalyzeLinearDependenies)
{
	FILE *fmatrix;
	if (!_tfopen_s(&fmatrix, _T("c:\\tmp\\dwfsingularmatrix.mtx"), _T("w+")))
	{
		ptrdiff_t *pAi = klu.Ap();
		double *pAx = klu.Ax();
		ptrdiff_t nRow = 0;
		set<ptrdiff_t> BadNumbers, FullZeros;
		vector<bool> NonZeros;
		vector<bool> Diagonals;
		vector<double> Expanded;

		NonZeros.resize(klu.MatrixSize(), false);
		Diagonals.resize(klu.MatrixSize(), false);
		Expanded.resize(klu.MatrixSize(), 0.0);

		for (ptrdiff_t *pAp = klu.Ai(); pAp < klu.Ai() + klu.MatrixSize(); pAp++, nRow++)
		{
			ptrdiff_t *pAiend = pAi + *(pAp + 1) - *pAp;
			bool bAllZeros = true;
			while (pAi < pAiend)
			{
				_ftprintf_s(fmatrix, _T("%10td %10td     %30g"), nRow, *pAi, *pAx);
				RightVector *pRowVector = pRightVector + nRow;
				RightVector *pColVector = pRightVector + *pAi;
				CDevice *pRowDevice = pRowVector->pDevice;
				CDevice *pColDevice = pColVector->pDevice;

				if (isnan(*pAx) || isinf(*pAx))
					BadNumbers.insert(nRow);

				if (fabs(*pAx) > 1E-7)
				{
					bAllZeros = false;
					NonZeros[*pAi] = true;

					if (nRow == *pAi)
						Diagonals[nRow] = true;
				}

				
				_ftprintf_s(fmatrix, _T("    %50s/%30s %50s/%30s"), pRowDevice->GetVerbalName(), pRowDevice->VariableNameByPtr(pRowVector->pValue),
												  				    pColDevice->GetVerbalName(), pColDevice->VariableNameByPtr(pColVector->pValue));
				_ftprintf_s(fmatrix, _T("    %30g %30g\n"), *pRowVector->pValue, *pColVector->pValue);
				pAx++; pAi++;
			}
			if (bAllZeros)
				FullZeros.insert(nRow);
		}

		for (const auto& it : BadNumbers)
			_ftprintf_s(fmatrix, _T("Bad Number in Row: %td\n"), it);

		for (const auto& it : FullZeros)
			_ftprintf_s(fmatrix, _T("Full Zero Row : %td\n"), it);

		for (auto&& it = NonZeros.begin() ; it != NonZeros.end() ; it++)
			if(!*it)
				_ftprintf_s(fmatrix, _T("Full Zero Column: %td\n"), it - NonZeros.begin());

		for (auto&& it = Diagonals.begin(); it != Diagonals.end(); it++)
			if (!*it)
				_ftprintf_s(fmatrix, _T("Zero Diagonal: %td\n"), it - Diagonals.begin());


		if(!bAnalyzeLinearDependenies)
			return;

		// пытаемся определить линейно зависимые строки с помощью неравенства Коши-Шварца
		// (v1 dot v2)^2 <= norm2(v1) * norm2(v2)

		pAi = klu.Ap(); pAx = klu.Ax(); nRow = 0;
		for (ptrdiff_t *pAp = klu.Ai(); pAp < klu.Ai() + klu.MatrixSize(); pAp++, nRow++)
		{
			fill(Expanded.begin(), Expanded.end(), 0.0);

			ptrdiff_t *pAiend = pAi + *(pAp + 1) - *pAp;
			bool bAllZeros = true;
			double normi = 0.0;

			set < pair<ptrdiff_t, double> > RowI;

			while (pAi < pAiend)
			{
				Expanded[*pAi] = *pAx;
				normi += *pAx * *pAx;

				RowI.insert(make_pair(*pAi, *pAx));

				pAx++; pAi++;
			}

			ptrdiff_t nRows = 0;
			ptrdiff_t *pAis = klu.Ap();
			double *pAxs = klu.Ax();
			for (ptrdiff_t *pAps = klu.Ai(); pAps < klu.Ai() + klu.MatrixSize(); pAps++, nRows++)
			{
				ptrdiff_t *pAiends = pAis + *(pAps + 1) - *pAps;
				double normj = 0.0;
				double inner = 0.0;

				set < pair<ptrdiff_t, double> > RowJ;

				while (pAis < pAiends)
				{
					normj += *pAxs * *pAxs;
					inner += Expanded[*pAis] * *pAxs;

					RowJ.insert(make_pair(*pAis, *pAxs));

					pAis++; pAxs++;
				}
				if (nRow != nRows)
				{
					double Ratio = inner * inner / normj / normi;
					if (fabs(Ratio - 1.0) < 1E-5)
					{
						_ftprintf_s(fmatrix, _T("Linear dependent rows %10td %10td with %g\n"), nRow, nRows, Ratio);
						for(auto & it : RowI)
							_ftprintf_s(fmatrix, _T("%10td %10td     %30g\n"), nRow, it.first, it.second);
						for (auto & it : RowJ)
							_ftprintf_s(fmatrix, _T("%10td %10td     %30g\n"), nRows, it.first, it.second);
					}

				}
			}
		}
		fclose(fmatrix);
	}
}

void CDynaModel::DumpStateVector()
{
	FILE *fdump(nullptr);
	setlocale(LC_ALL, "ru-ru");
	if (!_tfopen_s(&fdump, Cex(_T("c:\\tmp\\statevector_%d.csv"), sc.nStepsCount), _T("w+, ccs=UTF-8")))
	{
		_ftprintf(fdump, _T("Value;db;Device;N0;N1;N2;Error;WError;Atol;Rtol;EqType;SN0;SN1;SN2;SavError;Tminus2Val;PhysEqType;PrimBlockType;ErrorHits\n"));
		for (RightVector *pRv = pRightVector; pRv < pRightVector + klu.MatrixSize(); pRv++)
		{
			_ftprintf(fdump, _T("%g;"), *pRv->pValue);
			_ftprintf(fdump, _T("%g;"), fabs(pRv->b));
			_ftprintf(fdump, _T("%s - %s;"), pRv->pDevice->GetVerbalName(), pRv->pDevice->VariableNameByPtr(pRv->pValue));
			_ftprintf(fdump, _T("%g;%g;%g;"), pRv->Nordsiek[0], pRv->Nordsiek[1], pRv->Nordsiek[2]);
			_ftprintf(fdump, _T("%g;"), fabs(pRv->Error));
			_ftprintf(fdump, _T("%g;"), fabs(pRv->GetWeightedError(pRv->b, *pRv->pValue)));
			_ftprintf(fdump, _T("%g;%g;"), pRv->Atol, pRv->Rtol);
			_ftprintf(fdump, _T("%d;"), pRv->EquationType);
			_ftprintf(fdump, _T("%g;%g;%g;"), pRv->SavedNordsiek[0], pRv->SavedNordsiek[1], pRv->SavedNordsiek[2]);
			_ftprintf(fdump, _T("%g;"), pRv->SavedError);
			_ftprintf(fdump, _T("%g;"), pRv->Tminus2Value);
			_ftprintf(fdump, _T("%d;"), pRv->PhysicalEquationType);
			_ftprintf(fdump, _T("%d;"), pRv->PrimitiveBlock);
			_ftprintf(fdump, _T("%td;"), pRv->nErrorHits);
			_ftprintf(fdump, _T("\n"));
		}
		fclose(fdump);
	}
}

void CDynaModel::EnableAdamsCoefficientDamping(bool bEnable)
{
	if (bEnable == sc.bAdamsDampingEnabled) return;
	sc.bAdamsDampingEnabled = bEnable;
	double Alpha = bEnable ? m_Parameters.m_dAdamsDampingAlpha : 0.0;
	Methodl[3][0] = MethodlDefault[3][0] * (1.0 + Alpha);
	// Вместо MethodDefault[3][3] можно использовать честную формулу для LTE (см. Docs)
	Methodl[3][3] = 1.0 / fabs(-1.0 / MethodlDefault[3][3] - 0.5 * Alpha) / (1.0 + Alpha);
	sc.RefactorMatrix();
	Log(CDFW2Messages::DFW2LOG_DEBUG, Cex(DFW2::CDFW2Messages::m_cszAdamsDamping, bEnable ? DFW2::CDFW2Messages::m_cszOn: DFW2::CDFW2Messages::m_cszOff));
}

const double CDynaModel::MethodlDefault[4][4] = 
//									   l0			l1			l2			Tauq
								   { { 1.0,			1.0,		0.0,		2.0 },				//  BDF-1
									 { 2.0 / 3.0,	1.0,		1.0 /3.0,   4.5 },				//  BDF-2
									 { 1.0,			1.0,		0.0,		2.0 },				//  ADAMS-1
									 { 0.5,			1.0,		0.5,		12.0 } };			//  ADAMS-2

