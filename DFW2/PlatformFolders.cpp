#include "stdafx.h"
#include "PlatformFolders.h"
#include "Messages.h"

#ifdef _MSC_VER
#include <shlobj_core.h>
#include <tchar.h>
#else
#include <stdlib.h>
#endif

using namespace DFW2;

std::filesystem::path GetUserFolder()
{
#ifdef _MSC_VER
	PWSTR ppszPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &ppszPath)))
		throw dfw2errorGLE("CPlatformFolders::GetUserFolder - failed to get Documents path");
	std::wstring documents(ppszPath);
	CoTaskMemFree(ppszPath);
	return documents;
#else
	return getenv("HOME");
#endif
}

void CheckPath(const std::filesystem::path& path)
{
	if (!std::filesystem::is_directory(path))
		if (!std::filesystem::create_directories(path))
			throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszFailedToCreateFolder, path.generic_string()));
}

void CPlatformFolders::CheckFolderStructure(const std::filesystem::path WorkingFolder)
{
	const char* cszBuild = "Build";
	const char* cszModules = "Modules";
	
	if (WorkingFolder.is_absolute())
		pathRoot = WorkingFolder;
	else
		pathRoot = std::filesystem::path(GetUserFolder()).append(WorkingFolder.c_str());

	std::string strPlatform(Platform());
	strPlatform.append("/");

	pathRoot.append(CDFW2Messages::m_cszProjectName);
	CheckPath(pathRoot);

	pathAutomatic = std::filesystem::path(pathRoot).append("Automatic");
	CheckPath(pathAutomatic);

	pathAutomaticBuild = std::filesystem::path(pathAutomatic).append(cszBuild).append(Configuration()).append(strPlatform);
	CheckPath(pathAutomaticBuild);

	pathAutomaticModules = std::filesystem::path(pathAutomatic).append(cszModules).append(Configuration()).append(strPlatform);
	CheckPath(pathAutomaticModules);

}