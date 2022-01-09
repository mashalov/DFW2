#pragma once
#include "Messages.h"
#include <string>

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

	class CProgress
	{
	public:
		enum class ProgressStatus
		{
			Stop,
			Pause,
			Continue
		};
		virtual void StartProgress(std::string_view Caption, int RangeMin, int RangeMax) {}
		virtual ProgressStatus UpdateProgress(std::string_view Caption, int Progress) { return ProgressStatus::Continue; }
		virtual void EndProgress() {}
	};
}
