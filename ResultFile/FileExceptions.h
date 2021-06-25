#pragma once

#include "../DFW2/dfw2exception.h"
#include "../DFW2/Messages.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

#ifdef _DEBUG
bool _trace(char* format, ...);
#define TRACE _trace
#else
#define TRACE false && _trace
#endif

namespace DFW2
{
	class CStreamedFile;

	class CDFW2Exception
	{
	protected:
		std::string m_strMessage;
	public:
		CDFW2Exception(std::string_view Description) : m_strMessage(Description) {}
		const char* Message() noexcept { return m_strMessage.c_str(); }
	};

	class CFileException : public CDFW2Exception
	{
	public:
		CFileException(const std::string_view Description, CStreamedFile& file);
		CFileException(const std::string_view Description) : CDFW2Exception(Description)
		{
		}
	};

	class CFileReadException : public CFileException
	{
	public:
		CFileReadException() : CFileException(CDFW2Messages::m_cszFileReadError) {}
		CFileReadException(const std::string_view Description) :
			CFileException(fmt::format("{} {}", CDFW2Messages::m_cszFileReadError, Description)) {}
		CFileReadException(CStreamedFile& file) : CFileException(CDFW2Messages::m_cszFileReadError, file) {}
		CFileReadException(CStreamedFile& file, const std::string_view Description) :
			CFileException(fmt::format("{} {}", CDFW2Messages::m_cszFileReadError, Description), file) {}
	};

	class CFileWriteException : public CFileException
	{
	public:
		CFileWriteException() : CFileException(CDFW2Messages::m_cszFileWriteError) {}
		CFileWriteException(const std::string_view Description) :
			CFileException(fmt::format("{} {}", CDFW2Messages::m_cszFileWriteError, Description)) {}
		CFileWriteException(CStreamedFile& file) : CFileException(CDFW2Messages::m_cszFileWriteError, file) {}
		CFileWriteException(CStreamedFile& file, const std::string_view Description) :
			CFileException(fmt::format("{} {}", CDFW2Messages::m_cszFileWriteError, Description), file) {}
	};

	template<class T>
	class CFileExceptionGLE : public T
	{
	public:
		CFileExceptionGLE() : T() { AddGLE(); }
		CFileExceptionGLE(const std::string_view Description) : T(Description) { AddGLE(); }
		CFileExceptionGLE(CStreamedFile& file) : T(file) { AddGLE(); }
		CFileExceptionGLE(CStreamedFile& file, const std::string_view Description) : T(Description, file) { AddGLE(); }

	protected:
		void AddGLE()
		{
			std::string strGetLastErrorMsg = GetLastErrorMessage();
			if (!strGetLastErrorMsg.empty())
				T::m_strMessage += fmt::format(" {}", strGetLastErrorMsg);
			stringutils::removecrlf(T::m_strMessage);
		}

		static std::string GetLastErrorMessage()
		{
#ifdef _MSC_VER
			const DWORD dwError = ::GetLastError();
			if (dwError != 0)
			{
				LPTSTR messageBuffer = nullptr;
				const size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&messageBuffer, 0, NULL);
				std::wstring message(messageBuffer, size);
				LocalFree(messageBuffer);
				return stringutils::utf8_encode(message);
			}
			return std::string("");
#else
			std::error_code code(errno, std::system_category());
			return code.message();
#endif
		}
	};
}