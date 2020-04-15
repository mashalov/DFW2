#pragma once
#include "Header.h"
#include "Messages.h"

namespace DFW2
{
	class CFileException : public CDFW2GetLastErrorException
	{
	public:
		CFileException(const _TCHAR *cszMessage, FILE *pFile) : CDFW2GetLastErrorException(cszMessage)
		{
			if (pFile)
			{
				m_strMessage += Cex(CDFW2Messages::m_cszFilePostion, _ftelli64(pFile));
			}
		}
	};

	class CFileReadException : public CFileException
	{
	public:
		CFileReadException(FILE *pFile) : CFileException(CDFW2Messages::m_cszFileReadError, pFile) {}
		CFileReadException(FILE *pFile, const _TCHAR* cszDescription) : 
			CFileException(Cex(_T("%s %s"), CDFW2Messages::m_cszFileReadError, cszDescription), pFile) {}
	};

	class CFileWriteException : public CFileException
	{
	public:
		CFileWriteException(FILE *pFile) : CFileException(CDFW2Messages::m_cszFileWriteError, pFile) {}
		CFileWriteException(FILE *pFile, const _TCHAR* cszDescription) :
			CFileException(Cex(_T("%s %s"), CDFW2Messages::m_cszFileWriteError, cszDescription), pFile) {}
	};
}