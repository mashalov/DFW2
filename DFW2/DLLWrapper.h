﻿#pragma once
#include "Header.h"
#include <filesystem>

namespace DFW2
{
	class CDLLInstance
	{
	protected:
		LIBMODULE m_hDLL = NULL;
		std::filesystem::path m_ModulePath;
		void CleanUp()
		{
			if (m_hDLL)
#ifdef _MSC_VER
				FreeLibrary(m_hDLL);
#else 
				dlclose(m_hDLL);
#endif
			m_hDLL = NULL;
		}
		void Init(const std::filesystem::path& DLLFilePath)
		{
			// загружаем dll 
#ifdef _MSC_VER
			m_hDLL = LoadLibrary(DLLFilePath.c_str());
#else
			m_hDLL = dlopen(DLLFilePath.c_str(), RTLD_NOW);
#endif
			if (!m_hDLL)
				throw dfw2errorGLE(fmt::format("Ошибка загрузки DLL \"{}\".", stringutils::utf8_encode(DLLFilePath.c_str())));
			m_ModulePath = DLLFilePath;
		}
	public:
		virtual ~CDLLInstance() { CleanUp(); }
		CDLLInstance(std::filesystem::path DLLFilePath)
		{
			Init(DLLFilePath);
		}
		const std::filesystem::path GetModuleFilePath() const { return m_ModulePath; }
	};

	template<class Interface>
	class CDLLInstanceFactory : public CDLLInstance
	{
#ifdef _MSC_VER		
		using fnFactory = Interface* (__cdecl *)();
#else
		using fnFactory = Interface * (*)();
#endif
	protected:
		fnFactory m_pfnFactory = nullptr;
		void Init(std::string_view FactoryFunction)
		{
			std::string strFactoryFn(FactoryFunction);
#ifdef _MSC_VER
			m_pfnFactory = reinterpret_cast<fnFactory>(::GetProcAddress(m_hDLL, strFactoryFn.c_str()));
#else
			m_pfnFactory = reinterpret_cast<fnFactory>(dlsym(m_hDLL, strFactoryFn.c_str()));
#endif
			if (!m_pfnFactory)
				throw dfw2error(fmt::format("Функция \"{}\" не найдена в DLL \"{}\"", strFactoryFn, 
					stringutils::utf8_encode(m_ModulePath.c_str())));
		}
	public:
		using IntType = Interface;
		CDLLInstanceFactory(std::filesystem::path DLLFilePath, std::string_view FactoryFunction) : CDLLInstance(DLLFilePath)
		{
			Init(FactoryFunction);
		}
		Interface* Create()
		{
			if (!m_hDLL || !m_pfnFactory)
				throw dfw2error("DLL is not ready for Create call");
			return m_pfnFactory();
		}
	};

	template<class T>
	class CDLLInstanceWrapper
	{
	protected:
		std::shared_ptr<T> m_pDLL;
		typename T::IntType* m_pInstance = nullptr;
	public:
		void Create(std::shared_ptr<T>  pDLL) { m_pDLL = pDLL;  m_pInstance = m_pDLL->Create(); }
		CDLLInstanceWrapper() {}
		CDLLInstanceWrapper(std::shared_ptr<T>  pDLL) { Create(pDLL); }
		~CDLLInstanceWrapper() { if(m_pInstance) m_pInstance->Destroy(); }
		constexpr typename T::IntType* operator  -> () { return m_pInstance; }
		constexpr operator typename T::IntType* () { return m_pInstance; }
	};
}