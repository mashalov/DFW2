#include "stdafx.h"
#include "Sum.h"
#include "DynaModel.h"

using namespace DFW2;

bool CSum::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = true;
	pDynaModel->SetElement(m_Output.Index, m_Input->Index(), m_K1);
	pDynaModel->SetElement(m_Output.Index, m_Input1->Index(), m_K2);
	pDynaModel->SetElement(m_Output, m_Output, -1.0);
	return true;
}


bool CSum::BuildRightHand(CDynaModel *pDynaModel)
{
	pDynaModel->SetFunction(m_Output, m_K1 * m_Input->Value() + m_K2 * m_Input1->Value() - m_Output);
	return true;
}


