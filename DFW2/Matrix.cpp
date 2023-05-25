#include "stdafx.h"
#include "DynaModel.h"
#include "MixedAdamsBDF.h"

using namespace DFW2;

void CDynaModel::EstimateMatrix()
{
	const ptrdiff_t nOriginalMatrixSize{ klu.MatrixSize() };
	// если RightVector уже есть - ставим флаг сохранения данных в новый вектор
	bool bSaveRightVector{ pRightVector != nullptr };
	m_nEstimatedMatrixSize = 0;
	ptrdiff_t nNonZeroCount(0);
	// заставляем обновиться постоянные элементы в матрице, ставим оценочную сборку матрицы
	sc.UpdateConstElements(EstimateBuild_ = true);
	// взводим флаг рефакторизации
	sc.RefactorMatrix();

	// Если нужно обновить данные в общем векторе правой части
	// делаем это _до_ EstimateBlock, так как состав матрицы
	// может измениться и рассинхронизироваться с pRightVector
	if (bSaveRightVector)
		UpdateTotalRightVector();

	for (auto&& it : DeviceContainers_)
		it->EstimateBlock(this);

	m_pMatrixRowsUniq = std::make_unique<MatrixRow[]>(m_nEstimatedMatrixSize);
	m_pMatrixRows = m_pMatrixRowsUniq.get();

	ZeroCrossingDevices.reserve(m_nEstimatedMatrixSize);

	if (bSaveRightVector)
	{
		// make a copy of original right vector to new right vector
		CreateUniqueRightVector();
		UpdateNewRightVector();
	}
	else
	{
		CreateTotalRightVector();
		CreateUniqueRightVector();
	}

	// substitute element setter to counter (not actually setting values, just count)

	ElementSetter		= &CDynaModel::CountSetElement;
	ElementSetterNoDup  = &CDynaModel::CountSetElementNoDup;

	BuildMatrix();
	nNonZeroCount = 0;

	// allocate matrix row headers to access rows instantly

	MatrixRow* pRow{ m_pMatrixRows };
	const MatrixRow* pRowEnd{ pRow + m_nEstimatedMatrixSize };
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
	EstimateBuild_ = false;
}

void CDynaModel::BuildRightHand()
{
	for (auto&& it : DeviceContainers_)
		it->BuildRightHand(this);

	Integrator_->BOperator();

	sc.dRightHandNorm = 0.0;

	for (const auto& b : BRange())
		sc.dRightHandNorm += b * b;

	std::copy(klu.B(), klu.B() + m_nEstimatedMatrixSize, pRightHandBackup.get());
}

void CDynaModel::BuildMatrix(bool SkipRightHand)
{
	RESET_CHECK_MATRIX_ELEMENT();

	if (!EstimateBuild() && !SkipRightHand)
		BuildRightHand();

	if (sc.m_bRefactorMatrix)
	{
		ResetElement();

		if (FillConstantElements())
			Log(DFW2MessageStatus::DFW2LOG_DEBUG, "Обновление констант");

		for (auto&& it : DeviceContainers_)
			it->BuildBlock(this);

		RebuildMatrixFlag_ = false;
		sc.m_dLastRefactorH = H();

		if(Integrator_->ReportJacobiRefactor())
			Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(
					"Рефакторизация матрицы {} / {}", 
				klu.FactorizationsCount(), 
				klu.RefactorizationsCount()));

		if (!EstimateBuild())
			sc.UpdateConstElements(false);
	}
}

void CDynaModel::BuildDerivatives()
{
	ResetElement();
	// похоже что производные дифуров при нулевых
	// производных алгебры только мешают Ньютону
	return; 
	for (auto&& it : DeviceContainers_)
		it->BuildDerivatives(this);
}

ptrdiff_t CDynaModel::AddMatrixSize(ptrdiff_t nSizeIncrement)
{
	const ptrdiff_t nRet{ m_nEstimatedMatrixSize };
	m_nEstimatedMatrixSize += nSizeIncrement;
	return nRet;
}

void CDynaModel::ResetElement()
{
	_ASSERTE(m_pMatrixRows);
	MatrixRow* pRow{ m_pMatrixRows };
	const MatrixRow* pEnd{ pRow + m_nEstimatedMatrixSize };
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

	Integrator_->WOperator(nRow, nCol, dValue);
	MatrixRow* pRow{ m_pMatrixRows + nRow };
	_CheckNumber(dValue);

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
			throw dfw2error("CDynaModel::ReallySetElement Column count mismatch");
		*pRow->pAp = nCol;			*pRow->pAx = dValue;
		pRow->pAp++;				pRow->pAx++;
	}
}


