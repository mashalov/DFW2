#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

void CDynaModel::EstimateMatrix()
{
	ptrdiff_t nOriginalMatrixSize = klu.MatrixSize();
	bool bSaveRightVector = pRightVector != nullptr;
	CleanUpMatrix(bSaveRightVector);
	m_nEstimatedMatrixSize = 0;
	ptrdiff_t nNonZeroCount(0);
	sc.m_bFillConstantElements = m_bEstimateBuild = true;
	sc.RefactorMatrix();

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		(*it)->EstimateBlock(this);

	m_pMatrixRows = new MatrixRow[m_nEstimatedMatrixSize];
	m_pZeroCrossingDevices = new CDevice*[m_nEstimatedMatrixSize];

	if (bSaveRightVector)
	{
		// make a copy of original right vector to new right vector
		RightVector *pNewRightVector = new RightVector[m_nEstimatedMatrixSize];
		RightVector *pNewBegin = pNewRightVector;
		RightVector *pOldBegin = pRightVector;
		RightVector *pOldEnd = pOldBegin + min(nOriginalMatrixSize, m_nEstimatedMatrixSize);

		while (pOldBegin < pOldEnd)
		{
			*pNewBegin = *pOldBegin;
			pNewBegin++;
			pOldBegin++;
		}

		delete pRightVector;
		pRightVector = pNewRightVector;
		pRightHandBackup = new double[m_nEstimatedMatrixSize];
	}
	else
	{
		pRightVector = new RightVector[m_nEstimatedMatrixSize];
		pRightHandBackup = new double[m_nEstimatedMatrixSize];
	}

	// init RightVector primitive block types
	struct RightVector *pVectorBegin = pRightVector + nOriginalMatrixSize;
	struct RightVector *pVectorEnd = pRightVector + m_nEstimatedMatrixSize;
	while (pVectorBegin < pVectorEnd)
	{
		pVectorBegin->PrimitiveBlock = PBT_UNKNOWN;
		pVectorBegin++;
	}


	// substitute element setter to counter (not actually setting values, just count)

	ElementSetter		= &CDynaModel::CountSetElement;
	ElementSetterNoDup  = &CDynaModel::CountSetElementNoDup;

	BuildMatrix();
	nNonZeroCount = 0;

	// allocate matrix row headers to access rows instantly

	MatrixRow *pRow;
	for (pRow = m_pMatrixRows; pRow < m_pMatrixRows + m_nEstimatedMatrixSize; pRow++)
		nNonZeroCount += pRow->m_nColsCount;

	// allocate KLU matrix in CCS form
	klu.SetSize(m_nEstimatedMatrixSize, nNonZeroCount);
	m_pMatrixRows->pApRow = klu.Ap();
	m_pMatrixRows->pAxRow = klu.Ax();

	MatrixRow *pPrevRow = m_pMatrixRows;

	// spread pointers to Ap within matrix row headers

	for (pRow = pPrevRow + 1; pRow < m_pMatrixRows + klu.MatrixSize(); pRow++, pPrevRow++)
	{
		pRow->pApRow = pPrevRow->pApRow + pPrevRow->m_nColsCount;
		pRow->pAxRow = pPrevRow->pAxRow + pPrevRow->m_nColsCount;
	}

	// revert to real element setter
	ElementSetter		= &CDynaModel::ReallySetElement;
	ElementSetterNoDup  = &CDynaModel::ReallySetElementNoDup;
			
	InitDevicesNordsiek();

	if (bSaveRightVector && nOriginalMatrixSize < klu.MatrixSize())
	{
		pVectorBegin = pRightVector + nOriginalMatrixSize;

		while (pVectorBegin < pVectorEnd)
		{
			InitNordsiekElement(pVectorBegin,GetAtol(),GetRtol());
			PrepareNordsiekElement(pVectorBegin);
			pVectorBegin++;
		}
	}
	m_bEstimateBuild = false;
}

void CDynaModel::BuildRightHand()
{
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() ; it++)
		(*it)->BuildRightHand(this);

	sc.dRightHandNorm = 0.0;
	double *pBb = klu.B(), *pBe = pBb + m_nEstimatedMatrixSize;
	while (pBb < pBe)
	{
		sc.dRightHandNorm += *pBb * *pBb;
		pBb++;
	}
	memcpy(pRightHandBackup, klu.B(), sizeof(double) * m_nEstimatedMatrixSize);
	//sc.dRightHandNorm *= 0.5;
}

