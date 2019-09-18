#pragma once
#include "DeviceContainer.h"
#include "CustomDeviceDLL.h"

#include "LimitedLag.h"
#include "LimiterConst.h"
#include "Relay.h"
#include "RSTrigger.h"
#include "DerlagContinuous.h"
#include "Comparator.h"
#include "AndOrNot.h"
#include "Abs.h"
#include "ZCDetector.h"
#include "DeadBand.h"
#include "Expand.h"


namespace DFW2
{

	struct PrimitivePoolElement
	{
		unsigned char *m_pPrimitive;
		unsigned char *m_pHead;
		size_t nCount;
		PrimitivePoolElement() : m_pPrimitive(nullptr), nCount(0) {}
	};

	struct PrimitiveInfo
	{
		size_t nSize;
		long nEquationsCount;
		PrimitiveInfo(size_t Size, long EquationsCount) : nSize(Size),
														  nEquationsCount(EquationsCount)
		{

		}
	};

	/*
	class PrimitiveVariableExternalLink : public PrimitiveVariableExternal
	{
	public:
		ExternalVariable *m_pExternalVariable;
		PrimitiveVariableExternalLink() : m_pExternalVariable(NULL) {}
		void IndexAndValueByLink()
		{
			_ASSERTE(m_pExternalVariable);
			IndexAndValue(m_pExternalVariable->nIndex, m_pExternalVariable->pValue);
		}
	};
	*/

	class CCustomDeviceContainer : public CDeviceContainer
	{
	protected:
		CCustomDeviceDLL m_DLL;
		PrimitivePoolElement m_PrimitivePool[PrimitiveBlockType::PBT_LAST];
		
		PrimitiveVariable *m_pPrimitiveVarsPool, *m_pPrimitiveVarsHead;
		PrimitiveVariableExternal *m_pPrimitiveExtVarsPool, *m_pPrimitiveExtVarsHead;
		ExternalVariable *m_pExternalVarsPool, *m_pExternalVarsHead;

		double *m_pDoubleVarsPool, *m_pDoubleVarsHead;

		size_t m_nBlockEquationsCount;
		size_t m_nPrimitiveVarsCount;
		size_t m_nDoubleVarsCount;
		size_t m_nExternalVarsCount;

		void CleanUp();
		double *m_pParameterBuffer;
		size_t m_nParameterBufferSize;
	public:
		CCustomDeviceContainer(CDynaModel *pDynaModel);
		bool ConnectDLL(const _TCHAR *cszDLLFilePath);
		virtual ~CCustomDeviceContainer();
		bool BuildStructure();
		bool InitDLLEquations(BuildEquationsArgs *pArgs);
		bool BuildDLLEquations(BuildEquationsArgs *pArgs);
		bool BuildDLLRightHand(BuildEquationsArgs *pArgs);
		bool BuildDLLDerivatives(BuildEquationsArgs *pArgs);
		bool ProcessDLLDiscontinuity(BuildEquationsArgs *pArgs);

		PrimitiveInfo GetPrimitiveInfo(PrimitiveBlockType eType);
		size_t PrimitiveSize(PrimitiveBlockType eType);
		long PrimitiveEquationsCount(PrimitiveBlockType eType);
		PrimitiveVariable* NewPrimitiveVariable(ptrdiff_t nIndex, double& Value);
		PrimitiveVariableExternal* NewPrimitiveExtVariables();
		double* NewDoubleVariables();
		ExternalVariable* NewExternalVariables();
		void* NewPrimitive(PrimitiveBlockType eType);
		const CCustomDeviceDLL& DLL() { return m_DLL; }
		long GetParametersValues(ptrdiff_t nId, BuildEquationsArgs* pArgs, long nBlockIndex, double **ppParameters);
		inline size_t GetInputsCount()				{  return m_DLL.GetInputsCount();     }
		inline size_t GetConstsCount()				{  return m_DLL.GetConstsCount();     }
		inline size_t GetSetPointsCount()			{  return m_DLL.GetSetPointsCount();  }
	};
}

