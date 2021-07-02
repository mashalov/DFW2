#pragma once

#include "../DFW2/dfw2exception.h"
#include "../DFW2/Messages.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

namespace DFW2
{
	class CStreamedFile;


	class CFileException : public dfw2errorGLE
	{
	public:
		CFileException(CStreamedFile& file) : dfw2errorGLE(FileDescription(file)) {}
		CFileException(CStreamedFile& file, const std::string_view Description) : dfw2errorGLE(FileDescription(file, Description)) {}
		CFileException(std::string_view Description) : dfw2errorGLE(FileDescription(Description)) {}
	protected:
		std::string FileDescription(CStreamedFile& file);
		std::string FileDescription(CStreamedFile& file, const std::string_view Description);
		std::string FileDescription(std::string_view Description);
	};

	class CFileReadException : public CFileException
	{
	public:
		CFileReadException() : CFileException(CDFW2Messages::m_cszFileReadError) {}
		CFileReadException(const std::string_view Description) :
			CFileException(fmt::format("{} {}", CDFW2Messages::m_cszFileReadError, Description)) {}
		CFileReadException(CStreamedFile& file) : CFileException(file, CDFW2Messages::m_cszFileReadError) {}
		CFileReadException(CStreamedFile& file, const std::string_view Description) :
			CFileException(file, fmt::format("{} {}", CDFW2Messages::m_cszFileReadError, Description)) {}
	};

	class CFileWriteException : public CFileException
	{
	public:
		CFileWriteException() : CFileException(CDFW2Messages::m_cszFileWriteError) {}
		CFileWriteException(const std::string_view Description) :
			CFileException(fmt::format("{} {}", CDFW2Messages::m_cszFileWriteError, Description)) {}
		CFileWriteException(CStreamedFile& file) : CFileException(file, CDFW2Messages::m_cszFileWriteError) {}
		CFileWriteException(CStreamedFile& file, const std::string_view Description) :
			CFileException(file, fmt::format("{} {}", CDFW2Messages::m_cszFileWriteError, Description)) {}
	};
}