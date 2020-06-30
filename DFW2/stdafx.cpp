// stdafx.cpp : source file that includes just the standard includes
// DFW2.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#ifdef _DEBUG
#ifdef _WIN64
#import "..\Compiler\x64\debug\Compiler.tlb" no_namespace, named_guids
#import "..\ResultFile\x64\debug\ResultFile.tlb" no_namespace, named_guids
#else
#import "..\Compiler\debug\Compiler.tlb" no_namespace, named_guids
#import "..\ResultFile\debug\ResultFile.tlb" no_namespace, named_guids
#endif
#else
#ifdef _WIN64
#import "..\Compiler\x64\release\Compiler.tlb" no_namespace, named_guids
#import "..\ResultFile\x64\release\ResultFile.tlb" no_namespace, named_guids
#else
#import "..\Compiler\release\Compiler.tlb" no_namespace, named_guids
#import "..\ResultFile\release\ResultFile.tlb" no_namespace, named_guids
#endif
#endif

#import "C:\Program Files (x86)\RastrWin3\astra.dll" no_namespace, named_guids, no_dual_interfaces
#import <msxml6.dll> named_guids

