
#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#include <iostream>
#include "CompilerMSBuild.h"

#ifdef _MSC_VER
#include<windows.h>
#endif

int main()
{
#ifdef _MSC_VER    
    setvbuf(stdout, nullptr, _IOFBF, 1000);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(485421);
#endif    
    auto compiler = std::make_unique<CCompilerMSBuild>();
    compiler->SetProperty(PropertyMap::szPropRebuild, "yes");
    compiler->Compile("..\\UMC\\original_andornot.txt");
}
