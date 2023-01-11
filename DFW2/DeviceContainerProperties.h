#pragma once
#include "Header.h"
#include "DeviceContainerPropertiesBase.h"
#include "Device.h"
//#include "DLLStructs.h"


namespace DFW2
{
	// для ускорения обработки ссылок разделенных по иерархии и направлениям
	// используются те же карты, но с const указателями на second из основных карт ссылок
	using LINKSFROMMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionFrom const*>;
	using LINKSTOMAPPTR = std::map<eDFW2DEVICETYPE, LinkDirectionTo const*>;
	// ссылки без разделения на направления
	using LINKSUNDIRECTED = std::vector<LinkDirectionFrom const*>;

	// базовый класс для фабрики
	class CDeviceFactoryBase {
	public:
		// создает устройства какого-то типа и привязать их к переданному вектору указателей устройств
		virtual void Create(size_t nCount, DEVICEVECTOR& DevVec) = 0;
		virtual ~CDeviceFactoryBase() = default;
	};

	template<class T>
	class CDeviceFactory : public CDeviceFactoryBase
	{
		// вектор устройств, которым управляет фабрика
		using DeviceVectorType = std::unique_ptr<T[]>;
		DeviceVectorType pDevices;
	public:
		// создать некоторые устройства и привязать к вектору
		void Create(size_t nCount, DEVICEVECTOR& DevVec) override
		{
			CreateRet(nCount, DevVec);
		}
		// создать устройства по типу контейнера, вернуть указатель на вектор даного типа
		T* CreateRet(size_t Count, DEVICEVECTOR& DevVec)
		{
			// создаем новые устройства
			pDevices = std::make_unique<T[]>(Count);
			// ставим новый размер вектора указателей на устройства
			DevVec.resize(Count);
			auto it{ DevVec.begin() };
			// заполняем вектор указателей указателями на созданные фабрикой устройства
			for (T* p = pDevices.get(); p < pDevices.get() + Count; p++, it++)
				*it = p;
			// возвращаем  на вектор устройств типа
			return pDevices.get();
		}
	};

	class CDeviceContainerProperties : public CDeviceContainerPropertiesBase
	{
	public:
		std::unique_ptr<CDeviceFactoryBase> DeviceFactory;
		LINKSFROMMAPPTR MasterLinksFrom;
		LINKSTOMAPPTR  MasterLinksTo;
		LINKSUNDIRECTED Masters, Slaves;
		eDFW2DEVICETYPE GetType() const noexcept;
		const char* GetVerbalClassName() const noexcept;
		const char* GetSystemClassName() const noexcept;
		static const char* m_cszNameGenerator1C, *m_cszSysNameGenerator1C;
		static const char* m_cszNameGenerator3C, *m_cszSysNameGenerator3C;
		static const char* m_cszNameGeneratorPark3C, *m_cszSysNameGeneratorPark3C;
		static const char* m_cszNameGeneratorPark4C, *m_cszSysNameGeneratorPark4C;
		static const char* m_cszNameGeneratorPowerInjector, * m_cszSysNameGeneratorPowerInjector;
		static const char* m_cszNameGeneratorMustang, *m_cszSysNameGeneratorMustang;
		static const char* m_cszNameGeneratorInfPower, *m_cszSysNameGeneratorInfPower;
		static const char* m_cszNameGeneratorMotion, *m_cszSysNameGeneratorMotion;
		static const char* m_cszNameExciterMustang, *m_cszSysNameExciterMustang;
		static const char* m_cszNameExcConMustang, *m_cszSysNameExcConMustang;
		static const char* m_cszNameDECMustang, *m_cszSysNameDECMustang;
		static const char* m_cszNameNode, *m_cszSysNameNode;
		static const char* m_cszNameBranch, *m_cszSysNameBranch;
		static const char* m_cszNameBranchMeasure, *m_cszSysNameBranchMeasure;
		static const char* m_cszNameNodeMeasure, * m_cszSysNameNodeMeasure;
		static const char* m_cszNameZeroLoadFlow, * m_cszSysNameZeroLoadFlow;
		static const char* m_cszNameLRC, * m_cszSysNameLRC;
		static const char* m_cszNameReactor, * m_cszSysNameReactor;
		static const char* m_cszAliasNode;
		static const char* m_cszAliasBranch;
		static const char* m_cszAliasGenerator;
	};



}

