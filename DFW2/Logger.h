#pragma once
#include "Messages.h"

namespace DFW2
{
	class CLogger
	{
	public:
		virtual void Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t nDbIndex = -1) const = 0;
	};

	class CLoggerConsole : public CLogger
	{
		void Log(DFW2MessageStatus Status, std::string_view Message, ptrdiff_t nDbIndex = -1) const override;
	};
}
