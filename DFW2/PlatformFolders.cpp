#include "stdafx.h"
#include "PlatformFolders.h"
#include "Messages.h"
#include "DynaModel.h"

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

CPlatformFolders::CPlatformFolders(CDynaModel& model) : m_Model(model)
{

}

void CPlatformFolders::CheckPath(std::filesystem::path& path) const 
{
	if (!std::filesystem::is_directory(path))
	{
		path.replace_extension();
		if (path.has_filename())
		{
			m_Model.Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszPathShouldBeFolder, stringutils::utf8_encode(path.c_str())));
			path.append("/");
		}
		
		std::error_code ec;
		if (!std::filesystem::create_directories(path, ec))
			if (ec)
				throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszFailedToCreateFolder,
					stringutils::utf8_encode(path.c_str())));
	}
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

	pathCustomModels = std::filesystem::path(pathRoot).append("CustomModels");

	pathCustomModelsBuild = std::filesystem::path(pathCustomModels).append(cszBuild).append(Configuration()).append(strPlatform);
	CheckPath(pathCustomModelsBuild);

	pathCustomModelsModules = std::filesystem::path(pathCustomModels).append(cszModules).append(Configuration()).append(strPlatform);
	CheckPath(pathCustomModelsModules);

	pathSourceReference = std::filesystem::path(pathRoot).append(std::string("Reference").append("/"));
	CheckPath(pathSourceReference);

	pathLogs = std::filesystem::path(pathRoot).append("Logs/");
	CheckPath(pathLogs);

	pathResults = std::filesystem::path(pathRoot).append("Results/");
	CheckPath(pathResults);
}