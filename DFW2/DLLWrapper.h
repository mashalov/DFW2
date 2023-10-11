#pragma once
#include "Header.h"
#include <filesystem>

namespace DFW2
{
	class CDLLInstance
	{
	protected:
		LIBMODULE hDLL_ = NULL;
		std::filesystem::path ModulePath_;
		void CleanUp()
		{
			if (hDLL_)
#ifdef _MSC_VER
				FreeLibrary(hDLL_);
#else 
				dlclose(hDLL_);
#endif
			hDLL_ = NULL;
		}
		void Init(const std::filesystem::path& DLLFilePath)
		{
			// загружаем dll 
#ifdef _MSC_VER
			hDLL_ = LoadLibrary(DLLFilePath.c_str());
			if (!hDLL_)
				throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszModuleLoadError, stringutils::utf8_encode(DLLFilePath.c_str())));
#else
			hDLL_ = dlopen(DLLFilePath.c_str(), RTLD_NOW);
			if(!hDLL_)
				throw dfw2error(fmt::format("{} \"{}\"", fmt::format(CDFW2Messages::m_cszModuleLoadError, DLLFilePath.string()), dlerror()));
#endif
			ModulePath_ = DLLFilePath;
		}
	public:
		virtual ~CDLLInstance() { CleanUp(); }
		CDLLInstance(std::filesystem::path DLLFilePath)
		{
			Init(DLLFilePath);
		}
		const std::filesystem::path& GetModuleFilePath() const { return ModulePath_; }
		void* GetProcAddress(std::string_view FunctionName)
		{
#ifdef _MSC_VER			
			return ::GetProcAddress(hDLL_, std::string(FunctionName).c_str());
#else
			return dlsym(hDLL_, std::string(strFactoryFn).c_str());
#endif
		}
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
		fnFactory pfnFactory_ = nullptr;
		void Init(std::string_view FactoryFunction)
		{
			pfnFactory_ = reinterpret_cast<fnFactory>(GetProcAddress(FactoryFunction));
			if (!pfnFactory_)
				throw dfw2error(fmt::format(CDFW2Messages::m_cszDLLFunctionNotFound, FactoryFunction,
					stringutils::utf8_encode(ModulePath_.c_str())));
		}
	public:
		using IntType = Interface;
		CDLLInstanceFactory(std::filesystem::path DLLFilePath, std::string_view FactoryFunction) : CDLLInstance(DLLFilePath)
		{
			Init(FactoryFunction);
		}
		Interface* Create()
		{
			if (!hDLL_ || !pfnFactory_)
				throw dfw2error(CDFW2Messages::m_cszDLLIsNotReadyForCreateCall);
			return pfnFactory_();
		}
	};

	template<class T>
	class CDLLInstanceWrapper
	{
	protected:
		std::shared_ptr<T> pDLL_;
		typename T::IntType* pInstance_ = nullptr;
	public:
		void Create(std::shared_ptr<T>  pDLL) { pDLL_ = pDLL;  pInstance_ = pDLL_->Create(); }
		CDLLInstanceWrapper() {}
		CDLLInstanceWrapper(std::shared_ptr<T>  pDLL) { Create(pDLL); }
		~CDLLInstanceWrapper() { if(pInstance_) pInstance_->Destroy(); }
		constexpr typename T::IntType* operator  -> () { return pInstance_; }
		constexpr operator typename T::IntType* () { return pInstance_; }
	};
}