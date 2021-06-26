
#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#include <iostream>
#include "CompilerMSBuild.h"
#include "CompilerGCC.h"

#ifdef _MSC_VER
#include<windows.h>
#endif

int main()
{
#ifdef _MSC_VER    
    setvbuf(stdout, nullptr, _IOFBF, 1000);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(485421);
    auto compiler = std::make_unique<CCompilerMSBuild>();
#else
    auto compiler = std::make_unique<CCompilerGCC>();
#endif
    compiler->SetProperty(PropertyMap::szPropRebuild, "yes");
    compiler->Compile("automatic");
}
