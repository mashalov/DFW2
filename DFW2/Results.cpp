#include "stdafx.h"
#include "DynaModel.h"
#import "..\ResultFile\ResultFile.tlb" no_namespace, named_guids, no_implementation

using namespace DFW2;

//#define _WRITE_CSV

bool CDynaModel::WriteResultsHeaderBinary()
{
	bool bRes = true;
	IResultPtr spResults;
	CDFW2Messages DFWMessages;

	if (SUCCEEDED(spResults.CreateInstance(CLSID_Result)))
	{
		try
		{
			m_spResultWrite = spResults->Create(_T("c:\\tmp\\binresultCOM.rst"));
			m_spResultWrite->NoChangeTolerance = GetAtol();
			m_spResultWrite->Comment = _T("Тестовая схема mdp_debug5 с КЗ");

			for (VARNAMEITRCONST vnmit = DFWMessages.VarNameMap().begin(); vnmit != DFWMessages.VarNameMap().end(); vnmit++)
				m_spResultWrite->AddVariableUnit(static_cast<long>(vnmit->first), vnmit->second.c_str());


			for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
			{
				CDeviceContainer *pDevCon = *it;
				if (!ApproveContainerToWriteResults(pDevCon)) continue;
				IDeviceTypeWritePtr spDeviceType = m_spResultWrite->AddDeviceType((*it)->GetType(), (*it)->GetTypeName()); 

				long DeviceIdsCount = 1;
				long ParentIdsCount = 1;

				if (pDevCon->GetType() == DEVTYPE_BRANCH)
					DeviceIdsCount = 3;

				CDeviceContainerProperties &Props = pDevCon->m_ContainerProps;
				LINKSTOMAP	 &LinksTo = Props.m_MasterLinksTo;
				LINKSFROMMAP &LinksFrom = Props.m_MasterLinksFrom;
				ParentIdsCount = static_cast<long>(LinksTo.size() + LinksFrom.size());

				if (pDevCon->GetType() == DEVTYPE_BRANCH)
					ParentIdsCount = 2;

				spDeviceType->SetDeviceTypeMetrics(DeviceIdsCount, ParentIdsCount, static_cast<long>(pDevCon->Count()));

				for (VARINDEXMAPCONSTITR vit = pDevCon->VariablesBegin(); vit != pDevCon->VariablesEnd(); vit++)
				{
					if (vit->second.m_bOutput)
						spDeviceType->AddDeviceTypeVariable(vit->first.c_str(), vit->second.m_Units, vit->second.m_dMultiplier);
				}

				variant_t DeviceIds, ParentIds, ParentTypes;

				if (DeviceIdsCount > 1)
				{
					SAFEARRAYBOUND sabounds = { static_cast<ULONG>(DeviceIdsCount), 0 };
					DeviceIds.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
					DeviceIds.vt = VT_ARRAY | VT_I4;
				}

				if (ParentIdsCount > 1)
				{
					SAFEARRAYBOUND sabounds = { static_cast<ULONG>(ParentIdsCount), 0 };
					ParentIds.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
					ParentIds.vt = VT_ARRAY | VT_I4;
					ParentTypes.parray = SafeArrayCreate(VT_I4, 1, &sabounds);
					ParentTypes.vt = VT_ARRAY | VT_I4;
				}

				for (DEVICEVECTORITR dit = pDevCon->begin(); dit != pDevCon->end(); dit++)
				{
					CDevice *pDev = *dit;
					if (pDevCon->GetType() == DEVTYPE_BRANCH)
					{
						CDynaBranch *pBranch = static_cast<CDynaBranch*>(pDev);
						int *pDataIds;
						if (SUCCEEDED(SafeArrayAccessData(DeviceIds.parray, (void**)&pDataIds)))
						{
							pDataIds[0] = static_cast<long>(pBranch->Ip);
							pDataIds[1] = static_cast<long>(pBranch->Iq);
							pDataIds[2] = static_cast<long>(pBranch->Np);
							SafeArrayUnaccessData(DeviceIds.parray);
						}

						int *pParentIds, *pParentTypes;
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
						DeviceIds = pDev->GetId();
						if (ParentIdsCount > 1)
						{
							int *pParentIds, *pParentTypes;
							int nIndex = 0;
							if (SUCCEEDED(SafeArrayAccessData(ParentIds.parray, (void**)&pParentIds)) &&
								SUCCEEDED(SafeArrayAccessData(ParentTypes.parray, (void**)&pParentTypes)))
							{
								for (LINKSTOMAPITR it1 = LinksTo.begin(); it1 != LinksTo.end(); it1++)
								{
									CDevice *pLinkDev = pDev->GetSingleLink(it1->first);
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

								for (LINKSFROMMAPITR it2 = LinksFrom.begin(); it2 != LinksFrom.end(); it2++)
								{
									CDevice *pLinkDev = pDev->GetSingleLink(it2->first);
									if (pLinkDev)
									{
										pParentTypes[nIndex] = static_cast<long>(pLinkDev->GetType());
										pParentIds[nIndex] = static_cast<long>(pLinkDev->GetId());
									}
									else
									{
										pParentTypes[nIndex] = pParentIds[nIndex] = 0;
									}
								}

								SafeArrayUnaccessData(ParentIds.parray);
								SafeArrayUnaccessData(ParentTypes.parray);
							}
						}
						else
						{
							CDevice *pLinkDev = NULL; 

							if (!LinksFrom.empty())
								pLinkDev = pDev->GetSingleLink(LinksFrom.begin()->first);
							else if (!LinksTo.empty())
								pLinkDev = pDev->GetSingleLink(LinksTo.begin()->first);

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

					spDeviceType->AddDevice(pDev->GetName(), DeviceIds, ParentIds, ParentTypes);
				}
			}
				
			m_spResultWrite->WriteHeader();

			long nIndex = 0;

			for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
			{
				CDeviceContainer *pDevCon = *it;
				if (!ApproveContainerToWriteResults(pDevCon)) continue;

				for (DEVICEVECTORITR dit = pDevCon->begin(); dit != pDevCon->end(); dit++)
				{
					long nVarIndex = 0;
					for (VARINDEXMAPCONSTITR vit = (*it)->VariablesBegin(); vit != (*it)->VariablesEnd(); vit++)
						if (vit->second.m_bOutput)
							m_spResultWrite->SetChannel(static_cast<long>((*dit)->GetId()), 
														static_cast<long>((*dit)->GetType()), 
														nVarIndex++, 
														(*dit)->GetVariablePtr(vit->second.m_nIndex), 
														nIndex++);
				}
			}
		}
		catch (_com_error& ex)
		{
			Log(CDFW2Messages::DFW2LOG_ERROR, ex.Description());
			bRes = false;
		}
	}
	return bRes;
}

bool CDynaModel::WriteResultsHeader()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return true;

	bool bRes = false;
	setlocale(LC_ALL, "RU-ru");

#ifdef _WRITE_CSV
	if (!_tfopen_s(&fResult, _T("c:\\tmp\\results.csv"), _T("wb+")))
	{
		bRes = true;
		_ftprintf_s(fResult, _T("t;h;"));
		for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
		{
			for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
			{
				for (VARINDEXMAPCONSTITR vit = (*it)->VariablesBegin(); vit != (*it)->VariablesEnd(); vit++)
				{
					if (vit->second.m_bOutput)
					{
						wstring fltr = vit->first;
						std::replace(fltr.begin(), fltr.end(), ';', ':');

						wstring name = std::to_wstring((*dit)->GetId());

						std::replace(name.begin(), name.end(), ';', ':');
						_ftprintf_s(fResult, _T("\"%s [%s]\";"), fltr.c_str(), name.c_str());
					}
				}
			}
		}
		_ftprintf_s(fResult, _T("\n"));
	}
#endif

	bRes = true;

	m_dTimeWritten = 0.0;
	if (bRes)
		bRes = WriteResultsHeaderBinary();
	return bRes;
}

bool CDynaModel::WriteResults()
{
	bool bRes = true;
	if (m_Parameters.m_bDisableResultsWriter)
		return bRes;

	try
	{
		if (sc.m_bEnforceOut || GetCurrentTime() >= m_dTimeWritten)
		{

#ifdef _WRITE_CSV
			_ftprintf_s(fResult, _T("%g;%g;"), sc.t, sc.m_dCurrentH);
			ptrdiff_t nIndex = 0;
			for (DEVICECONTAINERITR it = m_DeviceContainers.begin(); it != m_DeviceContainers.end(); it++)
			{
				for (DEVICEVECTORITR dit = (*it)->begin(); dit != (*it)->end(); dit++)
				{
					for (VARINDEXMAPCONSTITR vit = (*it)->VariablesBegin(); vit != (*it)->VariablesEnd(); vit++)
					{
						if (vit->second.m_bOutput)
						{
							_ftprintf_s(fResult, _T("%g;"), (*dit)->GetValue(vit->second.m_nIndex));
						}
					}
				}
			}
			_ftprintf_s(fResult, _T("\n"));
#endif
			m_spResultWrite->WriteResults(GetCurrentTime(), GetH());

			m_dTimeWritten = GetCurrentTime() + m_Parameters.m_dOutStep;
			sc.m_bEnforceOut = false;
		}
	}
	catch (_com_error & ex)
	{
		Log(CDFW2Messages::DFW2LOG_ERROR, ex.Description());
		bRes = false;
	}

	return bRes;
}


bool CDynaModel::FinishWriteResults()
{
	bool bRes = true;

	if (m_Parameters.m_bDisableResultsWriter)
		return true;

#ifdef _WRITE_CSV
	fclose(fResult);
#endif

	// сброс результатов делается автоматически при вызове Close
	//m_spResultWrite->FlushChannels();
	m_spResultWrite->Close();
	return bRes;
}

