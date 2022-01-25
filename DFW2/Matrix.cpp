#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

void CDynaModel::EstimateMatrix()
{
	const ptrdiff_t nOriginalMatrixSize{ klu.MatrixSize() };
	// если RightVector уже есть - ставим флаг сохранения данных в новый вектор
	bool bSaveRightVector = pRightVector != nullptr;
	m_nEstimatedMatrixSize = 0;
	ptrdiff_t nNonZeroCount(0);
	// заставляем обновиться постоянные элементы в матрице, ставим оценочную сборку матрицы
	sc.m_bFillConstantElements = m_bEstimateBuild = true;
	// взводим флаг рефакторизации
	sc.RefactorMatrix();

	// Если нужно обновить данные в общем векторе правой части
	// делаем это _до_ EstimateBlock, так как состав матрицы
	// может измениться и рассинхронизироваться с pRightVector
	if (bSaveRightVector)
		UpdateTotalRightVector();

	for (auto&& it : m_DeviceContainers)
		it->EstimateBlock(this);

	m_pMatrixRowsUniq = std::make_unique<MatrixRow[]>(m_nEstimatedMatrixSize);
	m_pMatrixRows = m_pMatrixRowsUniq.get();

	ZeroCrossingDevices.reserve(m_nEstimatedMatrixSize);

	if (bSaveRightVector)
	{
		// make a copy of original right vector to new right vector
		pRightVectorUniq = std::make_unique<RightVector[]>(m_nEstimatedMatrixSize);
		pRightVector = pRightVectorUniq.get();
		pRightHandBackup = std::make_unique<double[]>(m_nEstimatedMatrixSize);
		UpdateNewRightVector();
	}
	else
	{
		CreateTotalRightVector();
		pRightVectorUniq = std::make_unique<RightVector[]>(m_nEstimatedMatrixSize);
		pRightVector = pRightVectorUniq.get();
		pRightHandBackup = std::make_unique<double[]>(m_nEstimatedMatrixSize);
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
	Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszMatrixSize, m_nEstimatedMatrixSize, nNonZeroCount));
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
	
	// В InitDevicesNordsieck устройства привязываются к pRightVector
	// указывая свои адреса и адреса своих переменных
	// следующий вызов контролирует корректность синхронизации рабочего вектора
	// после изменения его размерности или состава
	// по идее это можно сделать в UpdateNewRightVector, но он искажает тип блока
	// примитива, который вводится в BuildMatrix
	if (bSaveRightVector)
		DebugCheckRightVectorSync();

	/*
	// новые элементы рабочего вектора инициализируем
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
	*/
	m_bEstimateBuild = false;
}

void CDynaModel::BuildRightHand()
{
	for (auto&& it : m_DeviceContainers)
		it->BuildRightHand(this);

	sc.dRightHandNorm = 0.0;
	const double* pBb{ klu.B() };
	const double* const pBe{ pBb + m_nEstimatedMatrixSize };
	while (pBb < pBe)
	{
		sc.dRightHandNorm += *pBb * *pBb;
		pBb++;
	}
	std::copy(klu.B(), klu.B() + m_nEstimatedMatrixSize, pRightHandBackup.get());
}

void CDynaModel::BuildMatrix()
{
	RESET_CHECK_MATRIX_ELEMENT();

	if (!EstimateBuild())
		BuildRightHand();

	if (sc.m_bRefactorMatrix)
	{
		ResetElement();

		for (auto&& it : m_DeviceContainers)
			it->BuildBlock(this);

		m_bRebuildMatrixFlag = false;
		sc.m_dLastRefactorH = sc.m_dCurrentH;
		Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(
				"Рефакторизация матрицы {} / {}", klu.FactorizationsCount(), klu.RefactorizationsCount()));
		if(sc.m_bFillConstantElements)
			Log(DFW2MessageStatus::DFW2LOG_DEBUG, "Обновление констант");
		if (!EstimateBuild())
			sc.m_bFillConstantElements = false;
	}
}

