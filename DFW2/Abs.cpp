#include "stdafx.h"
#include "Abs.h"
#include "DynaModel.h"

using namespace DFW2;

CAbs::CAbs(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : 
									CDynaPrimitiveState(pDevice,pOutput,nOutputIndex,Input) 
									{}

bool CAbs::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;

	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), m_bPositive ? 1.0 : -1.0);
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);

	return bRes && pDynaModel->Status();
}

bool CAbs::BuildRightHand(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput = m_Input->Value();
		pDynaModel->SetFunction(A(m_OutputEquationIndex), *m_Output - (m_bPositive ? dInput : -dInput));
	}
	else
		pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);

	return pDynaModel->Status();
}



bool CAbs::Init(CDynaModel *pDynaModel)
{
	bool bRes = CDynaPrimitive::Init(pDynaModel);
	bRes = bRes && CDevice::IsFunctionStatusOK(ProcessDiscontinuity(pDynaModel));
	return bRes;
}

eDEVICEFUNCTIONSTATUS CAbs::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dInput = m_Input->Value();

		if (dInput >= 0)
			m_bPositive = true;
		else
			m_bPositive = false;

		*m_Output = fabs(dInput);
	}
	else
		*m_Output = 0.0;

	return DFS_OK;
}

// ��������� ������ ������

double CAbs::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	double rH = 1.0;

	if (!m_pDevice->IsStateOn())
		return rH;

	double dInput = m_Input->Value();
	// �������� ������� ������� ����������
	RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
	double dHyst = pDynaModel->GetHysteresis(dInput);

	bool bReadyToChangeState = false;
	if (m_bPositive)
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
				// �������� ��������� � ����������� ��������� �������
				m_bPositive = !m_bPositive;
				pDynaModel->DiscontinuityRequest();
				rH = 1.0;
			}
			else
			{
				if (pDynaModel->ZeroCrossingStepReached(rH))
				{
					// ����� � ����: ��������� �������� ������ ���� ����� ����� ������������� 
					// � �������� ������������
					m_bPositive = !m_bPositive;
					pDynaModel->DiscontinuityRequest();
				}
			}
		}
		// ���� ��������� ����� �� ���� - �� �������� ���������, ���������� ��� �� �������������
	}
	else if (bReadyToChangeState)
	{
		// ���� ����������� �� ���� �������, �� ���� ���������, ������ ���������
		m_bPositive = !m_bPositive;
		pDynaModel->DiscontinuityRequest();
		rH = 1.0;
		_ASSERTE(0); // ����� ���, �� ���� ��������� !
	}
	else
		rH = 1.0;

	return rH;
}