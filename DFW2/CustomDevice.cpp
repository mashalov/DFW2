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

	auto&& iit = Pins.begin();

	for (auto&& bit = Blocks.begin(); bit != Blocks.end(); bit++, iit++)
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
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, { Output }, { inputs[1] });
			break;
		case PBT_LIMITEDLAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, { Output }, { inputs[1] });
			break;
		case PBT_DERLAG:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CDerlag(*this, { Output, Output1 }, { inputs[1] });
			break;
		case PBT_INTEGRATOR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, { Output } , { inputs[1] });
			break;
		case PBT_LIMITEDINTEGRATOR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimitedLag(*this, { Output }, { inputs[1] });
			break;
		case PBT_LIMITERCONST:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CLimiterConst(*this, { Output }, { inputs[1] });
			break;
		case PBT_RELAY:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelay(*this, { Output }, { inputs[1] });
			break;
		case PBT_RELAYMIN:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayMin(*this, { Output }, { inputs[1] });
			break;
		case PBT_RELAYDELAY:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayDelay(*this, { Output }, { inputs[1] });
			break;
		case PBT_RELAYDELAYLOGIC:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayDelayLogic(*this, { Output }, { inputs[1] });
			break;
		case PBT_RELAYMINDELAYLOGIC:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRelayMinDelayLogic(*this, { Output }, { inputs[1] });
			break;
		case PBT_RSTRIGGER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CRSTrigger(*this, { Output }, { inputs[1], inputs[2] });
			break;
		case PBT_HIGHER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CComparator(*this, { Output }, { inputs[1], inputs[2] });
			break;
		case PBT_LOWER:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CComparator(*this, { Output }, { inputs[1], inputs[2] });
			break;
		case PBT_BIT:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CZCDetector(*this, { Output }, { inputs[1] });
			break;
		case PBT_AND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CAnd(*this, { Output }, { inputs[1], inputs[2] });
			break;
		case PBT_OR:
			pDummy = new(Container()->NewPrimitive(bit->eType)) COr(*this, { Output }, { inputs[1], inputs[2] });
			break;
		case PBT_NOT:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CNot(*this, { Output }, { inputs[1] });
			break;
		case PBT_ABS:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CAbs(*this, { Output }, { inputs[1] });
			break;
		case PBT_DEADBAND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CDeadBand(*this, { Output }, { inputs[1] });
			break;
		case PBT_EXPAND:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CExpand(*this, { Output }, { inputs[1] });
			break;
		case PBT_SHRINK:
			pDummy = new(Container()->NewPrimitive(bit->eType)) CShrink(*this, { Output }, { inputs[1] });
			break;
		}
	}

	if (!bRes)
		Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszDLLBadBlocks, Container()->DLL().GetModuleFilePath()));

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

	ConstructDLLParameters(pDynaModel);
	m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_OK;
	bRes = bRes && Container()->InitDLLEquations(&m_DLLArgs);

	if (!bRes)
		m_ExternalStatus = eDEVICEFUNCTIONSTATUS::DFS_FAILED;

	return m_ExternalStatus;
}

void CCustomDevice::BuildEquations(CDynaModel *pDynaModel)
{
	ConstructDLLParameters(pDynaModel);
	Container()->BuildDLLEquations(&m_DLLArgs);
	CDevice::BuildEquations(pDynaModel);
}

void CCustomDevice::BuildRightHand(CDynaModel *pDynaModel)
{
	ConstructDLLParameters(pDynaModel);
	Container()->BuildDLLRightHand(&m_DLLArgs);
	CDevice::BuildRightHand(pDynaModel);
}

void CCustomDevice::ConstructDLLParameters(CDynaModel *pDynaModel)
{
	m_DLLArgs.pEquations = static_cast<DLLVariableIndex*>(static_cast<void*>(m_pVars));
	m_DLLArgs.pConsts    = m_pConstVars;
	m_DLLArgs.pExternals = m_pExternals;
	m_DLLArgs.pSetPoints = m_pSetPoints;
	m_DLLArgs.BuildObjects.pModel = pDynaModel;
	m_DLLArgs.BuildObjects.pDevice = this;
	m_DLLArgs.Omega = pDynaModel->GetOmega0();
	m_DLLArgs.t = pDynaModel->GetCurrentTime();
}

