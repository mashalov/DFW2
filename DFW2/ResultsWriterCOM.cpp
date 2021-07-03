#include "stdafx.h"
#include "DynaModel.h"

using namespace DFW2;

void CResultsWriterCOM::WriteResults(const WriteResultsInfo& WriteInfo)
{
	try
	{
		m_spResultWrite->WriteResults(WriteInfo.Time, WriteInfo.Step);
	}
	catch (_com_error& ex)
	{
		throw dfw2error(ex.Description());
	}
}

void CResultsWriterCOM::FinishWriteResults()
{
	try
	{
		m_spResultWrite->Close();
	}
	catch (_com_error& ex)
	{
		throw dfw2error(ex.Description());
	}
}

void CResultsWriterCOM::CreateFile(std::filesystem::path path, ResultsInfo& Info)
{
	try
	{
		if (FAILED(spResults.CreateInstance(CLSID_Result)))
			throw dfw2error(CDFW2Messages::m_cszFailedToCreateCOMResultsWriter);
		m_spResultWrite = spResults->Create(path.c_str());
		m_spResultWrite->NoChangeTolerance = Info.NoChangeTolerance;
		m_spResultWrite->Comment = stringutils::utf8_decode(Info.Comment).c_str();
	}
	catch (_com_error& ex)
	{
		throw dfw2error(ex.Description());
	}
}

void CResultsWriterCOM::AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName)
{
	try
	{
		m_spResultWrite->AddVariableUnit(static_cast<long>(nUnitType), stringutils::utf8_decode(UnitName).c_str());
	}
	catch (_com_error& ex)
	{
		throw dfw2error(ex.Description());
	}
}


void CResultsWriterCOM::AddDeviceType(const CDeviceContainer& Container)
{
	try
	{
		IDeviceTypeWritePtr spDeviceType = m_spResultWrite->AddDeviceType(static_cast<long>(Container.GetType()), stringutils::utf8_decode(Container.GetTypeName()).c_str());
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

		spDeviceType->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, static_cast<long>(Container.CountNonPermanentOff()));

		for (const auto& var : Props.m_VarMap)
		{
			if (var.second.m_bOutput)
				spDeviceType->AddDeviceTypeVariable(stringutils::utf8_decode(var.first).c_str(),
					var.second.m_Units,
					var.second.m_dMultiplier);
		}

		variant_t DeviceIds, ParentIds, ParentTypes;

		auto MakeVariant = [](variant_t& variant, const ResultIds& content)
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

		// если у устройства более одного идентификатора, передаем их в SAFERRAY

		for (const auto& device : Container)
		{
			if (device->IsPermanentOff())
				continue;

			if (device->GetType() == DEVTYPE_BRANCH)
			{
				// для ветвей передаем номер начала, конца и номер параллельной цепи
				const CDynaBranch* pBranch = static_cast<const CDynaBranch*>(device);

				MakeVariant(DeviceIds, { pBranch->Ip, pBranch->Iq, pBranch->Np });
				MakeVariant(ParentIds, {
					pBranch->m_pNodeIp ? pBranch->m_pNodeIp->GetId() : 0,
					pBranch->m_pNodeIq ? pBranch->m_pNodeIq->GetId() : 0,
					});
				MakeVariant(ParentTypes, { DEVTYPE_NODE, DEVTYPE_NODE });

			}
			else
			{
				MakeVariant(DeviceIds, { device->GetId() });
				ResultIds idsParents, typesParents;

				for (const auto& master : Props.m_Masters)
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
		throw dfw2error(ex.Description());
	}
}

void CResultsWriterCOM::SetChannel(ptrdiff_t DeviceId, ptrdiff_t DeviceType, ptrdiff_t VarIndex, double* ValuePtr, ptrdiff_t ChannelIndex)
{
	try
	{
		m_spResultWrite->SetChannel(
			static_cast<long>(DeviceId),
			static_cast<long>(DeviceType),
			static_cast<long>(VarIndex),
			ValuePtr,
			static_cast<long>(ChannelIndex)
		);

	}
	catch (_com_error& ex)
	{
		throw dfw2error(ex.Description());
	}
}

void CResultsWriterCOM::FinishWriteHeader()
{
	try
	{
		m_spResultWrite->WriteHeader();
	}
	catch (_com_error& ex)
	{
		throw dfw2error(ex.Description());
	}
}
