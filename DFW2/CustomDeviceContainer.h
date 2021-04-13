﻿#pragma once
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
#include "array"


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
		std::vector<DLLExternalVariable> m_ExternalVarsPool;						// пул внешних переменных устройства
		std::vector<double> m_DoubleVarsPool;										// пул double - переменных 
		std::vector<VariableIndex> m_VariableIndexPool;								// пул для VariableIndexes
		std::vector<VariableIndexExternal> m_VariableIndexExternalPool;
		DLLExternalVariable* m_pExternalVarsHead;

		double *m_pDoubleVarsHead;
		size_t m_nBlockEquationsCount;												// количество внутренних уравнений хост-блоков
		size_t m_nDoubleVarsCount;													// количество необходимых одному устройству double-переменных
		size_t m_nExternalVarsCount;												// количество внешних переменных
		size_t m_nVariableIndexesCount;												// количество переменных для уравнений

		void CleanUp();
	public:
		CCustomDeviceContainer(CDynaModel *pDynaModel);
		bool ConnectDLL(std::string_view DLLFilePath);
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
		double* NewDoubleVariables();
		VariableIndex* NewVariableIndexVariables();
		VariableIndexExternal* NewVariableIndexExternals();
		DLLExternalVariable* NewExternalVariables();
		void* NewPrimitive(PrimitiveBlockType eType);
		const CCustomDeviceDLL& DLL() { return m_DLL; }
		long GetParametersValues(ptrdiff_t nId, BuildEquationsArgs* pArgs, long nBlockIndex, DOUBLEVECTOR& Parameters);
		size_t GetInputsCount()				{  return m_DLL.GetInputsInfo().size();     }
		size_t GetConstsCount()				{  return m_DLL.GetConstsInfo().size();     }
		size_t GetSetPointsCount()			{  return m_DLL.GetSetPointsInfo().size();  }
	};


	class CPrimitivePoolBase
	{
	protected:
		size_t m_nSize = 0;
	public:
		void IncSize() { m_nSize++; }
		virtual CDynaPrimitive* Create(CDevice& Device, const ORange& Output, const IRange& Input) = 0;
		virtual void Allocate(size_t nDevicesCount) = 0;
		virtual ~CPrimitivePoolBase() = default;
	};

	template<class T>
	class CPrimitivePoolT : public CPrimitivePoolBase
	{
	protected:
		std::vector<T> m_Primitives;
	public:
		CDynaPrimitive* Create(CDevice& Device, const ORange& Output, const IRange& Input) override
		{
			m_Primitives.emplace_back(Device, Output, Input);
			return &m_Primitives.back();
		}
		void Allocate(size_t nDevicesCount) override
		{
			m_Primitives.reserve(m_nSize * nDevicesCount);
		}
	};

	class CPrimitivePools
	{
	public:
		CPrimitivePools() : m_Pools { 
									  nullptr,
									  std::make_unique< CPrimitivePoolT<CLimitedLag>>(),
									  std::make_unique< CPrimitivePoolT<CLimitedLag>>(),
									  std::make_unique< CPrimitivePoolT<CDerlagContinuous>>(),
									  std::make_unique< CPrimitivePoolT<CLimitedLag>>(),
									  std::make_unique< CPrimitivePoolT<CLimitedLag>>(),
									  std::make_unique< CPrimitivePoolT<CLimiterConst>>(),
									  std::make_unique< CPrimitivePoolT<CRelay>>(),
									  std::make_unique< CPrimitivePoolT<CRelayDelay>>(),
									  std::make_unique< CPrimitivePoolT<CRelayDelayLogic>>(),
									  std::make_unique< CPrimitivePoolT<CRSTrigger>>(),
									  std::make_unique< CPrimitivePoolT<CComparator>>(),
									  std::make_unique< CPrimitivePoolT<CComparator>>(),
									  std::make_unique< CPrimitivePoolT<CZCDetector>>(),
									  std::make_unique< CPrimitivePoolT<COr>>(),
									  std::make_unique< CPrimitivePoolT<CAnd>>(),
									  std::make_unique< CPrimitivePoolT<CNot>>(),
									  std::make_unique< CPrimitivePoolT<CAbs>>(),
									  std::make_unique< CPrimitivePoolT<CDeadBand>>(),
									  std::make_unique< CPrimitivePoolT<CExpand>>(),
									  std::make_unique< CPrimitivePoolT<CShrink>>()
									}
		{
		}
		void CountPrimitive(PrimitiveBlockType ePrimitiveType)
		{
			CDevice::CheckIndex(m_Pools, ePrimitiveType);
			m_Pools[ePrimitiveType]->IncSize();
		}
		void Allocate(size_t nDevicesCount)
		{
			for (auto& pool : m_Pools)
				if(pool)
				pool->Allocate(nDevicesCount);
		}

		CDynaPrimitive* Create(PrimitiveBlockType ePrimitiveType, 
							   CDevice& Device, 
							   const ORange& Output, 
							   const IRange& Input)
		{
			CDevice::CheckIndex(m_Pools, ePrimitiveType);
			return m_Pools[ePrimitiveType]->Create(Device, Output, Input);
		}

		std::array<std::unique_ptr<CPrimitivePoolBase>, PrimitiveBlockType::PBT_LAST> m_Pools;
	};

	class CCustomDeviceCPPContainer : public CDeviceContainer
	{
	protected:
		std::shared_ptr<CCustomDeviceCPPDLL> m_pDLL;
		CPrimitivePools m_PrimitivePools;
	public:
		std::shared_ptr<CCustomDeviceCPPDLL> DLL() { return m_pDLL; }
		CCustomDeviceCPPContainer(CDynaModel* pDynaModel);
		virtual ~CCustomDeviceCPPContainer();
		void BuildStructure();
		void ConnectDLL(std::string_view DLLFilePath);

		CDynaPrimitive* CreatePrimitive(PrimitiveBlockType ePrimitiveType,
			CDevice& Device,
			const ORange& Output,
			const IRange& Input)
		{
				return m_PrimitivePools.Create(ePrimitiveType, Device, Output, Input);
		}
	};
}

