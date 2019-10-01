#include "stdafx.h"
#include "DynaModel.h"


using namespace DFW2;

// ������������ ������� Nordsieck ��� ��������� ����
void CDynaModel::Predict()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	// �������� ������� [Lsode 2.61]
	while (pVectorBegin < pVectorEnd)
	{
		pVectorBegin->Nordsiek[0] = *pVectorBegin->pValue;

		for (ptrdiff_t k = 0; k < sc.q; k++)
		{
			for (ptrdiff_t j = sc.q; j >= k + 1; j--)
			{
				pVectorBegin->Nordsiek[j - 1] += pVectorBegin->Nordsiek[j];
			}
		}

		// ���������� �������� ���������� ��������� ��������� �� Nordsieck
		*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
		pVectorBegin->Error = 0.0;	// �������� ������ ����
		pVectorBegin++;
	}

	ConvergenceTest::ProcessRange(ConvTest, ConvergenceTest::ResetIterations);

	// ��� ���������, ������� ������� ���������� ��������� ��������
	// (�������� ��� �����, ������� ����� ��������� ������� ��������� ���������� � �������������)
	// ������ ���� � ������� ������� �������� ����������
	for (DEVICECONTAINERITR it = m_DeviceContainersPredict.begin(); it != m_DeviceContainersPredict.end(); it++)
	{
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
		{
			(*dit)->Predict();
		}
	}

}

void CDynaModel::InitDevicesNordsiek()
{
	ChangeOrder(1);
	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
	{
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
		{
			(*dit)->InitNordsiek(this);
		}
	}
}

void CDynaModel::InitNordsiek()
{
	InitDevicesNordsiek();

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	while (pVectorBegin < pVectorEnd)
	{
		InitNordsiekElement(pVectorBegin,GetAtol(),GetRtol());
		pVectorBegin++;
	}

	sc.StepChanged();
	sc.OrderChanged();
	sc.m_bNordsiekSaved = false;
}

void CDynaModel::ResetNordsiek()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	if (sc.m_bNordsiekSaved)
	{
		while (pVectorBegin < pVectorEnd)
		{
			pVectorBegin->Tminus2Value = pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0] = *pVectorBegin->pValue;
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[1] = 0.0;
			pVectorBegin->SavedError = 0.0;
			pVectorBegin++;
		}
		BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
}

// ���������� Nordsieck ����� ����, ��� ���������� ��� ������� �������
// ���������
void CDynaModel::ReInitializeNordsiek()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	if (sc.m_bNordsiekSaved)
	{
		// ���� ���� ����������� ���
		while (pVectorBegin < pVectorEnd)
		{
			// �������� ���������������
			pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0];
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			// � ������� ������� ������� ������������ �� �������� ����� ���������� � ����-���������� ����������
			// �.�. ������ ������� ��� hy', � y' = (y(-1) - y(-2)) / h. Note - �� ����� ����������� � ����-����������� ����

			//											y(-1)						y(-2)
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[0] - pVectorBegin->Tminus2Value;
			pVectorBegin->SavedError = 0.0;
			pVectorBegin++;
		}
		// ������������� Nordsieck �� �������� ���
		RescaleNordsiek(sc.m_dCurrentH / sc.m_dOldH);
		BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
	sc.ResetStepsToFail();
}

// �������������� Nordsieck � ����������� ����
void CDynaModel::RestoreNordsiek()
{

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	if (sc.m_bNordsiekSaved)
	{
		// ���� ���� ������ ��� �������������� - ������ �������� ���������� ���
		// � �������
		while (pVectorBegin < pVectorEnd)
		{
			double *pN = pVectorBegin->Nordsiek;
			double *pS = pVectorBegin->SavedNordsiek;
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;
			*pN = *pS; pS++; pN++;

			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			pVectorBegin->Error = pVectorBegin->SavedError;

			pVectorBegin++;
		}
	}
	else
	{
		// ���� ������ ��� �������������� ���
		// ������� ��� ���������� �������
		// ��� ������ �������, �� ����� ��� ������
		while (pVectorBegin < pVectorEnd)
		{
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			pVectorBegin->Nordsiek[1] = pVectorBegin->Nordsiek[2] = 0.0;
			pVectorBegin->Error = 0.0;

			pVectorBegin++;
		}
	}

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
			(*dit)->RestoreStates();
}

