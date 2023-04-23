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
	path = path.make_preferred();
	if (!std::filesystem::is_directory(path))
	{
		path.replace_extension();
		if (path.has_filename())
		{
			m_Model.Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszPathShouldBeFolder, stringutils::utf8_encode(path.c_str())));
			path += std::filesystem::path::preferred_separator;
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
	const char* cszBuild{ "Build" };
	const char* cszModules{ "Modules" };

	const auto ThreadId{ m_Model.ThreadId() };
	
	if (WorkingFolder.is_absolute())
		pathRoot = WorkingFolder;
	else
		pathRoot = std::filesystem::path(GetUserFolder()).append(WorkingFolder.c_str());



	std::string strPlatform(Platform());
	strPlatform += std::filesystem::path::preferred_separator;

	pathRoot.append(CDFW2Messages::m_cszProjectName);
	CheckPath(pathRoot);

	pathThreadRoot = pathRoot;

	// если задан номер потока - все каталоги, которые
	// должны быть разделены для потоков создаем в подкаталоге
	// с номером потока
	if (ThreadId)
		pathThreadRoot.append(fmt::format("Threads/{:02d}/", ThreadId));

	pathFactory = std::filesystem::path(pathThreadRoot).append("Factory");

	pathAutomatic = std::filesystem::path(pathFactory).append(AutomaticModuleName());
	CheckPath(pathAutomatic);

	pathAutomaticBuild = std::filesystem::path(pathAutomatic).append(cszBuild).append(Configuration()).append(strPlatform);
	CheckPath(pathAutomaticBuild);

	pathAutomaticModules = std::filesystem::path(pathAutomatic).append(cszModules).append(Configuration()).append(strPlatform);
	CheckPath(pathAutomaticModules);

	pathScenario = std::filesystem::path(pathFactory).append(ScenarioModuleName());
	CheckPath(pathScenario);

	pathScenarioBuild = std::filesystem::path(pathScenario).append(cszBuild).append(Configuration()).append(strPlatform);
	CheckPath(pathScenarioBuild);

	pathScenarioModules = std::filesystem::path(pathScenario).append(cszModules).append(Configuration()).append(strPlatform);
	CheckPath(pathScenarioModules);

	pathCustomModels = std::filesystem::path(pathFactory).append("CustomModels");

	pathCustomModelsBuild = std::filesystem::path(pathCustomModels).append(cszBuild).append(Configuration()).append(strPlatform);
	CheckPath(pathCustomModelsBuild);

	pathCustomModelsModules = std::filesystem::path(pathCustomModels).append(cszModules).append(Configuration()).append(strPlatform);
	CheckPath(pathCustomModelsModules);

#ifdef _WINDLL
	pathSourceReference = m_Model.ModuleFilePath();
	pathSourceReference.append("RaidenEMS/Reference/");
#else
	pathSourceReference = std::filesystem::path(pathRoot).append("Reference/");
#endif
	CheckPath(pathSourceReference);

	pathLogs = std::filesystem::path(pathThreadRoot).append("Logs/");
	CheckPath(pathLogs);

	pathResults = std::filesystem::path(pathThreadRoot).append("Results/");
	CheckPath(pathResults);

	pathModelDebugFolder = std::filesystem::path(pathThreadRoot).append("ModelDebugFolder");

}