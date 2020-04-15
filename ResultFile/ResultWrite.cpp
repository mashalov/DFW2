// ResultWrite.cpp : Implementation of CResultWrite

#include "stdafx.h"
#include "ResultWrite.h"


// CResultWrite

STDMETHODIMP CResultWrite::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IResultWrite
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// устанавливает комментарий к файлу результатов
STDMETHODIMP CResultWrite::put_Comment(BSTR Comment)
{
	HRESULT hRes = S_OK;
	m_strComment = Comment;
	return hRes;
}

// устанавливает пороговое значение для игнорирования изменений значений переменных
STDMETHODIMP CResultWrite::put_NoChangeTolerance(DOUBLE Tolerance)
{
	HRESULT hRes = S_OK;
	m_ResultFileWriter.SetNoChangeTolerance(Tolerance);
	return hRes;
}

// запись заголовка результатов
STDMETHODIMP CResultWrite::WriteHeader()
{
	HRESULT hRes = E_FAIL;
	try
	{
		// в начало заголовка записываем время создания файла результатов
		SYSTEMTIME Now;
		GetLocalTime(&Now);
		double dCurrentDate = 0;
		if (!SystemTimeToVariantTime(&Now, &dCurrentDate))
			throw CFileWriteException(NULL);

		m_ResultFileWriter.WriteDouble(dCurrentDate);						// записываем время
		m_ResultFileWriter.WriteString(m_strComment.c_str());				// записываем строку комментария
		m_ResultFileWriter.AddDirectoryEntries(3);							// описатели разделов
		m_ResultFileWriter.WriteLEB(m_VarNameMap.size());					// записываем количество единиц измерения

		// записываем последовательность типов и названий единиц измерения переменных
		for (auto&& vnmit : m_VarNameMap)
		{
			m_ResultFileWriter.WriteLEB(vnmit.first);
			m_ResultFileWriter.WriteString(vnmit.second.c_str());
		}
		// записываем количество типов устройств
		m_ResultFileWriter.WriteLEB(m_DevTypeSet.size());

		long nChannelsCount = 0;

		// записываем устройства
		for (auto &di : m_DevTypeSet)
		{
			// для каждого типа устройств
			m_ResultFileWriter.WriteLEB(di->eDeviceType);							// идентификатор типа устройства
			m_ResultFileWriter.WriteString(di->strDevTypeName.c_str());				// название типа устройства
			m_ResultFileWriter.WriteLEB(di->DeviceIdsCount);						// количество идентификаторов устройства (90% - 1, для ветвей, например - 3)
			m_ResultFileWriter.WriteLEB(di->DeviceParentIdsCount);					// количество родительских устройств
			m_ResultFileWriter.WriteLEB(di->DevicesCount);							// количество устройств данного типа
			m_ResultFileWriter.WriteLEB(di->m_VarTypes.size());						// количество переменных устройства

			long nChannelsByDevice = 0;

			// записываем описания переменных типа устройства
			for (auto &vi : di->m_VarTypesList)
			{
				m_ResultFileWriter.WriteString(vi.Name.c_str());					// имя переменной
				m_ResultFileWriter.WriteLEB(vi.eUnits);								// единицы измерения переменной
				unsigned char BitFlags = 0x0;
				// если у переменной есть множитель -
				// добавляем битовый флаг
				if (!Equal(vi.Multiplier, 1.0))
					BitFlags |= 0x1;
				m_ResultFileWriter.WriteLEB(BitFlags);								// битовые флаги переменной
				// если есть множитель
				if (BitFlags & 0x1)
					m_ResultFileWriter.WriteDouble(vi.Multiplier);					// множитель переменной

				// считаем количество каналов для данного типа устройства
				nChannelsByDevice++;			
			}

			// записываем описания устройств
			for (CResultFileReader::DeviceInstanceInfo *pDev = di->m_pDeviceInstances.get(); pDev < di->m_pDeviceInstances.get() + di->DevicesCount; pDev++)
			{
				// для каждого устройства
				// записываем последовательность идентификаторов
				for (int i = 0; i < di->DeviceIdsCount; i++)
					m_ResultFileWriter.WriteLEB(pDev->GetId(i));
				// записываем имя устройства
				m_ResultFileWriter.WriteString(pDev->Name.c_str());
				// для каждого из родительских устройств
				for (int i = 0; i < di->DeviceParentIdsCount; i++)
				{
					const CResultFileReader::DeviceLinkToParent* pDevLink = pDev->GetParent(i);
					// записываем тип родительского устройства
					m_ResultFileWriter.WriteLEB(pDevLink->m_eParentType);
					// и его идентификатор
					m_ResultFileWriter.WriteLEB(pDevLink->m_nId);
				}
				// считаем общее количество каналов
				nChannelsCount += nChannelsByDevice;
			}
		}
		// готовим компрессоры для рассчитанного количества каналов
		m_ResultFileWriter.PrepareChannelCompressor(nChannelsCount);
		hRes = S_OK;
	}

	catch (CFileWriteException& ex)
	{
		Error(ex.Message(), IID_IResultWrite, hRes);
	}

	catch (std::bad_alloc& badAllocEx)
	{
		std::string strc(badAllocEx.what());
		std::wstring str(strc.begin(), strc.end());
		Error(Cex(CDFW2Messages::m_cszMemoryAllocError, str.c_str()), IID_IResultWrite, hRes);
	}

	catch (...)
	{
		Error(CDFW2Messages::m_cszUnknownError, IID_IResultWrite, hRes);
	}

	return hRes;
}

