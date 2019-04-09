#pragma once
#include "DLLHeader.h"
#include "vector"

namespace DFW2
{
	using namespace std;

	typedef vector<BlockDescriptions> BLOCKDESCRIPTIONS;
	typedef vector<BLOCKPININDEX> BLOCKPININDEXVECTOR;
	typedef vector<BLOCKPININDEXVECTOR> BLOCKSPINSINDEXES;
	typedef vector<long> LONGVECTOR;

	template <class T>
	class VarInfo
	{
	public:
		size_t m_nVarsCount;
		T *m_pVarInfo;
		VarInfo() : m_nVarsCount(0), m_pVarInfo(NULL) {}

		size_t SetVarsCount(size_t nVarsCount)
		{
			if (nVarsCount > 0)
				m_pVarInfo = new T[m_nVarsCount = nVarsCount];
			return m_nVarsCount;
		}

		~VarInfo()
		{
			if (m_nVarsCount && m_pVarInfo)
				delete[] m_pVarInfo;
		}
	};

	class CDeviceContainer;

	typedef VarInfo<VarsInfo> VARINFO;
	typedef VarInfo<InputVarsInfo> INPUTVARINFO;
	typedef VarInfo<ConstVarsInfo> CONSTVARINFO;

	class CCustomDeviceDLL
	{
	protected:
		bool m_bConnected;
		HMODULE m_hDLL;
		wstring m_strModulePath;
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
		bool Init(const _TCHAR *cszDLLFilePath);
		const BLOCKDESCRIPTIONS& GetBlocksDescriptions() const; 
		const BLOCKSPINSINDEXES& GetBlocksPinsIndexes() const;
		const _TCHAR* GetModuleFilePath() const;
		long GetBlockParametersCount(long nBlockIndex);
		long GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs* pArgs, double *pValues);

		inline size_t GetConstsCount()	   const { return m_ConstInfos.m_nVarsCount;    }
		inline size_t GetSetPointsCount()  const { return m_SetPointInfos.m_nVarsCount; }
		inline size_t GetInternalsCount()  const { return m_InternalInfos.m_nVarsCount; }
		inline size_t GetInputsCount()	   const { return m_InputInfos.m_nVarsCount;   }
		inline size_t GetOutputsCount()	   const { return m_OutputInfos.m_nVarsCount; }

		const ConstVarsInfo *GetConstInfo(size_t nConstIndex) const;
		const InputVarsInfo *GetInputInfo(size_t nInputIndex) const;
		const VarsInfo *GetSetPointInfo(size_t nSetPointIndex) const;
		const VarsInfo* GetInternalInfo(size_t nInternalIndex) const;
		const VarsInfo* GetOutputInfo(size_t nOutputIndex) const;

		bool InitEquations(BuildEquationsArgs *pArgs);
		bool ProcessDiscontinuity(BuildEquationsArgs *pArgs);
		bool BuildEquations(BuildEquationsArgs *pArgs);
		bool BuildRightHand(BuildEquationsArgs *pArgs);
		bool BuildDerivatives(BuildEquationsArgs *pArgs);


	};
}

