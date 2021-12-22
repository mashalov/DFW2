#ifndef _DLL_STRUCTS_
#define _DLL_STRUCTS_
#include "DeviceTypes.h"
#include "limits.h"

#define DFW2_NON_STATE_INDEX LONG_MAX
#define CUSTOMDEVICEDLL_MAXNAME 80
typedef char STRING80[CUSTOMDEVICEDLL_MAXNAME];


struct DLLVariableIndex
{
	ptrdiff_t Index;
	double Value;
};

enum eLINKTYPE
{
	eLINK_TO,
	eLINK_FROM
};

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

enum eVARIABLELOCATION
{
	eVL_INTERNAL,
	eVL_EXTERNAL
};

enum CONSTVARSFLAGS
{
	CVF_ZEROTODEFAULT = 0x1,
	CVF_INTERNALCONST = 0x2,
};

typedef struct
{
	enum eVARIABLELOCATION Location;
	size_t nIndex;
}
	BLOCKPININDEX;

typedef struct 
{
	size_t nIndex;
	STRING80 Name;
	int eUnits;
	int bOutput;
} 
	VarsInfo;

typedef struct
{
	VarsInfo VarInfo;
	int eDeviceType;
	STRING80 Key;
}
	InputVarsInfo;

typedef struct
{
	VarsInfo VarInfo;
	int eDeviceType;
	double dMin;
	double dMax;
	double dDefault;
	int VarFlags;
}
	ConstVarsInfo;

typedef struct 
{
	ptrdiff_t UserId;
	enum PrimitiveBlockType eType;
	STRING80 Name;
} 
	BlockDescriptions;

// структура внешей переменой
// про внешнюю переменную другого устройства 
// нужно знать:
typedef struct
{
	double *pValue;			// указатель на значение
	ptrdiff_t nIndex;		// индекс (номер строки) в системе Якоби
}
	DLLExternalVariable;

typedef struct 
{
	int eLinkType;
	int eDeviceType;
	int eLinkMode;
	int eDependency;
	STRING80 LinkField;
}
	LinkType;

typedef struct
{
	void *pModel;
	void *pDevice;
}
	BuildEquationsObjects;

	typedef long(*MDLSETELEMENT)(BuildEquationsObjects*, ptrdiff_t, ptrdiff_t, double, int);
	typedef long(*MDLSETFUNCTION)(BuildEquationsObjects*, ptrdiff_t, double);
	typedef long(*MDLSETFUNCTIONDIFF)(BuildEquationsObjects*, ptrdiff_t, double);
	typedef long(*MDLSETDERIVATIVE)(BuildEquationsObjects*, ptrdiff_t, double);
	typedef long(*MDLPROCESSBLOCKDISCONTINUITY)(BuildEquationsObjects*, long);
	typedef long(*MDLINITBLOCK)(BuildEquationsObjects*, long);

typedef struct 
{
	MDLSETELEMENT					pFnSetElement;
	MDLSETFUNCTION					pFnSetFunction;
	MDLSETFUNCTIONDIFF				pFnSetFunctionDiff;
	MDLSETDERIVATIVE				pFnSetDerivative;
	MDLPROCESSBLOCKDISCONTINUITY	pFnProcessBlockDiscontinuity;
	MDLINITBLOCK					pFnInitBlock;
	BuildEquationsObjects BuildObjects;
	DLLVariableIndex* pEquations;
	double* pConsts;
	double* pSetPoints;
	DLLExternalVariable* pExternals;
	double Omega;
	double t;
}
	BuildEquationsArgs;

#endif
