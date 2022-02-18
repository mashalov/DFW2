#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

void CResultsWriterCOM::WriteResults(const WriteResultsInfo& WriteInfo)
{
	try
	{
		spResultWrite->WriteResults(WriteInfo.Time, WriteInfo.Step);
	}
	catch (_com_error& ex)
	{
		throw dfw2error(static_cast<const char*>(ex.Description()));
	}
}

void CResultsWriterCOM::FinishWriteResults()
{
	try
	{
		spResultWrite->Close();
	}
	catch (_com_error& ex)
	{
		throw dfw2error(static_cast<const wchar_t*>(ex.Description()));
	}
}

void CResultsWriterCOM::CreateFile(std::filesystem::path path, ResultsInfo& Info)
{
	try
	{
		if (FAILED(spResults.CreateInstance(CLSID_Result)))
			throw dfw2error(CDFW2Messages::m_cszFailedToCreateCOMResultsWriter);
		spResultWrite = spResults->Create(path.c_str());
		spResultWrite->NoChangeTolerance = Info.NoChangeTolerance;
		spResultWrite->Comment = stringutils::utf8_decode(Info.Comment).c_str();
	}
	catch (_com_error& ex)
	{
		throw dfw2error(static_cast<const wchar_t*>(ex.Description()));
	}
}

void CResultsWriterCOM::AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName)
{
	try
	{
		spResultWrite->AddVariableUnit(static_cast<long>(nUnitType), stringutils::utf8_decode(UnitName).c_str());
	}
	catch (_com_error& ex)
	{
		throw dfw2error(static_cast<const wchar_t*>(ex.Description()));
	}
}


void MakeVariant(variant_t& variant, const ResultIds& content)
{
	variant.Clear();
	if (content.size() > 1)
	{
		SAFEARRAYBOUND sabounds{ static_cast<ULONG>(content.size()), 0 };
		variant.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
		if (!variant.parray)
			throw dfw2error("MakeVariant - SafeArrayCreate failed");
		variant.vt = VT_ARRAY | VT_I4;
		int* pData(nullptr);
		if (FAILED(SafeArrayAccessData(variant.parray, (void**)&pData)))
			throw dfw2error("MakeVariant - SafeArrayAccessData failed");

		for (const auto& val : content)
		{
			*pData = static_cast<long>(val);
			pData++;
		}

		if (FAILED(SafeArrayUnaccessData(variant.parray)))
			throw dfw2error("MakeVariant - SafeArrayUnaccessData failed");
	}
	else if (content.size() == 0)
		variant = static_cast<long>(0);
	else
		variant = static_cast<long>(content.front());
};


void CResultsWriterCOM::AddDeviceType(const CDeviceContainer& Container)
{
	try
	{
		IDeviceTypeWritePtr spDeviceType = spResultWrite->AddDeviceType(static_cast<long>(Container.GetType()), stringutils::utf8_decode(Container.GetTypeName()).c_str());
		const CDeviceContainerProperties& Props{ Container.ContainerProps()};
		// по умолчанию у устройства один идентификатор и одно родительское устройство
		long DeviceIdsCount = 1;
		long ParentIdsCount = static_cast<long>(Props.Masters.size());
		// у ветви - три идентификатора
		if (Container.GetType() == DEVTYPE_BRANCH)
		{
			DeviceIdsCount = 3;
			ParentIdsCount = 2;
		}

		spDeviceType->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, static_cast<long>(Container.CountNonPermanentOff()));

		for (const auto& var : Props.VarMap_)
		{
			if (var.second.Output_)
				spDeviceType->AddDeviceTypeVariable(stringutils::utf8_decode(var.first).c_str(),
					var.second.Units_,
					var.second.Multiplier_);
		}

		for (const auto& var : Props.ConstVarMap_)
		{
			if (var.second.Output_)
				spDeviceType->AddDeviceTypeVariable(stringutils::utf8_decode(var.first).c_str(),
					var.second.Units_,
					var.second.Multiplier_);
		}

		variant_t DeviceIds, ParentIds, ParentTypes;
		
		// если у устройства более одного идентификатора, передаем их в SAFERRAY

		for (const auto& device : Container)
		{
			if (device->IsPermanentOff())
				continue;

			if (device->GetType() == DEVTYPE_BRANCH)
			{
				// для ветвей передаем номер начала, конца и номер параллельной цепи
				const CDynaBranch* pBranch = static_cast<const CDynaBranch*>(device);

				MakeVariant(DeviceIds, { pBranch->key.Ip, pBranch->key.Iq, pBranch->key.Np });
				MakeVariant(ParentIds, {
					pBranch->pNodeIp_ ? pBranch->pNodeIp_->GetId() : 0,
					pBranch->pNodeIq_ ? pBranch->pNodeIq_->GetId() : 0,
					});
				MakeVariant(ParentTypes, { DEVTYPE_NODE, DEVTYPE_NODE });

			}
			else
			{
				MakeVariant(DeviceIds, { device->GetId() });
				ResultIds idsParents, typesParents;

				for (const auto& master : Props.Masters)
				{
					CDevice* pLinkDev = device->GetSingleLink(master->nLinkIndex);
					idsParents.push_back(pLinkDev ? pLinkDev->GetId() : 0);
					typesParents.push_back(pLinkDev ? pLinkDev->GetType() : 0);
				}

				MakeVariant(ParentIds, idsParents);
				MakeVariant(ParentTypes, typesParents);
			}

			spDeviceType->AddDevice(stringutils::utf8_decode(device->GetName()).c_str(),
				DeviceIds,
				ParentIds,
				ParentTypes);
		}

	}
	catch (_com_error& ex)
	{
		throw dfw2error(static_cast<const wchar_t*>(ex.Description()));
	}
}

void CResultsWriterCOM::SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t VarIndex, double* ValuePtr, ptrdiff_t ChannelIndex)
{
	try
	{
		spResultWrite->SetChannel(
			static_cast<long>(DeviceId),
			static_cast<long>(DeviceType),
			static_cast<long>(VarIndex),
			ValuePtr,
			static_cast<long>(ChannelIndex)
		);

	}
	catch (_com_error& ex)
	{
		throw dfw2error(static_cast<const wchar_t*>(ex.Description()));
	}
}

void CResultsWriterCOM::FinishWriteHeader()
{
	try
	{
		spResultWrite->WriteHeader();
	}
	catch (_com_error& ex)
	{
		throw dfw2error(static_cast<const wchar_t*>(ex.Description()));
	}
}

void CResultsWriterCOM::AddSlowVariable(ptrdiff_t nDeviceType,
	const ResultIds& DeviceIds,
	const std::string_view VariableName,
	double Time,
	double Value,
	double PreviousValue,
	const std::string_view ChangeDescription)
{
	variant_t vtDeviceIds;
	MakeVariant(vtDeviceIds, DeviceIds);

	spResultWrite->AddSlowVariable(static_cast<long>(nDeviceType),
		vtDeviceIds,
		stringutils::utf8_decode(VariableName).c_str(),
		Time,
		Value,
		PreviousValue,
		stringutils::utf8_decode(ChangeDescription).c_str());
}