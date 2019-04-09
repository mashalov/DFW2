#pragma once
#include "ExpressionToken.h"
#include "ExpressionTransform.h"

class CParserDictionary
{
public:
	CParserDictionary();
	FUNCTIONVEC FunctionsEnum;
	OPERATORVEC OperatorsEnum;
};

class CParserDefaultDictionary : public CParserDictionary
{
	static FunctionEnum DefaultFunctions[21];
	static OperatorEnum DefaultOperators[16];

	static CTransformPlus			TransPlus;
	static CTransformMinus			TransMinus;
	static CTransformUminus			TransUMinus;
	static CTransformMul			TransMul;
	static CTransformDiv			TransDiv;
	static CTransformPow			TransPow;
	static CTransformCos			TransCos;
	static CTransformSin			TransSin;
	static CTransformSqrt			TransSqrt;
	static CTransformAbs			TransAbs;
	static CTransformTan			TransTan;
	static CTransformAtan2			TransAtan2;
	static CTransformExp			TransExp;
	static CTransformLn				TransLn;
	static CTransformLog			TransLog;
	static CTransformAsin			TransAsin;
	static CTransformAcos			TransAcos;
	static CTransformRelayDelay		TransRelayDelay;
	static CTransformExpand			TransExpand;
	static CTransformShrink			TransShrink;
	static CTransformLimit			TransLimit;
	static CTransformHigher			TransHigher;
	static CTransformLower			TransLower;
	static CTransformDerlag			TransDerlag;
	static CTransformBit			TransBit;
	static CTransformAnd			TransAnd;
	static CTransformOr				TransOr;
	static CTransformNot			TransNot;
	static CTransformDeadband		TransDB;

public:
	CParserDefaultDictionary();
};

class CExpressionRulesDefault : public EXPRESSIONRULES
{
public:
	CExpressionRulesDefault();
	virtual ~CExpressionRulesDefault();
};