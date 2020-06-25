#include "stdafx.h"
#include "AndOrNot.h"
#include "DynaModel.h"

using namespace DFW2;

CAnd::CAnd(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) :
													CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input)
													{ 
														InitializeInputs({&m_Input, &m_Input2}, Input);
													}

COr::COr(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) :
													CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input)
													{ 
														InitializeInputs({&m_Input, &m_Input2}, Input);
													}

CNot::CNot(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) :
													CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input)
													{ }


bool CAnd::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	*m_Output = 0.0;
	ProcessDiscontinuity(pDynaModel);
	return bRes;
}

bool COr::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	*m_Output = 0.0;
	ProcessDiscontinuity(pDynaModel);
	return bRes;
}

bool CNot::Init(CDynaModel *pDynaModel)
{
	bool bRes = true;
	*m_Output = 0.0;
	ProcessDiscontinuity(pDynaModel);
	return bRes;
}


eDEVICEFUNCTIONSTATUS CAnd::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dOldOut = *m_Output;

		if (m_Input->Value() > 0 && m_Input2->Value() > 0)
			*m_Output = 1.0;
		else
			*m_Output = 0.0;

		if (dOldOut != *m_Output)
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS COr::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dOldOut = *m_Output;

		if (m_Input->Value() > 0 || m_Input2->Value() > 0)
			*m_Output = 1.0;
		else
			*m_Output = 0.0;

		if (dOldOut != *m_Output)
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}

eDEVICEFUNCTIONSTATUS CNot::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (m_pDevice->IsStateOn())
	{
		double dOldOut = *m_Output;

		if (m_Input->Value() > 0 )
			*m_Output = 0.0;
		else
			*m_Output = 1.0;

		if (dOldOut != *m_Output)
			pDynaModel->DiscontinuityRequest();
	}

	return eDEVICEFUNCTIONSTATUS::DFS_OK;
}