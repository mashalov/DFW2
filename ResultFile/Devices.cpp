// DeviceCollection.cpp : Implementation of CDeviceCollection

#include "stdafx.h"
#include "Devices.h"


// CDevices

STDMETHODIMP CDevices::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_IDevices
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
