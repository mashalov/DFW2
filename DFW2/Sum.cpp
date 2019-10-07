#include "stdafx.h"
#include "Sum.h"
#include "DynaModel.h"

using namespace DFW2;

bool CSum::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input->Index()), m_K1);
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_Input1->Index()), m_K2);
	pDynaModel->SetElement(A(m_OutputEquationIndex), A(m_OutputEquationIndex), -1.0);
	return true;
}


bool CSum::BuildRightHand(CDynaModel *pDynaModel)
{
	pDynaModel->SetFunction(A(m_OutputEquationIndex), m_K1 * m_Input->Value() + m_K2 * m_Input1->Value() - *m_Output);
	return true;
}


