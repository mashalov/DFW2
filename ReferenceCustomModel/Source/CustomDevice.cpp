#include "pch.h"
#include "CustomDevice.h"
using namespace DFW2;

#ifdef _MSC_VER
extern "C" __declspec(dllexport) ICustomDevice* __cdecl CustomDeviceFactory()
#else
extern "C" __attribute__((visibility("default"))) ICustomDevice* CustomDeviceFactory()
#endif
{
    return new CCustomDevice();
}

#ifdef _MSC_VER
extern "C" __declspec(dllexport) const char* __cdecl Source()
{
   return CCustomDevice::zipSource;
}
extern "C" __declspec(dllexport) const VersionInfo&  __cdecl ModelVersion()
{
   return CCustomDevice::modelVersion;
}
extern "C" __declspec(dllexport) const VersionInfo&  __cdecl CompilerVersion()
{
   return CCustomDevice::compilerVersion;
}
#else
extern "C" __attribute__((visibility("default"))) const char* Source()
{
    return CCustomDevice::zipSource;
}
extern "C" __attribute__((visibility("default"))) const VersionInfo& ModelVersion()
{
    return CCustomDevice::modelVersion;
}
extern "C" __attribute__((visibility("default"))) const VersionInfo& CompilerVersion()
{
    return CCustomDevice::compilerVersion;
}
#endif
