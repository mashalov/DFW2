#include "stdafx.h"
#include "DynaModel.h"
using namespace DFW2;

void CDynaModel::WriteResultsHeader()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	CResultsWriterBase::ResultsInfo resultsInfo {0.0, "Тестовая схема mdp_debug5 с КЗ"};
	m_ResultsWriter.CreateFile("c:\\tmp\\binresultCOM.rst", resultsInfo );

	// добавляем описание единиц измерения переменных
	CDFW2Messages vars;
	for (const auto& vn : vars.VarNameMap())
		m_ResultsWriter.AddVariableUnit(vn.first, vn.second);


	for (const auto& container : m_DeviceContainers)
	{
		// проверяем, нужно ли записывать данные для такого типа контейнера
		if (!ApproveContainerToWriteResults(container)) continue;
		// если записывать надо - добавляем тип устройства контейнера
		m_ResultsWriter.AddDeviceType(*container);
	}

	m_ResultsWriter.FinishWriteHeader();

	long nIndex = 0;

	for (const auto& container : m_DeviceContainers)
	{
		// собираем углы генераторов для детектора затухания колебаний
		if (m_Parameters.m_bAllowDecayDetector && container->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_MOTION))
		{
			ptrdiff_t deltaIndex(container->GetVariableIndex(CDynaNodeBase::m_cszDelta));
			if (deltaIndex >= 0)
				for (const auto& device : *container)
					m_OscDetector.add_value_pointer(device->GetVariableConstPtr(deltaIndex));
		}

		// устанавливаем адреса, откуда ResultWrite будет забирать значения
		// записываемых переменных

		// проверяем нужно ли писать данные от этого контейнера
		if (!ApproveContainerToWriteResults(container)) continue;

		for (const auto& device : *container)
		{
			if (device->IsPermanentOff())
				continue;

			long nVarIndex = 0;
			for (const auto& variable : container->m_ContainerProps.m_VarMap)
			{
				if (variable.second.m_bOutput)
				{
					m_ResultsWriter.SetChannel(device->GetId(),
						device->GetType(),
						nVarIndex++,
						device->GetVariablePtr(variable.second.m_nIndex),
						nIndex++);
				}
			}
		}
	}

	m_dTimeWritten = 0.0;
}

void CDynaModel::WriteResults()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	if (sc.m_bEnforceOut || GetCurrentTime() >= m_dTimeWritten)
	{
		m_ResultsWriter.WriteResults({ GetCurrentTime(), GetH() });
		m_dTimeWritten = GetCurrentTime() + m_Parameters.m_dOutStep;
		sc.m_bEnforceOut = false;
	}
}

void CDynaModel::FinishWriteResults()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	m_ResultsWriter.FinishWriteResults();
}

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

		auto MakeVariant = [](variant_t& variant, const std::vector<ptrdiff_t>& content)
		{
			variant.Clear();
			if (content.size() > 1)
			{
				SAFEARRAYBOUND sabounds{ static_cast<ULONG>(content.size()), 0 };
				variant.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
				if(!variant.parray)
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

				if(FAILED(SafeArrayUnaccessData(variant.parray)))
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
				std::vector<ptrdiff_t> idsParents, typesParents;

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
