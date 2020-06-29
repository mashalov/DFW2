#pragma once

#include "stack"
#include "list"
#include "set"
#include "vector"
#include "map"
#include "algorithm"
#include "string"
#include "memory"

#include "..\..\DFW2\cex.h"

class CExpressionToken;
class CCompilerEquation;
class CCompilerEquations;
class CExpressionParser;
class CExpressionParserRule;

// типы символов
enum eExpressionSymbolType
{
	EST_NUMBER,			// число 
	EST_OPERATOR,		// оператор
	EST_DOT,			// точка
	EST_COMMA,			// запятая
	EST_ALPHA,			// буквенный символ
	EST_ERROR,			// признак ошибки
	EST_EOF				// признак конца строки выражения
};

// типы токенов
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

// типы переменных модели
enum eCOMPILERVARTYPE
{
	eCVT_INTERNAL,				// внутренняя переменная (переменная состояния компилируемой системы выражений)
	eCVT_EXTERNAL,				// внешняя переменная (из внешней модели, не из компилируемой системы)
	eCVT_EXTERNALSETPOINT,		// внешняя уставка (не является переменной состояния, но может изменяться дискретно)
	eCVT_CONST,					// константа (именованная, для ссылки по имени)
	eCVT_HOST					// переменная, предоставляемая по имени движком
};

class CVariableExtendedInfoBase
{
public:
	virtual ~CVariableExtendedInfoBase() {}
};

// информация о переменной
struct VariableEnum
{
	ptrdiff_t m_nIndex;											// индекс (номер уравнения, если переменная в него входит)
	CExpressionToken *m_pToken;									// токен, который формирует значение переменной
	size_t m_nRefCount;											// счетчик ссылок на эту переменную
	eCOMPILERVARTYPE m_eVarType;								// тип переменной по физическому смыслу
	ptrdiff_t GetIndex() const;									// функция возвращающая индекс в зависимости от типа
	CVariableExtendedInfoBase *m_pVarExtendedInfo;				// дополнительная информация о переменной
	VariableEnum(CExpressionToken *pToken) : m_pToken(pToken), 
											 m_nRefCount(0), 
											 m_nIndex(0), 
											 m_eVarType(eCVT_INTERNAL),
											 m_pVarExtendedInfo(nullptr)
	{

	};
};


typedef std::stack<CExpressionToken*> PARSERSTACK;
typedef std::stack<const CExpressionToken*> CONSTPARSERSTACK;
typedef std::list<CExpressionToken*> TOKENLIST;
typedef TOKENLIST::iterator TOKENITR;
typedef TOKENLIST::reverse_iterator RTOKENITR;
typedef TOKENLIST::const_reverse_iterator CONSTRTOKENITR;
typedef TOKENLIST::const_iterator CONSTTOKENITR;
typedef std::set<CExpressionToken*> TOKENSET;
typedef TOKENSET::iterator TOKENSETITR;

typedef std::set<const CExpressionToken*> CONSTTOKENSET;
typedef CONSTTOKENSET::iterator CONSTTOKENSETITR;


// базовый класс для преобразований выражений в функциях и операторах
class CExpressionTransform
{
public:
	// упрощение
	virtual bool Simplify(CExpressionToken* pToken) = 0;		
	// развертывание
	virtual bool Expand(CExpressionToken* pToken) = 0;
	// формирование текстового представления производной
	virtual bool Derivative(CExpressionToken* pToken, CExpressionToken *pChild, std::wstring& Result) { return false;  };
	// возврат количества уравнений, описывающих функцию или оперетор
	virtual size_t EquationsCount() { return 1; }
	// признак реализации функции или оператора в движке, а не в компилируемом тексте
	virtual bool IsHostBlock() { return false;  }
	// возврат типа блока, если он реализован в движке ?
	virtual const _TCHAR* GetBlockType() { _ASSERTE(NULL);  return nullptr; }
	// возврат количества выводов в блоке (входы + выходы)
	virtual const long GetPinsCount() { return 2; }
	// функция, возвращающая строку особенной инициализации блока, если нужна
	virtual const _TCHAR* SpecialInitFunctionName() { return nullptr; }

	static void CompareTokens(CExpressionToken* pToken1,
		CExpressionToken*pToken2,
		TOKENLIST& MatchList1,
		TOKENLIST& MatchList2,
		TOKENLIST& DiffList1,
		TOKENLIST& DiffList2);

	static bool ConstantCoefficient(TOKENLIST& DiffList, double& Coe);

};

// информация о функции
struct FunctionEnum
{
	eExpressionTokenType m_eOperatorType;		// тип функции
	std::wstring m_strOperatorText;					// текстовое представление
	int m_nArgsCount;							// количество аргументов
	CExpressionTransform *m_pTransform;			// указатель на класс преобразования

	FunctionEnum(eExpressionTokenType eType, 
				 const _TCHAR* cszOperatorText, 
				 int nArgsCount,
				 CExpressionTransform *pTransform) :	

				 m_eOperatorType(eType),
				 m_strOperatorText(cszOperatorText),
				 m_nArgsCount(nArgsCount),
				 m_pTransform(pTransform) { }
};

