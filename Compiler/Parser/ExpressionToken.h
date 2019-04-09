#pragma once

#include "stack"
#include "list"
#include "set"
#include "vector"
#include "map"
#include "algorithm"
#include "string"
using namespace std;

#include "cex.h"

class CExpressionToken;
class CCompilerEquation;
class CCompilerEquations;
class CExpressionParser;
class CExpressionParserRule;

enum eExpressionSymbolType
{
	EST_NUMBER,
	EST_OPERATOR,
	EST_DOT,
	EST_COMMA,
	EST_ALPHA,
	EST_ERROR,
	EST_EOF
};

enum eExpressionTokenType
{
	ETT_UNDEFINED,
	ETT_EOF,
	ETT_ERROR_FIRST,
	ETT_ERROR_WRONGSYMBOL,
	ETT_ERROR_WRONGNUMBER,
	ETT_ERROR_UNKNOWNFUNCTION,
	ETT_ERROR_STACKERROR,
	ETT_ERROR_WRONGNUMBEROFARGS,
	ETT_ERROR_NOLEFTPARENTHESIS,
	ETT_ERROR_NORIGHTPARENTHESIS,
	ETT_ERROR_DIVISIONBYZERO,
	ETT_ERROR_NEGATIVELOG,
	ETT_ERROR_NEGATIVEROOT,
	ETT_ERROR_WRONGARG,
	ETT_ERROR_WRONGRANGE,
	ETT_ERROR_LAST,
	ETT_NUMERIC_CONSTANT,
	ETT_FUNCTION,
	ETT_VARIABLE,
	ETT_PROPERTY,
	ETT_MODELLINK,
	ETT_FIRST_OP,
	ETT_PLUS,
	ETT_MINUS,
	ETT_UMINUS,
	ETT_NOT,
	ETT_MUL,
	ETT_DIV,
	ETT_POW,
	ETT_LB,
	ETT_RB,
	ETT_HIGHER,
	ETT_LOWER,
	ETT_AND,
	ETT_OR,
	ETT_COMMA,
	ETT_LBS,
	ETT_RBS,
	ETT_LAST_OP,
	ETT_FIRST_FUNCTION,
	ETT_SIN,
	ETT_COS,
	ETT_TAN,
	ETT_ATAN2,
	ETT_SQRT,
	ETT_POWF,
	ETT_LN,
	ETT_LOG,
	ETT_EXP,
	ETT_ASIN,
	ETT_ACOS,
	ETT_RELAY,
	ETT_RELAYD,
	ETT_RELAYAL,
	ETT_EXPAND,
	ETT_SHRINK,
	ETT_ABS,
	ETT_LIMIT,
	ETT_DEADBAND,
	ETT_MODANGLE,
	ETT_BIT,
	ETT_MA,
	ETT_TIME,
	ETT_DERLAG,
	ETT_LAST_FUNCTION,
	ETT_ROOT
};

enum eCOMPILERVARTYPE
{
	eCVT_INTERNAL,
	eCVT_EXTERNAL,
	eCVT_EXTERNALSETPOINT,
	eCVT_CONST,
	eCVT_HOST
};

class CVariableExtendedInfoBase
{
public:
	virtual ~CVariableExtendedInfoBase() {}
};

// ���������� � ����������
struct VariableEnum
{
	ptrdiff_t m_nIndex;					// ������ (����� ���������, ���� ���������� � ���� ������)
	CExpressionToken *m_pToken;			// �����, ������� ��������� �������� ����������
	size_t m_nRefCount;					// ������� ������ �� ��� ����������
	eCOMPILERVARTYPE m_eVarType;		// ��� ���������� �� ����������� ������
	ptrdiff_t GetIndex() const;			// ������� ������������ ������ � ����������� �� ����
	CVariableExtendedInfoBase *m_pVarExtendedInfo;
	VariableEnum(CExpressionToken *pToken) : m_pToken(pToken), 
											 m_nRefCount(0), 
											 m_nIndex(0), 
											 m_eVarType(eCVT_INTERNAL),
											 m_pVarExtendedInfo(NULL)
	{

	};

	~VariableEnum()
	{
		if (m_pVarExtendedInfo)
			delete m_pVarExtendedInfo;
	}
};


