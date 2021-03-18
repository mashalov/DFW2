#pragma once
#ifdef _MSC_VER
#ifdef _DEBUG
#ifdef _WIN64
#include "x64\debug\resultfile.tlh"
#else
#include "debug\resultfile.tlh"
#endif
#else
#ifdef _WIN64
#include "x64\release\resultfile.tlh"
#else
#include "release\resultfile.tlh"
#endif
#endif
#endif