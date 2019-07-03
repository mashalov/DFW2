#include "stdafx.h"
#include "CustomDevice.h"
#include "DynaModel.h"

using namespace DFW2;

CCustomDevice::CCustomDevice() : m_pPrimitiveVars(NULL),
								 m_pPrimitiveExtVars(NULL)	
{
	m_DLLArgs.pFnSetElement						= &DLLEntrySetElement;
	m_DLLArgs.pFnSetFunction					= &DLLEntrySetFunction;
	m_DLLArgs.pFnSetFunctionDiff				= &DLLEntrySetFunctionDiff;
	m_DLLArgs.pFnSetDerivative					= &DLLEntrySetDerivative;
	m_DLLArgs.pFnProcessBlockDiscontinuity		= &DLLProcessBlockDiscontinuity;
	m_DLLArgs.pFnInitBlock						= &DLLInitBlock;
}


CCustomDevice::~CCustomDevice()
{
}

bool CCustomDevice::BuildStructure()
{
	// allocate block of double variables from the pool

	bool bRes = true;

	m_pConstVars = Container()->NewDoubleVariables();
	m_pSetPoints = m_pConstVars + Container()->GetConstsCount();
	m_pVars = m_pSetPoints + Container()->GetSetPointsCount();
	m_pExternals = Container()->NewExternalVariables();
	m_pPrimitiveExtVars = Container()->NewPrimitiveExtVariables();

	PrimitiveVariableExternal *pExtVarsEnd = m_pPrimitiveExtVars + Container()->GetInputsCount();
	for (PrimitiveVariableExternal *pStart = m_pPrimitiveExtVars; pStart < pExtVarsEnd; pStart++)
		pStart->IndexAndValue(pStart - m_pPrimitiveExtVars, NULL);

	const BLOCKDESCRIPTIONS&  Blocks = Container()->DLL().GetBlocksDescriptions();
	const BLOCKSPINSINDEXES&  Pins = Container()->DLL().GetBlocksPinsIndexes();

	_ASSERTE(Blocks.size() == Pins.size());

	BLOCKDESCRIPTIONS::const_iterator bit = Blocks.begin();
	BLOCKSPINSINDEXES::const_iterator iit = Pins.begin();

#define MAX_BLOCKVARIABLES 1000

	PrimitiveVariableBase *pPrimsPtrs[MAX_BLOCKVARIABLES];

	for (; bit != Blocks.end(); bit++, iit++)
	{
		PrimitiveVariable				*pBlockBegin = NULL;

		PrimitiveVariableBase **pBlockPrims = pPrimsPtrs;

		// for all inputs create one PrimitiveVariable from the pool and assign it to 
		// given input index and double variable at the same index

		for (BLOCKPININDEXVECTOR::const_iterator nit = iit->begin(); nit != iit->end(); nit++)
		{
			switch (nit->Location)
			{
			case eVL_INTERNAL:
				//_ASSERTE(nit->nIndex < Container()->EquationsCount());
				*pBlockPrims = Container()->NewPrimitiveVariable(nit->nIndex, m_pVars[nit->nIndex]);
				if (!pBlockBegin)
					pBlockBegin = static_cast<PrimitiveVariable*>(*pBlockPrims);
				break;
			case eVL_EXTERNAL:
				*pBlockPrims = m_pPrimitiveExtVars + nit->nIndex;
				break;
			}
			pBlockPrims++;
		}

		// assign PrimitiveVars block pointer to device
		// it is not quite necessary, but useful for debugging

		if (!m_pPrimitiveVars)
			m_pPrimitiveVars = pBlockBegin;

		CDynaPrimitive *pDummy = NULL;
		size_t nVarsCount = pBlockPrims - pPrimsPtrs;

		//_ASSERTE(nVarsCount == Container()->PrimitiveEquationsCount(bit->eType));

		switch (bit->eType)
		{
		case PBT_LAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_LIMITEDLAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_DERLAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CDerlag(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_INTEGRATOR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_LIMITEDINTEGRATOR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_LIMITERCONST:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimiterConst(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_RELAY:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelay(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_RELAYDELAY:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayDelay(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_RELAYDELAYLOGIC:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayDelayLogic(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_RSTRIGGER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRSTrigger(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1), *(pPrimsPtrs + 2), true);
			break;
		case PBT_HIGHER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CComparator(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1), *(pPrimsPtrs + 2));
			break;
		case PBT_LOWER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CComparator(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 2), *(pPrimsPtrs + 1));
			break;
		case PBT_BIT:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CZCDetector(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_AND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CAnd(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1), *(pPrimsPtrs + 2));
			break;
		case PBT_OR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) COr(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1), *(pPrimsPtrs + 2));
			break;
		case PBT_NOT:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CNot(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_ABS:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CAbs(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_DEADBAND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CDeadBand(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_EXPAND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CExpand(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		case PBT_SHRINK:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CShrink(this, m_pVars + (*pPrimsPtrs)->Index(), (*pPrimsPtrs)->Index(), *(pPrimsPtrs + 1));
			break;
		}

		/*

		ѕримитивы теперь добавл€ютс€ автоматически
		if (pDummy)
			m_Primitives.push_back(pDummy);
		*/
	}

	if (!bRes)
		Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszDLLBadBlocks, Container()->DLL().GetModuleFilePath()));

	return bRes;
}


bool CCustomDevice::SetConstValue(size_t nIndex, double dValue)
{
	bool bRes = false;

	if (GetId() == 102401)
		GetId();


	if (nIndex >= 0 && nIndex < Container()->GetConstsCount())
	{
		const ConstVarsInfo *pConstInfo = Container()->DLL().GetConstInfo(nIndex);

		if (pConstInfo)
		{
			if (pConstInfo->VarFlags & CVF_ZEROTODEFAULT)
			{
				dValue = pConstInfo->dDefault;
			}

			if (pConstInfo->dMax > pConstInfo->dMin)
			{
				if (dValue > pConstInfo->dMax) dValue = pConstInfo->dDefault;
				if (dValue < pConstInfo->dMin) dValue = pConstInfo->dDefault;
			}

			m_pConstVars[nIndex] = dValue;
			bRes = true;
		}
	}
	return bRes;
}

bool CCustomDevice::SetConstDefaultValues()
{
	bool bRes = true;
	size_t nConstsCount = Container()->GetConstsCount();

	for (size_t nIndex = 0; nIndex < nConstsCount; nIndex++)
	{
		const ConstVarsInfo *pConstInfo = Container()->DLL().GetConstInfo(nIndex);
		if (pConstInfo)
		{
			m_pConstVars[nIndex] = pConstInfo->dDefault;
			bRes = true;
		}
	}
	return bRes;
}

eDEVICEFUNCTIONSTATUS CCustomDevice::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_OK;
	bool bRes = true;

	size_t nConstsCount = Container()->DLL().GetConstsCount();
	for (size_t nConstIndex = 0; nConstIndex < nConstsCount && bRes; nConstIndex++)
	{
		const ConstVarsInfo *pConstInfo = Container()->DLL().GetConstInfo(nConstIndex);
		if (pConstInfo)
		{
			if (pConstInfo->eDeviceType != DEVTYPE_UNKNOWN && !(pConstInfo->VarFlags & CVF_INTERNALCONST))
				bRes = bRes && InitConstantVariable(m_pConstVars[nConstIndex], this, pConstInfo->VarInfo.Name, static_cast<eDFW2DEVICETYPE>(pConstInfo->eDeviceType));
		}
		else
			bRes = false;
	}

	bRes = bRes && ConstructDLLParameters(pDynaModel);
	m_ExternalStatus = DFS_OK;
	bRes = bRes && Container()->InitDLLEquations(&m_DLLArgs);

	if (!bRes)
		m_ExternalStatus = DFS_FAILED;

	return m_ExternalStatus;
}

bool CCustomDevice::BuildEquations(CDynaModel *pDynaModel)
{
	bool bRes = ConstructDLLParameters(pDynaModel);
	bRes = Container()->BuildDLLEquations(&m_DLLArgs) && bRes;

	bRes = bRes && CDevice::BuildEquations(pDynaModel);

	/*for (PRIMITIVEBLOCKITR it = m_Primitives.begin(); it != m_Primitives.end(); it++)
		bRes = (*it)->BuildEquations(pDynaModel) && bRes;*/

	return bRes;
}

bool CCustomDevice::BuildRightHand(CDynaModel *pDynaModel)
{
	bool bRes = ConstructDLLParameters(pDynaModel);
	bRes = Container()->BuildDLLRightHand(&m_DLLArgs) && bRes;

	bRes = bRes && CDevice::BuildRightHand(pDynaModel);

	/*for (PRIMITIVEBLOCKITR it = m_Primitives.begin(); it != m_Primitives.end(); it++)
		bRes = (*it)->BuildRightHand(pDynaModel) && bRes;*/

	return bRes;
}

bool CCustomDevice::ConstructDLLParameters(CDynaModel *pDynaModel)
{
	m_DLLArgs.pEquations = m_pVars;
	m_DLLArgs.pConsts    = m_pConstVars;
	m_DLLArgs.pExternals = m_pExternals;
	m_DLLArgs.pSetPoints = m_pSetPoints;
	m_DLLArgs.BuildObjects.pModel = pDynaModel;
	m_DLLArgs.BuildObjects.pDevice = this;
	m_DLLArgs.Omega = pDynaModel->GetOmega0();
	m_DLLArgs.t = pDynaModel->GetCurrentTime();
	return true;
}

bool CCustomDevice::BuildDerivatives(CDynaModel *pDynaModel)
{
	bool bRes = ConstructDLLParameters(pDynaModel);
	bRes = Container()->BuildDLLDerivatives(&m_DLLArgs) && bRes;

	
	bRes = bRes && CDevice::BuildDerivatives(pDynaModel);

	/*for (PRIMITIVEBLOCKITR it = m_Primitives.begin(); it != m_Primitives.end(); it++)
		bRes = (*it)->BuildDerivatives(pDynaModel) && bRes;*/

	return bRes;
}

double CCustomDevice::CheckZeroCrossing(CDynaModel *pDynaModel)
{
	bool bRes = true;
	double rH = CDevice::CheckZeroCrossing(pDynaModel);

	/*for (PRIMITIVEBLOCKITR it = m_Primitives.begin(); it != m_Primitives.end(); it++)
	{
		double rHcurrent = (*it)->CheckZeroCrossing(pDynaModel);
		if (rHcurrent < rH)
			rH = rHcurrent;
	}*/

	return rH;
}

eDEVICEFUNCTIONSTATUS CCustomDevice::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	m_ExternalStatus = DFS_NOTREADY;
	
	if (IsStateOn())
	{
		m_ExternalStatus = DFS_OK;
		bool bRes = ConstructDLLParameters(pDynaModel);
		Container()->ProcessDLLDiscontinuity(&m_DLLArgs);
	}
	else
		m_ExternalStatus = DFS_OK;

	return m_ExternalStatus;
}