typedef stack<CExpressionToken*> PARSERSTACK;
typedef stack<const CExpressionToken*> CONSTPARSERSTACK;
typedef list<CExpressionToken*> TOKENLIST;
typedef TOKENLIST::iterator TOKENITR;
typedef TOKENLIST::reverse_iterator RTOKENITR;
typedef TOKENLIST::const_reverse_iterator CONSTRTOKENITR;
typedef TOKENLIST::const_iterator CONSTTOKENITR;
typedef set<CExpressionToken*> TOKENSET;
typedef TOKENSET::iterator TOKENSETITR;

typedef set<const CExpressionToken*> CONSTTOKENSET;
typedef CONSTTOKENSET::iterator CONSTTOKENSETITR;


// ������� ����� ��� �������������� ��������� � �������� � ����������
class CExpressionTransform
{
public:
	// ���������
	virtual bool Simplify(CExpressionToken* pToken) = 0;		
	// �������������
	virtual bool Expand(CExpressionToken* pToken) = 0;
	// ������������ ���������� ������������� �����������
	virtual bool Derivative(CExpressionToken* pToken, CExpressionToken *pChild, wstring& Result) { return false;  };
	// ������� ���������� ���������, ����������� ������� ��� ��������
	virtual size_t EquationsCount() { return 1; }
	// ������� ���������� ������� ��� ��������� � ������, � �� � ������������� ������
	virtual bool IsHostBlock() { return false;  }
	// ������� ���� �����, ���� �� ���������� � ������ ?
	virtual const _TCHAR* GetBlockType() { _ASSERTE(NULL);  return NULL; }
	// ������� ���������� ������� � ����� (����� + ������)
	virtual const long GetPinsCount() { return 2; }
	// �������, ������������ ������ ��������� ������������� �����, ���� �����
	virtual const _TCHAR* SpecialInitFunctionName() { return NULL; }

	static void CompareTokens(CExpressionToken* pToken1,
		CExpressionToken*pToken2,
		TOKENLIST& MatchList1,
		TOKENLIST& MatchList2,
		TOKENLIST& DiffList1,
		TOKENLIST& DiffList2);

	static bool ConstantCoefficient(TOKENLIST& DiffList, double& Coe);

};

// ���������� � �������
struct FunctionEnum
{
	eExpressionTokenType m_eOperatorType;		// ��� �������
	wstring m_strOperatorText;					// ��������� �������������
	int m_nArgsCount;							// ���������� ����������
	CExpressionTransform *m_pTransform;			// ��������� �� ����� ��������������

	FunctionEnum(eExpressionTokenType eType, 
				 const _TCHAR* cszOperatorText, 
				 int nArgsCount,
				 CExpressionTransform *pTransform) :	

				 m_eOperatorType(eType),
				 m_strOperatorText(cszOperatorText),
				 m_nArgsCount(nArgsCount),
				 m_pTransform(pTransform) { }
};

// ���������� �� ���������
struct OperatorEnum : FunctionEnum
{
	int m_nPrecedence;		// ��� ��� � �������, �� ������������� �������� ����������� � ������� ������ ���������������
	int m_nRightAssoc;

	OperatorEnum(eExpressionTokenType eType, 
				 const _TCHAR* cszOperatorText, 
				 int nArgsCount,
				 CExpressionTransform* pTransform,
				 int nPrecedence, 
				 int nRightAssoc) :
																				
				 FunctionEnum(eType, cszOperatorText, nArgsCount, pTransform),
				 m_nPrecedence(nPrecedence), 
				 m_nRightAssoc(nRightAssoc) {}
	
};

typedef vector<FunctionEnum*> FUNCTIONVEC;
typedef FUNCTIONVEC::const_iterator FUNCTIONITR;
typedef vector<OperatorEnum*> OPERATORVEC;
typedef OPERATORVEC::const_iterator OPERATORITR;


// �������� ���������
class CCompilerEquation
{
public:
	CExpressionToken *m_pToken;		// �����, ��� �������� ������������ ���������
	ptrdiff_t m_nIndex;				// ����� ���������
	CCompilerEquation(CExpressionToken *pToken, ptrdiff_t nIndex) : m_pToken(pToken), m_nIndex(nIndex) {}
	// ��������� ������� ��� ������ �� ������ ���� ���������� � ���������
	static const _TCHAR *m_cszInternalVar;		
	static const _TCHAR *m_cszExternalVar;
	static const _TCHAR *m_cszConstVar;
	static const _TCHAR *m_cszSetpointVar;
	static const _TCHAR *m_cszHostVar;
	// ������� ��������� ���������� ������������� ��������� (��������� ��� ��������� �������������)
	wstring Generate(bool bInitEquiation = false) const;
	static bool IsParentNeedEquation(CExpressionToken *pToken);
};


