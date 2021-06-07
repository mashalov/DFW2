#ifdef _MSC_VER
#include "windows.h"
#endif

#include "DLLStructs.h"


typedef long (*DLLGETTYPESCOUNT)(void);
typedef long (*DLLGETTYPES)(long *pTypes);
typedef long (*DLLGETLINKSCOUNT)(void);
typedef long (*DLLGETLINKS)(LinkType* pLinks);
typedef const char* (*DLLGETDEVICETYPENAME)(void);

typedef long (*DLLDESTROYPTR)(void);
typedef long (*DLLGETBLOCKSCOUNT)(void);
typedef long (*DLLGETBLOCKSDESCRIPTIONS)(BlockDescriptions* pBlockDescriptions);
typedef long (*DLLGETBLOCKPINSCOUNT)(long nBlockIndex);
typedef long (*DLLGETBLOCKPINSINDEXES)(long nBlockIndex, BLOCKPININDEX* pBlockPins);
typedef long (*DLLGETBLOCKPARAMETERSCOUNT)(long nBlockIndex);
typedef long (*DLLGETBLOCKPARAMETERSVALUES)(long nBlockIndex, BuildEquationsArgs *pArgs, double* pBlockParameters);

typedef long (*DLLGETINPUTSCOUNT)(void);
typedef long (*DLLGETOUTPUTSCOUNT)(void);
typedef long (*DLLGETSETPOINTSCOUNT)(void);
typedef long (*DLLGETCONSTANTSCOUNT)(void);
typedef long (*DLLGETINTERNALSCOUNT)(void);
typedef long (*DLLGETSETPOINTSINFOS)(VarsInfo *pVarsInfo);
typedef long (*DLLGETCONSTANTSINFOS)(ConstVarsInfo *ppVarsInfo);
typedef long (*DLLGETOUTPUTSINFOS)(VarsInfo *pVarsInfo);
typedef long (*DLLGETINPUTSINFOS)(InputVarsInfo *pVarsInfo);
typedef long(*DLLGETINTERNALSINFOS)(VarsInfo *pVarsInfo);

typedef long (*DLLBUILDEQUATIONS)(BuildEquationsArgs *pArgs);
typedef long (*DLLBUILDRIGHTHAND)(BuildEquationsArgs *pArgs);
typedef long (*DLLBUILDDERIVATIVES)(BuildEquationsArgs *pArgs);
typedef long (*DLLDEVICEINIT)(BuildEquationsArgs *pArgs);
typedef long (*DLLPROCESSDISCONTINUITY)(BuildEquationsArgs *pArgs);

