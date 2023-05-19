#pragma once
#include "IntegratorBase.h"
namespace DFW2
{
	class Rodas4 : public IntegratorBase
	{
	public:
		Rodas4(CDynaModel& DynaModel);
		void Step() override;
	};
}