// добавляет в карту тип и название единиц измерения
STDMETHODIMP CResultWrite::AddVariableUnit(LONG UnitId, BSTR UnitName)
{
	HRESULT hRes = S_OK;
	if (!m_VarNameMap.insert(std::make_pair(UnitId, UnitName)).second)
	{
		hRes = E_INVALIDARG;
		Error(Cex(CDFW2Messages::m_cszDuplicatedVariableUnit, UnitId), IID_IResultWrite, hRes);
	}
	return hRes;
}

STDMETHODIMP CResultWrite::AddDeviceType(LONG DeviceTypeId, BSTR DeviceTypeName, VARIANT* DeviceType)
{
	HRESULT hRes = E_FAIL;
	CResultFileReader::DeviceTypeInfo *pDeviceType = new CResultFileReader::DeviceTypeInfo();
	pDeviceType->eDeviceType = DeviceTypeId;
	pDeviceType->strDevTypeName = DeviceTypeName;
	pDeviceType->DeviceParentIdsCount = pDeviceType->DeviceIdsCount = 1;
	pDeviceType->VariablesByDeviceCount = pDeviceType->DevicesCount = 0;

	if (SUCCEEDED(VariantClear(DeviceType)))
	{
		if (m_DevTypeSet.insert(pDeviceType).second)
		{
			CComObject<CDeviceTypeWrite> *pDeviceTypeWrite;
			if (SUCCEEDED(CComObject<CDeviceTypeWrite>::CreateInstance(&pDeviceTypeWrite)))
			{
				pDeviceTypeWrite->SetDeviceTypeInfo(pDeviceType);
				pDeviceTypeWrite->AddRef();
				DeviceType->vt = VT_DISPATCH;
				DeviceType->pdispVal = pDeviceTypeWrite;
				hRes = S_OK;
			}
		}
		else
		{
			Error(Cex(CDFW2Messages::m_cszDuplicatedDeviceType, DeviceTypeId), IID_IResultWrite, hRes);
			delete pDeviceType;
		}
	}
	return hRes;
}

// задает адрес значения double, которое будет использоваться для записи заданного устройства 
STDMETHODIMP CResultWrite::SetChannel(LONG DeviceId, LONG DeviceType, LONG VarIndex, DOUBLE *ValuePtr, LONG ChannelIndex)
{
	HRESULT hRes = S_OK;
	try
	{
		m_ResultFileWriter.SetChannel(DeviceId, DeviceType, VarIndex, ValuePtr, ChannelIndex);
	}
	catch (CFileWriteException& ex)
	{
		Error(ex.Message(), IID_IResultWrite, hRes);
		hRes = E_FAIL;
	}
	return hRes;
}

// запись результатов для заданного времени. Дополнительно записывает шаг интегрирования
STDMETHODIMP CResultWrite::WriteResults(DOUBLE Time, DOUBLE Step)
{
	HRESULT hRes = S_OK;
	try
	{
		m_ResultFileWriter.WriteResults(Time, Step);
	}
	catch (CFileWriteException& ex)
	{
		Error(ex.Message(), IID_IResultWrite, hRes);
		hRes = E_FAIL;
	}
	return hRes;
}

STDMETHODIMP CResultWrite::FlushChannels()
{
	HRESULT hRes = S_OK;
	try
	{
		m_ResultFileWriter.FlushChannels();
	}
	catch (CFileWriteException& ex)
	{
		Error(ex.Message(), IID_IResultWrite, hRes);
		hRes = E_FAIL;
	}
	return hRes;
}


void CResultWrite::CreateFile(const _TCHAR* cszPathName)
{
	m_ResultFileWriter.CreateResultFile(cszPathName);
}

STDMETHODIMP CResultWrite::Close()
{
	m_ResultFileWriter.CloseFile();

	for (auto &it : m_DevTypeSet)
		delete it;
	m_DevTypeSet.clear();

	return S_OK;
}

CResultWrite::~CResultWrite()
{
	Close();
}

STDMETHODIMP CResultWrite::AddSlowVariable(LONG DeviceTypeId, VARIANT DeviceIds, BSTR VariableName, DOUBLE Time, DOUBLE Value, DOUBLE PreviousValue, BSTR ChangeDescription)
{
	HRESULT hRes = E_FAIL;
	LONGVECTOR vecDeviceIds;

	CSlowVariablesSet& SlowVariables = m_ResultFileWriter.GetSlowVariables();

	if (SlowVariables.VariantToIds(&DeviceIds, vecDeviceIds))
		if (SlowVariables.Add(DeviceTypeId, vecDeviceIds, VariableName, Time, Value, PreviousValue, ChangeDescription))
			hRes = S_OK;

	return hRes;
}