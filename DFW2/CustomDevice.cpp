#include "stdafx.h"
#include "CustomDevice.h"
#include "DynaModel.h"

using namespace DFW2;

CCustomDevice::CCustomDevice() 
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

	// !------------------------
	// ! Constants
	// !------------------------
	// ! Set Points
	// !------------------------

	m_pConstVars = Container()->NewDoubleVariables();
	m_pSetPoints = m_pConstVars + Container()->GetConstsCount();
	m_pVars = Container()->NewVariableIndexVariables();
	m_pExternals = Container()->NewExternalVariables();
	m_pVarsExt = Container()->NewVariableIndexExternals();

	const BLOCKDESCRIPTIONS&  Blocks = Container()->DLL().GetBlocksDescriptions();
	const BLOCKSPINSINDEXES&  Pins = Container()->DLL().GetBlocksPinsIndexes();

	_ASSERTE(Blocks.size() == Pins.size());

	auto& iit = Pins.begin();

	for (auto& bit = Blocks.begin(); bit != Blocks.end(); bit++, iit++)
	{
		std::vector<InputVariable> inputs;
		inputs.reserve(iit->size());
		for (auto& nit : *iit)
		{
			switch (nit.Location)
			{
			case eVL_INTERNAL:
				{
					InputVariable vx = InputVariable(m_pVars[nit.nIndex]);
					inputs.push_back(vx);
				}
				break;
			case eVL_EXTERNAL:
				{
					InputVariable vx = InputVariable(m_pVarsExt[nit.nIndex]);
					inputs.push_back(vx);
				}
				break;
			}

			
		}

		CDynaPrimitive *pDummy = nullptr;
		VariableIndex& Output  = m_pVars[iit->front().nIndex];
		VariableIndex& Output1 = m_pVars[iit->front().nIndex + 1];

		switch (bit->eType)
		{
		case PBT_LAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, Output, { inputs[1] }, {});
			break;
		case PBT_LIMITEDLAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, Output, { inputs[1] }, {});
			break;
		case PBT_DERLAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CDerlag(*this, Output, { inputs[1] }, { Output1 });
			break;
		case PBT_INTEGRATOR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, Output, { inputs[1] }, {});
			break;
		case PBT_LIMITEDINTEGRATOR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, Output, { inputs[1] }, {});
			break;
		case PBT_LIMITERCONST:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimiterConst(*this, Output, { inputs[1] }, {});
			break;
		case PBT_RELAY:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelay(*this, Output, { inputs[1] }, {});
			break;
		case PBT_RELAYDELAY:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayDelay(*this, Output, { inputs[1] }, {});
			break;
		case PBT_RELAYDELAYLOGIC:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayDelayLogic(*this, Output, { inputs[1] }, {});
			break;
		case PBT_RSTRIGGER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRSTrigger(*this, Output, { inputs[1], inputs[2] }, {});
			break;
		case PBT_HIGHER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CComparator(*this, Output, { inputs[1], inputs[2] }, {});
			break;
		case PBT_LOWER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CComparator(*this, Output, { inputs[1], inputs[2] }, {});
			break;
		case PBT_BIT:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CZCDetector(*this, Output, { inputs[1] }, {});
			break;
		case PBT_AND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CAnd(*this, Output, { inputs[1], inputs[2] }, {});
			break;
		case PBT_OR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) COr(*this, Output, { inputs[1], inputs[2] }, {});
			break;
		case PBT_NOT:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CNot(*this, Output, { inputs[1] }, {});
			break;
		case PBT_ABS:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CAbs(*this, Output, { inputs[1] }, {});
			break;
		case PBT_DEADBAND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CDeadBand(*this, Output, { inputs[1] }, {});
			break;
		case PBT_EXPAND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CExpand(*this, Output, { inputs[1] }, {});
			break;
		case PBT_SHRINK:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CShrink(*this, Output, { inputs[1] }, {});
			break;
		}
	}

	if (!bRes)
		Log(CDFW2Messages::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszDLLBadBlocks, Container()->DLL().GetModuleFilePath()));

	return bRes;
}