class CExpressionToken
{

public:
	eExpressionTokenType m_eType;			//	��� ������
	size_t m_nTokenBegin;					//  ������ ������ � ����������� ������
	size_t m_nTokenLength;					//  ����� ������
	TOKENLIST m_Children;					//  ������, ���������� ��������� � �������
	TOKENSET m_Parents;						//  ��������� ���������� ��������� ������
	const FunctionEnum *m_pFunctionInfo;	//  ��� ������-������� - ���������� � �������
	wstring m_strStringValue;				//  �������� ������������� ������
	wstring m_strEquation;					//  �������� ������������� ��������� ������
	CExpressionParser *m_pParser;			//  ��������� �������, �������� ����������� �����
	CCompilerEquation *m_pEquation;			//  ��������� ������

	void GetOperand(CExpressionToken *pToken, wstring& Result, bool bRight) const;
	bool GetEquationOperandIndex(const CCompilerEquation *pEquation, wstring& Result) const;
	void GetEquationOperandType(const CCompilerEquation *pEquation, wstring& Result, VariableEnum*& pVarEnum) const;
	void GetEquationVariableType(const CCompilerEquation *pEquation, VariableEnum* pVarEnum, wstring& Result) const;
	

	CExpressionToken(CExpressionParser *pParser, eExpressionTokenType eType, size_t nTokenBegin, size_t nTokenLength);
	CExpressionToken(CExpressionParser *pParser, const FunctionEnum *pFunction, size_t nTokenBegin, size_t nTokenLength);
	virtual ~CExpressionToken() {}


	// ������� ��� ����������� ����������� ���� ������
	bool IsEOF() const			{ return m_eType == ETT_EOF; }
	bool IsVariable() const		{ return m_eType == ETT_VARIABLE || m_eType == ETT_MODELLINK; }
	bool IsLeaf() const			{ return m_eType == ETT_NUMERIC_CONSTANT || IsVariable() ;  }
	bool IsConst() const;
	bool IsConstParameter() const;
	bool IsOperator() const		{ return m_eType > ETT_FIRST_OP && m_eType < ETT_LAST_OP; }
	bool IsFunction() const		{ return m_eType > ETT_FIRST_FUNCTION && m_eType < ETT_LAST_FUNCTION; }
	bool IsUnary() const		{ return m_eType == ETT_UMINUS; }
	bool IsError() const		{ return m_eType > ETT_ERROR_FIRST && m_eType < ETT_ERROR_LAST; }
	bool IsNumeric() const		{ return m_eType == ETT_NUMERIC_CONSTANT; }

	CExpressionToken* GetChild();
	CExpressionToken* GetParent();
	bool GetChildren(CExpressionToken*& pChild1, CExpressionToken*& pChild2);
	double GetNumericValue() const;
	int Precedence() const;
	bool RightAssociativity() const;
	int ArgsCount() const;
	eExpressionTokenType GetType() const { return m_eType;  }
	size_t TextLength() const { return m_nTokenLength; }
	size_t TextBegin()  const { return m_nTokenBegin;  }
	void AddChild(CExpressionToken* pChild);
	void RemoveChildren();
	CExpressionToken* NewNumericConstant(double dConstant);
	CExpressionToken* NewExpressionToken(eExpressionTokenType eType);
	CExpressionToken* Clone();
	bool ChangeType(eExpressionTokenType eNewType);
	void Delete();
	void ReplaceChild(CExpressionToken* pOldChild, CExpressionToken* pNewChild, int nChildNumber = -1);
	void AddParent(CExpressionToken *pParent);
	void Unparent(CExpressionToken* pParent);
	void Eliminate();
	bool Join(CExpressionToken *pToken);
	CExpressionToken::TOKENITR RemoveChild(TOKENITR& rit);
	CExpressionToken::TOKENITR RemoveChild(CExpressionToken* pChild);
	void AdvanceChild(size_t nAdvance);
	void SetError(eExpressionTokenType eError) { m_eType = eError; }
	void SetError(eExpressionTokenType eError, size_t nPosition) { m_eType = eError; m_nTokenBegin = nPosition; m_nTokenLength = 1; }

