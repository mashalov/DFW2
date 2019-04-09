#include "DeviceDLLUtilities.h"
#include "windows.h"

long __declspec(dllexport) Destroy(void);

long __declspec(dllexport) GetTypesCount(void);
long __declspec(dllexport) GetTypes(long* pTypes);
long __declspec(dllexport) GetLinksCount(void);
long __declspec(dllexport) GetLinks(LinkType* pLink);
const _TCHAR* __declspec(dllexport) GetDeviceTypeName(void);

long __declspec(dllexport) GetBlocksCount(void);
long __declspec(dllexport) GetBlocksDescriptions(BlockDescriptions *pBlockDescriptions);
long __declspec(dllexport) GetBlockPinsCount(long nBlockIndex);
long __declspec(dllexport) GetBlockPinsIndexes(long nBlockIndex, BLOCKPININDEX* pBlockPins);
long __declspec(dllexport) GetBlockParametersCount(long nBlockIndex);
long __declspec(dllexport) GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs *pArgs, double* pBlockParameters);

long __declspec(dllexport) GetInputsCount(void);
long __declspec(dllexport) GetOutputsCount(void);
long __declspec(dllexport) GetSetPointsCount(void);
long __declspec(dllexport) GetConstantsCount(void);
long __declspec(dllexport) GetInternalsCount(void);

long __declspec(dllexport) GetSetPointsInfos(VarsInfo *pVarsInfo);
long __declspec(dllexport) GetConstantsInfos(ConstVarsInfo *pVarsInfo);
long __declspec(dllexport) GetOutputsInfos(VarsInfo *pVarsInfo);
long __declspec(dllexport) GetInputsInfos(InputVarsInfo *pVarsInfo);
long __declspec(dllexport) GetInternalsInfos(VarsInfo *pVarsInfo);

long __declspec(dllexport) BuildEquations(BuildEquationsArgs *pArgs);
long __declspec(dllexport) BuildRightHand(BuildEquationsArgs *pArgs);
long __declspec(dllexport) BuildDerivatives(BuildEquationsArgs *pArgs);
long __declspec(dllexport) DeviceInit(BuildEquationsArgs *pArgs);
long __declspec(dllexport) ProcessDiscontinuity(BuildEquationsArgs *pArgs);