bool CDynaModel::DetectAdamsRinging()
{
	bool bRes = false;

	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	if (sc.q == 2 && sc.m_dCurrentH > 0.01 && sc.m_dOldH > 0.0)
	{
		const double Methodl1[2] = { Methodl[sc.q - 1 + DET_ALGEBRAIC * 2][1],  Methodl[sc.q - 1 + DET_DIFFERENTIAL * 2][1] };

		while (pVectorBegin < pVectorEnd)
		{
			double newValue = pVectorBegin->Nordsiek[1] + pVectorBegin->Error * Methodl1[pVectorBegin->EquationType];

			// ���� ���� ����������� ��������� - ����������� ������� ������
			if (std::signbit(newValue) != std::signbit(pVectorBegin->SavedNordsiek[1]) && fabs(newValue) > pVectorBegin->Atol * sc.m_dCurrentH * 5.0)
				pVectorBegin->nRingsCount++;
			else
				pVectorBegin->nRingsCount = 0;

			// ���� ������� ������ ��������� ����� ������ ������
			if (pVectorBegin->nRingsCount > m_Parameters.m_nAdamsIndividualSuppressionCycles)
			{
				pVectorBegin->nRingsCount = 0;
				bRes = true;
				switch (m_Parameters.m_eAdamsRingingSuppressionMode)
				{
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
						if (pVectorBegin->EquationType == DET_DIFFERENTIAL)
						{
							// � RightVector ����������������������� �����, �� ���������� ������� ����������� ������ ����� ���������� 
							// �� �����������  BDF
							pVectorBegin->nRingsSuppress = m_Parameters.m_nAdamsIndividualSuppressStepsRange;
						}
						break;
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA:
						pVectorBegin->nRingsSuppress = m_Parameters.m_nAdamsIndividualSuppressStepsRange;
						break;
				}
				
				if (pVectorBegin->nRingsSuppress)
				{
					Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Ringing %s %s last values %g %g %d"),
						pVectorBegin->pDevice->GetVerbalName(),
						pVectorBegin->pDevice->VariableNameByPtr(pVectorBegin->pValue),
						newValue,
						pVectorBegin->SavedNordsiek[0],
						pVectorBegin->nRingsSuppress);
				}
			}

//			if (m_Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA)
//				break;
			pVectorBegin++;
		}

		if (bRes)
			sc.nNoRingingSteps = 0;
		else
			sc.nNoRingingSteps++;

		if (m_Parameters.m_eAdamsRingingSuppressionMode == ADAMS_RINGING_SUPPRESSION_MODE::ARSM_DAMPALPHA)
			if(bRes)
				EnableAdamsCoefficientDamping(true);
			else
				if(sc.nNoRingingSteps > m_Parameters.m_nAdamsDampingSteps)
					EnableAdamsCoefficientDamping(false);
	}
	return true;
}