void CDynaModel::ReallySetElementNoDup(ptrdiff_t nRow, ptrdiff_t nCol, double dValue)
{
	if (nRow >= m_nEstimatedMatrixSize || nCol >= m_nEstimatedMatrixSize || nRow < 0 || nCol < 0)
		throw dfw2error(fmt::format("CDynaModel::ReallySetElement matrix size overrun Row {} Col {} MatrixSize {}", nRow, nCol, m_nEstimatedMatrixSize));

	CHECK_MATRIX_ELEMENT(nRow, nCol);

	Integrator_->WOperator(nRow, nCol, dValue);
	MatrixRow* pRow{ m_pMatrixRows + nRow };
	_CheckNumber(dValue);

	if (pRow->pAp >= pRow->pApRow + pRow->m_nColsCount || pRow->pAx >= pRow->pAxRow + pRow->m_nColsCount)
		throw dfw2error("CDynaModel::ReallySetElementNoDup Column count mismatch");

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
	klu.B()[nRow] = dValue;
}

void CDynaModel::SetDerivative(ptrdiff_t nRow, double dValue)
{
	auto rv { GetRightVector(nRow) };
	_CheckNumber(dValue);
	rv->Nordsiek[1] = dValue * H();
}

// копирует значение переменной и компонентов ее Нордсика из источника
void CDynaModel::CopyVariableNordsiek(const VariableIndex& Variable, const InputVariable& Source)
{
	// если переменная индексирована
	if (Variable.Indexed())
	{
		// достаем вектор
		auto rv{ GetRightVector(Variable.Index) };

		// задаем значение самой переменной и нулевого компонента Нордсика
		*rv->pValue = rv->Nordsiek[0] = Source;
		// если переменная-источник индексирована
		if (Source.Indexed())
		{
			// достаем вектор переменной-истоничка
			auto rvs{ GetRightVector(Source.Index) };
			// и копируем высшие компоненты нордсика
			rv->Nordsiek[1] = rvs->Nordsiek[1];
			rv->Nordsiek[2] = rvs->Nordsiek[2];
		}
		else // если переменная-инсточник неидексирована, скоре всего она константа
			rv->Nordsiek[1] = rv->Nordsiek[2] = 0;
	}
}

// устанавливает значение переменной и компонентов Нордиска
void CDynaModel::SetVariableNordsiek(const VariableIndex& Variable, double v0, double v1, double v2)
{
	// если переменная индексирована
	if (Variable.Indexed()  && !EstimateBuild() )
	{
		// достаем вектор
		auto rv{ GetRightVector(Variable.Index) };
		// задаем значение самой переменной, нулевого компонента Нордиска
		*rv->pValue = rv->Nordsiek[0] = v0;
		// и высших компонентов Нордсика
		rv->Nordsiek[1] = v1;
		rv->Nordsiek[2] = v2;
	}
}

void CDynaModel::CorrectNordsiek(ptrdiff_t nRow, double dValue)
{
	auto rv{ GetRightVector(nRow) };
	_CheckNumber(dValue);
	rv->Nordsiek[0] = dValue;
	rv->Nordsiek[1] = rv->Nordsiek[2] = 0.0;

	rv->SavedNordsiek[0] = dValue;
	rv->SavedNordsiek[1] = rv->SavedNordsiek[2] = 0.0;

	rv->Tminus2Value = dValue;
}

// задает правую часть дифференциального уравнения
void CDynaModel::SetFunctionDiff(ptrdiff_t nRow, double dValue)
{
	_CheckNumber(dValue);
	// ставим тип метода для уравнения по параметрам в исходных данных
	SetFunctionEqType(nRow, dValue, GetDiffEquationType());
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
	auto rv{ GetRightVector(nRow) };
	_CheckNumber(dValue);
	klu.B()[nRow] = dValue;
	rv->EquationType = EquationType;					// тип метода для уравнения
	rv->PhysicalEquationType = DET_DIFFERENTIAL;		// уравнение, устанавливаемое этой функцией всегда дифференциальное
	return true;
}