long CCustomDevice::DLLEntrySetElement(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, ptrdiff_t nCol, double dValue, int bAddToPreviois)
{
	CDynaModel *pDynaModel = static_cast<CDynaModel*>(pBEObjs->pModel);
	CDevice    *pDevice    = static_cast<CDevice*>(pBEObjs->pDevice);
	_ASSERTE(nCol != DFW2_NON_STATE_INDEX);
	pDynaModel->SetElement(pDevice->A(nRow), pDevice->A(nCol), dValue, bAddToPreviois ? true : false );
	return 1;
}

long CCustomDevice::DLLEntrySetFunction(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, double dValue)
{
	CDynaModel *pDynaModel = static_cast<CDynaModel*>(pBEObjs->pModel);
	CDevice    *pDevice = static_cast<CDevice*>(pBEObjs->pDevice);
	pDynaModel->SetFunction(pDevice->A(nRow), dValue);
	return 1;
}

long CCustomDevice::DLLEntrySetFunctionDiff(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, double dValue)
{
	CDynaModel *pDynaModel = static_cast<CDynaModel*>(pBEObjs->pModel);
	CDevice    *pDevice = static_cast<CDevice*>(pBEObjs->pDevice);
	pDynaModel->SetFunctionDiff(pDevice->A(nRow), dValue);
	return 1;
}