void CDynaModel::BuildMatrix()
{
	if (!EstimateBuild())
		BuildRightHand();

	if (sc.m_bRefactorMatrix)
	{
		ResetElement();
		for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		{
			(*it)->BuildBlock(this);
		}
		m_bRebuildMatrixFlag = false;
		sc.m_dLastRefactorH = sc.m_dCurrentH;
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("Рефакторизация матрицы %d"), klu.FactorizationsCount());
		if(sc.m_bFillConstantElements)
			Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Обновление констант"));
		if (!EstimateBuild())
			sc.m_bFillConstantElements = false;
	}
}

void CDynaModel::BuildDerivatives()
{
	ResetElement();
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		(*it)->BuildDerivatives(this);
	}
}


void CDynaModel::CleanUpMatrix(bool bSaveRightVector)
{
	//if (bRightHand) delete bRightHand;
	//if (Solution) delete Solution;

	if (!bSaveRightVector)
		if (pRightVector) delete pRightVector;

	if (pRightHandBackup)
		delete pRightHandBackup;

	if (m_pMatrixRows)
		delete m_pMatrixRows;
	if (m_pZeroCrossingDevices)
		delete m_pZeroCrossingDevices;
}


ptrdiff_t CDynaModel::AddMatrixSize(ptrdiff_t nSizeIncrement)
{
	ptrdiff_t nRet = m_nEstimatedMatrixSize;
	m_nEstimatedMatrixSize += nSizeIncrement;
	return nRet;
}

void CDynaModel::ResetElement()
{
	_ASSERTE(m_pMatrixRows);
	MatrixRow *pRow = m_pMatrixRows;
	MatrixRow *pEnd = m_pMatrixRows + m_nEstimatedMatrixSize;
	while (pRow < pEnd)
	{
		pRow->Reset();
		pRow++;
	}
}

