
#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>
#include "CompilerBase.h"

#include<windows.h>
int main()
{
    setvbuf(stdout, nullptr, _IOFBF, 1000);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(485421);
    auto compiler = std::make_unique<CompilerBase>();
    compiler->SetProperty(PropertyMap::szPropRebuild, "yes");
    compiler->Compile("..\\UMC\\original_andornot.txt");
}
