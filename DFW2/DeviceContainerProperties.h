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

		static constexpr const char* cszSysNameGenerator1C_ = "Generator1C";
		static const char* cszNameGenerator1C_;

		static constexpr const char* cszSysNameGenerator2C_ = "Generator2C";
		static const char* cszNameGenerator2C_;

		static constexpr const char* cszSysNameGenerator3C_ = "Generator3C";
		static const char* cszNameGenerator3C_;

		static constexpr const char* cszSysNameGeneratorPark3C_ = "GeneratorPark3C";
		static const char* cszNameGeneratorPark3C_;

		static constexpr const char* cszSysNameGeneratorPark4C_ = "GeneratorPark4C";
		static const char* cszNameGeneratorPark4C_;

		static constexpr const char* cszSysNameGeneratorMustang_ = "GeneratorMustang";
		static const char* cszNameGeneratorMustang_;

		static constexpr const char* cszSysNameGeneratorInfPower_ = "GeneratorInfBus";
		static const char* cszNameGeneratorInfPower_;

		static constexpr const char* cszSysNameGeneratorPowerInjector_ = "GeneratorPowerInjector";
		static const char* cszNameGeneratorPowerInjector_;

		static constexpr const char* cszSysNameGeneratorMotion_ = "GeneratorMotion";
		static const char* cszNameGeneratorMotion_;

		static constexpr const char* cszSysNameExciterMustang_ = "ExciterMustang";
		static const char* cszNameExciterMustang_;

		static constexpr const char* cszSysNameExcConMustang_ = "ExcControlMustang";
		static const char* cszNameExcConMustang_;

		
		static constexpr const char* cszSysNameDECMustang_ = "DecMustang"; 
		static const char* cszNameDECMustang_;

		static constexpr const char* cszSysNameNode_ = "Node";
		static const char* cszNameNode_;

		static constexpr const char* cszSysNameBranch_ = "Branch";
		static const char* cszNameBranch_;

		static constexpr const char* cszSysNameBranchMeasure_ = "BranchMeasure";
		static const char* cszNameBranchMeasure_;

		static constexpr const char* cszSysNameNodeMeasure_ = "NodeMeasure";
		static const char* cszNameNodeMeasure_;

		static constexpr const char* cszSysNameZeroLoadFlow_ = "ZeroLoadFlow";
		static const char* cszNameZeroLoadFlow_;

		static constexpr const char* cszSysNameLRC_ = "LRC";
		static const char* cszNameLRC_;

		static constexpr const char* cszSysNameReactor_ = "Reactor";
		static const char* cszNameReactor_;

		static constexpr const char* cszSysNameSVC_ = "SVC";
		static const char* cszNameSVC_;

		static constexpr const char* cszSysNameSVCDEC_ = "SVCDEC";
		static const char* cszNameSVCDEC_;
	};
}