void CDynaModel::BuildDerivatives()
{
	ResetElement();
	// похоже что производные дифуров при нулевых
	// производных алгебры только мешают Ньютону
	return; 
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
	if(nRow >= m_nEstimatedMatrixSize || nCol >= m_nEstimatedMatrixSize || nRow < 0 || nCol < 0)
		throw dfw2error(fmt::format("CDynaModel::ReallySetElement matrix size overrun Row {} Col {} MatrixSize {}", nRow, nCol, m_nEstimatedMatrixSize));

	MatrixRow *pRow = m_pMatrixRows + nRow;
	ptrdiff_t nMethodIndx = static_cast<ptrdiff_t>((pRightVector + nCol)->EquationType) * 2 + (sc.q - 1);
	// в качестве типа уравнения используем __физический__ тип
	// потому что у алгебраических и дифференциальных уравнений
	// разная структура в матрице Якоби, а EquationType указывает лишь набор коэффициентов метода

	if ((pRightVector + nRow)->PhysicalEquationType == DET_ALGEBRAIC)
		dValue *= Methodl[nMethodIndx][0];		// если уравнение алгебраическое, ставим коэффициент метода интегрирования
	else
	{
		// если уравнение дифференциальное, ставим коэффициент метода умноженный на шаг
		dValue *= Methodlh[nMethodIndx];
		// если элемент диагональный, учитываем диагональную единичную матрицу
		if (nRow == nCol)
			dValue = 1.0 - dValue;
	}

	_CheckNumber(dValue);

	/*
	switch (sc.IterationMode)
	{
	case StepControl::eIterationMode::JN:
		if (nRow != nCol) dValue = 0.0;
		break;
	case StepControl::eIterationMode::FUNCTIONAL:
		if (nRow != nCol) dValue = 0.0; else dValue = 1.0;
		break;

	}
	*/

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
			throw dfw2error("CDynaModel::ReallySetElement Column count");
		*pRow->pAp = nCol;			*pRow->pAx = dValue;
		pRow->pAp++;				pRow->pAx++;
	}
}


void CDynaModel::ReallySetElementNoDup(ptrdiff_t nRow, ptrdiff_t nCol, double dValue)
{
	if (nRow >= m_nEstimatedMatrixSize || nCol >= m_nEstimatedMatrixSize || nRow < 0 || nCol < 0)
		throw dfw2error(fmt::format("CDynaModel::ReallySetElement matrix size overrun Row {} Col {} MatrixSize {}", nRow, nCol, m_nEstimatedMatrixSize));

	CHECK_MATRIX_ELEMENT(nRow, nCol);

	MatrixRow *pRow = m_pMatrixRows + nRow;
	ptrdiff_t nMethodIndx = static_cast<ptrdiff_t>((pRightVector + nCol)->EquationType) * 2 + (sc.q - 1);
	// в качестве типа уравнения используем __физический__ тип
	// потому что у алгебраических и дифференциальных уравнений
	// разная структура в матрице Якоби, а EquationType указывает лишь набор коэффициентов метода

	if ((pRightVector + nRow)->PhysicalEquationType == DET_ALGEBRAIC)
		dValue *= Methodl[nMethodIndx][0];		// если уравнение алгебраическое, ставим коэффициент метода интегрирования
	else
	{
		// если уравнение дифференциальное, ставим коэффициент метода умноженный на шаг
		dValue *= Methodlh[nMethodIndx];
		// если элемент диагональный, учитываем диагональную единичную матрицу
		if (nRow == nCol)
			dValue = 1.0 - dValue;
	}

	_CheckNumber(dValue);

	/*
	switch (sc.IterationMode)
	{
	case StepControl::eIterationMode::JN:
		if (nRow != nCol) dValue = 0.0;
		break;
	case StepControl::eIterationMode::FUNCTIONAL:
		if (nRow != nCol) dValue = 0.0; else dValue = 1.0;
		break;

	}
	*/

	if (pRow->pAp >= pRow->pApRow + pRow->m_nColsCount || pRow->pAx >= pRow->pAxRow + pRow->m_nColsCount)
		throw dfw2error("CDynaModel::ReallySetElementNoDup Column count");

	*pRow->pAp = nCol;			*pRow->pAx = dValue;
	pRow->pAp++;				pRow->pAx++;
}
// Функция подсчета количества элементов в строке матрицы
void CDynaModel::CountSetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious)
{
	if (nRow >= m_nEstimatedMatrixSize || nRow < 0 )
		throw dfw2error(fmt::format("CDynaModel::CountSetElement matrix size overrun Row {} Col {} MatrixSize {}",nRow, nCol, m_nEstimatedMatrixSize));
	if (!bAddToPrevious)
	{
		// учитываем элементы с учетом суммирования
		MatrixRow *pRow = m_pMatrixRows + nRow;
		pRow->m_nColsCount++;
	}
}