// информация об операторе
struct OperatorEnum : FunctionEnum
{
	int m_nPrecedence;		// все как у функции, но дополнительно задаются очередность и признак правой ассоциативности
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

typedef std::vector<FunctionEnum*> FUNCTIONVEC;
typedef FUNCTIONVEC::const_iterator FUNCTIONITR;
typedef std::vector<OperatorEnum*> OPERATORVEC;
typedef OPERATORVEC::const_iterator OPERATORITR;


// описание уравнения
class CCompilerEquation
{
public:
	CExpressionToken *m_pToken;		// токен, для которого записывается уравнение
	ptrdiff_t m_nIndex;				// номер уравнения
	CCompilerEquation(CExpressionToken *pToken, ptrdiff_t nIndex) : m_pToken(pToken), m_nIndex(nIndex) {}
	// текстовые шаблоны для ссылок на разные типы переменных в уравнении
	static const _TCHAR *m_cszInternalVar;		
	static const _TCHAR *m_cszExternalVar;
	static const _TCHAR *m_cszConstVar;
	static const _TCHAR *m_cszSetpointVar;
	static const _TCHAR *m_cszHostVar;
	// функция генерации текстового представления уравнения (основного или уравнения инициализации)
	std::wstring Generate(bool bInitEquiation = false) const;
	static bool IsParentNeedEquation(CExpressionToken *pToken);
};


class CExpressionToken
{

public:
	eExpressionTokenType m_eType;			//	тип токена
	size_t m_nTokenBegin;					//  начало токена в разбираемой строке
	size_t m_nTokenLength;					//  длина токена
	TOKENLIST m_Children;					//  токены, являющиеся дочерними к данному
	TOKENSET m_Parents;						//  множество уникальных родителей токена
	const FunctionEnum *m_pFunctionInfo;	//  для токена-функции - информация о функции
	std::wstring m_strStringValue;			//  строчное представление токена
	std::wstring m_strEquation;				//  строчное представление уравнения токена
	CExpressionParser *m_pParser;			//  экземпляр парсера, которому принадлежит токен
	CCompilerEquation *m_pEquation;			//  уравнение токена

	void GetOperand(CExpressionToken *pToken, std::wstring& Result, bool bRight) const;
	bool GetEquationOperandIndex(const CCompilerEquation *pEquation, std::wstring& Result) const;
	void GetEquationOperandType(const CCompilerEquation *pEquation, std::wstring& Result, VariableEnum*& pVarEnum) const;
	void GetEquationVariableType(const CCompilerEquation *pEquation, VariableEnum* pVarEnum, std::wstring& Result) const;
	

	CExpressionToken(CExpressionParser *pParser, eExpressionTokenType eType, size_t nTokenBegin, size_t nTokenLength);
	CExpressionToken(CExpressionParser *pParser, const FunctionEnum *pFunction, size_t nTokenBegin, size_t nTokenLength);
	virtual ~CExpressionToken() {}


	// функции для обобщенного определения типа токена
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
	bool Derivative(CExpressionToken *pChild, std::wstring& Result);
	ptrdiff_t Compare(const CExpressionToken *pToCompare) const;
	bool ParserChanged() const;
	bool SortChildren();
	virtual const _TCHAR* EvaluateText();
	void SetTextValue(const _TCHAR *cszValue);
	const _TCHAR* GetTextValue() const;
	void GetEquationOperand(const CCompilerEquation *pEquation, std::wstring& Result) const;
	const CExpressionParser* GetParser();
	static void GetNumericText(double dValue, std::wstring& TextValue);
	const _TCHAR* GetBlockName() const;
	static std::wstring GenerateFunction(CExpressionToken *pToken, bool bEquationMode = false);
	static bool CheckTokenIsConstant(const CExpressionToken *pToken);
};

class CExpressionTokenModelLink : public CExpressionToken
{
protected:
	TOKENLIST m_ChildrenSave;
public:

	// стадии обработки символов в ссылке на модель
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

typedef std::map<std::wstring,VariableEnum> VARIABLES;
typedef VARIABLES::iterator VARIABLEITR;
typedef VARIABLES::const_iterator VARIABLECONSTITR;

class CParserVariables
{
protected:
	VARIABLES m_Variables;
public:
	~CParserVariables();
	VariableEnum* Find(const _TCHAR* cszVarName);
	VariableEnum* Find(const std::wstring& strVarName);
	VariableEnum* Find(const CCompilerEquation *pEquation);
	void Add(const _TCHAR* cszVarName, CExpressionToken* pToken);
	void Add(const std::wstring& strVarName, CExpressionToken* pToken);
	VariableEnum* Rename(const _TCHAR *cszVarName, const _TCHAR *cszNewVarName);
	void Clear();
	bool ChangeToEquationNames();
	VARIABLECONSTITR Begin() const { return m_Variables.begin(); }
	VARIABLECONSTITR End()  const {  return m_Variables.end(); }
};


// шаблон для преобразования выражений
class CExpressionRule
{
protected:
	std::unique_ptr< CExpressionParserRule> m_pSource;		// входное правило
	std::unique_ptr< CExpressionParserRule> m_pDestination;	// выходное правило
	bool Match(CExpressionToken *pToken);
	VARIABLES m_Vars;
public:
	CExpressionRule(const _TCHAR *cszSource, const _TCHAR* cszDestination);
	CExpressionRule(const _TCHAR *cszSource, const _TCHAR* cszDestination, const CExpressionParser* pParser);
	bool Apply(CExpressionToken *pToken);
	virtual ~CExpressionRule();
};

typedef std::list<CExpressionRule*> EXPRESSIONRULES;
typedef EXPRESSIONRULES::const_iterator EXPRESSIONTRULEITR;
