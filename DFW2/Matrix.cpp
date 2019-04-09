#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

bool CDynaModel::EstimateMatrix()
{
	bool bRes = true;

	ptrdiff_t nOriginalMatrixSize = m_nMatrixSize;
	bool bSaveRightVector = pRightVector != NULL;

	CleanUpMatrix(bSaveRightVector);
	
	m_nMatrixSize = m_nNonZeroCount = 0;
	m_bEstimateBuild = true;

	sc.RefactorMatrix();

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		bRes = (*it)->EstimateBlock(this) && bRes;
	}
	if (bRes)
	{
		b = new double[m_nMatrixSize];
		//bRightHand = new double[m_nMatrixSize];
		//Solution = new double[m_nMatrixSize];

		m_pMatrixRows = new MatrixRow[m_nMatrixSize];
		m_pZeroCrossingDevices = new CDevice* [m_nMatrixSize];

		if (bSaveRightVector)
		{
			// make a copy of original right vector to new right vector
			RightVector *pNewRightVector = new RightVector[m_nMatrixSize];
			RightVector *pNewBegin = pNewRightVector;
			RightVector *pOldBegin = pRightVector;
			RightVector *pOldEnd = pOldBegin + min(nOriginalMatrixSize,m_nMatrixSize);

			while (pOldBegin < pOldEnd)
			{
				*pNewBegin = *pOldBegin;
				pNewBegin++;
				pOldBegin++;
			}

			delete pRightVector;
			pRightVector = pNewRightVector;
		}
		else
		{
			pRightVector = new RightVector[m_nMatrixSize];
			pRightHandBackup = new double[m_nMatrixSize];
		}

		// init RightVector primitive block types
		struct RightVector *pVectorBegin = pRightVector + nOriginalMatrixSize;
		struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;
		while (pVectorBegin < pVectorEnd)
		{
			pVectorBegin->PrimitiveBlock = PBT_UNKNOWN;
			pVectorBegin++;
		}


		// substitute element setter to counter (not actually setting values, just count)

		ElementSetter = &CDynaModel::CountSetElement;
		bRes = BuildMatrix();
		if (bRes)
		{
			m_nNonZeroCount = 0;

			// allocate matrix row headers to access rows instantly

			MatrixRow *pRow;
			for (pRow = m_pMatrixRows; pRow < m_pMatrixRows + m_nMatrixSize; pRow++)
				m_nNonZeroCount += pRow->m_nColsCount;

			// allocate KLU matrix in CCS form

			Ax = new double[m_nNonZeroCount];
			Ap = new ptrdiff_t[m_nNonZeroCount];
			Ai = new ptrdiff_t[m_nMatrixSize + 1];

			m_pMatrixRows->pApRow = Ap;
			m_pMatrixRows->pAxRow = Ax;

			MatrixRow *pPrevRow = m_pMatrixRows;

			// spread pointers to Ap within matrix row headers

			for (pRow = pPrevRow + 1; pRow < m_pMatrixRows + m_nMatrixSize; pRow++, pPrevRow++)
			{
				pRow->pApRow = pPrevRow->pApRow + pPrevRow->m_nColsCount;
				pRow->pAxRow = pPrevRow->pAxRow + pPrevRow->m_nColsCount;
			}

			// revert to real element setter
			ElementSetter = &CDynaModel::ReallySetElement;
			
			InitDevicesNordsiek();

			if (bSaveRightVector && nOriginalMatrixSize < m_nMatrixSize)
			{
				pVectorBegin = pRightVector + nOriginalMatrixSize;

				while (pVectorBegin < pVectorEnd)
				{
					InitNordsiekElement(pVectorBegin,GetAtol(),GetRtol());
					PrepareNordsiekElement(pVectorBegin);
					pVectorBegin++;
				}
			}

		}
	}
	m_bEstimateBuild = false;
	return bRes;
}

bool CDynaModel::BuildRightHand()
{
	bool bRes = true;

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() && m_bStatus; it++)
	{
		bRes = (*it)->BuildRightHand(this) && bRes && m_bStatus;
	}

	sc.dRightHandNorm = 0.0;
	double *pBb = b, *pBe = b + m_nMatrixSize; 
	while (pBb < pBe)
	{
		sc.dRightHandNorm += *pBb * *pBb;
		pBb++;
	}
	memcpy(pRightHandBackup, b, sizeof(double) * m_nMatrixSize);
	sc.dRightHandNorm *= 0.5;
	bRes = bRes && m_bStatus;

	return bRes;
}

