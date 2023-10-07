#pragma once
#include <map>
#include <filesystem>
#include "stringutils.h"

template<typename T>
class CLIParser
{
protected:
	using PairsT = std::map<std::string, std::string>;
	class Pairs : protected PairsT
	{
	public:
		PairsT::const_iterator begin() const { return PairsT::begin(); }
		PairsT::const_iterator end() const { return PairsT::options_.end(); }
		PairsT::const_iterator find(std::string_view OptName) const
		{
			return end();
		}

	};

	PairsT options_, commands_;
	std::filesystem::path path_;
public:
	CLIParser(int argc, T** argv)
	{
		if (argc > 0)
			path_ = argv[0];

		PairsT::iterator CurrentCmdPair{ commands_.end() };
		PairsT::iterator CurrentOptPair{ options_.end() };

		for (int count{ 1 }; count < argc; count++)
		{
			std::string arg{ stringutils::COM_decode(argv[count]) };
			stringutils::trim(arg);
			if (arg.rfind('-') == 0)
			{
				arg = arg.substr(1, arg.length());
				stringutils::tolower(arg);
				CurrentOptPair = options_.emplace(arg, "").first;
			}
			else
			{
				if (CurrentOptPair != options_.end())
				{
					CurrentOptPair->second = arg;
					CurrentOptPair = options_.end();
				}
				else if (CurrentCmdPair != commands_.end())
				{
					CurrentCmdPair->second = arg;
					CurrentCmdPair = options_.end();
				}
				else
				{
					stringutils::tolower(arg);
					CurrentCmdPair = commands_.emplace(arg, "").first;
				}
			}
		}
	}

	const PairsT Options()  const { return options_; }
	const PairsT Commands() const { return commands_; }
};
