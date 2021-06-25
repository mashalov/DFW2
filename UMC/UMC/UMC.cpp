
#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#include<windows.h>
#endif
#include <iostream>
#include "CompilerBase.h"


int main()
{
#ifdef _MSC_VER    
    setvbuf(stdout, nullptr, _IOFBF, 1000);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(485421);
#endif    
    auto compiler = std::make_unique<CompilerBase>();
    compiler->SetProperty(PropertyMap::szPropRebuild, "yes");
    compiler->Compile("..\\UMC\\original_andornot.txt");
}