bool CDynaModel::BuildMatrix()
{
	bool bRes = true;

	if (sc.m_bRefactorMatrix)
	{
		ResetElement();
		for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end() && m_bStatus; it++)
		{
			bRes = (*it)->BuildBlock(this) && bRes;
		}
		bRes = bRes && m_bStatus;
		m_bRebuildMatrixFlag = false;
		sc.m_dLastRefactorH = sc.m_dCurrentH;
		_tcprintf(_T("\n�������������� ������� %d"), sc.nFactorizationsCount);
	}

	if (bRes)
	{
		if (!EstimateBuild())
		{
			bRes = BuildRightHand();
		}
	}

	return bRes;
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
	if (Ax) delete Ax;
	if (Ap) delete Ap;
	if (Ai) delete Ai;
	if (b)  delete b;
	//if (bRightHand) delete bRightHand;
	//if (Solution) delete Solution;

	if (!bSaveRightVector)
		if (pRightVector) delete pRightVector;

	if (pRightHandBackup)
		delete pRightHandBackup;

	if (Symbolic)
		KLU_free_symbolic(&Symbolic, &Common);

	if (Numeric)
		KLU_free_numeric(&Numeric, &Common);

	if (m_pMatrixRows)
		delete m_pMatrixRows;
	if (m_pZeroCrossingDevices)
		delete m_pZeroCrossingDevices;
}


ptrdiff_t CDynaModel::AddMatrixSize(ptrdiff_t nSizeIncrement)
{
	ptrdiff_t nRet = m_nMatrixSize;
	m_nMatrixSize += nSizeIncrement;
	return nRet;
}

void CDynaModel::ResetElement()
{
	m_bStatus = true;
	if (m_pMatrixRows)
	{
		MatrixRow *pRow = m_pMatrixRows;
		MatrixRow *pEnd = m_pMatrixRows + m_nMatrixSize;
		while (pRow < pEnd)
		{
			pRow->Reset();
			pRow++;
		}
	}
}

bool CDynaModel::ReallySetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious)
{
	if (nRow >= 0 && nRow < m_nMatrixSize &&
		nCol >= 0 && nCol < m_nMatrixSize)
	{
		m_bStatus = false;
		MatrixRow *pRow = m_pMatrixRows + nRow;

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

		if (bAddToPrevious)
		{
			ptrdiff_t *pSp = pRow->pAp - 1;
			while (pSp >= pRow->pApRow)
			{
				if (*pSp == nCol)
				{
					ptrdiff_t pDfr = pSp - pRow->pAp;
					*(pRow->pAx + pDfr) += dValue;
					m_bStatus = true;
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
				m_bStatus = true;
			}
		}
	}
	else
		m_bStatus = false;

	_ASSERTE(m_bStatus);

	return m_bStatus;
}

bool CDynaModel::CountSetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious)
{
	if (nRow < m_nMatrixSize)
	{
		if (!bAddToPrevious)
		{
			MatrixRow *pRow = m_pMatrixRows + nRow;
			pRow->m_nColsCount++;
		}
	}
	else
		m_bStatus = false;

	_ASSERTE(m_bStatus);

	return m_bStatus;
}

bool CDynaModel::SetElement(ptrdiff_t nRow, ptrdiff_t nCol, double dValue, bool bAddToPrevious)
{
	
	//_ASSERTE(sc.m_bRefactorMatrix);

	if (!m_bStatus)
		return m_bStatus;

	(this->*(ElementSetter))(nRow, nCol, dValue, bAddToPrevious);

	return m_bStatus;
}

bool CDynaModel::SetFunction(ptrdiff_t nRow, double dValue)
{
	if (!m_bStatus)
		return m_bStatus;

	_CheckNumber(dValue);

	RightVector *pRv = GetRightVector(nRow);
	if (pRv)
		(this->*(RightHandSetter))(b + nRow, dValue);
		//b[nRow] = -dValue / CDynaModel::l[DET_ALGEBRAIC * 2 + sc.q - 1][0];
	else
		m_bStatus = false;

	return m_bStatus;
}

bool CDynaModel::AddFunction(ptrdiff_t nRow, double dValue)
{
	if (!m_bStatus)
		return m_bStatus;

	_CheckNumber(dValue);

	if (nRow >= 0 && nRow < m_nMatrixSize)
		b[nRow] -= dValue;
	else
		m_bStatus = false;

	return m_bStatus;
}


