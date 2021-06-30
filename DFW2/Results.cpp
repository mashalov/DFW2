#include "stdafx.h"
#include "DynaModel.h"
using namespace DFW2;

void CDynaModel::WriteResultsHeader()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	try
	{
		CResultsWriterBase::ResultsInfo resultsInfo {0.0, "Тестовая схема"};
		m_ResultsWriter.CreateFile("c:\\tmp\\binresultCOM.rst", resultsInfo );

		// добавляем описание единиц измерения переменных
		for (const auto& vn : CDFW2Messages().VarNameMap())
			m_ResultsWriter.AddVariableUnit(vn.first, vn.second);


		for (const auto& container : m_DeviceContainers)
		{
			// проверяем, нужно ли записывать данные для такого типа контейнера
			if (!ApproveContainerToWriteResults(container)) continue;
			// если записывать надо - добавляем тип устройства контейнера
			auto DeviceType = m_ResultsWriter.AddDeviceType(container->GetType(), container->GetTypeName());

			// по умолчанию у устройства один идентификатор и одно родительское устройство
			long DeviceIdsCount = 1;
			long ParentIdsCount = 1;

			// у ветви - три идентификатора
			if (container->GetType() == DEVTYPE_BRANCH)
				DeviceIdsCount = 3;

			const CDeviceContainerProperties& Props = container->m_ContainerProps;
			// количество родительских устройств равно количеству ссылок на ведущие устройства
			ParentIdsCount = static_cast<long>(Props.m_Masters.size());

			// у ветви два ведущих узла
			if (container->GetType() == DEVTYPE_BRANCH)
				ParentIdsCount = 2;

			long nDevicesCount = static_cast<long>(std::count_if(container->begin(), container->end(), [](const CDevice* pDev)->bool {return !pDev->IsPermanentOff(); }));

			// добавляем описание устройства: количество идентификаторов, количество ведущих устройств и общее количество устройств данного типа
			spDeviceType->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, nDevicesCount);

			// добавляем описания перемнных данного контейнера
			for (const auto& var : Props.m_VarMap)
			{
				if (var.second.m_bOutput)
					spDeviceType->AddDeviceTypeVariable(stringutils::utf8_decode(var.first).c_str(),
						var.second.m_Units,
						var.second.m_dMultiplier);
			}

			variant_t DeviceIds, ParentIds, ParentTypes;

			// если у устройства более одного идентификатора, передаем их в SAFERRAY
			if (DeviceIdsCount > 1)
			{
				SAFEARRAYBOUND sabounds = { static_cast<ULONG>(DeviceIdsCount), 0 };
				DeviceIds.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
				DeviceIds.vt = VT_ARRAY | VT_I4;
			}
			// если у устройства более одного ведущего, передаем их идентификаторы в SAFERRAY
			if (ParentIdsCount > 1)
			{
				SAFEARRAYBOUND sabounds = { static_cast<ULONG>(ParentIdsCount), 0 };
				ParentIds.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
				ParentIds.vt = VT_ARRAY | VT_I4;
				ParentTypes.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
				ParentTypes.vt = VT_ARRAY | VT_I4;
			}

			for (const auto& device : *container)
			{
				if (device->IsPermanentOff())
					continue;

				if (device->GetType() == DEVTYPE_BRANCH)
				{
					// для ветвей передаем номер начала, конца и номер параллельной цепи
					const CDynaBranch* pBranch = static_cast<const CDynaBranch*>(device);
					int* pDataIds;
					if (SUCCEEDED(SafeArrayAccessData(DeviceIds.parray, (void**)&pDataIds)))
					{
						pDataIds[0] = static_cast<long>(pBranch->Ip);
						pDataIds[1] = static_cast<long>(pBranch->Iq);
						pDataIds[2] = static_cast<long>(pBranch->Np);
						SafeArrayUnaccessData(DeviceIds.parray);
					}

					int* pParentIds, * pParentTypes;
					if (SUCCEEDED(SafeArrayAccessData(ParentIds.parray, (void**)&pParentIds)) &&
						SUCCEEDED(SafeArrayAccessData(ParentTypes.parray, (void**)&pParentTypes)))
					{
						if (pBranch->m_pNodeIp)
							pParentIds[0] = static_cast<long>(pBranch->m_pNodeIp->GetId());
						else
							pParentIds[0] = 0;

						if (pBranch->m_pNodeIq)
							pParentIds[1] = static_cast<long>(pBranch->m_pNodeIq->GetId());
						else
							pParentIds[1] = 0;

						pParentTypes[0] = pParentTypes[1] = DEVTYPE_NODE;

						SafeArrayUnaccessData(ParentIds.parray);
						SafeArrayUnaccessData(ParentTypes.parray);
					}
				}
				else
				{
					DeviceIds = device->GetId();
					if (ParentIdsCount > 1)
					{
						int* pParentIds, * pParentTypes;
						int nIndex = 0;
						if (SUCCEEDED(SafeArrayAccessData(ParentIds.parray, (void**)&pParentIds)) &&
							SUCCEEDED(SafeArrayAccessData(ParentTypes.parray, (void**)&pParentTypes)))
						{
							for (const auto& master : Props.m_Masters)
							{
								CDevice* pLinkDev = device->GetSingleLink(master->nLinkIndex);
								if (pLinkDev)
								{
									pParentTypes[nIndex] = static_cast<long>(pLinkDev->GetType());
									pParentIds[nIndex] = static_cast<long>(pLinkDev->GetId());
								}
								else
								{
									pParentTypes[nIndex] = pParentIds[nIndex] = 0;
								}
								nIndex++;
							}
							SafeArrayUnaccessData(ParentIds.parray);
							SafeArrayUnaccessData(ParentTypes.parray);
						}
					}
					else
					{
						CDevice* pLinkDev(nullptr);
						if (!Props.m_Masters.empty())
							pLinkDev = device->GetSingleLink(Props.m_Masters[0]->nLinkIndex);

						if (pLinkDev)
						{
							ParentIds = pLinkDev->GetId();
							ParentTypes = pLinkDev->GetType();
						}
						else
						{
							ParentIds = 0;
							ParentTypes = 0;
						}

					}
				}

				spDeviceType->AddDevice(stringutils::utf8_decode(device->GetName()).c_str(),
					DeviceIds,
					ParentIds,
					ParentTypes);
			}
		}

		m_ResultsWriter.WriteHeader();

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
						m_ResultsWriter.SetChannel(device->GetId(),
							device->GetType(),
							nVarIndex++,
							device->GetVariablePtr(variable.second.m_nIndex),
							nIndex++);
				}
			}
		}
	}
	catch (_com_error& ex)
	{
		throw dfw2error(ex.Description());
	}

	m_ResultsWriter.WriteResultsHeader();
	m_dTimeWritten = 0.0;
}

void CDynaModel::WriteResults()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	if (sc.m_bEnforceOut || GetCurrentTime() >= m_dTimeWritten)
	{
		m_ResultsWriter.WriteResults(GetCurrentTime(), GetH());
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

void CResultsWriterCOM::WriteResults(double Time, double Step)
{
	try
	{
		m_spResultWrite->WriteResults(Time, Step);
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

void CResultsWriterCOM::WriteResultsHeader()
{
	if (FAILED(spResults.CreateInstance(CLSID_Result)))
		throw dfw2error(CDFW2Messages::m_cszFailedToCreateCOMResultsWriter);
}

void CResultsWriterCOM::CreateFile(std::filesystem::path path, ResultsInfo& Info)
{
	try
	{
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


CResultsWriterBase::DeviceType& CResultsWriterCOM::AddDeviceType(ptrdiff_t nDeviceType, const std::string_view DeviceTypeName)
{
	try
	{
		//IDeviceTypeWritePtr spDeviceType = 
		m_spResultWrite->AddDeviceType(static_cast<long>(nDeviceType), stringutils::utf8_decode(DeviceTypeName).c_str());
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

void CResultsWriterCOM::WriteHeader()
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