bool CCustomDevice::SetConstValue(size_t nIndex, double dValue)
{
	bool bRes = false;

	if (GetId() == 102401)
		GetId();

	auto& ConstInfo = Container()->DLL().GetConstsInfo();

	if (nIndex >= 0 && nIndex < ConstInfo.size())
	{
		const ConstVarsInfo& constInfo = ConstInfo[nIndex];

		if (constInfo.VarFlags & CVF_ZEROTODEFAULT)
		{
			dValue = constInfo.dDefault;
		}

		if (constInfo.dMax > constInfo.dMin)
		{
			if (dValue > constInfo.dMax) dValue = constInfo.dDefault;
			if (dValue < constInfo.dMin) dValue = constInfo.dDefault;
		}

		m_pConstVars[nIndex] = dValue;
		bRes = true;
	}
	return bRes;
}

bool CCustomDevice::SetConstDefaultValues()
{
	size_t nIndex(0);
	for (const auto& it : Container()->DLL().GetConstsInfo())
		m_pConstVars[nIndex++] = it.dDefault;

	return true;
}

eDEVICEFUNCTIONSTATUS CCustomDevice::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
	bool bRes = true;

	ptrdiff_t nConstIndex(0);
	for (const auto& it : Container()->DLL().GetConstsInfo())
	{
		if (it.eDeviceType != DEVTYPE_UNKNOWN && !(it.VarFlags & CVF_INTERNALCONST))
			bRes = bRes && InitConstantVariable(m_pConstVars[nConstIndex++], this, it.VarInfo.Name, static_cast<eDFW2DEVICETYPE>(it.eDeviceType));
	}

	bRes = bRes && ConstructDLLParameters(pDynaModel);
	m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;
	bRes = bRes && Container()->InitDLLEquations(&m_DLLArgs);

	if (!bRes)
		m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

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
	m_DLLArgs.pEquations = static_cast<DLLVariableIndex*>(static_cast<void*>(m_pVars));
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
	m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;
	
	if (IsStateOn())
	{
		m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;
		bool bRes = ConstructDLLParameters(pDynaModel);
		Container()->ProcessDLLDiscontinuity(&m_DLLArgs);
	}
	else
		m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;

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
		DOUBLEVECTOR Parameters;
		static_cast<CCustomDeviceContainer*>(pDevice->Container())->GetParametersValues(pDevice->GetId(), &pDevice->m_DLLArgs, nBlockIndex, Parameters);
		if (!pPrimitive->UnserializeParameters(pDynaModel, Parameters))
			pDevice->m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
		else
			if (!pPrimitive->Init(pDynaModel))
				pDevice->m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
	}

	return 1;
}


eDEVICEFUNCTIONSTATUS CCustomDevice::UpdateExternalVariables(CDynaModel *pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes(eDEVICEFUNCTIONSTATUS::DFS_OK);

	const auto& InputInfos = Container()->DLL().GetInputsInfo();
	VariableIndexExternal * pStart = m_pVarsExt;
	DLLExternalVariable* pExt = m_pExternals;

	for (const auto& it : InputInfos)
	{
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(*pStart, this, it.VarInfo.Name, static_cast<eDFW2DEVICETYPE>(it.eDeviceType)));
		if (eRes != eDEVICEFUNCTIONSTATUS::DFS_FAILED)
		{
			pExt->pValue = &(double&)*pStart;
			pExt->nIndex = pStart->Index;
		}
		else
			break;

		pExt++;		
		pStart++;
	}

	/*
	PrimitiveVariable* pPs(m_pPrimitiveVars);
	for (ptrdiff_t i = 0; i < Container()->GetPrimitiveVariablesCount(); i++)
		pPs->Index(i + m_nMatrixRow);*/
	
	return eRes;
}


double* CCustomDevice::GetConstVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);

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

VariableIndexRefVec& CCustomDevice::GetVariables(VariableIndexRefVec& ChildVec)
{
	ChildVec.insert(ChildVec.begin(), m_pVars, m_pVars + m_pContainer->EquationsCount());
	return ChildVec;
}




double* CCustomDevice::GetVariablePtr(ptrdiff_t nVarIndex)
{
	double *p(nullptr);

	if (nVarIndex >= 0 && nVarIndex < m_pContainer->EquationsCount())
		p = &(m_pVars + nVarIndex)->Value;

	_ASSERTE(p);
	return p;
}


CDynaPrimitive* CCustomDevice::GetPrimitiveForNamedOutput(const _TCHAR* cszOutputName)
{
	const double *pValue = GetVariableConstPtr(cszOutputName);

	for (auto&& it : m_Primitives)
		if (&(const double)*it == pValue)
			return it;

	return nullptr;
}

