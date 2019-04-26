#include "stdafx.h"
#include "DerlagContinuous.h"
#include "DynaModel.h"
using namespace DFW2;

bool CDerlagContinuous::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double T = m_T;

	if (pDynaModel->EstimateBuild())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_OutputEquationIndex));
		pRightVector->PrimitiveBlock = PBT_DERLAG;
	}

	if (m_pDevice->IsStateOn())
	{
		//if (!pDynaModel->IsInDiscontinuityMode())
		{
			// dOutput/ dOutput
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
			// dOutput / dY2
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex + 1), m_T * m_K);
			// dOutput / dInput
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), -m_K * m_T);
		}
		//else
		//{
		//	// dOutput / dOutput
		//	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
		//	// dOutput / dY2
		//	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex + 1), 0.0);
		//	// dOutput / dInput
		//	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), 0.0);
		//}
	}
	else
	{
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex + 1), 0.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), 0.0);
		T = 0.0;
	}

	// dY2 / dY2
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0 + hb0 * m_T);
	pDynaModel->SetElement2(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), -T);
	// dY2 / dInput
	//pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_Input->Index()), -hb0 * m_T);
	pDynaModel->SetElement2(A(m_OutputEquationIndex + 1), A(m_Input->Index()), -T);

	return bRes && pDynaModel->Status();
}

bool CDerlagContinuous::BuildRightHand(CDynaModel *pDynaModel)
{

	if (m_pDevice->GetId() == 108701 && m_OutputEquationIndex == 9)
		m_pDevice->GetId();

	if (m_pDevice->IsStateOn())
	{
		double Input = m_Input->Value();
		double dY2 = (Input - *m_Y2) * m_T;
		double dOut = *m_Output + m_K * m_T * (*m_Y2 - Input);

		/*if (pDynaModel->IsInDiscontinuityMode())
		{
			dOut = 0.0;
			*m_Output = m_K * m_T * (*m_Y2 - Input);
		}*/
	
		pDynaModel->SetFunction(A(m_OutputEquationIndex), dOut);
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex + 1), dY2);
	}
	else
	{
		pDynaModel->SetFunction(A(m_OutputEquationIndex), 0.0);
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex + 1), 0.0);
	}

	return pDynaModel->Status();
}

bool CDerlagContinuous::Init(CDynaModel *pDynaModel)
{
	if (m_pDevice->GetId() == 102401 && m_OutputEquationIndex == 9)
		m_pDevice->GetId();

	bool bRes = true;

	if (Equal(m_K, 0.0))
		m_T = 0.0;
	else
	{
		_ASSERTE(!Equal(m_T, 0.0));
		m_T = 1.0 / m_T;
	}

	double Input = m_Input->Value();
	*m_Y2 = Input;
	*m_Output = 0.0;
	return bRes;
}


bool CDerlagContinuous::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_pDevice->GetId() == 108701 && m_OutputEquationIndex == 9)
		m_pDevice->GetId();

	if (m_pDevice->IsStateOn())
	{
		double Input = m_Input->Value();
		double dY2 = (Input - *m_Y2) * m_T;
		pDynaModel->SetDerivative(A(m_OutputEquationIndex + 1), dY2);
	}
	else
	{
		pDynaModel->SetDerivative(A(m_OutputEquationIndex + 1), 0.0);
	}

	return pDynaModel->Status();
}

eDEVICEFUNCTIONSTATUS CDerlagContinuous::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	
	if (m_pDevice->GetId() == 105701)
		m_pDevice->GetId();

	if (m_pDevice->IsStateOn())
	{
		double Input = m_Input->Value();
		if (Equal(m_K, 0.0))
			*m_Y2 = 0.0;
		else
		{
			// ���� �� ����, ���� ������ �� ��� ������ �� ������,
			// ��� �������� ��������� ��� �� �����

			*m_Y2 = Input - *m_Output / m_T / m_K;			// �������� ���� �� �����
			//*m_Output = m_K * m_T * (Input - *m_Y2);		// ����� �� �����
		}
	}
	return DFS_OK;
}

bool CDerlagContinuous::UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount)
{
	bool bRes = true;
	double TK[2] = { 1E-4, 1.0 };

	nParametersCount = min(nParametersCount, sizeof(TK) / sizeof(TK[0]));

	for (size_t i = 0; i < nParametersCount; i++)
		TK[i] = pParameters[i];
	SetTK(TK[0], TK[1]);
	return bRes;
}


// ----------------------------- Derlag Nordsieck --------------------------------------
bool CDerlagNordsieck::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;

	if (Equal(m_K, 0.0))
		m_T = 0.0;
	else
	{
		_ASSERTE(!Equal(m_T, 0.0));
		m_T = 1.0 / m_T;
	}
	*m_Output = 0.0;
	return bRes;
}

bool CDerlagNordsieck::BuildRightHand(CDynaModel *pDynaModel)
{

	if (m_pDevice->IsStateOn())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double hOld = pDynaModel->GetOldH();

		if (pDynaModel->IsInDiscontinuityMode())
			pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), 0.0);
		else
			pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), (pRightVector->SavedNordsiek[1] / hOld * m_K - *m_Output) * m_T);

		pDynaModel->SetFunction(A(m_OutputEquationIndex + 1), 0.0);
		pDynaModel->SetFunction(A(m_OutputEquationIndex + 2), 0.0);
	}
	else
	{
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex), 0.0);
		pDynaModel->SetFunctionDiff(A(m_OutputEquationIndex + 1), 0.0);
		pDynaModel->SetFunction(A(m_OutputEquationIndex + 2), 0.0);
	}

	return pDynaModel->Status();
}

bool CDerlagNordsieck::BuildDerivatives(CDynaModel *pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double hOld = pDynaModel->GetOldH();
		pDynaModel->SetDerivative(A(m_OutputEquationIndex), (pRightVector->SavedNordsiek[1] / hOld * m_K - *m_Output) * m_T);
	}
	else
	{
		pDynaModel->SetDerivative(A(m_OutputEquationIndex), 0.0);
	}

	return pDynaModel->Status();
}

bool CDerlagNordsieck::UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount)
{
	bool bRes = true;
	double TK[2] = { 1E-4, 1.0 };

	nParametersCount = min(nParametersCount, sizeof(TK) / sizeof(TK[0]));

	for (size_t i = 0; i < nParametersCount; i++)
		TK[i] = pParameters[i];
	SetTK(TK[0], TK[1]);
	return bRes;
}

bool CDerlagNordsieck::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double hb0 = pDynaModel->GetHB0();

	if (m_pDevice->IsStateOn())
	{

		RightVector *pRightVector = pDynaModel->GetRightVector(A(m_Input->Index()));
		double hOld = pDynaModel->GetOldH();


		
		if (!pDynaModel->IsInDiscontinuityMode())
		{
			// dOut / dOut
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0 + hb0 * m_T);
		}
		else
		{
			// dOut / dOut
			pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);

		}
		pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex + 2), A(m_OutputEquationIndex + 2), 1.0);
	}
	else
	{
		pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex + 1), A(m_OutputEquationIndex + 1), 1.0);
		pDynaModel->SetElement(A(m_OutputEquationIndex + 2), A(m_OutputEquationIndex + 2), 1.0);
		hb0 = 0.0;
	}

	return bRes && pDynaModel->Status();
}


