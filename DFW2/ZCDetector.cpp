#include "stdafx.h"
#include "ZCDetector.h"
#include "DynaModel.h"

using namespace DFW2;


double CZCDetector::ChangeState(CDynaModel *pDynaModel)
{
	double rH = 1.0;
	if (!m_pDevice->IsStateOn())
		return rH;

	double dInput = m_Input->Value();
	// �������� ������� ������� ����������
	RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dHyst = pDynaModel->GetHysteresis(dInput);
	bool bReadyToChangeState = false;
	if (GetCurrentState() == RS_ON)
	{
		// ���� ���� ������������� �������� ����������
		// � ����� ������������� - �������� ���������� � ��������� ���������
		dHyst -= dHyst;
		if (dInput <= dHyst)
			bReadyToChangeState = true;
	}
	else
	{
		// ���� ���� ������������� �������� ����������
		// � ����� �������������- �������� ���������� � ��������� ���������
		if (dInput >= dHyst)
			bReadyToChangeState = true;
	}

	// ���������, ����� �� ���� ��������� ����� ������� ���������� �� ���� ��������������
	rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector, dHyst);

	if (pDynaModel->GetZeroCrossingInRange(rH))
	{
		if (bReadyToChangeState)
		{
			// ���� ���� ���������� � ��������� ��������� (���� ������� ���������� ���������)
			// ������������ ����������� ������� ����������
			// ����������� ���������� �� ���� � ���������� �� ����� �� ����������,
			// ������� ��������� �� ����� � GetWeightedError ����������
			double derr = fabs(pRightVector->GetWeightedError(dInput + dHyst, dInput + dHyst));
			// ���� ����������� ������ ��� �������� ����� ��� �������������
			if (derr < pDynaModel->GetZeroCrossingTolerance())
			{
				// �������� ���������
				InvertState(pDynaModel);
				rH = 1.0;
			}
			else
			{
				if (pDynaModel->ZeroCrossingStepReached(rH))
					InvertState(pDynaModel);
			}
		}
		// ���� ��������� ����� �� ���� - �� �������� ���������, ���������� ��� �� �������������
	}
	else if (bReadyToChangeState)
	{
		// ���� ����������� �� ���� �������, �� ���� ���������, ������ ���������
		InvertState(pDynaModel);
		rH = 1.0;
		_ASSERTE(0); // ����� ���, �� ���� ��������� !
	}
	else
		rH = 1.0;

	return rH;
}

double CZCDetector::OnStateOff(CDynaModel *pDynaModel)
{
	return ChangeState(pDynaModel);
	/*
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dValue = m_Input->Value();
	double dHyst = pDynaModel->GetHysteresis(dValue);

	if (dValue >= dHyst)
	{
		double derr = fabs(pRightVector1->GetWeightedError(dValue, dValue));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(pDynaModel, RS_ON);
		}
		else
		{
			rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector1, dHyst);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(pDynaModel, RS_ON);
		}
	}
	return rH;
	*/
}

double CZCDetector::OnStateOn(CDynaModel *pDynaModel)
{
	return ChangeState(pDynaModel);
	/*
	double rH = 1.0;
	RightVector *pRightVector1 = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dValue = m_Input->Value();
	double dHyst = pDynaModel->GetHysteresis(dValue);

	if (dValue <= 0)
	{
		double derr = fabs(pRightVector1->GetWeightedError(dValue, dValue));
		if (derr < pDynaModel->GetZeroCrossingTolerance())
		{
			SetCurrentState(pDynaModel, RS_OFF);
		}
		else
		{
			rH = CDynaPrimitiveLimited::FindZeroCrossingToConst(pDynaModel, pRightVector1, 0.0);
			if (pDynaModel->ZeroCrossingStepReached(rH))
				SetCurrentState(pDynaModel, RS_OFF);
		}
	}
	return rH;
	*/
}

bool CZCDetector::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	eCurrentState = RS_OFF;
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));

	return bRes;
}

eDEVICEFUNCTIONSTATUS CZCDetector::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		if (m_Input->Value() > 0)
			SetCurrentState(pDynaModel, RS_ON);
		else
			SetCurrentState(pDynaModel, RS_OFF);

		*m_Output = (eCurrentState == RS_ON) ? 1.0 : 0.0;
	}
	return DFS_OK;
}
