#include "stdafx.h"
#include "FileExceptions.h"
#include "Filewrapper.h"

using namespace DFW2;

CFileException::CFileException(const std::string_view Description, CStreamedFile& file) : CDFW2Exception(Description)
{
	m_strMessage += fmt::format("\"{}\".", file.path());
	if(file.is_open())
		m_strMessage += fmt::format(CDFW2Messages::m_cszFilePostion, file.tellg());
}


bool _trace(TCHAR* format, ...)
{
	TCHAR buffer[1000];

	va_list argptr;
	va_start(argptr, format);
	wvsprintf(buffer, format, argptr);
	va_end(argptr);

	OutputDebugString(buffer);

	return true;
}