CCustomDeviceCPP::CCustomDeviceCPP() 
{ 
	// может быть вынести в единственном экземпл€ре в контейнер ?
	CustomDeviceData.pFnSetFunction		= DLLSetFunction;
	CustomDeviceData.pFnSetElement		= DLLSetElement;
	CustomDeviceData.pFnSetDerivative	= DLLSetDerivative;
	CustomDeviceData.pFnSetFunctionDiff = DLLSetFunctionDiff;
	CustomDeviceData.pFnInitPrimitive	= DLLInitPrimitive;
	CustomDeviceData.pFnProcPrimDisco	= DLLProcPrimDisco;
}

void CCustomDeviceCPP::CreateDLLDeviceInstance(CCustomDeviceCPPContainer& Container)
{
	m_pDevice.Create(Container.DLL());
	PRIMITIVEVECTOR& Prims = m_pDevice->GetPrimitives();
	VARIABLEVECTOR& VarVec = m_pDevice->GetVariables();
	EXTVARIABLEVECTOR& ExtVec = m_pDevice->GetExternalVariables();

	for (auto&& prim : Prims)
	{

		PrimitivePinVec& Inputs = prim.Inputs;
		if (Inputs.empty())
			throw dfw2error(_T("CCustomDeviceCPP::CreateDLLDeviceInstance - no primitive inputs"));

		PrimitivePinVec& Outputs = prim.Outputs;
		if (Outputs.empty())
			throw dfw2error(_T("CCustomDeviceCPP::CreateDLLDeviceInstance - no primitive output"));
		auto& PrimaryOutput = Outputs.front();
		if (PrimaryOutput.Variable.index() != 0)
			throw dfw2error(_T("CCustomDeviceCPP::CreateDLLDeviceInstance - output variable must have type VariableIndex"));
		VariableIndex& OutputVar(std::get<0>(PrimaryOutput.Variable));

		auto& Input = Inputs.front();

		switch (Input.Variable.index())
		{
		case 0:
			{
				VariableIndex& inputvar(std::get<0>(Input.Variable));
				Container.CreatePrimitive(prim.eBlockType, *this, OutputVar, { inputvar }, {});
			}
			break;
		case 1:
			{
				VariableIndexExternal& inputvar(std::get<1>(Input.Variable));
				Container.CreatePrimitive(prim.eBlockType, *this, OutputVar, { inputvar }, {});
			}
			break;
		}


	}
}

void CCustomDeviceCPP::SetConstsDefaultValues()
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	m_pDevice->SetConstsDefaultValues();
}

DOUBLEVECTOR& CCustomDeviceCPP::GetConstantData()
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	return m_pDevice->GetConstantData();
}

VariableIndexRefVec& CCustomDeviceCPP::GetVariables(VariableIndexRefVec& ChildVec)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	VARIABLEVECTOR& vecDevice = m_pDevice->GetVariables();
	VariableIndexRefVec VarVec;
	VarVec.reserve(vecDevice.size());
	for (auto& var : m_pDevice->GetVariables())
		ChildVec.emplace_back(var);
	return CDevice::GetVariables(JoinVariables(VarVec, ChildVec));
}

double* CCustomDeviceCPP::GetVariablePtr(ptrdiff_t nVarIndex)
{
	VariableIndexRefVec vec;
	GetVariables(vec);
	CDevice::CheckIndex(vec, nVarIndex, _T("CCustomDeviceCPP::GetVariablePtr"));
	return &vec[nVarIndex].get().Value;
}


bool CCustomDeviceCPP::BuildRightHand(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	PrepareCustomDeviceData(pDynaModel);
	m_pDevice->BuildRightHand(CustomDeviceData);
	CDevice::BuildRightHand(pDynaModel);
	return true;
}

