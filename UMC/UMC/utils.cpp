#include "utils.h"

static std::locale GetLocaleUTF8()
{
	std::locale  currentLocale { std::locale() };
	std::string localeName {currentLocale.name()};
	std::transform(localeName.begin(), localeName.end(), localeName.begin(), [](unsigned char c) { return std::tolower(c); });
	if(localeName.find("utf-8") == std::string::npos && localeName.find("utf8") == std::string::npos)
	{
		try
		{
			currentLocale = std::locale("en_US.UTF-8");
		}
		catch(const std::exception&) 
		{
			try
			{
				currentLocale = std::locale("ru_RU.UTF-8");
			}
			catch(const std::exception&) {}
		}
	}
	return currentLocale;
}

std::locale utf8locale = GetLocaleUTF8();
