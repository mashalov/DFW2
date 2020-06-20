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

	// пул памяти для хост-блоков
	struct PrimitivePoolElement
	{
		unsigned char *m_pPrimitive;			// начало пула
		unsigned char *m_pHead;					// текущая позиция в пуле
		size_t nCount;							// количество элементов в пуле
		PrimitivePoolElement() : m_pPrimitive(nullptr), 
							     nCount(0) {}
	};

	// описание хост-блока
	struct PrimitiveInfo
	{
		size_t nSize;							// размер в байтах
		long nEquationsCount;					// количество уравнений
		PrimitiveInfo(size_t Size, long EquationsCount) : nSize(Size),
														  nEquationsCount(EquationsCount) {}
	};

	class CCustomDeviceContainer : public CDeviceContainer
	{
	protected:
		CCustomDeviceDLL m_DLL;
		// пулы для всех устройств контейнера
		PrimitivePoolElement m_PrimitivePool[PrimitiveBlockType::PBT_LAST];			// таблица пулов для каждого типа хост-блоков
		std::vector<PrimitiveVariable> m_PrimitiveVarsPool;							// пул переменных
		std::vector<PrimitiveVariableExternal> m_PrimitiveExtVarsPool;				// пул внешних переменных (не входящих внутрь блоков)
		std::vector<ExternalVariable> m_ExternalVarsPool;							// пул внешних переменных устройства
		std::vector<double> m_DoubleVarsPool;										// пул double - переменных 
		ExternalVariable *m_pExternalVarsHead;
		double *m_pDoubleVarsHead;

		size_t m_nBlockEquationsCount;												// количество внутренних уравнений хост-блоков
		size_t m_nPrimitiveVarsCount;												// количество внутренних переменных устройства
		size_t m_nDoubleVarsCount;													// количество необходимых одному устройству double-переменных
		size_t m_nExternalVarsCount;												// количество внешних переменных

		void CleanUp();
		std::vector<double> m_ParameterBuffer;
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
		size_t GetInputsCount()				{  return m_DLL.GetInputsInfo().size();     }
		size_t GetConstsCount()				{  return m_DLL.GetConstsInfo().size();     }
		size_t GetSetPointsCount()			{  return m_DLL.GetSetPointsInfo().size();  }
	};
}

