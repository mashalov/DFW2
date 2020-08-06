#include "pch.h"
#include "CustomDevice.h"
using namespace DFW2;

extern "C" __declspec(dllexport) ICustomDevice* __cdecl CustomDeviceFactory()
{
    return new CCustomDevice();
}

extern "C" __declspec(dllexport) const char* __cdecl Source()
{
    return CCustomDevice::zipSource;
}