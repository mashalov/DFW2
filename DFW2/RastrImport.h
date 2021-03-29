#pragma once
#include "DynaModel.h"
#include "Automatic.h"

#ifdef _DEBUG
#ifdef _WIN64
#include "x64\debug\astra.tlh"
#else
#include "debug\astra.tlh"
#endif
#else
#ifdef _WIN64
#include "x64\release\astra.tlh"
#else
#include "release\astra.tlh"
#endif
#endif

//#import "C:\Program Files (x86)\RastrWin3\astra.dll" no_namespace, named_guids, no_dual_interfaces 

namespace DFW2
{


	// Словарь синонимов названий контейнеров и их полей с таблицами и полями Rastr
	class CRastrSynonyms 
	{
	protected:
		using StringSet = std::set<std::string, std::less<> >;					// сет строк с возможностью искать string_view (std::less<>)
		using FieldSynonyms = std::map<std::string, StringSet, std::less<> >;	// карта соответствия имени поля контейнера полям Rastr

		// Таблица Rastr
		struct RastrTable 
		{
			std::string name;				// название таблицы
			FieldSynonyms m_FieldSynonyms;	// словарь синонимов полей

			RastrTable(std::string_view Name) : name(Name) {}

			// Добавляет синонимы полей к полю field
			template <typename ...Args>
			RastrTable& AddFieldSynonyms(std::string_view field, Args... synonyms)
			{
				// находим или создаем новый пустой синоним по имени
				FieldSynonyms::iterator it = m_FieldSynonyms.find(field);
				if (it == m_FieldSynonyms.end())
					it = m_FieldSynonyms.insert(std::make_pair(field, StringSet())).first;
				// добавляем синонимы в сет имени
				it->second.insert({ std::string(synonyms)... });
				// возвращаем таблицу Rastr
				return *this;
			}

			// Возвращает список синонимов поля в виде сета. Добавляет и само поле
			StringSet GetFieldSynonyms(std::string_view field) const
			{
				StringSet ret;
				if (auto it = m_FieldSynonyms.find(field); it != m_FieldSynonyms.end())
					ret = it->second;
				return ret;
			}
		};

		// сравнение unique_ptr<RastrTable>
		struct RastrTableComparer
		{
			bool operator () (const std::unique_ptr<RastrTable>& lhs, const std::unique_ptr<RastrTable>& rhs) const
			{
				return lhs->name < rhs->name;
			}
		};


		

		using RastrTables = std::set<std::unique_ptr<RastrTable>, RastrTableComparer>;
		using RastrTablesPtrs = std::set<RastrTable*>;


		// сет таблиц Rastr заданных в качестве синонимов, он же storagе
		RastrTables rastrTables;

		// синонимы контейнера
		struct ContainerSynonym 
		{
			std::string name;

			ContainerSynonym(std::string_view Name) : name(Name) {}

			// список указателей на таблицы-синонимы данного контейнера
			RastrTablesPtrs rastrSynonyms;

			// возвращает все таблицы-синонимы контейнера
			const RastrTablesPtrs& GetRastrSynonyms() const
			{
				return rastrSynonyms;
			}

			// возвращает все синонимы заданного поля для всех таблиц-синонимов
			StringSet GetFieldSynonyms(std::string_view field) const
			{
				StringSet ret;
				// обходим все таблицы-синонимы и ищем в них заданное поле
				for (const auto& rastrSyn : rastrSynonyms)
				{
					// получаем сет синонимов заданного поля
					const auto fields = rastrSyn->GetFieldSynonyms(field);
					// вставляем полученный сет в возврат
					ret.insert(fields.begin(), fields.end());
				}
				// добавляем само поле контейнера, чтобы не делать ветвлений
				ret.insert(std::string(field));
				return ret;
			}

			// Добавляет поле с синонимами
			template <typename ...Args>
			ContainerSynonym& AddFieldSynonyms(std::string_view field, Args... synonyms)
			{
				// обходим все таблицы-синонимы и добавляем в их списки синонимов заданное поле
				for (auto&& rastrSyn : rastrSynonyms)
					rastrSyn->AddFieldSynonyms(field, synonyms...);
				// возвращаем этот же объект для цепочечного вызова .AddFieldSynonums()
				return *this;
			}
		};

		struct ContainerSynonymComparer
		{
			bool operator () (const std::unique_ptr<ContainerSynonym>& lhs, const std::unique_ptr<ContainerSynonym>& rhs) const
			{
				return lhs->name < rhs->name;
			}
		};


		// сет именованных контейнеров с синонимами
		using RastrSynonyms = std::set <std::unique_ptr<ContainerSynonym>, ContainerSynonymComparer> ;
		RastrSynonyms m_Synonyms;

		// возвращает новый или существующий объект синонимов контейнера с заданным именем
		ContainerSynonym& GetRastrSynonym(std::string_view containerName)
		{
			// грязь конечно, но хочется точки
			return *(m_Synonyms.insert(std::make_unique<ContainerSynonym>(containerName)).first->get());
		}

		// хелпер для добавления таблицы в variadic template
		RastrTable* AddTable(std::string_view table)
		{
			return rastrTables.insert(std::make_unique<RastrTable>(table)).first->get();
		}

	public:
		// добавляет список таблиц синонимов к контейнеру с заданным именем
		template<typename... Args>
		ContainerSynonym& AddRastrSynonym(std::string_view containerName, Args... args)
		{
			// получаем объект синонимов заданного контейнера
			auto& table = GetRastrSynonym(containerName);
			// добавляем синонимы имен таблиц
			table.rastrSynonyms.insert(AddTable(args)...);
			return table;
		}

