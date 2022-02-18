#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

void CResultsWriterABI::WriteResults(const WriteResultsInfo& WriteInfo)
{
	ABIWriter->WriteResults(WriteInfo.Time, WriteInfo.Step);
}

void CResultsWriterABI::FinishWriteResults()
{
	ABIWriter->Close();
}

void CResultsWriterABI::CreateFile(std::filesystem::path path, ResultsInfo& Info)
{
#ifdef _MSC_VER	
	ABIWriterDLL = std::make_shared<WriterDLL>("ResultFile.dll", "ResultWriterABIFactory");
#else
	ABIWriterDLL = std::make_shared<WriterDLL>("./Res/libResultFile.so", "ResultWriterABIFactory");
#endif	
	ABIWriter.Create(ABIWriterDLL);
	ABIWriter->CreateResultFile(path);
	ABIWriter->SetNoChangeTolerance(Info.NoChangeTolerance);
	ABIWriter->SetComment(Info.Comment);
}

void CResultsWriterABI::AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName)
{
	ABIWriter->AddVariableUnit(nUnitType, UnitName);
}

void CResultsWriterABI::AddDeviceType(const CDeviceContainer& Container)
{
	auto pDeviceType{ ABIWriter->AddDeviceType(Container.GetType(), Container.GetTypeName()) };

	const CDeviceContainerProperties& Props{ Container.ContainerProps()};
	// по умолчанию у устройства один идентификатор и одно родительское устройство
	long DeviceIdsCount{ 1 }, ParentIdsCount{ static_cast<long>(Props.m_Masters.size()) };
	// у ветви - три идентификатора
	if (Container.GetType() == DEVTYPE_BRANCH)
	{
		DeviceIdsCount = 3;
		ParentIdsCount = 2;
	}

	pDeviceType->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, static_cast<long>(Container.CountNonPermanentOff()));

	for (const auto& var : Props.VarMap_)
	{
		if (var.second.Output_)
			pDeviceType->AddDeviceTypeVariable(var.first, var.second.Units_, var.second.Multiplier_);
	}

	for (const auto& var : Props.ConstVarMap_)
	{
		if (var.second.Output_)
			pDeviceType->AddDeviceTypeVariable(var.first, var.second.Units_, var.second.Multiplier_);
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
	ABIWriter->SetChannel(DeviceId, DeviceType, VarIndex, ValuePtr, ChannelIndex);
}

void CResultsWriterABI::FinishWriteHeader()
{
	ABIWriter->FinishWriteHeader();
}

void CResultsWriterABI::AddSlowVariable(ptrdiff_t nDeviceType,
	const ResultIds& DeviceIds,
	const std::string_view VariableName,
	double Time,
	double Value,
	double PreviousValue,
	const std::string_view ChangeDescription)
{
	ABIWriter->AddSlowVariable(nDeviceType,
		DeviceIds,
		VariableName,
		Time,
		Value,
		PreviousValue,
		ChangeDescription);
}