long CCustomDevice::DLLEntrySetDerivative(BuildEquationsObjects *pBEObjs, ptrdiff_t nRow, double dValue)
{
	CDynaModel *pDynaModel = static_cast<CDynaModel*>(pBEObjs->pModel);
	CDevice    *pDevice = static_cast<CDevice*>(pBEObjs->pDevice);
	pDynaModel->SetDerivative(pDevice->A(nRow), dValue);
	return 1;
}

long CCustomDevice::DLLProcessBlockDiscontinuity(BuildEquationsObjects *pBEObjs, long nBlockIndex)
{
	
	CCustomDevice *pDevice = static_cast<CCustomDevice*>(pBEObjs->pDevice);
	CDynaModel *pDynaModel = static_cast<CDynaModel*>(pBEObjs->pModel);
	if (nBlockIndex >= 0 && nBlockIndex < static_cast<ptrdiff_t>(pDevice->m_Primitives.size()))
	{
		switch (pDevice->m_Primitives[nBlockIndex]->ProcessDiscontinuity(pDynaModel))
		{
		case eDEVICEFUNCTIONSTATUS::DFS_FAILED:
			pDevice->m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
			break;
		case eDEVICEFUNCTIONSTATUS::DFS_NOTREADY:
			pDevice->m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
			break;
		}
	}
	return 1;
}


