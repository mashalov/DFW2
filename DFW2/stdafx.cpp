// stdafx.cpp : source file that includes just the standard includes
// DFW2.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#ifdef _DEBUG
#ifdef _WIN64
#import "..\ResultFile\x64\debug\ResultFile.tlb" no_namespace, named_guids
#else
#import "..\ResultFile\debug\ResultFile.tlb" no_namespace, named_guids
#endif
#else
#ifdef _WIN64
#import "..\ResultFile\x64\release\ResultFile.tlb" no_namespace, named_guids
#else
#import "..\ResultFile\release\ResultFile.tlb" no_namespace, named_guids
#endif
#endif

#import "C:\Program Files (x86)\RastrWin3\astra.dll" no_namespace, named_guids, no_dual_interfaces

