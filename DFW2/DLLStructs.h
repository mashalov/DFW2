﻿#ifndef _DLL_STRUCTS_
#define _DLL_STRUCTS_
#include "DeviceTypes.h"

enum PrimitiveBlockType
{
	PBT_UNKNOWN,
	PBT_LAG,
	PBT_LIMITEDLAG,
	PBT_DERLAG,
	PBT_INTEGRATOR,
	PBT_LIMITEDINTEGRATOR,
	PBT_LIMITERCONST,
	PBT_RELAY,
	PBT_RELAYMIN,
	PBT_RELAYDELAY,
	PBT_RELAYMINDELAY,
	PBT_RELAYDELAYLOGIC,
	PBT_RELAYMINDELAYLOGIC,
	PBT_RSTRIGGER,
	PBT_HIGHER,
	PBT_LOWER,
	PBT_BIT,
	PBT_OR,
	PBT_AND,
	PBT_NOT,
	PBT_ABS,
	PBT_DEADBAND,
	PBT_EXPAND,
	PBT_SHRINK,
	PBT_LAST
};

#endif
