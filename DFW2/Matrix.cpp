#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

void CDynaModel::EstimateMatrix()
{
	ptrdiff_t nOriginalMatrixSize = klu.MatrixSize();
	bool bSaveRightVector = pRightVector != nullptr;
	m_nEstimatedMatrixSize = 0;
	ptrdiff_t nNonZeroCount(0);
	sc.m_bFillConstantElements = m_bEstimateBuild = true;
	sc.RefactorMatrix();

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		(*it)->EstimateBlock(this);

	m_pMatrixRowsUniq = make_unique<MatrixRow[]>(m_nEstimatedMatrixSize);
	m_pMatrixRows = m_pMatrixRowsUniq.get();

	ZeroCrossingDevices.reserve(m_nEstimatedMatrixSize);

	if (bSaveRightVector)
	{
		// make a copy of original right vector to new right vector
		unique_ptr<RightVector[]>  pNewRightVector = make_unique<RightVector[]>(m_nEstimatedMatrixSize);
		std::copy(pRightVectorUniq.get(), pRightVectorUniq.get() + min(nOriginalMatrixSize, m_nEstimatedMatrixSize), pNewRightVector.get());
		pRightVectorUniq = std::move(pNewRightVector);
		pRightVector = pRightVectorUniq.get();
		pRightHandBackup = make_unique<double[]>(m_nEstimatedMatrixSize);
	}
	else
	{
		pRightVectorUniq = make_unique<RightVector[]>(m_nEstimatedMatrixSize);
		pRightVector = pRightVectorUniq.get();
		pRightHandBackup = make_unique<double[]>(m_nEstimatedMatrixSize);
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

	MatrixRow *pRow = m_pMatrixRows;
	const MatrixRow *pRowEnd = pRow + m_nEstimatedMatrixSize;
	while (pRow < pRowEnd)
	{
		nNonZeroCount += pRow->m_nColsCount;
		pRow++;
	}

	// allocate KLU matrix in CCS form
	klu.SetSize(m_nEstimatedMatrixSize, nNonZeroCount);
	m_pMatrixRows->pApRow = klu.Ap();
	m_pMatrixRows->pAxRow = klu.Ax();

	MatrixRow *pPrevRow = m_pMatrixRows;

	// spread pointers to Ap within matrix row headers

	for (pRow = pPrevRow + 1; pRow < pRowEnd ; pRow++, pPrevRow++)
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
	memcpy(pRightHandBackup.get(), klu.B(), sizeof(double) * m_nEstimatedMatrixSize);
	//sc.dRightHandNorm *= 0.5;
}

void CDynaModel::BuildMatrix()
{
	if (!EstimateBuild())
		BuildRightHand();

	if (sc.m_bRefactorMatrix)
	{
		ResetElement();

		for (auto&& it : m_DeviceContainers)
			it->BuildBlock(this);

		m_bRebuildMatrixFlag = false;
		sc.m_dLastRefactorH = sc.m_dCurrentH;
		Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, _T("Рефакторизация матрицы %d / %d"), klu.FactorizationsCount(), klu.RefactorizationsCount());
		if(sc.m_bFillConstantElements)
			Log(CDFW2Messages::DFW2LOG_DEBUG, _T("Обновление констант"));
		if (!EstimateBuild())
			sc.m_bFillConstantElements = false;
	}
}

void CDynaModel::BuildDerivatives()
{
	ResetElement();
	for (auto&& it : m_DeviceContainers)
		it->BuildDerivatives(this);
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
	MatrixRow *pEnd = pRow + m_nEstimatedMatrixSize;
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
		if (pRow->pAp >= pRow->pApRow + pRow->m_nColsCount || pRow->pAx >= pRow->pAxRow + pRow->m_nColsCount)
			throw dfw2error(_T("CDynaModel::ReallySetElement Column count"));
		*pRow->pAp = nCol;			*pRow->pAx = dValue;
		pRow->pAp++;				pRow->pAx++;
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

	if (pRow->pAp >= pRow->pApRow + pRow->m_nColsCount || pRow->pAx >= pRow->pAxRow + pRow->m_nColsCount)
		throw dfw2error(_T("CDynaModel::ReallySetElementNoDup Column count"));

	*pRow->pAp = nCol;			*pRow->pAx = dValue;
	pRow->pAp++;				pRow->pAx++;
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
	MatrixRow *pRowBase = m_pMatrixRows;
	MatrixRow *pRow = pRowBase;
	MatrixRow *pEnd = pRow + m_nEstimatedMatrixSize;
	while (pRow < pEnd)
	{
		Ai[pRow - pRowBase] = pRow->pApRow - Ap;
		pRow++;
	}
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
				MatrixRow *pRowBase = m_pMatrixRows;
				MatrixRow *pRow = pRowBase;
				MatrixRow *pRowEnd = pRow + m_nEstimatedMatrixSize;

				// просматриваем строки матрицы, ищем столбцы с индексом, соответствующим уравнению РДЗ
				for (; pRow < pRowEnd ; pRow++)
				{
					ptrdiff_t nEqIndex = pRow - pRowBase;
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
	MatrixRow *pRow = m_pMatrixRows  + nRow;
	pRow->pAp = pRow->pApRow + pRow->m_nConstElementsToSkip;
	pRow->pAx = pRow->pAxRow + pRow->m_nConstElementsToSkip;
	return true;
}

