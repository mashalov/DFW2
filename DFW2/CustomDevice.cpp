#include "stdafx.h"
#include "CustomDevice.h"
#include "DynaModel.h"

using namespace DFW2;

CCustomDeviceCPP::CCustomDeviceCPP() 
{ 
	// может быть вынести в единственном экземпляре в контейнер ?
	CustomDeviceData.SetFnSetFunction(DLLSetFunction);
	CustomDeviceData.SetFnSetElement(DLLSetElement);
	CustomDeviceData.SetFnSetDerivative(DLLSetDerivative);
	CustomDeviceData.SetFnSetFunctionDiff(DLLSetFunctionDiff);
	CustomDeviceData.SetFnInitPrimitive(DLLInitPrimitive);
	CustomDeviceData.SetFnProcessPrimitiveDisco(DLLProcPrimDisco);
	CustomDeviceData.SetFnIndexedVariable(DLLIndexedVariable);
}

void CCustomDeviceCPP::CreateDLLDeviceInstance(CCustomDeviceCPPContainer& Container)
{
	m_pDevice.Create(Container.DLL());

	// проходим по примитивам устройства

	auto PrimitiveName{ Container.PrimitiveNames().begin() };

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
		Container.CreatePrimitive(prim.eBlockType, *this, *PrimitiveName, ORange(outputs), IRange(inputs));
		PrimitiveName++;
	}
}

void CCustomDeviceCPP::SetConstsDefaultValues()
{
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
	m_pDevice->SetConstsDefaultValues();
}

void CCustomDeviceCPP::SetSourceConstant(size_t nIndex, double Value)
{
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
	if (!m_pDevice->SetSourceConstant(nIndex, Value))
		throw dfw2error("CCustomDeviceCPP::SetSourceConstant - Constants index overrun");
}


VariableIndexRefVec& CCustomDeviceCPP::GetVariables(VariableIndexRefVec& ChildVec)
{
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
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
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
	PrepareCustomDeviceData(pDynaModel);
	m_pDevice->BuildRightHand(CustomDeviceData);
	CDevice::BuildRightHand(pDynaModel);
}

void CCustomDeviceCPP::BuildEquations(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
	PrepareCustomDeviceData(pDynaModel);
	m_pDevice->BuildEquations(CustomDeviceData);
	CDevice::BuildEquations(pDynaModel);
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::Init(CDynaModel* pDynaModel)
{
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
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
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
	PrepareCustomDeviceData(pDynaModel);
	return CDevice::DeviceFunctionResult(m_pDevice->ProcessDiscontinuity(CustomDeviceData), CDevice::ProcessDiscontinuity(pDynaModel));
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::UpdateExternalVariables(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS eRes(eDEVICEFUNCTIONSTATUS::DFS_OK);
	if (!m_pDevice) throw dfw2error(cszNoDeviceDLL_);
	const VariableIndexExternalRefVec& ExtVec = m_pDevice->GetExternalVariables();
	for (const auto& ext : pContainer_->ContainerProps().ExtVarMap_)
	{
		CDevice::CheckIndex(ExtVec, ext.second.Index_, "CCustomDeviceCPP::UpdateExternalVariables");
		eRes = DeviceFunctionResult(eRes, InitExternalVariable(ExtVec[ext.second.Index_], this, ext.first, ext.second.DeviceToSearch_));
		if (eRes == eDEVICEFUNCTIONSTATUS::DFS_FAILED)
			break;
	}

	// очень грубый  тест инициализации внешних переменных
	//if(pDynaModel->GetCurrentTime() > 0.01)
	//	for (const auto& x : Primitives_)
	//		if(x->Input().Indexed())
	//			pDynaModel->GetRightVector(x->Input().Index);
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
	CDevice::CheckIndex(pDevice->Primitives_, nPrimitiveIndex, "CCustomDeviceCPP::DLLInitPrimitive - index out of range");
	CDynaPrimitive* pPrimitive = pDevice->Primitives_[nPrimitiveIndex];
	if(pPrimitive->UnserializeParameters(pDynaModel, pDevice->m_pDevice->GetBlockParameters(nPrimitiveIndex)) && pPrimitive->Init(pDynaModel))
		return eDEVICEFUNCTIONSTATUS::DFS_OK;
	else
		return eDEVICEFUNCTIONSTATUS::DFS_FAILED;
}

eDEVICEFUNCTIONSTATUS CCustomDeviceCPP::DLLProcPrimDisco(CDFWModelData& DFWModelData, ptrdiff_t nPrimitiveIndex)
{
	CCustomDeviceCPP* pDevice(GetDevice(DFWModelData));
	CDynaModel* pDynaModel(GetModel(DFWModelData));
	CDevice::CheckIndex(pDevice->Primitives_, nPrimitiveIndex, "CCustomDeviceCPP::DLLProcPrimDisco - index out of range");
	return pDevice->Primitives_[nPrimitiveIndex]->ProcessDiscontinuity(pDynaModel);
}

void CCustomDeviceCPP::DLLSetElement(CDFWModelData& DFWModelData, const VariableIndexBase& Row, const VariableIndexBase& Col, double dValue)
{
	GetModel(DFWModelData)->SetElement(Row,Col,dValue);
}

void CCustomDeviceCPP::DLLSetFunction(CDFWModelData& DFWModelData, const VariableIndexBase& Row, double dValue)
{
	GetModel(DFWModelData)->SetFunction(Row, dValue);
}

bool CCustomDeviceCPP::DLLIndexedVariable(CDFWModelData& DFWModelData, const VariableIndexBase& Variable)
{
	return GetModel(DFWModelData)->IndexedVariable(Variable);
}

CCustomDeviceCPPContainer* CCustomDeviceCPP::GetContainer()
{ 
	if (!pContainer_)
		throw dfw2error("CCustomDeviceCPP::GetContainer - container not set");
	return static_cast<CCustomDeviceCPPContainer*>(pContainer_); 
}

void CCustomDeviceCPP::PrepareCustomDeviceData(CDynaModel *pDynaModel)
{
	CustomDeviceData.SetModelData(pDynaModel, this);
}

void CCustomDeviceDataHost::SetModelData(CDynaModel* Model, CDevice* Device)
{
	*static_cast<CDFWModelData*>(this) = CDFWModelData{ Model, Device, Model->GetCurrentTime() };
}


CDynaPrimitive* CCustomDeviceCPP::GetPrimitiveForNamedOutput(std::string_view OutputName)
{
	const double* pValue{ GetVariableConstPtr(OutputName) };

	for (const auto& it : Primitives_)
		if (static_cast<const double*>(*it) == pValue)
			return it;

	return nullptr;
}