void CCustomDevice::BuildDerivatives(CDynaModel *pDynaModel)
{
	ConstructDLLParameters(pDynaModel);
	Container()->BuildDLLDerivatives(&m_DLLArgs);
	CDevice::BuildDerivatives(pDynaModel);
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
		ConstructDLLParameters(pDynaModel);
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


CDynaPrimitive* CCustomDevice::GetPrimitiveForNamedOutput(std::string_view OutputName)
{
	const double *pValue = GetVariableConstPtr(OutputName);

	for (const auto& it : m_Primitives)
		if (static_cast<const double*>(*it) == pValue)
			return it;

	return nullptr;
}

CCustomDeviceCPP::CCustomDeviceCPP() 
{ 
	// может быть вынести в единственном экземпляре в контейнер ?
	CustomDeviceData.SetFnSetFunction(DLLSetFunction);
	CustomDeviceData.SetFnSetElement(DLLSetElement);
	CustomDeviceData.SetFnSetDerivative(DLLSetDerivative);
	CustomDeviceData.SetFnSetFunctionDiff(DLLSetFunctionDiff);
	CustomDeviceData.SetFnInitPrimitive(DLLInitPrimitive);
	CustomDeviceData.SetFnProcessPrimitiveDisco(DLLProcPrimDisco);
}

void CCustomDeviceCPP::CreateDLLDeviceInstance(CCustomDeviceCPPContainer& Container)
{
	m_pDevice.Create(Container.DLL());

	// проходим по примитивам устройства
	for (const auto& prim : m_pDevice->GetPrimitives())
	{
		// проверяем количество входов и выходов
		const PrimitivePinVec& Inputs = prim.Inputs;
		if (Inputs.empty())
			throw dfw2error("CCustomDeviceCPP::CreateDLLDeviceInstance - no primitive inputs");

		const PrimitivePinVec& Outputs = prim.Outputs;
		if (Outputs.empty())
			throw dfw2error("CCustomDeviceCPP::CreateDLLDeviceInstance - no primitive output");

		// собираем и проверяем выходы
		VariableIndexRefVec outputs;
		for (auto& vo : Outputs)
		{
			if (vo.Variable.index() != 0)
				throw dfw2error("CCustomDeviceCPP::CreateDLLDeviceInstance - output variable must have type VariableIndex");
			outputs.emplace_back(std::get<0>(vo.Variable));
		}

		// собираем и проверяем входы
		InputVariableVec inputs;
		for (auto& vi : Inputs)
		{

			switch (vi.Variable.index())
			{
			case 0:
				inputs.emplace_back(std::get<0>(vi.Variable));
				break;
			case 1:
				inputs.emplace_back(std::get<1>(vi.Variable));
			}
		}
		// создаем примитив из пула
		Container.CreatePrimitive(prim.eBlockType, *this, ORange(outputs), IRange(inputs));
	}
}

void CCustomDeviceCPP::SetConstsDefaultValues()
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	m_pDevice->SetConstsDefaultValues();
}

void CCustomDeviceCPP::SetSourceConstant(size_t nIndex, double Value)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	if (!m_pDevice->SetSourceConstant(nIndex, Value))
		throw dfw2error("CCustomDeviceCPP::SetSourceConstant - Constants index overrun");
}


VariableIndexRefVec& CCustomDeviceCPP::GetVariables(VariableIndexRefVec& ChildVec)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	return CDevice::GetVariables(JoinVariables(m_pDevice->GetVariables(), ChildVec));
}

double* CCustomDeviceCPP::GetVariablePtr(ptrdiff_t nVarIndex)
{
	VariableIndexRefVec vec;
	GetVariables(vec);
	CDevice::CheckIndex(vec, nVarIndex, "CCustomDeviceCPP::GetVariablePtr");
	return &vec[nVarIndex].get().Value;
}


void CCustomDeviceCPP::BuildRightHand(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	PrepareCustomDeviceData(pDynaModel);
	m_pDevice->BuildRightHand(CustomDeviceData);
	CDevice::BuildRightHand(pDynaModel);
}

void CCustomDeviceCPP::BuildEquations(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(m_cszNoDeviceDLL);
	PrepareCustomDeviceData(pDynaModel);
	m_pDevice->BuildEquations(CustomDeviceData);
	CDevice::BuildEquations(pDynaModel);
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
	const VariableIndexExternalRefVec& ExtVec = m_pDevice->GetExternalVariables();
	for (const auto& ext : m_pContainer->m_ContainerProps.m_ExtVarMap)
	{
		CDevice::CheckIndex(ExtVec, ext.second.m_nIndex, "CCustomDeviceCPP::UpdateExternalVariables");
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
	CDevice::CheckIndex(pDevice->m_Primitives, nPrimitiveIndex, "CCustomDeviceCPP::DLLInitPrimitive - index out of range");
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
	CDevice::CheckIndex(pDevice->m_Primitives, nPrimitiveIndex, "CCustomDeviceCPP::DLLProcPrimDisco - index out of range");
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
		throw dfw2error("CCustomDeviceCPP::GetContainer - container not set");
	return static_cast<CCustomDeviceCPPContainer*>(m_pContainer); 
}

void CCustomDeviceCPP::PrepareCustomDeviceData(CDynaModel *pDynaModel)
{
	CustomDeviceData.SetModelData(pDynaModel, this);
}

void CCustomDeviceDataHost::SetModelData(CDynaModel* Model, CDevice* Device)
{
	*static_cast<CDFWModelData*>(this) = CDFWModelData{ Model, Device, Model->GetCurrentTime() };
}

const char* CCustomDeviceCPP::m_cszNoDeviceDLL = "CCustomDeviceCPP - no DLL device initialized";