		// возвращает объект синонимов контейнера с заданным именем
		const ContainerSynonym& GetTable(std::string_view containerName)
		{
			return GetRastrSynonym(containerName);
		}
	};

	class CustomDeviceConnectInfo
	{
	public:
		std::string m_TableName;
		std::string m_ModelTypeField;
		long m_nModelType;
		CustomDeviceConnectInfo(std::string_view TableName, std::string_view ModelTypeField, long nModelType) : m_TableName(TableName),
																										    m_ModelTypeField(ModelTypeField),
																											m_nModelType(nModelType)
		{

		}
		CustomDeviceConnectInfo(std::string_view TableName, long nModelType) : m_TableName(TableName),
																		     m_ModelTypeField("ModelType"),
																			 m_nModelType(nModelType)
		{

		}
	};

	// адаптер значения для БД RastrWin
	class CSerializedValueAuxDataRastr : public CSerializedValueAuxDataBase
	{
	public:
		IColPtr m_spCol; // указатель на поле БД
		// конструктор по полю БД
		CSerializedValueAuxDataRastr(IColPtr spCol) : m_spCol(spCol) { }
	};

	class CRastrImport
	{
	public:
		CRastrImport();
		virtual ~CRastrImport();
		void GetData(CDynaModel& Network);
	protected:
		IRastrPtr m_spRastr;
		CRastrSynonyms m_rastrSynonyms;

		//bool CreateLRCFromDBSLCS(CDynaModel& Network, DBSLC *pLRCBuffer, ptrdiff_t nLRCCount);
		bool GetCustomDeviceData(CDynaModel& Network, IRastrPtr spRastr, CustomDeviceConnectInfo& ConnectInfo, CCustomDeviceContainer& CustomDeviceContainer);
		bool GetCustomDeviceData(CDynaModel& Network, IRastrPtr spRastr, CustomDeviceConnectInfo& ConnectInfo, CCustomDeviceCPPContainer& CustomDeviceContainer);
		void ReadRastrRow(SerializerPtr& Serializer, long Row);

		// Читаем контейнер из таблицы RastrWin
		void ReadTable(CDeviceContainer& Container, const char* cszRastrSelection = "")
		{
			const auto containerClass = Container.m_ContainerProps.GetSystemClassName();
			// находим синнонимы названий контейнера для Rastr
			auto tableSynonyms = m_rastrSynonyms.GetTable(containerClass);
			auto synSet = tableSynonyms.GetRastrSynonyms();
			long rastrTableIndex = -1;

			for (const auto& synonym : synSet)
			{
				// пытаемся найти таблицу названную синонимом в базе Rastr
				rastrTableIndex = m_spRastr->Tables->GetFind(stringutils::utf8_decode(synonym->name).c_str());
				if (rastrTableIndex < 0) continue;
				// таблица есть, читаем по выборке
				ITablePtr spTable = m_spRastr->Tables->Item(rastrTableIndex);
				spTable->SetSel(cszRastrSelection);
				int nSize = spTable->Count;		// определяем размер контейнера по размеру таблицы с выборкой
				// даже если размер нулевой - делаем контейнер пустым	
				Container.CreateDevices(nSize);
				if (!nSize)
					break;

				IColsPtr spCols = spTable->Cols;

				// создаем вектор устройств заданного типа
				auto pDevs = Container.begin();
				// достаем сериализатор для устройства данного типа
				auto pSerializer = (*pDevs)->GetSerializer();
				// обходим значения в карте сериализаторе
				// если значение не является переменной состояния
				// добавляем добавляем к ней адаптер для БД RastrWin
				// адаптер связываем с полем таблицы

				for (auto&& serializervalue : *pSerializer)
				{
					if (!serializervalue.second->bState)
					{
						auto synonyms = tableSynonyms.GetFieldSynonyms(serializervalue.first);

						// в почти всех контейнерах и таблицах
						// есть состояния, поэтому для всех
						// вводим "глобальный" синоним
						if (serializervalue.first == "state")
							synonyms.insert("sta");

						for (const auto& synonym : synonyms)
						{
							if (long index = spCols->GetFind(synonym.c_str()); index >= 0)
							{
								serializervalue.second->pAux = std::make_unique<CSerializedValueAuxDataRastr>(spCols->Item(index));
								break;
							}
						}

						if (!serializervalue.second->pAux)
							throw dfw2error(fmt::format("RastrImport::ReadTable - no synonyms for field \"{}\" from \"{}\" of container \"{}\" found in table \"{}\"",
								serializervalue.first,
								fmt::join(synonyms, ","),
								pSerializer->GetClassName(),
								synonym->name));
					}
				}

				long selIndex = spTable->GetFindNextSel(-1);
				while (true)
				{
					ReadRastrRow(pSerializer, selIndex);
					selIndex = spTable->GetFindNextSel(selIndex);

					if (selIndex < 0)
						break;

					if(!pSerializer->NextItem())
						throw dfw2error(fmt::format("RastrImport::ReadTable - container \"{}\" size set to {} does not fit devices",
							pSerializer->GetClassName(),
							Container.Count()));
				}
				break;
			}
			if (rastrTableIndex < 0)
				throw dfw2error(fmt::format("RastrImport::ReadTable - container \"{}\" has no readable synonyms for Rastr tables",
					containerClass));
		}
		
		CDynaNodeBase::eLFNodeType NodeTypeFromRastr(long RastrType);
		static const CDynaNodeBase::eLFNodeType RastrTypesMap[5];
	};
}


/*
* 
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

*/