double CDynaModel::GetFunction(ptrdiff_t nRow)  const
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
	ptrdiff_t nMarked{ 0 };

	double DerlagCoe{ DerlagTolerance() };
	// если задан коэффициент изменения Atol/Rtol  1.0 - ничего не делаем
	if (Equal(DerlagCoe, 1.0))
		return;

	const double AtolDerlag{ DerlagCoe * Atol() };
	const double RtolDerlag{ DerlagCoe * Rtol() };

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
				pVectorBegin->Atol = AtolDerlag;
				pVectorBegin->Rtol = RtolDerlag;

				const ptrdiff_t nDerLagEqIndex{ pVectorBegin - pRightVector };
				MatrixRow* const pRowBase{ m_pMatrixRows };
				MatrixRow* pRow{ pRowBase };
				const MatrixRow* const pRowEnd{ pRow + m_nEstimatedMatrixSize };

				// просматриваем строки матрицы, ищем столбцы с индексом, соответствующим уравнению РДЗ
				for (; pRow < pRowEnd ; pRow++)
				{
					const ptrdiff_t nEqIndex{ pRow - pRowBase };
					if (nEqIndex != nDerLagEqIndex)
					{
						for (ptrdiff_t *pc = pRow->pApRow; pc < pRow->pApRow + pRow->m_nColsCount; pc++)
						{
							if (*pc == nDerLagEqIndex)
							{
								// нашли столбец с индексом уравнения РДЗ
								RightVector* const pMarkEq{ pRightVector + nEqIndex };
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
		//Log(DFW2MessageStatus::DFW2LOG_DEBUG, "Marked = {}", nMarked);
	} while (nMarked);

}

void CDynaModel::SolveLinearSystem(bool Refined)
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
		if (Refined)
			klu.SolveRefine();
		else
			klu.Solve();
	}
	else
		if (Refined)
			klu.SolveRefine();
		else
			klu.Solve();
}



void CDynaModel::SolveRefine()
{
	const double maxresidual{ klu.SolveRefine( Atol() * Atol() * Atol()) };
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
	auto pVectorBegin{ pRightVector };
	const auto pVectorEnd{ pRightVector + m_nEstimatedMatrixSize };
	double h{ H() };

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

void CDynaModel::CreateUniqueRightVector()
{
	pRightVectorUniq = std::make_unique<RightVector[]>(m_nEstimatedMatrixSize);
	pRightVector = pRightVectorUniq.get();
	pRightHandBackup = std::make_unique<double[]>(m_nEstimatedMatrixSize);
}

void CDynaModel::CreateTotalRightVector()
{
	// стром полный вектор всех возможных переменных вне зависимости от состояния устройств
	m_nTotalVariablesCount = 0;
	// считаем количество устройств
	for (auto&& cit : DeviceContainers_)
	{
		// для volatile устройств зон в TotalRightVector не оставляем места
		if (cit->ContainerProps().bVolatile)
			continue;
		m_nTotalVariablesCount += cit->ContainerProps().EquationsCount * cit->Count();
	}
	// забираем полный вектор для всех переменных
	pRightVectorTotal = std::make_unique<RightVectorTotal[]>(m_nTotalVariablesCount);
	RightVectorTotal* pb{ pRightVectorTotal.get() };

	for (auto&& cit : DeviceContainers_)
	{
		if (cit->ContainerProps().bVolatile)
			continue;

		// запоминаем в полном векторе адреса всех переменных и всех устройств
		for (auto&& dit : *cit)
			for (ptrdiff_t z = 0; z < cit->EquationsCount(); z++, pb++)
			{
				pb->pValue = dit->GetVariablePtr(z);
				pb->pDevice = dit;
				pb->ErrorHits = 0;
				pb->Atol = Atol();
				pb->Rtol = Rtol();
			}
	}
}

void CDynaModel::UpdateNewRightVector()
{
	RightVectorTotal* pRvB = pRightVectorTotal.get();
	RightVector* pRv = pRightVectorUniq.get();
	const RightVector*  pEnd{ pRv + m_nEstimatedMatrixSize };

	// логика синхронизации векторов такая же как в UpdateTotalRightVector
	for (auto&& cit : DeviceContainers_)
	{
		// для устройств в контейнере, которые включены в матрицу

		for (auto&& dit : *cit)
		{
			const bool VolatileContainer{ cit->ContainerProps().bVolatile };
			if (dit->AssignedToMatrix())
			{
				for (ptrdiff_t z = 0; z < cit->EquationsCount(); z++, pRv++)
				{
					// проверяем, совпадают ли адрес устройства и переменной в полном векторе
					// с адресом устройства и переменной во рабочем векторе
					if (pRv >= pEnd)
						throw dfw2error("CDynaModel::UpdateNewRightVector - New right vector overrun");

					// обходим устройства, которые volatile
					if (VolatileContainer)
						MixedAdamsBDF::InitNordsiekElement(pRv, Atol(), Rtol());
					else
					{
						MixedAdamsBDF::InitNordsiekElement(pRv, pRvB->Atol, pRvB->Rtol);
						*static_cast<RightVectorTotal*>(pRv) = *pRvB;
						pRvB++;
					}
					MixedAdamsBDF::PrepareNordsiekElement(pRv);
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
	for (auto&& cit : DeviceContainers_)
	{
		if (cit->ContainerProps().bVolatile)
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
	for (auto&& cit : DeviceContainers_)
	{
		if (cit->ContainerProps().bVolatile)
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
					ptrdiff_t nNewErrorHits = pRvB->ErrorHits + pRv->ErrorHits;
					*pRvB = *pRv;
					pRvB->ErrorHits = nNewErrorHits;
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