void CDynaModel::CountSetElementNoDup(ptrdiff_t nRow, ptrdiff_t nCol, double dValue)
{
	if (nRow >= m_nEstimatedMatrixSize || nRow < 0)
		throw dfw2error(fmt::format("CDynaModel::CountSetElementNoDup matrix size overrun Row {} Col {} MatrixSize {}", nRow, nCol, m_nEstimatedMatrixSize));
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

void CDynaModel::SetElement(const VariableIndexBase& Row, const VariableIndexBase& Col, double dValue)
{
	(this->*(ElementSetterNoDup))(Row.Index, Col.Index, dValue);
}

void CDynaModel::SetElement(const VariableIndexBase& Row, const InputVariable& Col, double dValue)
{
	(this->*(ElementSetterNoDup))(Row.Index, Col.Index, dValue);
}

// задает правую часть алгебраического уравнения
void CDynaModel::SetFunction(ptrdiff_t nRow, double dValue)
{
	if (nRow >= m_nEstimatedMatrixSize || nRow < 0)
		throw dfw2error(fmt::format("CDynaModel::SetFunction matrix size overrun Row {} MatrixSize {}", nRow, m_nEstimatedMatrixSize));
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
#ifdef USE_FMA
	SetFunctionEqType(nRow, std::fma(GetH(), dValue, - pRv->Nordsiek[1] - pRv->Error), GetDiffEquationType());
#else
	SetFunctionEqType(nRow, GetH() * dValue - pRv->Nordsiek[1] - pRv->Error, GetDiffEquationType());
#endif
}

void CDynaModel::SetFunction(const VariableIndexBase& Row, double dValue)
{
	SetFunction(Row.Index, dValue);
}

void CDynaModel::SetFunctionDiff(const VariableIndexBase& Row, double dValue)
{
	SetFunctionDiff(Row.Index, dValue);
}

void CDynaModel::SetDerivative(const VariableIndexBase& Row, double dValue)
{
	SetDerivative(Row.Index, dValue);
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
	double dVal{ std::numeric_limits<double>::quiet_NaN() };

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
		RightVector* pVectorBegin{ pRightVector };
		RightVector* const pVectorEnd{ pRightVector + m_nEstimatedMatrixSize };

		while (pVectorBegin < pVectorEnd)
		{
			if (pVectorBegin->PrimitiveBlock == PBT_DERLAG)
			{
				// уравнение отмечено как уравнение РДЗ, меняем отметку, чтобы указать, что это уравнение уже прошли
				pVectorBegin->PrimitiveBlock = PBT_LAST;
				//pVectorBegin->Atol = 1E-2;
				//pVectorBegin->Rtol = 1E-5;

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
									//Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_DEBUG, "Точность %s %s", pVectorBegin->pDevice->GetVerbalName(), pVectorBegin->pDevice->VariableNameByPtr((pRightVector + nEqIndex)->pValue));
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
		Log(DFW2MessageStatus::DFW2LOG_DEBUG, "Marked = {}", nMarked);
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

			bool bFullFactor{ !m_Parameters.m_bUseRefactor };
			if (!bFullFactor) // делаем вторую и более итерацию Ньютона и разрешен рефактор
			{
				const double dLastRcond{ sc.dLastConditionNumber };			// запоминаем предыдущее число обусловленности
				bFullFactor = !klu.TryRefactor();							// делаем рефакторизацию
				if (!bFullFactor)											// получилось, считаем отношение нового числа обусловленности к старому
					bFullFactor = (1.0 / klu.Rcond()) / dLastRcond > 1E4;	// если рост значительный - делаем полную факторизацию
			}

			if (bFullFactor)
			{
				klu.Factor();
				// обновляем число обусловленности
				// и определяем максимальное за весь расчет
				UpdateRcond();
			}
		}
		klu.Solve();
	}
	else
		klu.Solve();
}



void CDynaModel::SolveRefine()
{
	const double maxresidual{ klu.SolveRefine( GetAtol() * GetAtol() * GetAtol()) };
	if (maxresidual > sc.dMaxSLEResidual)
	{
		sc.dMaxSLEResidual = maxresidual;
		sc.dMaxSLEResidualTime = sc.t;
	}
	UpdateRcond();

}

void CDynaModel::UpdateRcond()
{
	const double rCond{ 1.0 / klu.Rcond() };
	//double Cond = klu.Condest();
	sc.dLastConditionNumber = rCond;
	if (rCond > sc.dMaxConditionNumber)
	{
		sc.dMaxConditionNumber = rCond;
		sc.dMaxConditionNumberTime = sc.t;
	}
}

void CDynaModel::ScaleAlgebraicEquations()
{
	RightVector* pVectorBegin{ pRightVector };
	RightVector* const pVectorEnd{ pRightVector + m_nEstimatedMatrixSize };
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
	if (nRow >= m_nEstimatedMatrixSize || nRow < 0)
		throw dfw2error(fmt::format("CDynaModel::CountConstElementsToSkip matrix size overrun Row {} MatrixSize {}", nRow, m_nEstimatedMatrixSize));
	MatrixRow *pRow = m_pMatrixRows + nRow;
	pRow->m_nConstElementsToSkip = pRow->pAp - pRow->pApRow;
	return true;
}

bool CDynaModel::SkipConstElements(ptrdiff_t nRow)
{
	if (nRow >= m_nEstimatedMatrixSize)
		throw dfw2error(fmt::format("CDynaModel::SkipConstElements matrix size overrun Row {} MatrixSize {}", nRow, m_nEstimatedMatrixSize));
	MatrixRow *pRow = m_pMatrixRows  + nRow;
	pRow->pAp = pRow->pApRow + pRow->m_nConstElementsToSkip;
	pRow->pAx = pRow->pAxRow + pRow->m_nConstElementsToSkip;
	return true;
}


void CDynaModel::CreateTotalRightVector()
{
	// стром полный вектор всех возможных переменных вне зависимости от состояния устройств
	m_nTotalVariablesCount = 0;
	// считаем количество устройств
	for (auto&& cit : m_DeviceContainers)
	{
		// для volatile устройств зон в TotalRightVector не оставляем места
		if (cit->m_ContainerProps.bVolatile)
			continue;
		m_nTotalVariablesCount += cit->m_ContainerProps.nEquationsCount * cit->Count();
	}
	// забираем полный вектор для всех переменных
	pRightVectorTotal = std::make_unique<RightVectorTotal[]>(m_nTotalVariablesCount);
	RightVectorTotal *pb = pRightVectorTotal.get();

	for (auto&& cit : m_DeviceContainers)
	{
		if (cit->m_ContainerProps.bVolatile)
			continue;

		// запоминаем в полном векторе адреса всех переменных и всех устройств
		for (auto&& dit : *cit)
			for (ptrdiff_t z = 0; z < cit->EquationsCount(); z++, pb++)
			{
				pb->pValue = dit->GetVariablePtr(z);
				pb->pDevice = dit;
				pb->nErrorHits = 0;
			}
	}
}

void CDynaModel::UpdateNewRightVector()
{
	RightVectorTotal* pRvB = pRightVectorTotal.get();
	RightVector* pRv = pRightVectorUniq.get();

	// логика синхронизации векторов такая же как в UpdateTotalRightVector
	for (auto&& cit : m_DeviceContainers)
	{
		if (cit->m_ContainerProps.bVolatile)
		{
			pRv += cit->Count() * cit->EquationsCount();
			continue;
		}

		// для устройств в контейнере, которые включены в матрицу

		RightVector* pEnd = pRv + m_nEstimatedMatrixSize;
		for (auto&& dit : *cit)
		{
			if (dit->AssignedToMatrix())
			{
				for (ptrdiff_t z = 0; z < cit->EquationsCount(); z++)
				{
					// проверяем, совпадают ли адрес устройства и переменной в полном векторе
					// с адресом устройства и переменной во рабочем векторе
					if (pRv >= pEnd)
						throw dfw2error("CDynaModel::UpdateNewRightVector - New right vector overrun");

					/*
					if (pRvB->pDevice != pRv->pDevice || pRvB->pValue != pRv->pValue)
						throw dfw2error("CDynaModel::UpdateNewRightVector - TotalRightVector Out Of Sync");
						*/
					InitNordsiekElement(pRv, pRvB->Atol, pRvB->Rtol);
					PrepareNordsiekElement(pRv);
					*static_cast<RightVectorTotal*>(pRv) = *pRvB;
					pRv++;	pRvB++;
				}
			}
			else
				pRvB += cit->EquationsCount();
		}
	}
}


void CDynaModel::DebugCheckRightVectorSync()
{
	RightVectorTotal* pRvB = pRightVectorTotal.get();
	RightVector* pRv = pRightVectorUniq.get();

	// логика синхронизации векторов такая же как в UpdateTotalRightVector
	for (auto&& cit : m_DeviceContainers)
	{
		if (cit->m_ContainerProps.bVolatile)
		{
			pRv += cit->Count() * cit->EquationsCount();
			continue;
		}

		// для устройств в контейнере, которые включены в матрицу
		RightVector* pEnd = pRv + m_nEstimatedMatrixSize;
		for (auto&& dit : *cit)
		{
			if (dit->AssignedToMatrix())
			{
				for (ptrdiff_t z = 0; z < cit->EquationsCount(); z++)
				{
					// проверяем, совпадают ли адрес устройства и переменной в полном векторе
					// с адресом устройства и переменной во рабочем векторе
					if (pRv >= pEnd)
						throw dfw2error("CDynaModel::DebugCheckRightVectorSync - New right vector overrun");
					if (pRvB->pDevice != pRv->pDevice || pRvB->pValue != pRv->pValue)
						throw dfw2error("CDynaModel::DebugCheckRightVectorSync - TotalRightVector Out Of Sync");
					pRv++;	pRvB++;
				}
			}
			else
				pRvB += cit->EquationsCount();
		}
	}
}

void CDynaModel::UpdateTotalRightVector()
{
	RightVectorTotal *pRvB = pRightVectorTotal.get();
	RightVector *pRv = pRightVectorUniq.get();

	// проходим по всем устройствам, пропускаем фрагменты RightVectorTotal для
	// устройств без уравнений, для всех остальных копируем то что посчитано в RightVector в RightVectorTotal
	for (auto&& cit : m_DeviceContainers)
	{
		if (cit->m_ContainerProps.bVolatile)
		{
			// если встретили volatile конетейнер (например - синхронные зоны) - пропускаем его в RightVector,
			// потому что в RightVectorTotal их нет
			pRv += cit->Count() * cit->EquationsCount();
			continue;
		}

		// для устройств в контейнере, которые включены в матрицу
		for (auto&& dit : *cit)
		{
			if (dit->AssignedToMatrix())
			{
				for (ptrdiff_t z = 0; z < cit->EquationsCount(); z++)
				{
					// проверяем, совпадают ли адрес устройства и переменной в полном векторе
					// с адресом устройства и переменной во рабочем векторе
					if (pRvB->pDevice != pRv->pDevice || pRvB->pValue != pRv->pValue)
						throw dfw2error("CDynaModel::UpdateTotalRightVector - TotalRightVector Out Of Sync");

					// количество ошибок в полном векторе суммируем с накопленным количеством ошибок в рабочем
					ptrdiff_t nNewErrorHits = pRvB->nErrorHits + pRv->nErrorHits;
					*pRvB = *pRv;
					pRvB->nErrorHits = nNewErrorHits;
					pRv++;	pRvB++;
				}
			}
			else
				pRvB += cit->EquationsCount();
		}
	}
}

void CDynaModel::CheckMatrixElement(ptrdiff_t nRow, ptrdiff_t nCol)
{
	/*
	auto& ins = m_MatrixChecker.insert(std::make_pair(nRow, std::set<ptrdiff_t>{nCol}));
	if (!ins.second)
		if (!ins.first->second.insert(nCol).second)
			throw dfw2error(Cex("CDynaModel::CheckMatrixElement dup element at %d %d", nRow, nCol));
	*/
}

void CDynaModel::ResetCheckMatrixElement()
{
	m_MatrixChecker.clear();
}