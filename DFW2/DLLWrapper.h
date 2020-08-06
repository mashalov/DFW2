#pragma once
#include "cex.h"
#include "dfw2exception.h"

namespace DFW2
{
	class CDLLInstance
	{
	protected:
		HMODULE m_hDLL = NULL;
		std::wstring m_strModulePath;
		void CleanUp()
		{
			if (m_hDLL)
				FreeLibrary(m_hDLL);
			m_hDLL = NULL;
		}
	public:
		virtual ~CDLLInstance() { CleanUp(); }
		void Init(std::wstring_view DLLFilePath)
		{
			// загружаем dll
			m_hDLL = LoadLibrary(std::wstring(DLLFilePath).c_str());
			if (!m_hDLL)
				throw dfw2error(fmt::format(_T("Failed to load DLL {}. GetLastError {}"), DLLFilePath, ::GetLastError()));
			m_strModulePath = DLLFilePath;
		}
		std::wstring_view GetModuleFilePath() const { return m_strModulePath.c_str(); }
	};

	template<class Interface>
	class CDLLInstanceFactory : public CDLLInstance
	{
		using fnFactory = Interface* (__cdecl *)();
	protected:
		fnFactory m_pfnFactory = nullptr;
	public:
		using IntType = Interface;
		void Init(std::wstring_view DLLFilePath, std::string_view FactoryFunction)
		{
			CDLLInstance::Init(DLLFilePath);
			std::string strFactoryFn(FactoryFunction);
			m_pfnFactory = reinterpret_cast<fnFactory>(::GetProcAddress(m_hDLL, strFactoryFn.c_str()));
			if (!m_pfnFactory)
				throw dfw2error(fmt::format(_T("Функция \"{}\" не найдена в DLL {}"), stringutils::utf8_decode(strFactoryFn), DLLFilePath));
		}
		Interface* Create()
		{
			if (!m_hDLL || !m_pfnFactory)
				throw dfw2error(_T("DLL is not ready for Create call"));
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
		~CDLLInstanceWrapper() { m_pInstance->Destroy(); }
		constexpr typename T::IntType* operator  -> () { return m_pInstance; }
		constexpr operator typename T::IntType* () { return m_pInstance; }
	};
}