void CDynaModel::ReallySetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious)
{
	if(nRow >= m_nEstimatedMatrixSize || nCol >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::ReallySetElement matrix size overrun Row %d Col %d MatrixSize %d"), nRow, nCol, m_nEstimatedMatrixSize));

	MatrixRow *pRow = m_pMatrixRows + nRow;
	double l0 = Methodl[GetRightVector(nCol)->EquationType * 2 + (sc.q - 1)][0];
	// в качестве типа уравнения используем __физический__ тип
	// потому что у алгебраических и дифференциальных уравнений
	// разная структура в матрице Якоби, а EquationType указывает лишь набор коэффициентов метода

	if (GetRightVector(nRow)->PhysicalEquationType == DET_ALGEBRAIC)
		dValue *= l0;		// если уравнение алгебраическое, ставим коэффициент метода интегрирования
	else
	{
		// если уравнение дифференциальное, ставим коэффициент метода умноженный на шаг
		dValue *= l0 * GetH();
		// если элемент диагональный, учитываем диагональную единичную матрицу
		if (nRow == nCol)
			dValue = 1.0 - dValue;
	}

	_CheckNumber(dValue);

	switch (sc.IterationMode)
	{
	case StepControl::eIterationMode::JN:
		if (nRow != nCol) dValue = 0.0;
		break;
	case StepControl::eIterationMode::FUNCTIONAL:
		if (nRow != nCol) dValue = 0.0; else dValue = 1.0;
		break;

	}

	// если нужно суммировать элементы, на входа задан флаг bAddToPrevious
	// пока нужно для параллельных ветвей
	if (bAddToPrevious)
	{
		ptrdiff_t *pSp = pRow->pAp - 1;
		while (pSp >= pRow->pApRow)
		{
			if (*pSp == nCol)
			{
				ptrdiff_t pDfr = pSp - pRow->pAp;
				*(pRow->pAx + pDfr) += dValue;
				break;
			}
			pSp--;
		}
	}
	else
	{
		if (pRow->pAp < pRow->pApRow + pRow->m_nColsCount &&
			pRow->pAx < pRow->pAxRow + pRow->m_nColsCount)
		{
			*pRow->pAp = nCol;
			*pRow->pAx = dValue;
			pRow->pAp++;
			pRow->pAx++;
		}
	}
}


void CDynaModel::ReallySetElementNoDup(ptrdiff_t nRow, ptrdiff_t nCol, double dValue)
{
	if (nRow >= m_nEstimatedMatrixSize || nCol >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::ReallySetElement matrix size overrun Row %d Col %d MatrixSize %d"), nRow, nCol, m_nEstimatedMatrixSize));

	MatrixRow *pRow = m_pMatrixRows + nRow;
	double l0 = Methodl[GetRightVector(nCol)->EquationType * 2 + (sc.q - 1)][0];
	// в качестве типа уравнения используем __физический__ тип
	// потому что у алгебраических и дифференциальных уравнений
	// разная структура в матрице Якоби, а EquationType указывает лишь набор коэффициентов метода

	if (GetRightVector(nRow)->PhysicalEquationType == DET_ALGEBRAIC)
		dValue *= l0;		// если уравнение алгебраическое, ставим коэффициент метода интегрирования
	else
	{
		// если уравнение дифференциальное, ставим коэффициент метода умноженный на шаг
		dValue *= l0 * GetH();
		// если элемент диагональный, учитываем диагональную единичную матрицу
		if (nRow == nCol)
			dValue = 1.0 - dValue;
	}

	_CheckNumber(dValue);

	switch (sc.IterationMode)
	{
	case StepControl::eIterationMode::JN:
		if (nRow != nCol) dValue = 0.0;
		break;
	case StepControl::eIterationMode::FUNCTIONAL:
		if (nRow != nCol) dValue = 0.0; else dValue = 1.0;
		break;

	}

	if (pRow->pAp < pRow->pApRow + pRow->m_nColsCount &&
		pRow->pAx < pRow->pAxRow + pRow->m_nColsCount)
	{
		*pRow->pAp = nCol;
		*pRow->pAx = dValue;
		pRow->pAp++;
		pRow->pAx++;
	}
}
// Функция подсчета количества элементов в строке матрицы
void CDynaModel::CountSetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious)
{
	if (nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::CountSetElement matrix size overrun Row %d Col %d MatrixSize %d"),nRow, nCol, m_nEstimatedMatrixSize));
	if (!bAddToPrevious)
	{
		// учитываем элементы с учетом суммирования
		MatrixRow *pRow = m_pMatrixRows + nRow;
		pRow->m_nColsCount++;
	}
}


void CDynaModel::CountSetElementNoDup(ptrdiff_t nRow, ptrdiff_t nCol, double dValue)
{
	if (nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::CountSetElementNoDup matrix size overrun Row %d Col %d MatrixSize %d"), nRow, nCol, m_nEstimatedMatrixSize));
	// учитываем элементы с учетом суммирования
	MatrixRow *pRow = m_pMatrixRows + nRow;
	pRow->m_nColsCount++;
}

void CDynaModel::SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious)
{
	(this->*(ElementSetter))(nRow, nCol, dValue, bAddToPrevious);
}

void CDynaModel::SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue)
{
	(this->*(ElementSetterNoDup))(nRow, nCol, dValue);
}

// задает правую часть алгебраического уравнения
void CDynaModel::SetFunction(ptrdiff_t nRow, double dValue)
{
	if (nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::SetFunction matrix size overrun Row %d MatrixSize %d"), nRow, m_nEstimatedMatrixSize));
	_CheckNumber(dValue);
	klu.B()[nRow] = -dValue;
}

void CDynaModel::SetDerivative(ptrdiff_t nRow, double dValue)
{
	struct RightVector *pRv = GetRightVector(nRow);
	_CheckNumber(dValue);
	pRv->Nordsiek[1] = dValue * GetH();
}

void CDynaModel::CorrectNordsiek(ptrdiff_t nRow, double dValue)
{
	RightVector *pRightVector = GetRightVector(nRow);
	_CheckNumber(dValue);
	pRightVector->Nordsiek[0] = dValue;
	pRightVector->Nordsiek[1] = pRightVector->Nordsiek[2] = 0.0;

	pRightVector->SavedNordsiek[0] = dValue;
	pRightVector->SavedNordsiek[1] = pRightVector->SavedNordsiek[2] = 0.0;

	pRightVector->Tminus2Value = dValue;
}

// задает правую часть дифференциального уравнения
void CDynaModel::SetFunctionDiff(ptrdiff_t nRow, double dValue)
{
	struct RightVector *pRv = GetRightVector(nRow);
	_CheckNumber(dValue);
	// ставим тип метода для уравнения по параметрам в исходных данных
	SetFunctionEqType(nRow, GetH() * dValue - pRv->Nordsiek[1] - pRv->Error, GetDiffEquationType());
}



bool CDynaModel::SetFunctionEqType(ptrdiff_t nRow, double dValue, DEVICE_EQUATION_TYPE EquationType)
{
	RightVector *pRv = GetRightVector(nRow);
	_CheckNumber(dValue);
	klu.B()[nRow] = dValue;
	pRightVector[nRow].EquationType = EquationType;					// тип метода для уравнения
	pRightVector[nRow].PhysicalEquationType = DET_DIFFERENTIAL;		// уравнение, устанавливаемое этой функцией всегда дифференциальное
	return true;
}

double CDynaModel::GetFunction(ptrdiff_t nRow)
{
	double dVal = NAN;

	if (nRow >= 0 && nRow < m_nEstimatedMatrixSize)
		dVal = klu.B()[nRow];

	return dVal;
}

void CDynaModel::ConvertToCCSMatrix()
{
	ptrdiff_t *Ai = klu.Ai();
	ptrdiff_t *Ap = klu.Ap();
	for (MatrixRow *pRow = m_pMatrixRows; pRow < m_pMatrixRows + m_nEstimatedMatrixSize; pRow++)
		Ai[pRow - m_pMatrixRows] = pRow->pApRow - Ap;
	klu.Ai()[m_nEstimatedMatrixSize] = klu.NonZeroCount();
}

void CDynaModel::SetDifferentiatorsTolerance()
{
	// Для всех алгебраических уравнений, в которые входит РДЗ, рекурсивно ставим точность Atol не превышающую РДЗ
	// если доходим до дифференциального уравнения - его точность и точность связанных с ним уравнений оставляем
	ptrdiff_t nMarked = 0;
	do
	{
		nMarked = 0;
		RightVector *pVectorBegin = pRightVector;
		RightVector *pVectorEnd = pRightVector + m_nEstimatedMatrixSize;

		while (pVectorBegin < pVectorEnd)
		{
			if (pVectorBegin->PrimitiveBlock == PBT_DERLAG)
			{
				// уравнение отмечено как уравнение РДЗ, меняем отметку, чтобы указать, что это уравнение уже прошли
				pVectorBegin->PrimitiveBlock = PBT_LAST;
				pVectorBegin->Atol = 1E-2;

				ptrdiff_t nDerLagEqIndex = pVectorBegin - pRightVector;
				MatrixRow *pRow = m_pMatrixRows;

				// просматриваем строки матрицы, ищем столбцы с индексом, соответствующим уравнению РДЗ
				for (; pRow < m_pMatrixRows + m_nEstimatedMatrixSize; pRow++)
				{
					ptrdiff_t nEqIndex = pRow - m_pMatrixRows;
					if (nEqIndex != nDerLagEqIndex)
					{
						for (ptrdiff_t *pc = pRow->pApRow; pc < pRow->pApRow + pRow->m_nColsCount; pc++)
						{
							if (*pc == nDerLagEqIndex)
							{
								// нашли столбец с индексом уравнения РДЗ
								RightVector *pMarkEq = pRightVector + nEqIndex;
								// если уравнение с этим столбцом алгебраическое и еще не просмотрено
								if (pMarkEq->PrimitiveBlock == PBT_UNKNOWN && pMarkEq->EquationType == DET_ALGEBRAIC)
								{
									// отмечаем его как уравнение РДЗ, ставим точность РДЗ
									//Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("Точность %s %s"), pVectorBegin->pDevice->GetVerbalName(), pVectorBegin->pDevice->VariableNameByPtr((pRightVector + nEqIndex)->pValue));
									pMarkEq->PrimitiveBlock = PBT_DERLAG;
									pMarkEq->Atol = pVectorBegin->Atol;
									pMarkEq->Rtol = pVectorBegin->Rtol;
									// считаем сколько уравнений обработали
									nMarked++;
								}
							}
						}
					}
				}
			}
			pVectorBegin++;
		}
		// продолжаем, пока есть необработанные уравнения
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("Marked = %d"), nMarked);
	} while (nMarked);

}

void CDynaModel::SolveLinearSystem()
{
	if (sc.m_bRefactorMatrix || !klu.Factored())
	{
		// если нужен рефактор или в KLU не было факторизации
		ConvertToCCSMatrix();
		if (!klu.Analyzed())
		{
			// если в KLU не было анализа
			klu.Analyze();
			// формируем точности для РДЗ
			SetDifferentiatorsTolerance();
		}

		if (klu.Factored())
		{
			// если в KLU нет факторизации
			if (m_Parameters.m_bUseRefactor) // делаем вторую и более итерацию Ньютона и разрешен рефактор
				klu.Refactor();
		}
		SolveRcond();
	}
	else
		SolveRcond();
}

void CDynaModel::SolveRcond()
{
	klu.Solve();
	double rCond = klu.Rcond();
	if (rCond > sc.dMaxConditionNumber)
	{
		sc.dMaxConditionNumber = rCond;
		sc.dMaxConditionNumberTime = sc.t;
	}
}

/*
Steps count 15494
Steps by 1st order count 11971, failures 920 Newton failures 24 Time passed 140,572023
Steps by 2nd order count 1289, failures 0 Newton failures 945 Time passed 9,420801
Factors count 4780 Analyzings count 1
Newtons count 30380 1,960759 per step, failures at step 969 failures at discontinuity 0

Steps count 15841
Steps by 1st order count 12094, failures 843 Newton failures 194 Time passed 138,938238
Steps by 2nd order count 1374, failures 0 Newton failures 988 Time passed 11,053219
Factors count 4897 Analyzings count 1
Newtons count 31078 1,961871 per step, failures at step 1182 failures at discontinuity 0

Steps count 16637
Steps by 1st order count 12809, failures 835 Newton failures 278 Time passed 139,021073
Steps by 2nd order count 1325, failures 0 Newton failures 1034 Time passed 10,971442
Factors count 4754 Analyzings count 1
Newtons count 31815 1,912304 per step, failures at step 1312 failures at discontinuity 0

Steps count 14369
Steps by 1st order count 10745, failures 772 Newton failures 0 Time passed 133,496504
Steps by 2nd order count 1574, failures 65 Newton failures 854 Time passed 16,492013
Factors count 5244 Analyzings count 1
Newtons count 32846 2,285893 per step, failures at step 854 failures at discontinuity 0

Steps count 13286
Steps by 1st order count 9922, failures 681 Newton failures 0 Time passed 148,551765
Steps by 2nd order count 1514, failures 21 Newton failures 779 Time passed 1,439926
Factors count 4952 Analyzings count 1
Newtons count 29139 2,193211 per step, failures at step 779 failures at discontinuity 0

Steps count 13590
Steps by 1st order count 10125, failures 711 Newton failures 0 Time passed 143,802777
Steps by 2nd order count 1542, failures 62 Newton failures 790 Time passed 6,185724
Factors count 4854 Analyzings count 1
Newtons count 30930 2,275938 per step, failures at step 790 failures at discontinuity 0

Steps count 13721
Steps by 1st order count 10196, failures 751 Newton failures 0 Time passed 143,360266
Steps by 2nd order count 1558, failures 41 Newton failures 815 Time passed 6,628234
Factors count 4952 Analyzings count 1
Newtons count 31147 2,270024 per step, failures at step 815 failures at discontinuity 0

Steps count 8604
Steps by 1st order count 3942, failures 388 Newton failures 0 Time passed 74,981291
Steps by 2nd order count 3563, failures 148 Newton failures 199 Time passed 75,005818
Factors count 2699 Analyzings count 1
Newtons count 18065 2,099605 per step, failures at step 199 failures at discontinuity 0

Steps count 8484
Steps by 1st order count 3868, failures 350 Newton failures 0 Time passed 74,388690
Steps by 2nd order count 3563, failures 159 Newton failures 181 Time passed 75,599181
Factors count 2564 Analyzings count 1
Newtons count 17954 2,116219 per step, failures at step 181 failures at discontinuity 0

Steps count 8295
Steps by 1st order count 3367, failures 338 Newton failures 0 Time passed 46,176702
Steps by 2nd order count 3901, failures 173 Newton failures 152 Time passed 103,811461
Factors count 2589 Analyzings count 1
Newtons count 17709 2,134901 per step, failures at step 152 failures at discontinuity 0
*/

void CDynaModel::ScaleAlgebraicEquations()
{
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + m_nEstimatedMatrixSize;
	double h = GetH();

	if (h <= 0)
		h = 1.0;

	while (pVectorBegin < pVectorEnd)
	{
		if (pVectorBegin->EquationType == DET_ALGEBRAIC)
		{
			ptrdiff_t j = pVectorBegin - pRightVector;
			for (ptrdiff_t p = klu.Ai()[j]; p < klu.Ai()[j + 1]; p++)
				klu.Ax()[p] /= h;
		}

		pVectorBegin++;

	}
}

bool CDynaModel::CountConstElementsToSkip(ptrdiff_t nRow)
{
	if (nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::CountConstElementsToSkip matrix size overrun Row %d MatrixSize %d"), nRow, m_nEstimatedMatrixSize));
	MatrixRow *pRow = m_pMatrixRows + nRow;
	pRow->m_nConstElementsToSkip = pRow->pAp - pRow->pApRow;
	return true;
}
bool CDynaModel::SkipConstElements(ptrdiff_t nRow)
{
	if (nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(Cex(_T("CDynaModel::SkipConstElements matrix size overrun Row %d MatrixSize %d"), nRow, m_nEstimatedMatrixSize));
	MatrixRow *pRow = m_pMatrixRows + nRow;
	pRow->pAp = pRow->pApRow + pRow->m_nConstElementsToSkip;
	pRow->pAx = pRow->pAxRow + pRow->m_nConstElementsToSkip;
	return true;
}

