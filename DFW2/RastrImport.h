#pragma once
#include "DynaModel.h"
#include "Automatic.h"
#import "C:\Program Files (x86)\RastrWin3\astra.dll" no_namespace, named_guids, no_dual_interfaces 

namespace DFW2
{

	struct DBSLC
	{
		ptrdiff_t m_Id;
		double Vmin;
		double P0, P1, P2;
		double Q0, Q1, Q2;
		DBSLC() : m_Id(0), Vmin(0.0), P0(0.0), P1(0.0), P2(0.0), Q0(0.0), Q1(0.0), Q2(0.0) {}
	};

	struct CSLCPolynom
	{
	public:
		CSLCPolynom(double kV, double k0, double k1, double k2) : m_kV(kV), m_k0(k0), m_k1(k1), m_k2(k2) {}
		bool IsEmpty()
		{
			return (Equal(m_k0,0.0) && Equal(m_k1,0.0) && Equal(m_k2,0.0));
		}
		double m_kV;
		double m_k0;
		double m_k1;
		double m_k2;

		// оператор сортировки сегментов по напряжению
		bool operator < (const CSLCPolynom& rhs) const
		{
			return (m_kV < rhs.m_kV);
		}

		// сравнение сегмента СХН с заданным сегментом по коэффициентам
		bool CompareWith(const CSLCPolynom& rhs) const
		{
			return (Equal(m_k0,rhs.m_k0) && Equal(m_k1,rhs.m_k1) && Equal(m_k2,rhs.m_k2));
		}
	};

	class SLCPOLY : public std::list<CSLCPolynom>
	{
	public:
		bool InsertLRCToShuntVmin(double Vmin);
	};

	typedef SLCPOLY::iterator SLCPOLYITR;
	typedef SLCPOLY::reverse_iterator SLCPOLYRITR;

	class CStorageSLC
	{
	public:
		SLCPOLY P;
		SLCPOLY Q;
	};

	typedef std::map<ptrdiff_t, CStorageSLC*> SLCSTYPE;
	typedef SLCSTYPE::iterator SLCSITERATOR;

	class CSLCLoader : public SLCSTYPE
	{
	public:
		virtual ~CSLCLoader()
		{
			for (SLCSITERATOR it = begin(); it != end(); it++)
				delete it->second;
		}
	};

	class CustomDeviceConnectInfo
	{
	public:
		std::wstring m_TableName;
		std::wstring m_ModelTypeField;
		long m_nModelType;
		CustomDeviceConnectInfo(const _TCHAR *cszTableName, const _TCHAR *cszModelTypeField, long nModelType) : m_TableName(cszTableName),
																												m_ModelTypeField(cszModelTypeField),
																												m_nModelType(nModelType)
		{

		}
		CustomDeviceConnectInfo(const _TCHAR *cszTableName, long nModelType) : m_TableName(cszTableName),
																			   m_ModelTypeField(_T("ModelType")),
																			   m_nModelType(nModelType)
		{

		}
	};

	class CRastrImport
	{
	public:
		CRastrImport();
		virtual ~CRastrImport();
		void GetData(CDynaModel& Network);
	protected:
		IRastrPtr m_spRastr;
		bool CreateLRCFromDBSLCS(CDynaModel& Network, DBSLC *pLRCBuffer, ptrdiff_t nLRCCount);
		bool GetCustomDeviceData(CDynaModel& Network, IRastrPtr spRastr, CustomDeviceConnectInfo& ConnectInfo, CCustomDeviceContainer& CustomDeviceContainer);
		bool GetCustomDeviceData(CDynaModel& Network, IRastrPtr spRastr, CustomDeviceConnectInfo& ConnectInfo, CCustomDeviceCPPContainer& CustomDeviceContainer);
		void ReadRastrRow(SerializerPtr& Serializer, long Row);

		template<typename T>
		CDevice* ReadTable(const _TCHAR *cszTableName, CDeviceContainer& Container)
		{
			ITablePtr spTable = m_spRastr->Tables->Item(cszTableName);
			IColsPtr spCols = spTable->Cols;
			int nSize = spTable->Size;
			T *pDevs(nullptr), *pDev(nullptr);
			if (nSize)
			{
				pDevs = pDev = new T[nSize];
				Container.AddDevices(pDevs, nSize);
				auto pSerializer = pDevs->GetSerializer();
				for (auto&& serializervalue : *pSerializer)
					if (!serializervalue.second->bState)
						serializervalue.second->pAux = std::make_unique<CSerializedValueAuxDataRastr>(spCols->Item(serializervalue.first.c_str()));

				for (int Row = 0; Row < nSize; Row++, pDev++)
				{
					pDev->UpdateSerializer(pSerializer);
					ReadRastrRow(pSerializer, Row);
				}
			}
			return pDevs;
		}

		CDynaNodeBase::eLFNodeType NodeTypeFromRastr(long RastrType);
		static const CDynaNodeBase::eLFNodeType RastrTypesMap[5];
	};

	class CSerializedValueAuxDataRastr : public CSerializedValueAuxDataBase
	{
	public:
		IColPtr m_spCol;
		CSerializedValueAuxDataRastr(IColPtr spCol) : m_spCol(spCol) { }
	};
}