bool CDynaModel::SetDerivative(ptrdiff_t nRow, double dValue)
{
	struct RightVector *pRv = GetRightVector(nRow);

	_CheckNumber(dValue);

	if (pRv)
	{
		pRv->Nordsiek[1] = dValue * GetH();
		return true;
	}
	else
		return m_bStatus = false;
}

bool CDynaModel::CorrectNordsiek(ptrdiff_t nRow, double dValue)
{
	bool bRes = false;
	RightVector *pRightVector = GetRightVector(nRow);
	if (pRightVector)
	{
		bRes = true;
		pRightVector->Nordsiek[0] = dValue;
		pRightVector->Nordsiek[1] = pRightVector->Nordsiek[2] = 0.0;

		pRightVector->SavedNordsiek[0] = dValue;
		pRightVector->SavedNordsiek[1] = pRightVector->SavedNordsiek[2] = 0.0;

		pRightVector->Tminus2Value = dValue;
	}
	return bRes;
}

bool CDynaModel::SetFunctionDiff(ptrdiff_t nRow, double dValue)
{
	struct RightVector *pRv = GetRightVector(nRow);

	_CheckNumber(dValue);


	if (pRv)
	{
		SetFunctionEqType(nRow, -(GetH() * dValue - pRv->Nordsiek[1] - pRv->Error), GetDiffEquationType());
		return true;
	}
	else
		return false;
}



bool CDynaModel::SetFunctionEqType(ptrdiff_t nRow, double dValue, DEVICE_EQUATION_TYPE EquationType)
{
	if (!m_bStatus)
		return m_bStatus;

	if (nRow >= 0 && nRow < m_nMatrixSize)
	{
		b[nRow] = -dValue;
		pRightVector[nRow].EquationType = EquationType;
		pRightVector[nRow].PhysicalEquationType = DET_DIFFERENTIAL;
	}
	else
		m_bStatus = false;

	return m_bStatus;
}

double CDynaModel::GetFunction(ptrdiff_t nRow)
{
	double dVal = NAN;

	if (nRow >= 0 && nRow < m_nMatrixSize)
		dVal = b[nRow];

	return dVal;
}

bool CDynaModel::ConvertToCCSMatrix()
{
	bool bRes = true;

/*
#ifdef _DEBUG
	for (MatrixRow *pRow = m_pMatrixRows; pRow < m_pMatrixRows + m_nMatrixSize; pRow++)
	{
		bool bZeroDiagonal = false;
		for (ptrdiff_t i = 0; i < pRow->m_nColsCount; i++)
		{
			if (pRow->pApRow[i] == pRow - m_pMatrixRows)
			{
				if (!Equal(pRow->pAxRow[i], 0.0))
				{
					bZeroDiagonal = true;
					break;
				}
			}
		}
		_ASSERTE(bZeroDiagonal);
	}
#endif
*/

	for (MatrixRow *pRow = m_pMatrixRows; pRow < m_pMatrixRows + m_nMatrixSize; pRow++)
		Ai[pRow - m_pMatrixRows] = pRow->pApRow - Ap;
	Ai[m_nMatrixSize] = m_nNonZeroCount;
	return bRes;
}

