#pragma once
#include <filesystem>
#include "dfw2exception.h"

namespace DFW2
{
	class CPlatformFolders
	{
	protected:

		std::filesystem::path pathRoot;
		std::filesystem::path pathAutomatic;
		std::filesystem::path pathAutomaticBuild;
		std::filesystem::path pathAutomaticModules;

#ifdef _MSC_VER
#ifdef _WIN64
		static constexpr const char* platform = "x64";
#else
		static constexpr const char* platform = "Win32";
#endif
#else
#ifdef __x86_64__ 
		static constexpr const char* platform = "x64";
#else
		static constexpr const char* platform = "x86";
#endif
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
		static constexpr const char* configuration = "Debug";
#else
		static constexpr const char* configuration = "Release";
#endif 
	public:

		constexpr const char* Platform() const
		{
			return CPlatformFolders::platform;
		};

		constexpr const char* Configuration() const
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
	};
}
