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
#include "array"


namespace DFW2
{

	// ��� ������ ��� ����-������
	struct PrimitivePoolElement
	{
		unsigned char *m_pPrimitive;			// ������ ����
		unsigned char *m_pHead;					// ������� ������� � ����
		size_t nCount;							// ���������� ��������� � ����
		PrimitivePoolElement() : m_pPrimitive(nullptr), 
							     nCount(0) {}
	};

	// �������� ����-�����
	struct PrimitiveInfo
	{
		size_t nSize;							// ������ � ������
		long nEquationsCount;					// ���������� ���������
		PrimitiveInfo(size_t Size, long EquationsCount) : nSize(Size),
														  nEquationsCount(EquationsCount) {}
	};

	class CCustomDeviceContainer : public CDeviceContainer
	{
	protected:
		CCustomDeviceDLL m_DLL;
		// ���� ��� ���� ��������� ����������
		PrimitivePoolElement m_PrimitivePool[PrimitiveBlockType::PBT_LAST];			// ������� ����� ��� ������� ���� ����-������
		std::vector<PrimitiveVariable> m_PrimitiveVarsPool;							// ��� ����������
		std::vector<PrimitiveVariableExternal> m_PrimitiveExtVarsPool;				// ��� ������� ���������� (�� �������� ������ ������)
		std::vector<ExternalVariable> m_ExternalVarsPool;							// ��� ������� ���������� ����������
		std::vector<double> m_DoubleVarsPool;										// ��� double - ���������� 
		std::vector<VariableIndex> m_VariableIndexPool;								// ��� ��� VariableIndexes
		ExternalVariable *m_pExternalVarsHead;
		double *m_pDoubleVarsHead;

		size_t m_nBlockEquationsCount;												// ���������� ���������� ��������� ����-������
		size_t m_nPrimitiveVarsCount;												// ���������� ���������� ���������� ����������
		size_t m_nDoubleVarsCount;													// ���������� ����������� ������ ���������� double-����������
		size_t m_nExternalVarsCount;												// ���������� ������� ����������
		size_t m_nVariableIndexesCount;												// ���������� ���������� ��� ���������

		void CleanUp();
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
		VariableIndex* NewVariableIndexVariables();
		ExternalVariable* NewExternalVariables();
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
		virtual CDynaPrimitive* Create(CDevice* pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) = 0;
		virtual void Allocate(size_t nDevicesCount) = 0;
		virtual ~CPrimitivePoolBase() {};
	};

	template<class T>
	class CPrimitivePoolT : public CPrimitivePoolBase
	{
	protected:
		std::vector<T> m_Primitives;
	public:
		size_t m_nSize = 0;
		CDynaPrimitive* Create(CDevice* pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input) override
		{
			m_Primitives.emplace_back(pDevice, pOutput, nOutputIndex, Input);
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

		CDynaPrimitive* Create(PrimitiveBlockType ePrimitiveType, CDevice* pDevice, double* pOutput, ptrdiff_t nOutputIndex, std::initializer_list<PrimitiveVariableBase*> Input)
		{
			CDevice::CheckIndex(m_Pools, ePrimitiveType);
			return m_Pools[ePrimitiveType]->Create(pDevice, pOutput, nOutputIndex, Input);
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
		void AllocatePools(size_t nDevicesCount);
		bool ConnectDLL(const _TCHAR* cszDLLFilePath);
		CDerlagContinuous* CreateDerLag(CDevice* pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input);
	};
}

