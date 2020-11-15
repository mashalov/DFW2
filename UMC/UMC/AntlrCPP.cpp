// AntlrCPP.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

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
    //compiler->Compile("..\\AntlrCPP\\automatic.txt");
    //compiler->Compile("..\\AntlrCPP\\automatic_small.txt");
    compiler->SetProperty(PropertyMap::szPropRebuild, "yes");
    compiler->Compile("..\\AntlrCPP\\original_andornot.txt");
}
