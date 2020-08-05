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
		virtual void Init(std::wstring_view DLLFilePath)
		{
			// загружаем dll
			m_hDLL = LoadLibrary(std::wstring(DLLFilePath).c_str());
			if (!m_hDLL)
				throw dfw2error(fmt::format(_T("Failed to load DLL {}. GetLastError {}"), DLLFilePath, ::GetLastError()));
			m_strModulePath = DLLFilePath;
		}
		std::wstring_view GetModuleFilePath() const { return m_strModulePath.c_str(); }
	};


	/*
	class CCustomDeviceDLLWrapper
	{
	protected:
		std::shared_ptr<CCustomDeviceCPPDLL> m_pDLL;
		ICustomDevice* m_pDevice = nullptr;
	public:
		CCustomDeviceDLLWrapper() {}
		void Create(std::shared_ptr<CCustomDeviceCPPDLL>  pDLL) { m_pDLL = pDLL;  m_pDevice = m_pDLL->CreateDevice(); }
		CCustomDeviceDLLWrapper(std::shared_ptr<CCustomDeviceCPPDLL>  pDLL) { Create(pDLL); }
		~CCustomDeviceDLLWrapper() { m_pDevice->Destroy(); }
		constexpr ICustomDevice* operator  -> () { return m_pDevice; }
		constexpr operator ICustomDevice* () { return m_pDevice; }
	};
	*/
}