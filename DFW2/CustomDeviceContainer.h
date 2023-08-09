#pragma once
#include "DeviceContainer.h"
#include "Header.h"
#include "DLLHeader.h"
#include "vector"
#include "ICustomDevice.h"
#include "DLLWrapper.h"
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
	using CCustomDeviceCPPDLL = CDLLInstanceFactory<ICustomDevice>;

	// пул памяти для хост-блоков
	struct PrimitivePoolElement
	{
		unsigned char *m_pPrimitive = nullptr;			// начало пула
		unsigned char* m_pHead = nullptr;				// текущая позиция в пуле
		size_t nCount = 0;								// количество элементов в пуле
	};

	// описание хост-блока
	struct PrimitiveInfo
	{
		size_t nSize;							// размер в байтах
		long nEquationsCount;					// количество уравнений
		PrimitiveInfo(size_t Size, long EquationsCount) : nSize(Size),
														  nEquationsCount(EquationsCount) {}
	};

	class CPrimitivePoolBase
	{
	protected:
		size_t m_nSize = 0;
	public:
		void IncSize() { m_nSize++; }
		virtual CDynaPrimitive* Create(CDevice& Device, const std::string_view& Name, const ORange& Output, const IRange& Input) = 0;
		virtual void Allocate(size_t nDevicesCount) = 0;
		virtual ~CPrimitivePoolBase() = default;
	};

	template<class T>
	class CPrimitivePoolT : public CPrimitivePoolBase
	{
	protected:
		std::vector<T> m_Primitives;
	public:
		CDynaPrimitive* Create(CDevice& Device, const std::string_view& Name, const ORange& Output, const IRange& Input) override
		{
			m_Primitives.emplace_back(Device, Output, Input);
			auto Primitive{ &m_Primitives.back() };
			Primitive->SetName(Name);
			return Primitive;
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
									  std::make_unique< CPrimitivePoolT<CRelayMin>>(),
									  std::make_unique< CPrimitivePoolT<CRelayDelay>>(),
									  std::make_unique< CPrimitivePoolT<CRelayMinDelay>>(),
									  std::make_unique< CPrimitivePoolT<CRelayDelayLogic>>(),
									  std::make_unique< CPrimitivePoolT<CRelayMinDelayLogic>>(),
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
							   std::string_view Name,
							   const ORange& Output, 
							   const IRange& Input)
		{
			CDevice::CheckIndex(m_Pools, ePrimitiveType);
			return m_Pools[ePrimitiveType]->Create(Device, Name, Output, Input);
		}

		std::array<std::unique_ptr<CPrimitivePoolBase>, PrimitiveBlockType::PBT_LAST> m_Pools;
	};

	class CCustomDeviceCPPContainer : public CDeviceContainer
	{
	protected:
		std::shared_ptr<CCustomDeviceCPPDLL> m_pDLL;
		CPrimitivePools m_PrimitivePools;
		STRINGLIST PrimitiveNames_;
	public:
		std::shared_ptr<CCustomDeviceCPPDLL> DLL() { return m_pDLL; }
		CCustomDeviceCPPContainer(CDynaModel* pDynaModel) : CDeviceContainer(pDynaModel) {}
		~CCustomDeviceCPPContainer() = default;
		void BuildStructure();
		void ConnectDLL(std::filesystem::path DLLFilePath);

		const STRINGLIST& PrimitiveNames() const { return PrimitiveNames_; }

		CDynaPrimitive* CreatePrimitive(PrimitiveBlockType ePrimitiveType,
			CDevice& Device,
			std::string_view Name,
			const ORange& Output,
			const IRange& Input)
		{
				return m_PrimitivePools.Create(ePrimitiveType, Device, Name, Output, Input);
		}
	};
}

