#include "stdafx.h"
#include "FileExceptions.h"
#include "Filewrapper.h"

using namespace DFW2;

std::string CFileException::FileDescription(CStreamedFile& file)
{
	std::string fileInfo(fmt::format("\"{}\"", file.path()));
	if (file.is_open())
		fileInfo += fmt::format(CDFW2Messages::m_cszFilePostion, file.tellg());
	return fileInfo;
}

std::string CFileException::FileDescription(CStreamedFile& file, const std::string_view Description)
{
	return fmt::format("{} {}", Description, FileDescription(file));
}

std::string CFileException::FileDescription(const std::string_view Description)
{
	return std::string(Description);
}


