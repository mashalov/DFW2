// DeviceTypes.cpp : Implementation of CDeviceTypes

#include "stdafx.h"
#include "DeviceTypes.h"


// CDeviceTypes

STDMETHODIMP CDeviceTypes::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IDeviceTypes
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
