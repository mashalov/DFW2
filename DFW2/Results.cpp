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
		for (auto&& vnmit : CDFW2Messages().VarNameMap())
			m_spResultWrite->AddVariableUnit(static_cast<long>(vnmit.first), stringutils::utf8_decode(vnmit.second).c_str());


		for (auto&& it : m_DeviceContainers)
		{
			CDeviceContainer* pDevCon = it;
			// проверяем, нужно ли записывать данные для такого типа контейнера
			if (!ApproveContainerToWriteResults(pDevCon)) continue;
			// если записывать надо - добавляем тип устройства контейнера
			IDeviceTypeWritePtr spDeviceType = m_spResultWrite->AddDeviceType(it->GetType(), stringutils::utf8_decode(it->GetTypeName()).c_str());

			// по умолчанию у устройства один идентификатор и одно родительское устройство
			long DeviceIdsCount = 1;
			long ParentIdsCount = 1;

			// у ветви - три идентификатора
			if (pDevCon->GetType() == DEVTYPE_BRANCH)
				DeviceIdsCount = 3;

			CDeviceContainerProperties& Props = pDevCon->m_ContainerProps;
			// количество родительских устройств равно количеству ссылок на ведущие устройства
			ParentIdsCount = static_cast<long>(Props.m_Masters.size());

			// у ветви два ведущих узла
			if (pDevCon->GetType() == DEVTYPE_BRANCH)
				ParentIdsCount = 2;

			long nDevicesCount = static_cast<long>(std::count_if(pDevCon->begin(), pDevCon->end(), [](const CDevice* pDev)->bool {return !pDev->IsPermanentOff(); }));

			// добавляем описание устройства: количество идентификаторов, количество ведущих устройств и общее количество устройств данного типа
			spDeviceType->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, nDevicesCount);

			// добавляем описания перемнных данного контейнера
			for (auto&& vit : pDevCon->m_ContainerProps.m_VarMap)
			{
				if (vit.second.m_bOutput)
					spDeviceType->AddDeviceTypeVariable(stringutils::utf8_decode(vit.first).c_str(),
						vit.second.m_Units,
						vit.second.m_dMultiplier);
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

			for (auto&& dit : *pDevCon)
			{
				if (dit->IsPermanentOff())
					continue;

				if (pDevCon->GetType() == DEVTYPE_BRANCH)
				{
					// для ветвей передаем номер начала, конца и номер параллельной цепи
					CDynaBranch* pBranch = static_cast<CDynaBranch*>(dit);
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
					DeviceIds = dit->GetId();
					if (ParentIdsCount > 1)
					{
						int* pParentIds, * pParentTypes;
						int nIndex = 0;
						if (SUCCEEDED(SafeArrayAccessData(ParentIds.parray, (void**)&pParentIds)) &&
							SUCCEEDED(SafeArrayAccessData(ParentTypes.parray, (void**)&pParentTypes)))
						{
							for (auto&& it1 : Props.m_Masters)
							{
								CDevice* pLinkDev = dit->GetSingleLink(it1->nLinkIndex);
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
							pLinkDev = dit->GetSingleLink(Props.m_Masters[0]->nLinkIndex);

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

				spDeviceType->AddDevice(stringutils::utf8_decode(dit->GetName()).c_str(),
					DeviceIds,
					ParentIds,
					ParentTypes);
			}
		}

		m_spResultWrite->WriteHeader();

		long nIndex = 0;



		for (auto&& it : m_DeviceContainers)
		{
			CDeviceContainer* pDevCon = it;

			// собираем углы генераторов для детектора затухания колебаний
			if (m_Parameters.m_bAllowDecayDetector && pDevCon->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_MOTION))
			{
				ptrdiff_t deltaIndex(pDevCon->GetVariableIndex(CDynaNodeBase::m_cszDelta));
				if (deltaIndex >= 0)
					for (auto&& dit : *pDevCon)
						m_OscDetector.add_value_pointer(dit->GetVariableConstPtr(deltaIndex));
			}

			// устанавливаем адреса, откуда ResultWrite будет забирать значения
			// записываемых переменных

			// проверяем нужно ли писать данные от этого контейнера
			if (!ApproveContainerToWriteResults(pDevCon)) continue;

			for (auto&& dit : *pDevCon)
			{
				if (dit->IsPermanentOff())
					continue;

				long nVarIndex = 0;
				for (auto&& vit : it->m_ContainerProps.m_VarMap)
				{
					if (vit.second.m_bOutput)
						m_spResultWrite->SetChannel(static_cast<long>(dit->GetId()),
							static_cast<long>(dit->GetType()),
							nVarIndex++,
							dit->GetVariablePtr(vit.second.m_nIndex),
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
	m_spResultWrite = spResults->Create(path.c_str());
	m_spResultWrite->NoChangeTolerance = Info.NoChangeTolerance;
	m_spResultWrite->Comment = stringutils::utf8_decode(Info.Comment).c_str();
}