// ����������� Nordsieck ����� ���������� ����
void CDynaModel::UpdateNordsiek(bool bAllowSuppression)
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	double alpha = sc.m_dCurrentH / sc.m_dOldH > 0.0 ? sc.m_dOldH : 1.0;
	double alphasq = alpha * alpha;
	double alpha1 = (1.0 + alpha);
	double alpha2 = (1.0 + 2.0 * alpha);
	bool bSuprressRinging = false;

	// ����� ���������� �������� ���������� ���� ������� ������ 2
	// ��� ��������� 0.01 � UpdateNordsieck ������ ��� �������� � ����������
	// ���� ����� ��������� ���������� �����������
	if (sc.q == 2 && bAllowSuppression && sc.m_dCurrentH > 0.01 && sc.m_dOldH > 0.0)
	{
		switch (m_Parameters.m_eAdamsRingingSuppressionMode)
		{
		case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL:
			// � ���������� ������ ���������� ��������� ���������� �� ������ ������� m_nAdamsGlobalSuppressionStep ����
			if (sc.nStepsCount % m_Parameters.m_nAdamsGlobalSuppressionStep == 0 && sc.nStepsCount > m_Parameters.m_nAdamsGlobalSuppressionStep)
				bSuprressRinging = true;
			break;
		case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
			// � �������������� ������ ��������� ������ ���������� �� ������� ��� ����������� �� ������ ����
			bSuprressRinging = true;
			break;
		}
	}

	// ���������� �� [Lsode 2.76]

	// ������ ��������� ����� ������������� ������ ��� �������� �������
	double LocalMethodl[2][3];
	std::copy(&Methodl[sc.q - 1][0], &Methodl[sc.q - 1][3], &LocalMethodl[0][0]);
	std::copy(&Methodl[sc.q + 1][0], &Methodl[sc.q + 1][3], &LocalMethodl[1][0]);

	while (pVectorBegin < pVectorEnd)
	{
		// �������� ����������� ������ �� ���� ��������� EquationType
		const double *lm = LocalMethodl[pVectorBegin->EquationType];

		double dError = pVectorBegin->Error;
		pVectorBegin->Nordsiek[0] += dError * *lm;	lm++;
		pVectorBegin->Nordsiek[1] += dError * *lm;	lm++;
		pVectorBegin->Nordsiek[2] += dError * *lm;	

		// ���������� ��������
		if (bSuprressRinging)
		{
			if (pVectorBegin->EquationType == DET_DIFFERENTIAL)
			{
				// ������� ��������� ������ ��� ������� (���� ������ �������� BDF ���� ���������� ���������� � ARSM_NONE)
				switch (m_Parameters.m_eAdamsRingingSuppressionMode)
				{
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_INDIVIDUAL:
					{
						/*
						if (pVectorBegin->nRingsSuppress == 0)
						{
							// ���� ���� ���������� ��������� - ����������� ������� ������
							if (std::signbit(*pVectorBegin->pValue) != std::signbit(pVectorBegin->SavedNordsiek[0]))
								pVectorBegin->nRingsCount++;

							if (pVectorBegin->nRingsCount > m_Parameters.m_nAdamsIndividualSuppressionCycles)
							{
								// ���� ������� ������ ��������� ����� ������ ������ - � RightVector �������������
								// ���������� �����, �� ���������� ������� ����������� ������ ����� ���������� 
								// �� �����������  BDF
								pVectorBegin->nRingsSuppress = m_Parameters.m_nAdamsIndividualSuppressStepsRange;
								Log(CDFW2Messages::DFW2MessageStatus::DFW2LOG_INFO, _T("Ringing %s %s"), pVectorBegin->pDevice->GetVerbalName(), pVectorBegin->pDevice->VariableNameByPtr(pVectorBegin->pValue));
							}
						}
						*/

						if (pVectorBegin->nRingsSuppress > 0)
						{
							// ��� ����������, � ������� ���������� ����� ��� ������ ������ �� BDF �� ���������
							// ������ ��� ������ � ��������� �������
							pVectorBegin->Nordsiek[1] = (alphasq * pVectorBegin->Tminus2Value - alpha1 * alpha1 * pVectorBegin->SavedNordsiek[0] + alpha2 * pVectorBegin->Nordsiek[0]) / alpha1;
							pVectorBegin->nRingsSuppress--;
							pVectorBegin->nRingsCount = 0;
						}
					}
					break;
					case ADAMS_RINGING_SUPPRESSION_MODE::ARSM_GLOBAL:
					{
						// � ���������� ������ ������ �������� ����������� ������ �� ����������� BDF
						pVectorBegin->Nordsiek[1] = (alphasq * pVectorBegin->Tminus2Value - alpha1 * alpha1 * pVectorBegin->SavedNordsiek[0] + alpha2 * pVectorBegin->Nordsiek[0]) / alpha1;
					}
					break;
				}
			}
		}

		// ��������� ����-���������� �������� ���������� ���������
		pVectorBegin->Tminus2Value = pVectorBegin->SavedNordsiek[0];

		// � ��������� ������� �������� ���������� ��������� ����� ����� �����
		pVectorBegin->SavedNordsiek[0] = pVectorBegin->Nordsiek[0];
		pVectorBegin->SavedNordsiek[1] = pVectorBegin->Nordsiek[1];
		pVectorBegin->SavedNordsiek[2] = pVectorBegin->Nordsiek[2];

		pVectorBegin->SavedError = dError;

		pVectorBegin++;
	}

	sc.m_dOldH = sc.m_dCurrentH;
	sc.m_bNordsiekSaved = true;

	for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
			(*dit)->StoreStates();


	// ���� ���������� ��� ��������� �������� � ���, ��� ������ ������
	// ������� �������
	m_Discontinuities.PassTime(GetCurrentTime());
	sc.ResetStepsToFail();
}

