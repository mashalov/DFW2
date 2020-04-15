#pragma once
#include "ExpressionToken.h"

class CTransformPlus : public CExpressionTransform
{
protected:
	static bool SimplifyPlusConstants(CExpressionToken *pToken);
	static bool SimplifySum(CExpressionToken *pToken);
	static bool SimplifyPlusDiminished(CExpressionToken *pToken);
	static bool SimplifySumCombineBraces(CExpressionToken* pSum);
public:
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);

};

class CTransformMinus : public CExpressionTransform
{
public:
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
};

class CTransformMul : public CExpressionTransform
{
protected:
	static bool SimplifyMulConstant(CExpressionToken* pToken);
	static bool SimplifyMulDiminished(CExpressionToken* pToken); 
	static bool SimplifyMulExpandBraces(CExpressionToken *pToken);
	static bool SimplifyMulCombinePow(CExpressionToken *pToken);
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
	static const _TCHAR *cszMul;
};

class CTransformDiv : public CExpressionTransform
{
protected:
	static bool SimplifyDivConvertToPow(CExpressionToken *pToken);
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
};

class CTransformSin : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};

class CTransformCos : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};

class CTransformTan : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};

class CTransformAtan2 : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};


class CTransformAsin : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};

class CTransformAcos : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};

class CTransformUminus : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
};

class CTransformPow : public CExpressionTransform
{
	static bool SimplifyPowConstant(CExpressionToken* pToken);
	static bool SimplifyPowChain(CExpressionToken* pToken);
	static bool SimplifyPowNested(CExpressionToken* pToken);
public:
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);

	static const _TCHAR *cszPow;
};


class CTransformExp : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};


class CTransformLn : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};

class CTransformLog : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool Derivative(CExpressionToken *pToken, CExpressionToken *pChildToken, std::wstring& Result);
};


class CTransformSqrt : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
};

class CTransformAbs : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);	
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;

};


class CTransformRelayDelay : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
};

class CTransformRelayDelayLogic : public CTransformRelayDelay
{
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
};

class CTransformExpand : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
};

class CTransformShrink : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;

};

class CTransformLimit : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
};

class CTransformHigher : public CExpressionTransform
{
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
	virtual const long GetPinsCount() { return 3; }
};

class CTransformLower : public CExpressionTransform
{
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
	virtual const long GetPinsCount() { return 3; }
};

class CTransformBit : public CExpressionTransform
{
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
};

class CTransformAnd : public CExpressionTransform
{
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	virtual const _TCHAR* SpecialInitFunctionName() { return m_cszInitFunction; };
	static const _TCHAR* m_cszBlockType;
	static const _TCHAR* m_cszInitFunction;
	virtual const long GetPinsCount() { return 3; }
};

class CTransformOr : public CExpressionTransform
{
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	virtual const _TCHAR* SpecialInitFunctionName() { return m_cszInitFunction; };
	static const _TCHAR* m_cszBlockType;
	static const _TCHAR* m_cszInitFunction;
	virtual const long GetPinsCount() { return 3; }
};


class CTransformNot : public CExpressionTransform
{
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	virtual const _TCHAR* SpecialInitFunctionName() { return m_cszInitFunction; };
	static const _TCHAR* m_cszBlockType;
	static const _TCHAR* m_cszInitFunction;
};

class CTransformDeadband : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken);
	virtual bool Expand(CExpressionToken* pToken);
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
};

class CTransformDerlag : public CExpressionTransform
{
public:
	virtual bool Simplify(CExpressionToken* pToken) { return true; }
	virtual bool Expand(CExpressionToken* pToken)   { return true; }
	virtual size_t EquationsCount()	{ return 2; }
	virtual bool IsHostBlock() { return true; }
	virtual const _TCHAR* GetBlockType() { return m_cszBlockType; }
	static const _TCHAR* m_cszBlockType;
};