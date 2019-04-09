#include "DeviceDLLUtilities.h"

extern "C" __declspec(dllexport) long Destroy(void);

extern "C" __declspec(dllexport) long GetTypesCount(void);
extern "C" __declspec(dllexport) long GetTypes(long* pTypes);
extern "C" __declspec(dllexport) long GetLinksCount(void);
extern "C" __declspec(dllexport) long GetLinks(LinkType* pLink);
extern "C" __declspec(dllexport) const _TCHAR* GetDeviceTypeName(void);

extern "C" __declspec(dllexport) long GetBlocksCount(void);
extern "C" __declspec(dllexport) long GetBlocksDescriptions(BlockDescriptions *pBlockDescriptions);
extern "C" __declspec(dllexport) long GetBlockPinsCount(long nBlockIndex);
extern "C" __declspec(dllexport) long GetBlockPinsIndexes(long nBlockIndex, BLOCKPININDEX* pBlockPins);
extern "C" __declspec(dllexport) long GetBlockParametersCount(long nBlockIndex);
extern "C" __declspec(dllexport) long GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs *pArgs, double* pBlockParameters);

extern "C" __declspec(dllexport) long GetInputsCount(void);
extern "C" __declspec(dllexport) long GetOutputsCount(void);
extern "C" __declspec(dllexport) long GetSetPointsCount(void);
extern "C" __declspec(dllexport) long GetConstantsCount(void);
extern "C" __declspec(dllexport) long GetInternalsCount(void);

extern "C" __declspec(dllexport) long GetSetPointsInfos(VarsInfo *pVarsInfo);
extern "C" __declspec(dllexport) long GetConstantsInfos(ConstVarsInfo *pVarsInfo);
extern "C" __declspec(dllexport) long GetOutputsInfos(VarsInfo *pVarsInfo);
extern "C" __declspec(dllexport) long GetInputsInfos(InputVarsInfo *pVarsInfo);
extern "C" __declspec(dllexport) long GetInternalsInfos(VarsInfo *pVarsInfo);

extern "C" __declspec(dllexport) long BuildEquations(BuildEquationsArgs *pArgs);
extern "C" __declspec(dllexport) long BuildRightHand(BuildEquationsArgs *pArgs);
extern "C" __declspec(dllexport) long BuildDerivatives(BuildEquationsArgs *pArgs);
extern "C" __declspec(dllexport) long DeviceInit(BuildEquationsArgs *pArgs);
extern "C" __declspec(dllexport) long ProcessDiscontinuity(BuildEquationsArgs *pArgs);