// ���������� ����� Nordsieck ����� ����������� ����
void CDynaModel::SaveNordsiek()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	while (pVectorBegin < pVectorEnd)
	{
		double *pN = pVectorBegin->Nordsiek;
		double *pS = pVectorBegin->SavedNordsiek;

		// ��������� ����-���������� �������� ���������� ���������
		pVectorBegin->Tminus2Value = *pS;

		// ���������� ��� ��������, �� ������ �� ������� �������
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;

		pVectorBegin->SavedError = pVectorBegin->Error;
		pVectorBegin++;
	}
	sc.m_dOldH = sc.m_dCurrentH;
	sc.m_bNordsiekSaved = true;
}

// ��������������� Nordsieck �� �������� ����������� ��������� ����
void CDynaModel::RescaleNordsiek(double r)
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;
	
	// ������ ����������� ����� ��������� �������� Nordsieck �� ������������ ������� C[q+1;q+1]
	// � ���������� C[i,i] = r^(i-1) [Lsode 2.64]
	while (pVectorBegin < pVectorEnd)
	{
		double R = 1.0;
		for (ptrdiff_t j = 1; j < sc.q + 1; j++)
		{
			R *= r;
			pVectorBegin->Nordsiek[j] *= R;
		}
		pVectorBegin++;
	}

	// �������� ������� ��������� ��������� ���� � �������
	// ���� ��� ��������� ���������� ���������� ���� �� ����������
	// ��������� ���������� �����
	sc.StepChanged();
	sc.OrderChanged();

	// ������������ ����������� ��������� ����
	double dRefactorRatio = sc.m_dCurrentH / sc.m_dLastRefactorH;
	// ���� ��� ��������� ����� � �������� ���������� ��� - ������� ���� �������������� �����
	// sc.m_dLastRefactorH ����������� ����� ��������������
	if (dRefactorRatio > m_Parameters.m_dRefactorByHRatio || dRefactorRatio < 1.0 / m_Parameters.m_dRefactorByHRatio)
		sc.RefactorMatrix(true);
}

void CDynaModel::ConstructNordsiekOrder()
{
	struct RightVector *pVectorBegin = pRightVector;
	struct RightVector *pVectorEnd = pRightVector + m_nMatrixSize;

	while (pVectorBegin < pVectorEnd)
	{
		pVectorBegin->Nordsiek[2] = pVectorBegin->Error / 2.0;
		pVectorBegin++;
	}
}


void CDynaModel::InitNordsiekElement(struct RightVector *pVectorBegin, double Atol, double Rtol)
{
	ZeroMemory(pVectorBegin->Nordsiek + 1, sizeof(double) * 2);
	ZeroMemory(pVectorBegin->SavedNordsiek, sizeof(double) * 3);
	pVectorBegin->EquationType = DET_ALGEBRAIC;
	pVectorBegin->Atol = Atol;
	pVectorBegin->Rtol = Rtol;
	pVectorBegin->SavedError = pVectorBegin->Tminus2Value = 0.0;
	pVectorBegin->nErrorHits = 0;
	pVectorBegin->nRingsCount = 0;
	pVectorBegin->nRingsSuppress = 0;
}

void CDynaModel::PrepareNordsiekElement(struct RightVector *pVectorBegin)
{
	pVectorBegin->Error = 0.0;
	pVectorBegin->PhysicalEquationType = DET_ALGEBRAIC;
}