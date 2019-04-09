#include "stdafx.h"
#include "ExpressionDictionary.h"


CParserDictionary::CParserDictionary()
{

}

CParserDefaultDictionary::CParserDefaultDictionary() : CParserDictionary()
{

	for (FunctionEnum *pFnEnum = DefaultFunctions; pFnEnum < DefaultFunctions + sizeof(DefaultFunctions) / sizeof(DefaultFunctions[0]); pFnEnum++)
		FunctionsEnum.push_back(pFnEnum);
	for (OperatorEnum *pOpEnum = DefaultOperators; pOpEnum < DefaultOperators + sizeof(DefaultOperators) / sizeof(DefaultOperators[0]); pOpEnum++)
		OperatorsEnum.push_back(pOpEnum);
}

FunctionEnum CParserDefaultDictionary::DefaultFunctions[21] = {
	FunctionEnum(ETT_ATAN2,		_T("atan2"),		2, &TransAtan2),
	FunctionEnum(ETT_SIN,		_T("sin"),			1, &TransSin),
	FunctionEnum(ETT_COS,		_T("cos"),			1, &TransCos),
	FunctionEnum(ETT_TAN,		_T("tan"),			1, &TransTan),
	FunctionEnum(ETT_POWF,		_T("pow"),			2, &TransPow),
	FunctionEnum(ETT_SQRT,		_T("sqrt"),			1, &TransSqrt),
	FunctionEnum(ETT_LN,		_T("log"),			1, &TransLn),
	FunctionEnum(ETT_LOG,		_T("log10"),		1, &TransLog),
	FunctionEnum(ETT_EXP,		_T("exp"),			1, &TransExp),
	FunctionEnum(ETT_ASIN,		_T("asin"),			1, &TransAsin),
	FunctionEnum(ETT_ACOS,		_T("acos"),			1, &TransAcos),
	FunctionEnum(ETT_RELAY,		_T("relay"),	   -1, &TransRelayDelay),
	FunctionEnum(ETT_RELAYD,	_T("relayd"),	   -1, &TransRelayDelay),
	FunctionEnum(ETT_EXPAND,	_T("expand"),		2, &TransExpand),
	FunctionEnum(ETT_SHRINK,	_T("shrink"),		2, &TransShrink),
	FunctionEnum(ETT_ABS,		_T("abs"),			1, &TransAbs),
	FunctionEnum(ETT_LIMIT,		_T("limit"),		3, &TransLimit),
	FunctionEnum(ETT_DEADBAND,	_T("deadband"),		2, &TransDB),
	FunctionEnum(ETT_MODANGLE,	_T("modangle"),		3, NULL),
	FunctionEnum(ETT_DERLAG,    _T("derlag"),		3, &TransDerlag),
	FunctionEnum(ETT_BIT,		_T("bit"),			1, &TransBit)
};

OperatorEnum CParserDefaultDictionary::DefaultOperators[16] = {
	OperatorEnum(ETT_PLUS,		_T("+"),	2,	&TransPlus,		5,	0),
	OperatorEnum(ETT_MINUS,		_T("-"),	2,	&TransMinus,	5,	0),
	OperatorEnum(ETT_UMINUS,	_T("-"),	1,	&TransUMinus,	7,	1),
	OperatorEnum(ETT_NOT,		_T("!"),	1,	&TransNot,		7,	1),
	OperatorEnum(ETT_MUL,		CTransformMul::cszMul,		2,	&TransMul,		6,	0),
	OperatorEnum(ETT_DIV,		_T("/"),	2,	&TransDiv,		6,	0),
	OperatorEnum(ETT_POW,		_T("^"),	2,	&TransPow,		8,	1),
	OperatorEnum(ETT_HIGHER,	_T(">"),	2,	&TransHigher,	4,	0),
	OperatorEnum(ETT_LOWER,		_T("<"),	2,	&TransLower,	4,	0),
	OperatorEnum(ETT_AND,		_T("&"),	2,	&TransAnd,		2,	0),
	OperatorEnum(ETT_OR,		_T("|"),	2,	&TransOr,		2,	0),
	OperatorEnum(ETT_LB,		_T("("),	0,	NULL,			1,	0),
	OperatorEnum(ETT_RB,		_T(")"),	0,	NULL,			99, 0),
	OperatorEnum(ETT_LBS,		_T("["),	0,	NULL,			1,	0),
	OperatorEnum(ETT_RBS,		_T("]"),	0,	NULL,			99, 0),
	OperatorEnum(ETT_COMMA,		_T(","),	0,	NULL,			99, 0),
};


CTransformPlus			CParserDefaultDictionary::TransPlus;
CTransformMinus			CParserDefaultDictionary::TransMinus;
CTransformUminus		CParserDefaultDictionary::TransUMinus;
CTransformMul			CParserDefaultDictionary::TransMul;
CTransformDiv			CParserDefaultDictionary::TransDiv;
CTransformPow			CParserDefaultDictionary::TransPow;
CTransformCos			CParserDefaultDictionary::TransCos;
CTransformSin			CParserDefaultDictionary::TransSin;
CTransformSqrt			CParserDefaultDictionary::TransSqrt;
CTransformAbs			CParserDefaultDictionary::TransAbs;
CTransformTan			CParserDefaultDictionary::TransTan;
CTransformAtan2			CParserDefaultDictionary::TransAtan2;
CTransformExp			CParserDefaultDictionary::TransExp;
CTransformLn			CParserDefaultDictionary::TransLn;
CTransformLog			CParserDefaultDictionary::TransLog;
CTransformAsin			CParserDefaultDictionary::TransAsin;
CTransformAcos			CParserDefaultDictionary::TransAcos;
CTransformRelayDelay	CParserDefaultDictionary::TransRelayDelay;
CTransformExpand		CParserDefaultDictionary::TransExpand;
CTransformShrink		CParserDefaultDictionary::TransShrink;
CTransformLimit			CParserDefaultDictionary::TransLimit;
CTransformDerlag		CParserDefaultDictionary::TransDerlag;
CTransformDeadband		CParserDefaultDictionary::TransDB;
CTransformHigher		CParserDefaultDictionary::TransHigher;
CTransformLower			CParserDefaultDictionary::TransLower;
CTransformBit			CParserDefaultDictionary::TransBit;
CTransformAnd			CParserDefaultDictionary::TransAnd;
CTransformOr			CParserDefaultDictionary::TransOr;
CTransformNot			CParserDefaultDictionary::TransNot;


CExpressionRulesDefault::CExpressionRulesDefault()
{
	push_back(new CExpressionRule(_T("x^2+2*x*y+y^2"),				_T("(x+y)^2")));
	push_back(new CExpressionRule(_T("x^2-2*x*y+y^2"),				_T("(x-y)^2")));
	push_back(new CExpressionRule(_T("x^2-y^2"),					_T("(x+y)*(x-y)")));
	push_back(new CExpressionRule(_T("sin(x)^2+cos(x)^2"),			_T("1")));
	push_back(new CExpressionRule(_T("2*sin(x)*cos(x)"),			_T("sin(2*x)")));
}

CExpressionRulesDefault::~CExpressionRulesDefault()
{
	while (!empty())
	{
		delete front();
		pop_front();
	}
}