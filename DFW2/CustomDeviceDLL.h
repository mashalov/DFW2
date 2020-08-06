#pragma once
#include "DLLHeader.h"
#include "vector"
#include "ICustomDevice.h"
#include "DLLWrapper.h"

namespace DFW2
{
	typedef std::vector<BlockDescriptions> BLOCKDESCRIPTIONS;
	typedef std::vector<BLOCKPININDEX> BLOCKPININDEXVECTOR;
	typedef std::vector<BLOCKPININDEXVECTOR> BLOCKSPINSINDEXES;
	typedef std::vector<long> LONGVECTOR;
	class CDeviceContainer;

	using VARINFO = std::vector<VarsInfo>;
	using INPUTVARINFO = std::vector<InputVarsInfo>;
	using CONSTVARINFO = std::vector<ConstVarsInfo>;

	class CCustomDeviceDLL
	{
	protected:
		bool m_bConnected;
		HMODULE m_hDLL;
		std::wstring m_strModulePath;
		CDeviceContainer *m_pDeviceContainer;
		ptrdiff_t m_nGetProcAddresFailureCount;

		FARPROC GetProcAddress(const char *cszFunctionName);
		void CleanUp();
		void ClearFns();

		CONSTVARINFO m_ConstInfos;
		VARINFO m_SetPointInfos;
		VARINFO m_OutputInfos;
		INPUTVARINFO m_InputInfos;
		VARINFO m_InternalInfos;


		BLOCKDESCRIPTIONS m_BlockDescriptions;
		BLOCKSPINSINDEXES m_BlockPinsIndexes;

		DLLDESTROYPTR	  m_pFnDestroy;
		DLLGETBLOCKSCOUNT m_pFnGetBlocksCount;
		DLLGETBLOCKSDESCRIPTIONS m_pFnGetBlocksDescriptions;
		DLLGETBLOCKPINSCOUNT m_pFnGetBlockPinsCount;
		DLLGETBLOCKPINSINDEXES m_pFnGetBlockPinsIndexes;
		DLLGETBLOCKPARAMETERSCOUNT m_pFnGetBlockParametersCount;
		DLLGETBLOCKPARAMETERSVALUES m_pFnGetBlockParametersValues;
		DLLBUILDEQUATIONS m_pFnBuildEquations;
		DLLBUILDRIGHTHAND m_pFnBuildRightHand;
		DLLBUILDDERIVATIVES m_pFnBuildDerivatives;
		DLLDEVICEINIT m_pFnDeviceInit;
		DLLPROCESSDISCONTINUITY m_pFnProcessDiscontinuity;

		DLLGETINTERNALSCOUNT  m_pFnGetInternalsCount;
		DLLGETINPUTSCOUNT	  m_pFnGetInputsCount;
		DLLGETOUTPUTSCOUNT	  m_pFnGetOutputsCount;
		DLLGETSETPOINTSCOUNT  m_pFnGetSetPointsCount;
		DLLGETCONSTANTSCOUNT  m_pFnGetConstantsCount;

		DLLGETSETPOINTSINFOS  m_pFnGetSetPointsInfos;
		DLLGETCONSTANTSINFOS  m_pFnGetConstantsInfos;
		DLLGETOUTPUTSINFOS	  m_pFnGetOutputsInfos;
		DLLGETINPUTSINFOS	  m_pFnGetInputsInfos;
		DLLGETINTERNALSINFOS  m_pFnGetInternalsInfos;

		DLLGETTYPESCOUNT	  m_pFnGetTypesCount;
		DLLGETTYPES			  m_pFnGetTypes;
		DLLGETLINKSCOUNT	  m_pFnGetLinksCount;
		DLLGETLINKS			  m_pFnGetLinks;
		DLLGETDEVICETYPENAME  m_pFnGetDeviceTypeName;




		LONGVECTOR m_BlockParametersCount;
	public:
		CCustomDeviceDLL(CDeviceContainer *pDeviceContainer);
		virtual ~CCustomDeviceDLL();
		bool IsConnected();
		bool Init(std::wstring_view DLLFilePath);
		const BLOCKDESCRIPTIONS& GetBlocksDescriptions() const; 
		const BLOCKSPINSINDEXES& GetBlocksPinsIndexes() const;
		const _TCHAR* GetModuleFilePath() const;
		long GetBlockParametersCount(long nBlockIndex);
		long GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs* pArgs, double *pValues);

		const CONSTVARINFO& GetConstsInfo() const	{ return m_ConstInfos; };
		const VARINFO& GetSetPointsInfo() const		{ return m_SetPointInfos; }
		const VARINFO& GetOutputsInfo()	const		{ return m_OutputInfos; }
		const INPUTVARINFO& GetInputsInfo() const	{ return m_InputInfos; }
		const VARINFO& GetInternalsInfo() const		{ return m_InternalInfos; }

		bool InitEquations(BuildEquationsArgs *pArgs);
		bool ProcessDiscontinuity(BuildEquationsArgs *pArgs);
		bool BuildEquations(BuildEquationsArgs *pArgs);
		bool BuildRightHand(BuildEquationsArgs *pArgs);
		bool BuildDerivatives(BuildEquationsArgs *pArgs);
	};

	using CCustomDeviceCPPDLL = CDLLInstanceFactory<ICustomDevice>;
}