bool CCustomDeviceCPP::BuildEquations(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	PrepareCustomDeviceData(pDynaModel);
	m_pDevice->BuildEquations(CustomDeviceData);
	CDevice::BuildEquations(pDynaModel);
	return true;
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::Init(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	PrepareCustomDeviceData(pDynaModel);
	eDEVICEFUNCTIONSTATUS eRes = m_pDevice->Init(CustomDeviceData);


	
	/*
	eDEVICEFUNCTIONSTATUS Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
	bool bRes = true;

	ptrdiff_t nConstIndex(0);
	for (const auto& it : Container()->DLL().GetConstsInfo())
	{
		if (it.eDeviceType != DEVTYPE_UNKNOWN && !(it.VarFlags & CVF_INTERNALCONST))
			bRes = bRes && InitConstantVariable(m_pConstVars[nConstIndex++], this, it.VarInfo.Name, static_cast<eDFW2DEVICETYPE>(it.eDeviceType));
	}

	bRes = bRes && ConstructDLLParameters(pDynaModel);
	m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;
	bRes = bRes && Container()->InitDLLEquations(&m_DLLArgs);

	if (!bRes)
		m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	return m_ExternalStatus;
	*/



	return eRes;
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::ProcessDiscontinuity(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	PrepareCustomDeviceData(pDynaModel);
	return CDevice::DeviceFunctionResult(m_pDevice->ProcessDiscontinuity(CustomDeviceData), CDevice::ProcessDiscontinuity(pDynaModel));
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::UpdateExternalVariables(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes(eDEVICEFUNCTIONSTATUS::DFS_OK);
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	EXTVARIABLEVECTOR& ExtVec = m_pDevice->GetExternalVariables();
	for (const auto& ext : m_pContainer->m_ContainerProps.m_ExtVarMap)
	{
		CDevice::CheckIndex(ExtVec, ext.second.m_nIndex, _T("CCustomDeviceCPP::UpdateExternalVariables"));
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtVec[ext.second.m_nIndex], this, ext.first.c_str(), ext.second.m_DeviceToSearch));
		if (eRes == eDEVICEFUNCTIONSTATUS::DFS_FAILED)
			break;
	}
	return eRes;
}


void CCustomDeviceCPP::DLLSetFunctionDiff(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue)
{
	GetModel(DFWModelData)->SetFunctionDiff(Row, dValue);
}

void CCustomDeviceCPP::DLLSetDerivative(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue)
{
	GetModel(DFWModelData)->SetDerivative(Row, dValue);
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::DLLInitPrimitive(CDFWModelData& DFWModelData, ptrdiff_t nPrimitiveIndex)
{
	CCustomDeviceCPP* pDevice(GetDevice(DFWModelData));
	CDynaModel* pDynaModel(GetModel(DFWModelData));
	CDevice::CheckIndex(pDevice->m_Primitives, nPrimitiveIndex, _T("CCustomDeviceCPP::DLLInitPrimitive - index out of range"));
	CDynaPrimitive* pPrimitive = pDevice->m_Primitives[nPrimitiveIndex];
	if(pPrimitive->UnserializeParameters(pDynaModel, pDevice->m_pDevice->GetBlockParameters(nPrimitiveIndex)) && pPrimitive->Init(pDynaModel))
		return eDEVICEFUNCTIONSTATUS::DFS_OK;
	else
		return eDEVICEFUNCTIONSTATUS::DFS_FAILED;
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::DLLProcPrimDisco(CDFWModelData& DFWModelData, ptrdiff_t nPrimitiveIndex)
{
	CCustomDeviceCPP* pDevice(GetDevice(DFWModelData));
	CDynaModel* pDynaModel(GetModel(DFWModelData));
	CDevice::CheckIndex(pDevice->m_Primitives, nPrimitiveIndex, _T("CCustomDeviceCPP::DLLProcPrimDisco - index out of range"));
	return pDevice->m_Primitives[nPrimitiveIndex]->ProcessDiscontinuity(pDynaModel);
}

void CCustomDeviceCPP::DLLSetElement(CDFWModelData& DFWModelData, const VariableIndexBase& Row, const VariableIndexBase& Col, double dValue)
{
	GetModel(DFWModelData)->SetElement(Row,Col,dValue);
}

void CCustomDeviceCPP::DLLSetFunction(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue)
{
	GetModel(DFWModelData)->SetFunction(Row, dValue);
}

CCustomDeviceCPPContainer* CCustomDeviceCPP::GetContainer()
{ 
	if (!m_pContainer)
		throw dfw2error(_T("CCustomDeviceCPP::GetContainer - container not set"));
	return static_cast<CCustomDeviceCPPContainer*>(m_pContainer); 
}

void CCustomDeviceCPP::PrepareCustomDeviceData(CDynaModel *pDynaModel)
{
	CustomDeviceData.pModel = pDynaModel;
	CustomDeviceData.pDevice = this;
}

const _TCHAR* CCustomDeviceCPP::m_cszNoDeviceDLL = _T("CCustomDeviceCPP - no DLL device initialized");