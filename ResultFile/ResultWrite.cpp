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
	HRESULT hRes{ S_OK };
	ResultFileWriter_.SetComment(stringutils::utf8_encode(Comment));
	return hRes;
}

// устанавливает пороговое значение для игнорирования изменений значений переменных
STDMETHODIMP CResultWrite::put_NoChangeTolerance(DOUBLE Tolerance)
{
	HRESULT hRes{ S_OK };
	ResultFileWriter_.SetNoChangeTolerance(Tolerance);
	return hRes;
}

// запись заголовка результатов
STDMETHODIMP CResultWrite::WriteHeader()
{
	HRESULT hRes{ E_FAIL };
	try
	{
		ResultFileWriter_.FinishWriteHeader();
		hRes = S_OK;
	}

	catch (CFileWriteException& ex)
	{
		Error(ex.whatw(), IID_IResultWrite, hRes = E_FAIL);
	}

	catch (std::bad_alloc& badAllocEx)
	{
		std::string strc(badAllocEx.what());
		std::string str(strc.begin(), strc.end());
		Error(fmt::format(CDFW2Messages::m_cszMemoryAllocError, str).c_str(), IID_IResultWrite, hRes = E_FAIL);
	}

	catch (...)
	{
		Error(CDFW2Messages::m_cszUnknownError, IID_IResultWrite, hRes = E_FAIL);
	}

	return hRes;
}

// добавляет в карту тип и название единиц измерения
STDMETHODIMP CResultWrite::AddVariableUnit(LONG UnitId, BSTR UnitName)
{
	HRESULT hRes{ S_OK };
	try
	{
		ResultFileWriter_.AddVariableUnit(UnitId, stringutils::utf8_encode(UnitName));
	}
	catch (const dfw2error& er)
	{
		Error(stringutils::utf8_decode(er.what()).c_str(), IID_IResultWrite, hRes = E_INVALIDARG);
	}

	return hRes;
}

STDMETHODIMP CResultWrite::AddDeviceType(LONG DeviceTypeId, BSTR DeviceTypeName, VARIANT* DeviceType)
{
	HRESULT hRes{ E_FAIL };

	try
	{
		auto pDeviceType{ ResultFileWriter_.AddDeviceType(DeviceTypeId, stringutils::utf8_encode(DeviceTypeName)) };
		CComObject<CDeviceTypeWrite>* pDeviceTypeWrite;
		if (SUCCEEDED(CComObject<CDeviceTypeWrite>::CreateInstance(&pDeviceTypeWrite)))
		{
			pDeviceTypeWrite->SetDeviceTypeInfo(pDeviceType);
			pDeviceTypeWrite->AddRef();
			DeviceType->vt = VT_DISPATCH;
			DeviceType->pdispVal = pDeviceTypeWrite;
			hRes = S_OK;
		}
	}
	catch (const dfw2error& er)
	{
		Error(er.whatw(), IID_IResultWrite, hRes = E_FAIL);
	}
	return hRes;
}

// задает адрес значения double, которое будет использоваться для записи заданного устройства 
STDMETHODIMP CResultWrite::SetChannel(LONG DeviceId, LONG DeviceType, LONG VarIndex, DOUBLE *ValuePtr, LONG ChannelIndex)
{
	HRESULT hRes{ S_OK };
	try
	{
		ResultFileWriter_.SetChannel(DeviceId, DeviceType, VarIndex, ValuePtr, ChannelIndex);
	}
	catch (CFileWriteException& ex)
	{
		Error(ex.whatw(), IID_IResultWrite, hRes = E_FAIL);
	}
	return hRes;
}

// запись результатов для заданного времени. Дополнительно записывает шаг интегрирования
STDMETHODIMP CResultWrite::WriteResults(DOUBLE Time, DOUBLE Step)
{
	HRESULT hRes{ S_OK };
	try
	{
		ResultFileWriter_.WriteResults(Time, Step);
	}
	catch (CFileWriteException& ex)
	{
		Error(ex.whatw(), IID_IResultWrite, hRes = E_FAIL);
	}
	return hRes;
}

STDMETHODIMP CResultWrite::FlushChannels()
{
	HRESULT hRes{ S_OK };
	try
	{
		ResultFileWriter_.FlushChannels();
	}
	catch (CFileWriteException& ex)
	{
		Error(ex.whatw(), IID_IResultWrite, hRes = E_FAIL);
	}
	return hRes;
}


void CResultWrite::CreateFile(std::string_view PathName)
{
	ResultFileWriter_.CreateResultFile(PathName);
}

STDMETHODIMP CResultWrite::Close()
{
	ResultFileWriter_.Close();
	return S_OK;
}

CResultWrite::~CResultWrite()
{
	Close();
}

STDMETHODIMP CResultWrite::AddSlowVariable(LONG DeviceTypeId, VARIANT DeviceIds, BSTR VariableName, DOUBLE Time, DOUBLE Value, DOUBLE PreviousValue, BSTR ChangeDescription)
{
	HRESULT hRes{ E_FAIL };
	try
	{
		ResultFileWriter_.AddSlowVariable(DeviceTypeId,
			CDeviceTypeWrite::GetVariantVec(DeviceIds),
			stringutils::utf8_encode(VariableName),
			Time,
			Value,
			PreviousValue,
			stringutils::utf8_encode(ChangeDescription)
		);
		hRes = S_OK;
	}
	catch (const dfw2error& ex)
	{
		Error(ex.whatw(), IID_IResultWrite, hRes = E_FAIL);
	}

	return hRes;
}