long CCustomDevice::DLLInitBlock(BuildEquationsObjects *pBEObjs, long nBlockIndex)
{

	CCustomDevice *pDevice = static_cast<CCustomDevice*>(pBEObjs->pDevice);
	CDynaModel *pDynaModel = static_cast<CDynaModel*>(pBEObjs->pModel);

	if (nBlockIndex >= 0 && nBlockIndex < static_cast<ptrdiff_t>(pDevice->m_Primitives.size()))
	{
		CDynaPrimitive *pPrimitive = pDevice->m_Primitives[nBlockIndex];
		double *pParBuffer = NULL;
		long nParCount = static_cast<CCustomDeviceContainer*>(pDevice->Container())->GetParametersValues(pDevice->GetId(), &pDevice->m_DLLArgs, nBlockIndex, &pParBuffer);
		if (!pPrimitive->UnserializeParameters(pDynaModel, pParBuffer, nParCount))
			pDevice->m_ExternalStatus = DFS_FAILED;
		else
			if (!pPrimitive->Init(pDynaModel))
				pDevice->m_ExternalStatus = DFS_FAILED;
	}

	return 1;
}


eDEVICEFUNCTIONSTATUS CCustomDevice::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes = DFS_OK;

		
	PrimitiveVariableExternal *pExtVarsEnd = m_pPrimitiveExtVars + Container()->GetInputsCount();

	for (PrimitiveVariableExternal *pStart = m_pPrimitiveExtVars; pStart < pExtVarsEnd; pStart++)
	{
		ptrdiff_t nExternalsIndex = pStart - m_pPrimitiveExtVars;
		const InputVarsInfo *pInputInfo = Container()->DLL().GetInputInfo(nExternalsIndex);
		if (pInputInfo)
		{
			eRes = DeviceFunctionResult(eRes, InitExternalVariable(*pStart, this, pInputInfo->VarInfo.Name,static_cast<eDFW2DEVICETYPE>(pInputInfo->eDeviceType)));
			// create contiguous copy of primitive external variables to send to dll
			if (eRes != DFS_FAILED)
			{
				if (nExternalsIndex >= 0 && nExternalsIndex < static_cast<ptrdiff_t>(Container()->GetInputsCount()))
				{
					m_pExternals[nExternalsIndex].pValue = &pStart->Value();
					m_pExternals[nExternalsIndex].nIndex = pStart->Index();
				}
				else
					eRes = DFS_FAILED;
			}
		}
		else
			eRes = DFS_FAILED;
	}

	return eRes;
}


double* CCustomDevice::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;

	if (nVarIndex >= 0)
	{
		if (nVarIndex < static_cast<ptrdiff_t>(Container()->GetConstsCount()))
			p = m_pConstVars + nVarIndex;
		else
			if (nVarIndex < static_cast<ptrdiff_t>(Container()->GetSetPointsCount()))
				p = m_pSetPoints + nVarIndex - Container()->GetConstsCount();
	}

	_ASSERTE(p);
	return p;

}

double* CCustomDevice::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p = NULL;

	if (nVarIndex >= 0 && nVarIndex < m_pContainer->EquationsCount())
		p = m_pVars + nVarIndex;

	_ASSERTE(p);
	return p;
}


CDynaPrimitive* CCustomDevice::GetPrimitiveForNamedOutput(const _TCHAR* cszOutputName)
{
	const double *pValue = GetVariableConstPtr(cszOutputName);

	for (auto& it : m_Primitives)
		if (it->Output() == pValue)
			return it;

	/*for (PRIMITIVEBLOCKITR it = m_Primitives.begin(); it != m_Primitives.end(); it++)
		if ((*it)->Output() == pValue)
			return *it;*/

	return NULL;
}