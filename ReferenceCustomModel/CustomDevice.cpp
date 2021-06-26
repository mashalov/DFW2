#include "pch.h"
#include "CustomDevice.h"
using namespace DFW2;

#ifdef _MSC_VER
extern "C" __declspec(dllexport) ICustomDevice* __cdecl CustomDeviceFactory()
#else
 __attribute__((visibility("default"))) ICustomDevice* CustomDeviceFactory()
#endif

{
    return new CCustomDevice();
}

#ifdef _MSC_VER
extern "C" __declspec(dllexport) const char* __cdecl Source()
#else
 __attribute__((visibility("default"))) const char* Source()
#endif
{
    return CCustomDevice::zipSource;
}