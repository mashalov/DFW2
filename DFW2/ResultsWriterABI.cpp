#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

void CResultsWriterABI::WriteResults(const WriteResultsInfo& WriteInfo)
{
	m_ABIWriter->WriteResults(WriteInfo.Time, WriteInfo.Step);
}

void CResultsWriterABI::FinishWriteResults()
{
	m_ABIWriter->Close();
}

void CResultsWriterABI::CreateFile(std::filesystem::path path, ResultsInfo& Info)
{
#ifdef _MSC_VER	
	m_ABIWriterDLL = std::make_shared<WriterDLL>("ResultFile.dll", "ResultWriterABIFactory");
#else
	m_ABIWriterDLL = std::make_shared<WriterDLL>("./Res/libResultFile.so", "ResultWriterABIFactory");
#endif	
	m_ABIWriter.Create(m_ABIWriterDLL);
	m_ABIWriter->CreateResultFile(path);
	m_ABIWriter->SetNoChangeTolerance(Info.NoChangeTolerance);
	m_ABIWriter->SetComment(Info.Comment);
}

void CResultsWriterABI::AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName)
{
	m_ABIWriter->AddVariableUnit(nUnitType, UnitName);
}

void CResultsWriterABI::AddDeviceType(const CDeviceContainer& Container)
{
	auto pDeviceType = m_ABIWriter->AddDeviceType(Container.GetType(), Container.GetTypeName());

	const CDeviceContainerProperties& Props = Container.m_ContainerProps;
	// по умолчанию у устройства один идентификатор и одно родительское устройство
	long DeviceIdsCount = 1;
	long ParentIdsCount = static_cast<long>(Props.m_Masters.size());
	// у ветви - три идентификатора
	if (Container.GetType() == DEVTYPE_BRANCH)
	{
		DeviceIdsCount = 3;
		ParentIdsCount = 2;
	}

	pDeviceType->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, static_cast<long>(Container.CountNonPermanentOff()));

	for (const auto& var : Props.m_VarMap)
	{
		if (var.second.m_bOutput)
			pDeviceType->AddDeviceTypeVariable(var.first, var.second.m_Units, var.second.m_dMultiplier);
	}

	for (const auto& var : Props.m_ConstVarMap)
	{
		if (var.second.m_bOutput)
			pDeviceType->AddDeviceTypeVariable(var.first, var.second.m_Units, var.second.m_dMultiplier);
	}

	ResultIds DeviceIds, ParentIds, ParentTypes;

	for (const auto& device : Container)
	{
		if (device->IsPermanentOff())
			continue;

		DeviceIds.clear();
		ParentIds.clear();
		ParentTypes.clear();

		if (device->GetType() == DEVTYPE_BRANCH)
		{
			// для ветвей передаем номер начала, конца и номер параллельной цепи
			const CDynaBranch* pBranch = static_cast<const CDynaBranch*>(device);
			DeviceIds = { pBranch->key.Ip, pBranch->key.Iq, pBranch->key.Np };
			ParentIds = { pBranch->m_pNodeIp ? pBranch->m_pNodeIp->GetId() : 0,  pBranch->m_pNodeIq ? pBranch->m_pNodeIq->GetId() : 0 };
			ParentTypes = { DEVTYPE_NODE, DEVTYPE_NODE };
		}
		else
		{
			DeviceIds = { device->GetId() };
			for (const auto& master : Props.m_Masters)
			{
				CDevice* pLinkDev = device->GetSingleLink(master->nLinkIndex);
				ParentIds.push_back(pLinkDev ? pLinkDev->GetId() : 0);
				ParentTypes.push_back(pLinkDev ? pLinkDev->GetType() : 0);
			}
		}

		pDeviceType->AddDevice(device->GetName(),  DeviceIds, ParentIds, ParentTypes);
	}
}

void CResultsWriterABI::SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t VarIndex, double* ValuePtr, ptrdiff_t ChannelIndex)
{
	m_ABIWriter->SetChannel(DeviceId, DeviceType, VarIndex, ValuePtr, ChannelIndex);
}

void CResultsWriterABI::FinishWriteHeader()
{
	m_ABIWriter->FinishWriteHeader();
}

void CResultsWriterABI::AddSlowVariable(ptrdiff_t nDeviceType,
	const ResultIds& DeviceIds,
	const std::string_view VariableName,
	double Time,
	double Value,
	double PreviousValue,
	const std::string_view ChangeDescription)
{
	m_ABIWriter->AddSlowVariable(nDeviceType,
		DeviceIds,
		VariableName,
		Time,
		Value,
		PreviousValue,
		ChangeDescription);
}