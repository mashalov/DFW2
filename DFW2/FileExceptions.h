#pragma once
#include "Header.h"
#include "Messages.h"

namespace DFW2
{
	class CFileException : public CDFW2GetLastErrorException
	{
	public:
		CFileException(std::wstring_view Description, FILE *pFile) : CDFW2GetLastErrorException(Description)
		{
			if (pFile)
			{
				m_strMessage += fmt::format(CDFW2Messages::m_cszFilePostion, _ftelli64(pFile));
			}
		}
	};

	class CFileReadException : public CFileException
	{
	public:
		CFileReadException(FILE *pFile) : CFileException(CDFW2Messages::m_cszFileReadError, pFile) {}
		CFileReadException(FILE *pFile, std::wstring_view Description) : 
			CFileException(fmt::format(_T("{} {}"), CDFW2Messages::m_cszFileReadError, Description), pFile) {}
	};

	class CFileWriteException : public CFileException
	{
	public:
		CFileWriteException(FILE *pFile) : CFileException(CDFW2Messages::m_cszFileWriteError, pFile) {}
		CFileWriteException(FILE *pFile, std::wstring_view Description) :
			CFileException(fmt::format(_T("{} {}"), CDFW2Messages::m_cszFileWriteError, Description), pFile) {}
	};
}