bool CDynaModel::SolveLinearSystem()
{
	bool bRes = false;

	if (!Symbolic || !Numeric)
		sc.RefactorMatrix();

	if (sc.m_bRefactorMatrix)
	{
		if (ConvertToCCSMatrix())
		{
			if (!Symbolic)
			{
				Symbolic = KLU_analyze(m_nMatrixSize, Ai, Ap, &Common);
				RightVector *pVectorEnd = pRightVector + m_nMatrixSize;


				// ��� ���� �������������� ���������, � ������� ������ ���, ���������� ������ �������� Atol �� ����������� ���
				// ���� ������� �� ����������������� ��������� - ��� �������� � �������� ��������� � ��� ��������� ���������
				ptrdiff_t nMarked = 0;
				do
				{
					nMarked = 0;
					RightVector *pVectorBegin = pRightVector;

					while (pVectorBegin < pVectorEnd)
					{
						if (pVectorBegin->PrimitiveBlock == PBT_DERLAG)
						{
							// ��������� �������� ��� ��������� ���, ������ �������, ����� �������, ��� ��� ��������� ��� ������
							pVectorBegin->PrimitiveBlock = PBT_LAST;
							pVectorBegin->Atol = 1E-2;

							ptrdiff_t nDerLagEqIndex = pVectorBegin - pRightVector;
							MatrixRow *pRow = m_pMatrixRows;

							// ������������� ������ �������, ���� ������� � ��������, ��������������� ��������� ���
							for (; pRow < m_pMatrixRows + m_nMatrixSize; pRow++)
							{
								ptrdiff_t nEqIndex = pRow - m_pMatrixRows;
								if (nEqIndex != nDerLagEqIndex)
								{
									for (ptrdiff_t *pc = pRow->pApRow; pc < pRow->pApRow + pRow->m_nColsCount; pc++)
									{
										if (*pc == nDerLagEqIndex)
										{
											// ����� ������� � �������� ��������� ���
											RightVector *pMarkEq = pRightVector + nEqIndex;
											// ���� ��������� � ���� �������� �������������� � ��� �� �����������
											if (pMarkEq->PrimitiveBlock == PBT_UNKNOWN && pMarkEq->EquationType == DET_ALGEBRAIC)
											{
												// �������� ��� ��� ��������� ���, ������ �������� ���
												_tcprintf(_T("\n%s %s"), pVectorBegin->pDevice->GetVerbalName(), pVectorBegin->pDevice->VariableNameByPtr((pRightVector + nEqIndex)->pValue));
												pMarkEq->PrimitiveBlock = PBT_DERLAG;
												pMarkEq->Atol = pVectorBegin->Atol;
												pMarkEq->Rtol = pVectorBegin->Rtol;
												// ������� ������� ��������� ����������
												nMarked++;
											}
										}
									}
								}
							}
						}
						pVectorBegin++;
					}
					// ����������, ���� ���� �������������� ���������
					_tcprintf(_T("\nMarked = %d"), nMarked);
				} 
				while (nMarked);

				sc.nAnalyzingsCount++;
			}

			if (Symbolic)
			{
				if (Numeric)
					KLU_free_numeric(&Numeric, &Common);

				//_tcprintf(_T("\n----------------Refactor---------------------"));

				// ScaleAlgebraicEquations();
				Numeric = KLU_factor(Ai, Ap, Ax, Symbolic, &Common);

				if (Numeric)
				{
					sc.nFactorizationsCount++;
					if (KLU_tsolve(Symbolic, Numeric, m_nMatrixSize, 1, b, &Common))
					{
						KLU_rcond(Symbolic, Numeric, &Common);
						//memcpy(Solution, b, sizeof(double) * m_nMatrixSize);
						bRes = true;
					}

				}
			}
		}
	}
	else
	{
		_ASSERTE(Symbolic && Numeric);
		//_tcprintf(_T("\n----------------Solve---------------------"));
		if (KLU_tsolve(Symbolic, Numeric, m_nMatrixSize, 1, b, &Common))
		{
			KLU_rcond(Symbolic, Numeric, &Common);
			//memcpy(Solution, b, sizeof(double) * m_nMatrixSize);
			bRes = true;
		}
	}

	if (!bRes)
		ReportKLUError();

	return bRes;
}


void CDynaModel::ResetMatrixStructure()
{
	KLU_free_symbolic(&Symbolic, &Common);

	Symbolic = NULL;
	if (m_pMatrixRows)
	{
		delete m_pMatrixRows;
		m_pMatrixRows = NULL;
	}
}

void CDynaModel::RightHandSetter1(double *pRightHand, double& Value)
{
	*pRightHand = -Value;
}

void CDynaModel::RightHandSetter2(double *pRightHand, double& Value)
{
	*pRightHand = -Value / CDynaModel::l[1][0];
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

*/

void CDynaModel::ScaleAlgebraicEquations()
{
	RightVector *pVectorBegin = pRightVector;
	RightVector *pVectorEnd = pRightVector + m_nMatrixSize;
	double h = GetH();

	if (h <= 0)
		h = 1.0;

	while (pVectorBegin < pVectorEnd)
	{
		if (pVectorBegin->EquationType == DET_ALGEBRAIC)
		{
			ptrdiff_t j = pVectorBegin - pRightVector;
			for (ptrdiff_t p = Ai[j]; p < Ai[j + 1]; p++)
				Ax[p] /= h;
		}

		pVectorBegin++;

	}
}