	TOKENITR ChildrenBegin()		{ return m_Children.begin();  }
	TOKENITR ChildrenEnd()			{ return m_Children.end();	  }
	RTOKENITR FirstChild()			{ return m_Children.rbegin(); }
	RTOKENITR LastChild()			{ return m_Children.rend();   }

	CONSTRTOKENITR FirstChild() const	{ return m_Children.rbegin(); }
	CONSTRTOKENITR LastChild()	const	{ return m_Children.rend(); }

	
	CONSTTOKENITR ChildrenBegin() const	{ return m_Children.begin(); }
	CONSTTOKENITR ChildrenEnd()	const	{ return m_Children.end(); }

	size_t ChildrenCount() const	{ return m_Children.size();  }
	size_t ParentsCount() const		{ return m_Parents.size();   }
	void SetNumericConstant(const _TCHAR *cszNumericConstant);
	void SetNumericConstant(double Constant);
	void SetOperator(FunctionEnum *pOpEnum);
	bool Simplify();
	bool RequireSpecialInit();
	const _TCHAR* SpecialInitFunctionName();
	size_t EquationsCount() const;
	bool IsHostBlock() const;
	const _TCHAR *GetBlockType() const;
	long GetPinsCount() const;
	bool Expand();
	bool Derivative(CExpressionToken *pChild, wstring& Result);
	ptrdiff_t Compare(const CExpressionToken *pToCompare) const;
	bool ParserChanged() const;
	bool SortChildren();
	virtual const _TCHAR* EvaluateText();
	void SetTextValue(const _TCHAR *cszValue);
	const _TCHAR* GetTextValue() const;
	void GetEquationOperand(const CCompilerEquation *pEquation, wstring& Result) const;
	const CExpressionParser* GetParser();
	static void GetNumericText(double dValue, wstring& TextValue);
	const _TCHAR* GetBlockName() const;
	static wstring GenerateFunction(CExpressionToken *pToken, bool bEquationMode = false);
	static bool CheckTokenIsConstant(const CExpressionToken *pToken);
};

class CExpressionTokenModelLink : public CExpressionToken
{
protected:
	TOKENLIST m_ChildrenSave;
public:

	// ������ ��������� �������� � ������ �� ������
	enum eModelLinkSymbolWait
	{
		MLS_NUMBER,
		MLS_NUMBER_CONTINUE,
		MLS_DOT,
		MLS_ALPHA,
		MLS_ALPHA_CONTINUE,
		MLS_END
	} 
		nWait;

	CExpressionTokenModelLink(CExpressionParser *pParser, size_t nTokenBegin, size_t nTokenLength) : 
																CExpressionToken(pParser, ETT_MODELLINK, nTokenBegin, nTokenLength),
																nWait(MLS_NUMBER)
																{}

	virtual const _TCHAR* EvaluateText();
};

typedef map<wstring,VariableEnum> VARIABLES;
typedef VARIABLES::iterator VARIABLEITR;
typedef VARIABLES::const_iterator VARIABLECONSTITR;

class CParserVariables
{
protected:
	VARIABLES m_Variables;
public:
	~CParserVariables();
	VariableEnum *Find(const _TCHAR* cszVarName);
	VariableEnum *Find(const wstring& strVarName);
	VariableEnum *Find(const CCompilerEquation *pEquation);
	void Add(const _TCHAR* cszVarName, CExpressionToken* pToken);
	void Add(const wstring& strVarName, CExpressionToken* pToken);
	bool Rename(const _TCHAR *cszVarName, const _TCHAR *cszNewVarName);
	void Clear();
	bool ChangeToEquationNames();
	VARIABLECONSTITR Begin() const { return m_Variables.begin(); }
	VARIABLECONSTITR End()  const {  return m_Variables.end(); }
};

class CExpressionRule
{
protected:
	CExpressionParserRule *m_pSource;
	CExpressionParserRule *m_pDestination;
	bool Match(CExpressionToken *pToken);
	VARIABLES m_Vars;
public:
	CExpressionRule(const _TCHAR *cszSource, const _TCHAR* cszDestination);
	CExpressionRule(const _TCHAR *cszSource, const _TCHAR* cszDestination, const CExpressionParser* pParser);
	bool Apply(CExpressionToken *pToken);
	virtual ~CExpressionRule();
};

typedef list<CExpressionRule*> EXPRESSIONRULES;
typedef EXPRESSIONRULES::const_iterator EXPRESSIONTRULEITR;



