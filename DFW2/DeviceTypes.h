#pragma once
//namespace DFW2
//{
	// 	режим связи
	enum eDFW2DEVICELINKMODE
	{
		DLM_SINGLE,		// связь одного устройства с одним устройством
		DLM_MULTI		// связь одного устройства с несколькими устройствами (узел-генератор, узел-ветвь)
	};

	// режим зависимости связи
	enum eDFW2DEVICEDEPENDENCY
	{
		DPD_MASTER,		// устройство является ведущим
		DPD_SLAVE		// устройство является ведомым
	};
	
	enum eVARUNITS
	{
		VARUNIT_KVOLTS,
		VARUNIT_VOLTS,
		VARUNIT_KAMPERES,
		VARUNIT_AMPERES,
		VARUNIT_DEGREES,
		VARUNIT_RADIANS,
		VARUNIT_MW,
		VARUNIT_MVAR,
		VARUNIT_MVA,
		VARUNIT_PU,
		VARUNIT_NOTSET
	};
	
	// Типы устройств
	// пользовательские устройства отдельным типом
	// не предусмотрены и должны соответствовать одному
	// из встроенных
	
	enum eDFW2DEVICETYPE
	{
		DEVTYPE_UNKNOWN,
		DEVTYPE_NODE,
		DEVTYPE_BRANCH,
		DEVTYPE_BRANCHMEASURE,
		DEVTYPE_LRC,
		DEVTYPE_SYNCZONE,
		DEVTYPE_VOLTAGE_SOURCE,
		DEVTYPE_POWER_INJECTOR,
		DEVTYPE_GEN_INFPOWER,
		DEVTYPE_GEN_MOTION,
		DEVTYPE_GEN_1C,
		DEVTYPE_GEN_3C,
		DEVTYPE_GEN_MUSTANG,
		DEVTYPE_EXCITER,
		DEVTYPE_EXCITER_MUSTANG,
		DEVTYPE_EXCCON,
		DEVTYPE_EXCCON_MUSTANG,
		DEVTYPE_DEC,
		DEVTYPE_DEC_MUSTANG,
		DEVTYPE_MODEL,
		DEVTYPE_AUTOMATIC
	};
//}