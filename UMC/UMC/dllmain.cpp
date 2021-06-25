#ifdef _MSC_VER

#include "CompilerMSBuild.h"
#include <windows.h>


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) ICompiler*__cdecl CompilerFactory()
{
    return new CCompilerMSBuild();
}
#else
#include "CompilerGCC.h"
 __attribute__((visibility("default"))) ICompiler* CompilerFactory()
{
    return new CCompilerGCC();
}

#endif
