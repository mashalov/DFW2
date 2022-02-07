#pragma once
#include <filesystem>
#include "dfw2exception.h"

namespace DFW2
{
	class CDynaModel;

	class CPlatformFolders
	{
	protected:

		std::filesystem::path pathRoot;
		std::filesystem::path pathAutomatic;
		std::filesystem::path pathAutomaticBuild;
		std::filesystem::path pathAutomaticModules;
		std::filesystem::path pathCustomModels;
		std::filesystem::path pathCustomModelsBuild;
		std::filesystem::path pathCustomModelsModules;
		std::filesystem::path pathSourceReference;
		std::filesystem::path pathLogs;
		std::filesystem::path pathResults;

		static constexpr const std::string_view automaticModuleName = "Automatic";

#ifdef _MSC_VER
		static constexpr const std::string_view moduleExtension = ".dll";
#else
		static constexpr const std::string_view moduleExtension = ".so";
#endif

#ifdef _MSC_VER
#ifdef _WIN64
		static constexpr const std::string_view platform = "x64";
#else
		static constexpr const std::string_view platform = "Win32";
#endif
#else
#ifdef __x86_64__ 
		static constexpr const  std::string_view platform = "x64";
#else
		static constexpr const  std::string_view platform = "x86";
#endif
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
		static constexpr const std::string_view configuration = "Debug";
#else
		static constexpr const std::string_view configuration = "Release";
#endif 
		CDynaModel& m_Model;
	public:

		CPlatformFolders(CDynaModel& model);

		void CheckPath(std::filesystem::path& path) const;

		constexpr const  std::string_view& Platform() const
		{
			return CPlatformFolders::platform;
		};

		constexpr const std::string_view& Configuration() const
		{
			return CPlatformFolders::configuration;
		}

		void CheckFolderStructure(const std::filesystem::path WorkingFolder);

		const std::filesystem::path& Root() const
		{
			return pathRoot;
		}

		const std::filesystem::path& Automatic() const
		{
			return pathAutomatic;
		}

		const std::filesystem::path& AutomaticBuild() const
		{
			return pathAutomaticBuild;
		}

		const std::filesystem::path& AutomaticModules() const
		{
			return pathAutomaticModules;
		}

		const std::filesystem::path& SourceReference() const
		{
			return pathSourceReference;
		}

		const std::filesystem::path& Logs() const
		{
			return pathLogs;
		}

		constexpr const std::string_view& ModuleExtension() const
		{
			return CPlatformFolders::moduleExtension;
		}

		constexpr const std::string_view& AutomaticModuleName() const
		{
			return CPlatformFolders::automaticModuleName;
		}

		const std::filesystem::path& Results() const
		{
			return pathResults;
		}

		const std::filesystem::path ResultFile(std::string_view FileName) const
		{
			return std::filesystem::path(Results()).append(FileName);
		}
	};
}
