#include "..\stdafx.h"
#include "ExpressionParser.h"
#include "ExpressionDictionary.h"

CExpressionParserRules::CExpressionParserRules() : CExpressionParser()
{
	m_pRules = &DefaultRules;
}

CExpressionParserRules::CExpressionParserRules(CParserVariables& CommonVariables) : CExpressionParser(CommonVariables)
{
	m_pRules = &DefaultRules;
}

CExpressionParser::CExpressionParser() : m_szExpression(NULL),
										 m_szTokenText(NULL),
										 m_Variables(m_InternalVariables),
										 m_bUsingInternalVars(true),
										 m_pDictionary(&DefaultDictionary)
{
	
}

CExpressionParser::CExpressionParser(CParserVariables& CommonVariables) : m_szExpression(NULL),
																   m_szTokenText(NULL),
																   m_Variables(CommonVariables),
																   m_bUsingInternalVars(false),
																   m_pDictionary(&DefaultDictionary)
{

}

CExpressionParser::CExpressionParser(const CExpressionParser* pParser) : m_szExpression(NULL),
																		 m_szTokenText(NULL),
																		 m_pDictionary(pParser->m_pDictionary),
																		 m_Variables(m_InternalVariables),
																		 m_bUsingInternalVars(true),
																		 m_pRules(NULL)
{

}

CExpressionParser::~CExpressionParser()
{
	CleanUp();
}

void CExpressionParser::CleanUp()
{
	m_nHeadPosition = 0;
	m_nAdvanceNextSymbol = 0;
	if (m_szExpression)
	{
		delete m_szExpression;
		m_szExpression = NULL;
	}
	if (m_szTokenText)
	{
		delete m_szTokenText;
		m_szTokenText = NULL;
	}

	m_ResultStack = PARSERSTACK();
	m_ParserStack = PARSERSTACK();
	m_ArityStack = stack<ptrdiff_t>();

	for (TOKENSET::iterator it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
		delete *it;

	m_TokenPool.clear();

	if (m_bUsingInternalVars)
		m_Variables.Clear();
}

CExpressionToken* CExpressionParser::GetToken()
{
	// сбрасываем все атрибуты токена
	ResetToken();
	
	// пробуем извлечь оператор
	// если оператор будет извлечен, будет создан токен внутри ProcessOperator
	if (!ProcessOperator())
	{
		// если не оператор
		while (m_bContinueToken)	// пока токен не разобран полностью
		{
			// если следующий символ ошибочный - создаем токен и выходим
			if (m_eNextSymbolType == EST_ERROR)
			{
				Advance();
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGSYMBOL);
				break;
			}

			// если токен есть и он ошибочный - выход
			if (m_pCurrentToken && m_pCurrentToken->IsError())
				break;

			// выбираем способ обработки токена по ожидаемому типу
			switch (m_eAwaitTokenType)
			{
			case ETT_UNDEFINED:
				ProcessUndefined();			// пока не знаем какой токен, определяем состояние
				break;
			case ETT_NUMERIC_CONSTANT:
				ProcessNumericConstant();	// извлекаем численную константу
				break;
			case ETT_VARIABLE:
				ProcessVariable();			// извлекаем имя переменной
				break;
			case ETT_MODELLINK:
				ProcessModelLink();			// извлекаем ссылку на модель в формате table[key].prop
				break;
			}

			CurrentSymbolType();			// определяем тип следующего символа
		}
	}

	// если текущий токен является оператором, не является унарным минусом и не является закрывающей скобкой
	// разрешаем ожидание унарного минуса
	if (m_pCurrentToken->IsOperator() && !m_pCurrentToken->IsUnary() && m_pCurrentToken->GetType() != ETT_RB)
		m_bUnaryMinus = true;
	else
		m_bUnaryMinus = false;

	return m_pCurrentToken;
}

bool CExpressionParser::Parse(const _TCHAR* cszExpression)
{
	bool bRes = false;
	eExpressionTokenType eError = ETT_UNDEFINED;
	// копируем выражение во временный буфер
	m_nExpressionLength = _tcslen(cszExpression);
	m_szExpression = new _TCHAR[m_nExpressionLength + 1];
	_tcscpy_s(m_szExpression, m_nExpressionLength + 1, cszExpression);

	m_bUnaryMinus = true;
	CExpressionToken *pToken = GetToken();

	while (pToken)
	{
		// работаем пока не конец строки или не ошибка
		if (pToken->IsEOF())	break;
		if (pToken->IsError())	break;

		if (pToken->IsLeaf())								// если токен не может иметь детей - помещаем в стек
		{
			m_ResultStack.push(pToken);
		} 
		else if (pToken->IsFunction())						// если функция - то помещаем стек и в стек порядка (для многоаргументных функций)
		{
			m_ParserStack.push(pToken);
			m_ArityStack.push(1);
		}
		else if (pToken->IsOperator())						// если оператор
		{
			if (pToken->GetType() == ETT_COMMA)				
			{
				bool bMatchParent = false;
				bool bAtLeastOneOperatorinParenthesis = false;

				while (!m_ParserStack.empty() && !bMatchParent)
				{
					if (m_ParserStack.top()->GetType() == ETT_LB)
						bMatchParent = true;
					else
					{
						eError = BuildOperatorTree();
						bAtLeastOneOperatorinParenthesis = true;

						if (eError != ETT_UNDEFINED)
							break;
					}
				}

				if (eError == ETT_UNDEFINED)
				{
					if (!bMatchParent)
						pToken->SetError(ETT_ERROR_NOLEFTPARENTHESIS);
					else
						if (!m_ArityStack.empty())
							m_ArityStack.top()++;
						else
							pToken->SetError(ETT_ERROR_STACKERROR);
				}
				else
					pToken->SetError(eError);
			}
			else if (pToken->GetType() == ETT_RB)
			{
				bool bMatchParent = false;

				while (!m_ParserStack.empty() && !bMatchParent)
				{
					if (m_ParserStack.top()->GetType() == ETT_LB)
					{
						bMatchParent = true;
						m_ParserStack.pop();
					}
					else
					{
						eError = BuildOperatorTree();
						if (eError != ETT_UNDEFINED)
							break;
					}
				}

				if (eError == ETT_UNDEFINED)
				{
					if (!bMatchParent)
						pToken->SetError(ETT_ERROR_NOLEFTPARENTHESIS);

					if (!m_ParserStack.empty() && eError == ETT_UNDEFINED)
					{
						if (m_ParserStack.top()->IsFunction())
						{
							eError = BuildOperatorTree();
							if (eError == ETT_UNDEFINED)
							{
								if (!m_ArityStack.empty())
									m_ArityStack.pop();
								else
									pToken->SetError(ETT_ERROR_STACKERROR);
							}
							else
							{
								pToken = m_ParserStack.top();
								pToken->SetError(eError);
							}
						}
					}
				}
				else
					pToken->SetError(eError);
			}
			else if (pToken->GetType() == ETT_LB)
			{
				m_ParserStack.push(pToken);
			}
			else
			{
				if (!pToken->IsError())
				{
					while (!m_ParserStack.empty())
					{
						CExpressionToken *pStackToken = m_ParserStack.top();

						if (pStackToken->RightAssociativity())
						{
							if (pStackToken->Precedence() > pToken->Precedence() && !pToken->IsUnary())
							{
								eError = BuildOperatorTree();
								if (eError != ETT_UNDEFINED)
								{
									pToken->SetError(eError);
									break;
								}
							}
							else
								break;
						}
						else
						{
							if (pStackToken->Precedence() >= pToken->Precedence() && !pToken->IsUnary())
							{
								eError = BuildOperatorTree();
								if (eError != ETT_UNDEFINED)
								{
									pToken->SetError(eError);
									break;
								}
							}
							else
								break;
						}
					}
					m_ParserStack.push(pToken);
				}
			}
		}

		if (pToken->IsError())
			break;
		else
			pToken = GetToken();
	}

	if (pToken && !pToken->IsError())
	{
		while (!m_ParserStack.empty())
		{
			if (m_ParserStack.top()->GetType() == ETT_LB)
			{
				pToken->SetError(ETT_ERROR_NORIGHTPARENTHESIS);
				break;
			}
			eError = BuildOperatorTree();
			if (eError != ETT_UNDEFINED)
			{
				pToken->SetError(eError);
				break;
			}
		}

		m_ParserStack = PARSERSTACK();

		if (!pToken->IsError())
			if (m_ResultStack.size() != 1)
				pToken->SetError(ETT_ERROR_STACKERROR);
	}

	if (pToken && !pToken->IsError())
	{
		CExpressionToken *pRoot = NewExpressionToken(ETT_ROOT);
		pRoot->AddChild(m_ResultStack.top());
		m_ResultStack.pop();
		m_ResultStack.push(pRoot);
		CleanTree();
		bRes = true;
	}

	return bRes;
}

void CExpressionParser::SkipSpaces()
{
	while (m_nHeadPosition < m_nExpressionLength)
	{
		if (!_istspace(*(m_szExpression + m_nHeadPosition))) 
			break;
		m_nHeadPosition++;
	}
}

// возвращает тип символа 
void CExpressionParser::CurrentSymbolType()
{
	m_eNextSymbolType = EST_ERROR;															// возвращаем ошибку, если ни один из нижеследующих типов не подойдет
	m_nAdvanceNextSymbol = 1;																// по умолчанию длина символа - 1
	_TCHAR *pCurrentChar = m_szExpression + m_nHeadPosition;
	if (m_nHeadPosition >= m_nExpressionLength) m_eNextSymbolType = EST_EOF;				// если текущий символ за концом строки - возвращаем тип конец строки
	else if (_istspace(*pCurrentChar)) m_eNextSymbolType = EST_EOF;							// если пробел - то конец строки
	else if (_istdigit(*pCurrentChar)) m_eNextSymbolType = EST_NUMBER;						// если цифра - число
	else if (_istalpha(*pCurrentChar)) m_eNextSymbolType = EST_ALPHA;						// если буква - алфавитно-цифровой
	else if (*pCurrentChar == _T('.')) m_eNextSymbolType = EST_DOT;							// если точка - разделитель
	else if (_istoperator(pCurrentChar) != NULL) m_eNextSymbolType = EST_OPERATOR;			// если один из операторов - оператор
}

// проверяет является ли текущий символ символом экспоненты
int CExpressionParser::_isdecimalexponent()
{
	_ASSERTE(m_nHeadPosition < m_nExpressionLength);
	_TCHAR pCurrentChar = *(m_szExpression + m_nHeadPosition);
	// проверяем латинские заглавную и строчную e
	if (pCurrentChar == _T('E') || pCurrentChar == _T('e')) 
		return 1; 
	return 0;
}

const OperatorEnum* CExpressionParser::_istoperator(const _TCHAR* pChar)
{
	// сбрасываем текущий оператор
	m_pCurrentOperator = NULL;
	size_t nMaxMatchLength = 0;

	// идем по словарю операторов
	for (OPERATORITR opit = m_pDictionary->OperatorsEnum.begin(); opit != m_pDictionary->OperatorsEnum.end(); opit++)
	{
		OperatorEnum *pOpEnum = *opit;
		size_t nOpLength = pOpEnum->m_strOperatorText.length();
		// если длина оператора в словаре не превышает оставшуюся длину строки
		if (nOpLength <= m_nExpressionLength - m_nHeadPosition)
		{
			// сравниваем оператор из словаря с входной строкой на длину оператора
			if (!_tcsncmp(pChar, pOpEnum->m_strOperatorText.c_str(), nOpLength))
			{
				// если есть попадание
				if (pOpEnum->m_eOperatorType == ETT_MINUS)			// и текущий оператор - минус
					if (m_bUnaryMinus)								// и возможен унарный минус
						pOpEnum = GetOperatorEnum(ETT_UMINUS);		// то меняем оператор на унарный минус

				if (nOpLength > nMaxMatchLength)					// если длина текущего оператора максимальна для данной проверки
				{
					m_pCurrentOperator = pOpEnum;					// ставим текущему оператору в соответствие найденный
					m_nAdvanceNextSymbol = nOpLength;				// определяем длину оператора для продвижения по выражению дальше
					nMaxMatchLength = nOpLength;				
				}
			}
		}
	}
	return m_pCurrentOperator;
}


// проверяет, является ли содержимое неразобранной строки оператором,
// если является - создает соответствующий токен, если нет - возвращает код false
bool CExpressionParser::ProcessOperator()
{
	bool bRes = false;
	CurrentSymbolType();			// проверяем есть ли впереди оператор
	switch (m_eNextSymbolType)
	{
	case EST_OPERATOR:
		Advance();													// если да, продвигаемся дальше
		m_pCurrentToken = NewExpressionToken(m_pCurrentOperator);	// создаем токен для оператора
		bRes = true;
		break;
	}
	return bRes;
}

void CExpressionParser::ProcessUndefined()
{
	// по текущему типу символа определяем ожидаемый тип токена
	switch (m_eNextSymbolType)
	{
	case EST_NUMBER:												// для цифры - число
		m_bDecimalInteger = true;
		m_eAwaitTokenType = ETT_NUMERIC_CONSTANT;
		Advance();
		break;
	case EST_DOT:													// для точки - число
		m_eAwaitTokenType = ETT_NUMERIC_CONSTANT;
		Advance();
		break;
	case EST_ALPHA:													// для буквы - переменная
		m_eAwaitTokenType = ETT_VARIABLE;
		Advance();
		break;
	case EST_OPERATOR:												// для оператора - оператор
		Advance();
		m_eAwaitTokenType = m_pCurrentOperator->m_eOperatorType;
		break;
	case EST_EOF:													// для конца строки - сразу создаем токен конца строки
		m_pCurrentToken = NewExpressionToken(ETT_EOF);
		break;
	default:
		m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGSYMBOL);	// токен ошибки если ничего не подошло
		break;
	}
}

void CExpressionParser::ProcessNumericConstant()
{
	// разбираем число с плавающей точкой
	switch (m_eNextSymbolType)
	{
	case EST_NUMBER:							// если видим число

		if (m_bDecimalExponent)					// если разрешена экспонента
			m_bDecimalExponentValue = true;		// это экспонента

		if (m_bDecimalSeparator)				// если было разделитель 
			m_bDecimalFraction = true;			// это дробная часть
		else
			m_bDecimalInteger = true;			// иначе это пока целая часть

		Advance();

		break;
	case EST_DOT:								// если видим точку

		if (m_bDecimalSeparator || m_bDecimalExponent)		// и была уже точка или экспонента - ошибка
			m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		else
		{
			m_bDecimalSeparator = true;			// иначе отмечаем что видели точку
			Advance();
		}

		break;

	case EST_ALPHA:

		if (m_bDecimalExponent)					// если видим букву и была уже экспонента - ошибка ("E" уже не может быть)
			m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		else
		{
			if (_isdecimalexponent())			// если видим экспоненту - отмечаем что видели
			{
				m_bDecimalExponent = true;
				Advance();
			}
			else
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);	// никаких других букв в числе не может быть
		}

		break;

	case EST_OPERATOR:		// если видим оператор, то надо заканчивать разбор числа, если это не знак экспоненты

		if (m_bDecimalExponentValue || !m_bDecimalExponent)		// если дошли до цифр экспоненты или не видели экспоненты
		{
			if (m_bDecimalInteger || m_bDecimalFraction)		// и прошли целую часть или дробную после точки
				m_pCurrentToken = NewExpressionToken(ETT_NUMERIC_CONSTANT);	// то это численная константа
			else
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER); // иначе ошибка
		}
		else
			if (m_bDecimalExponentSign)							// если уже был знак экспоненты - ошибка
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
			else
				if (m_pCurrentOperator && 
						(m_pCurrentOperator->m_eOperatorType == ETT_PLUS  ||	// если оператор является знаком и был символ экспоненты
						 m_pCurrentOperator->m_eOperatorType == ETT_MINUS ||
						 m_pCurrentOperator->m_eOperatorType == ETT_UMINUS ))
				{
					Advance();
					m_bDecimalExponentSign = true;								// отмечаем что знак был
				}
				else
					m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);	// если оператор не знак - ошибка

		break;

	case EST_EOF:									// если конец строки, надо заканчивать разбор числа
		if (m_bDecimalExponentValue || !m_bDecimalExponent)
		{
			if (m_bDecimalInteger || m_bDecimalFraction)
				m_pCurrentToken = NewExpressionToken(ETT_NUMERIC_CONSTANT);		// получаем корректное целое или дробное число, аналогично ветке с оператором
			else
				m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);

		}
		else
			m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);
		break;
	default:
		m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGNUMBER);			// странный символ - число задано с ошибкой
		break;
	}
}

void CExpressionParser::ProcessVariable()
{
	switch (m_eNextSymbolType)
	{
	case EST_ALPHA:						// пока идут буквы и цифры - это имя переменной
	case EST_NUMBER:					// причем если в токене была первой цифра - он будет разбираться как число. Сюда попадаем только если первый символ - буква
		Advance();
		break;
	case EST_OPERATOR:					// если нашелся оператор
		if (m_pCurrentOperator->m_eOperatorType == ETT_LB)		// и этот оператор - левая круглая скобка
			m_pCurrentToken = NewExpressionTokenFunction(m_szExpression);  // то создаем токен функции
		else if (m_pCurrentOperator->m_eOperatorType == ETT_LBS)
		{
			// если это квадратная скобка, то это ссылка на модель, дальнейший процессинг будет в ProcessModelLink
			m_pCurrentToken = new CExpressionTokenModelLink(this, m_nTokenBegin, m_nTokenLength);
			m_pCurrentToken->SetTextValue(GetText(m_nTokenBegin, m_nTokenLength));
			m_eAwaitTokenType = ETT_MODELLINK;
			Advance();
		}
		else 
			m_pCurrentToken = NewExpressionToken(ETT_VARIABLE);		// иначе создаем переменную с именем до символа оператора
		break;
	case EST_EOF:
		m_pCurrentToken = NewExpressionToken(ETT_VARIABLE);			// точно также создаем пермеменную до конца строки
		break;
	default:
		m_pCurrentToken = NewExpressionToken(ETT_ERROR_WRONGSYMBOL);	// если какой-то неожиданный символ - ошибка
	}
}


void CExpressionParser::ProcessModelLink()
{
	CExpressionTokenModelLink *pModelLink = static_cast<CExpressionTokenModelLink*>(m_pCurrentToken);
	switch (m_eNextSymbolType)
	{
	case EST_ALPHA:
		switch (pModelLink->nWait)					// если видим букву
		{
			case CExpressionTokenModelLink::MLS_ALPHA_CONTINUE:
			case CExpressionTokenModelLink::MLS_NUMBER_CONTINUE:		// если в обработке букв или цифр - добавляем в текущем дочернему тексту
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);	
				Advance();
			break;
			case CExpressionTokenModelLink::MLS_ALPHA:
				pModelLink->nWait = CExpressionTokenModelLink:: MLS_ALPHA_CONTINUE;		// если за буквой идет буква - обрабатываем буквы
				pModelLink->AddChild(NewChildExpressionToken(ETT_PROPERTY));			// создаем дочерний токен для свойства ссылки
				m_bContinueToken = true;
				Advance();
				break;
			default:
				pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		}
		break;
	case EST_NUMBER:							// если видим цифру
		switch (pModelLink->nWait)
		{
			case CExpressionTokenModelLink::MLS_ALPHA_CONTINUE:
			case CExpressionTokenModelLink::MLS_NUMBER_CONTINUE:
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);		// если в обработке букв или цифр - добавляем к текущему тексту
				Advance();
			break;
			case CExpressionTokenModelLink::MLS_NUMBER:
				pModelLink->nWait = CExpressionTokenModelLink::MLS_NUMBER_CONTINUE;			// если в обработке цифр создаем дочерний токен численной константы
				pModelLink->AddChild(NewChildExpressionToken(ETT_NUMERIC_CONSTANT));
				m_bContinueToken = true;
				Advance();
				break;
			default:
				pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		}
		break;

	case EST_OPERATOR:						// если видим оператор 
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_ALPHA_CONTINUE)
		{
			BuildModelLink(pModelLink);		// если работали с буквами - (свойство ссылки), заканчиваем и строим ссылку полностью
		}
		else
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_NUMBER_CONTINUE)
		{
			// если работали с цифрами 
			if (m_pCurrentOperator->m_eOperatorType == ETT_RBS)
			{
				// и видим правую квадратную скобку, пропускаем ее и входим в режим ожидания точки
				Advance();
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
				pModelLink->nWait = CExpressionTokenModelLink::MLS_DOT;
			}
			else if (m_pCurrentOperator->m_eOperatorType == ETT_COMMA)
			{
				// если видим запятую - пропускаем и ждем следующую цифру
				Advance();
				pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
				pModelLink->nWait = CExpressionTokenModelLink::MLS_NUMBER;
			}
			else
				pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		}
		else
			pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);

		break;

	case EST_DOT:	// если видим точку
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_DOT)
		{
			// и ждали точку, то переходим в режим букв для обработки свойства ссылки
			Advance();
			pModelLink->nWait = CExpressionTokenModelLink::MLS_ALPHA;
		}
		else
			pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);

		break;

	case EST_EOF: // если видим eof
		if (pModelLink->nWait == CExpressionTokenModelLink::MLS_ALPHA_CONTINUE) // то можем быть только в режиме букв
			BuildModelLink(pModelLink);											// и если да - создаем ссылку
		else
			pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
		break;

	default:
		pModelLink->SetError(ETT_ERROR_WRONGSYMBOL, m_nHeadPosition);
	}
}

// передвигает позицию в выражении на длину найденного символа
// и задает длину найденного токена
void CExpressionParser::Advance()
{
	m_nTokenLength += m_nAdvanceNextSymbol;
	m_nHeadPosition += m_nAdvanceNextSymbol;
}

CExpressionToken* CExpressionParser::NewExpressionToken(CExpressionToken *pCloneToken)
{
	CExpressionToken *pNewToken = NULL;
	if (pCloneToken->IsVariable())
	{

		if (pCloneToken->GetParser()->IsRuleParser())
		{
			VARIABLEITR vit = m_SubstituteVars.find(pCloneToken->GetTextValue());
			if (vit != m_SubstituteVars.end())
			{
				pNewToken = CloneToken(vit->second.m_pToken);
			}
			else
				_ASSERTE(0);
		}
		else
		{
			VariableEnum *pEnum = m_Variables.Find(pCloneToken->GetTextValue());
			_ASSERTE(pEnum);
			pNewToken = pEnum->m_pToken;
		}
	}
	else
	{
		pNewToken = NewExpressionToken(pCloneToken->GetType());
		pNewToken->SetTextValue(pCloneToken->GetTextValue());
	}
	return pNewToken;
}

// создает токен по типу
CExpressionToken* CExpressionParser::NewExpressionToken(eExpressionTokenType eType)
{
	CExpressionToken* pToken = NULL;
	// если тип - переменна или ссылка на модель
	if (eType == ETT_VARIABLE || eType == ETT_MODELLINK)
	{
		// используем текст токена для поиска/создания переменной
		wstring VarName = GetText(m_nTokenBegin, m_nTokenLength);
		VariableEnum *pEnum = m_Variables.Find(VarName);
		if (pEnum)
			pToken = pEnum->m_pToken;		// если переменная уже есть - возвращаем ее токен
		else
		{
			// иначе создаем новый токен для новой переменной
			pToken = new CExpressionToken(this, eType, m_nTokenBegin, m_nTokenLength);
			pToken->SetTextValue(VarName.c_str());
			m_TokenPool.insert(pToken);
			m_Variables.Add(VarName, pToken);
		}
	}

	if (!pToken)
	{
		// если токен не был ссылкой или переменной
		OperatorEnum *pOpEnum = GetOperatorEnum(eType);
		// пытаемся создать токен оператора
		if (pOpEnum)
			pToken = new CExpressionToken(this, pOpEnum, m_nTokenBegin, m_nTokenLength);
		else
		{
			// если и это не получилось - токен функции
			FunctionEnum *pOpFunction = GetFunctionEnum(eType);
			if (pOpFunction)
				pToken = new CExpressionToken(this, pOpFunction, m_nTokenBegin, m_nTokenLength);
			else
				pToken = new CExpressionToken(this, eType, m_nTokenBegin, m_nTokenLength); // иначе - просто некий токен
		}
	
		m_TokenPool.insert(pToken);  // созданные токены накапливаем в пуле, чтобы потом удалить или искать по адресу
	}

	m_bContinueToken = false;		// если запросили создание токена - обработка текущего токена закончена
	return pToken;
}


// функция создания "заготовки" дочернего токена. В него будет накапливаться текст при разборе
CExpressionToken* CExpressionParser::NewChildExpressionToken(eExpressionTokenType eType)
{
	m_nTokenLength = 0;
	m_nTokenBegin = m_nHeadPosition;
	// длина пустая, пока токен не будет разобран полностью
	CExpressionToken* pToken = new CExpressionToken(this, eType, m_nHeadPosition, m_nTokenLength);
	m_TokenPool.insert(pToken);
	m_bContinueToken = false;
	return pToken;
}

CExpressionToken* CExpressionParser::NewExpressionTokenFunction(const _TCHAR *cszExpression)
{
	CExpressionToken* pToken = NULL;
	// в эту функциб попадаем из разбора переменной в случае, если нашли некое символьное имя с левой круглой скобкой
	for (FUNCTIONITR fit = m_pDictionary->FunctionsEnum.begin(); fit != m_pDictionary->FunctionsEnum.end(); fit++)
	{
		FunctionEnum *pFunc = *fit;
		if (!_tcsncmp(pFunc->m_strOperatorText.c_str(), cszExpression + m_nTokenBegin, m_nTokenLength))
		{
			// если нашли имя функции в словаре - создаем токе функции
			pToken = new CExpressionToken(this, pFunc, m_nTokenBegin, m_nTokenLength);
			break;
		}
	}

	// если в словаре функции не нашли созаем токен ошибки неизвестной функции
	if (!pToken)
		pToken = new CExpressionToken(this, ETT_ERROR_UNKNOWNFUNCTION, m_nTokenBegin, m_nTokenLength);

	// вставляем созданный токен в пул, чтобы потом его удалить корректно
	m_TokenPool.insert(pToken);
	// отмечаем что разбор токена завершен, функция определена
	m_bContinueToken = false;
	return pToken;
}

CExpressionToken* CExpressionParser::NewExpressionToken(const OperatorEnum *pOperator)
{
	// создаем токен оператора
	CExpressionToken* pToken = new CExpressionToken(this, pOperator, m_nTokenBegin, m_nTokenLength);
	// вставляем созданный токен в пул, чтобы потом его удалить корректно
	m_TokenPool.insert(pToken);
	// отмечаем что разбор токена завершен, оператор определен
	m_bContinueToken = false;

	return pToken;
}

void CExpressionParser::ResetToken()
{
	// убираем пробелы то следующего токена
	SkipSpaces();
	// сбрасываем все состояния
	m_eAwaitTokenType = ETT_UNDEFINED;
	m_nTokenBegin = m_nHeadPosition;
	m_pCurrentOperator = NULL;
	m_nTokenLength = 0;
	m_bContinueToken = true;
	m_pCurrentToken = NULL;
	m_bDecimalSeparator = false;
	m_bDecimalExponent = false;
	m_bDecimalExponentSign = false;
	m_bDecimalExponentValue = false;
	m_bDecimalFraction = false;
	m_bDecimalInteger = false;
}

const _TCHAR* CExpressionParser::GetText(const CExpressionToken *pToken)
{
	return GetText(pToken->TextBegin(), pToken->TextLength());
}

const _TCHAR* CExpressionParser::GetText(size_t nTokenBegin, size_t nTokenLenght)
{
	if (m_szTokenText) delete m_szTokenText;
	m_szTokenText = new _TCHAR[nTokenLenght + 1];
	_tcsncpy_s(m_szTokenText, nTokenLenght + 1, m_szExpression + nTokenBegin, nTokenLenght);
	return m_szTokenText;
}

eExpressionTokenType CExpressionParser::BuildOperatorTree()
{
	eExpressionTokenType eError = ETT_UNDEFINED;
	if (m_ParserStack.empty())
		return ETT_ERROR_STACKERROR;

	CExpressionToken *pStackToken = m_ParserStack.top();
	ptrdiff_t nArgsCount = pStackToken->ArgsCount();

	if (pStackToken->IsFunction())
	{
		if (!m_ArityStack.empty())
		{
			if (nArgsCount >= 0)
			{
				if (m_ArityStack.top() != nArgsCount)
					eError = ETT_ERROR_WRONGNUMBEROFARGS;
			}
			else
				nArgsCount = m_ArityStack.top();
		}
	}

	if (eError == ETT_UNDEFINED)
	{
		if (m_ResultStack.size() >= static_cast<size_t>(nArgsCount))
		{
			while (nArgsCount--)
			{
				pStackToken->AddChild(m_ResultStack.top());
				m_ResultStack.pop();
			}
		}
		else
			eError = ETT_ERROR_STACKERROR;
	}

	if (eError == ETT_UNDEFINED)
	{
		m_ParserStack.pop();
		m_ResultStack.push(pStackToken);
	}
	return eError;

}


const _TCHAR* CExpressionParser::GetErrorDescription(CExpressionToken *pToken)
{
	const _TCHAR* cszAtPosition = _T(" в позиции ");

	if (pToken)
	{
		if (pToken->IsError())
		{
			switch (pToken->GetType())
			{
			case ETT_ERROR_WRONGSYMBOL:
				m_szErrorDescription = Cex(_T("Неверный символ %s%s%d"), GetText(pToken), cszAtPosition, pToken->TextBegin());
				break;
			case ETT_ERROR_WRONGNUMBER:
				m_szErrorDescription = Cex(_T("Неверный формат числа %s%s%d"), GetText(pToken), cszAtPosition, pToken->TextBegin());
				break;
			case ETT_ERROR_UNKNOWNFUNCTION:
				m_szErrorDescription = Cex(_T("Неизвестная функция %s%s%d"), GetText(pToken), cszAtPosition, pToken->TextBegin());
				break;
			case ETT_ERROR_STACKERROR:
				m_szErrorDescription = Cex(_T("Ошибка стека%s%d"),cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_WRONGNUMBEROFARGS:
				m_szErrorDescription = Cex(_T("Неверное число аргументов %s%s%d"),GetText(pToken),cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_NOLEFTPARENTHESIS:
				m_szErrorDescription = Cex(_T("Нет левой скобки %s%d"), cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_NORIGHTPARENTHESIS:
				m_szErrorDescription = Cex(_T("Нет правой скобки %s%d"),cszAtPosition,pToken->TextBegin());
				break;
			case ETT_ERROR_DIVISIONBYZERO:
				m_szErrorDescription = Cex(_T("Деление на ноль"));
				break;
			case ETT_ERROR_NEGATIVEROOT:
				m_szErrorDescription = Cex(_T("Дробная степень отрицательного числа"));
				break;
			case ETT_ERROR_NEGATIVELOG:
				m_szErrorDescription = Cex(_T("Логарифм из отрицательного числа"));
				break;
			case ETT_ERROR_WRONGARG:
				m_szErrorDescription = Cex(_T("Аргумент вне допустимого диапазона"));
				break;
			case ETT_ERROR_WRONGRANGE:
				m_szErrorDescription = Cex(_T("Неправильный диапазон"));
				break;
			}
		}
		else
			_tcprintf(_T("\nOK"));
	}
	return m_szErrorDescription.c_str();
}

bool CExpressionParser::Simplify()
{
	bool bRes = true;

	CleanTree();
	m_bChanged = true;
	wstring infix;

	if (bRes)
	{
		do
		{
			m_bChanged = false;
			PARSERSTACK Stack;
			TOKENSET Visited;

			if (m_ResultStack.empty())
			{
				bRes = false;
				break;
			}

			Stack.push(m_ResultStack.top());
			
			bool bPushChanged = false;

			while (!Stack.empty() && bRes)
			{
				m_bChanged = bPushChanged;

				GetExpressionConstants();
				CExpressionToken *pToken = Stack.top();

				CValidToken ValidToken(pToken);

				pToken->SortChildren();

				m_bChanged = false;

				for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
				{
					if (Visited.find(*it) == Visited.end())
						Stack.push(*it);
				}

				if (Stack.top() == pToken)
				{
					Visited.insert(pToken);

					if (m_pRules)
					{
						for (EXPRESSIONTRULEITR rit = m_pRules->begin(); rit != m_pRules->end() && bRes; rit++)
						{
							m_bChanged = false;

							if (!(*rit)->Apply(pToken))
							{
								bRes = false;
								break;
							}

							if (GetChanged() || ! ValidToken.IsValidToken())
							{
								bPushChanged = true;
								break;
							}
						}
					}

					if (GetChanged())
						break;

					if (ValidToken.IsValidToken())
					{
						m_bChanged = false;

						if (!pToken->Simplify())
						{
							bRes = false;
							break;
						}
						else
						{
							if (GetChanged())
								bPushChanged = true;

							if(ValidToken.IsValidToken())
								pToken->SortChildren();

							m_bChanged = false;

							Stack.pop();
						}
					}
					else
						Stack.pop();
				}
			}

			m_bChanged = bPushChanged;
			//Infix(infix);
			//_tcprintf(_T("\n%s"), infix.c_str());
		} 
		while (GetChanged() && bRes);
	}

	return bRes;
}


//#define ROOT

bool CExpressionParser::Expand()
{
#ifdef ROOT

	bool bRes = true;

	CleanTree();

	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		if (pToken->IsLeaf() && !pToken->GetTextValue())
			pToken->SetTextValue(GetText(pToken));
	}

	m_bChanged = true;
	wstring infix;

	if (bRes)
	{
		do
		{
			m_bChanged = false;
			PARSERSTACK Stack;
			Stack.push(m_ResultStack.top());
			while (!Stack.empty())
			{
				CExpressionToken *pToken = Stack.top();
				Stack.pop();
				pToken->SortChildren();
				if (!pToken->Expand(this))
				{
					bRes = false;
					break;
				}
				if (m_TokenPool.find(pToken) != m_TokenPool.end())
				{
					pToken->SortChildren();
					for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
						Stack.push(*it);
				}
			}
			Infix(infix);
			_tcprintf(_T("\n%s"), infix.c_str());
		} 
		while (GetChanged() && bRes);
	}

	return bRes;

#else

	bool bRes = true;

	CleanTree();
	m_bChanged = true;
	wstring infix;

	if (bRes)
	{
		do
		{
			m_bChanged = false;
			PARSERSTACK Stack;
			TOKENSET Visited;
			Stack.push(m_ResultStack.top());

			bool bPushChanged = false;

			while (!Stack.empty() && bRes)
			{
				m_bChanged = bPushChanged;
				GetExpressionConstants();

				CExpressionToken *pToken = Stack.top();

				CValidToken ValidToken(pToken);

				pToken->SortChildren();

				m_bChanged = false;

				for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
				{
					if (Visited.find(*it) == Visited.end())
						Stack.push(*it);
				}

				if (Stack.top() == pToken)
				{
					Visited.insert(pToken);
					
					m_bChanged = false;

					if (!pToken->Expand())
					{
						bRes = false;
						break;
					}
					else
					{
						if (GetChanged())
							bPushChanged = true;

						if (ValidToken.IsValidToken())
							pToken->SortChildren();

						m_bChanged = false;

						Stack.pop();
					}
				}
			}

			m_bChanged = bPushChanged;
//			Infix(infix);
//			_tcprintf(_T("\n%s"), infix.c_str());
		} 
		while (GetChanged() && bRes);
	}

	return bRes;

#endif

}


bool CExpressionParser::Infix(wstring& strInfix)
{
	bool bRes = true;
	if (m_ResultStack.size() != 1)
		return false;

	PARSERSTACK Stack;
	Stack.push(m_ResultStack.top());
	TOKENSET Visited;

	while (!Stack.empty())
	{
		CExpressionToken *pToken = Stack.top();
		for (TOKENITR it = pToken->ChildrenBegin(); it != pToken->ChildrenEnd(); it++)
		{
			CExpressionToken *pChild = *it;
			if (Visited.find(pChild) == Visited.end())
			Stack.push(pChild);
		}

		if (Stack.top() == pToken)
		{
			Visited.insert(pToken);
			pToken->EvaluateText();
			Stack.pop();
		}
	}

	if (m_ResultStack.top()->GetType() == ETT_ROOT)
		strInfix = (*m_ResultStack.top()->ChildrenBegin())->GetTextValue();
	else
		strInfix = m_ResultStack.top()->GetTextValue();

	return bRes;
}

void CExpressionParser::ChangeTokenType(CExpressionToken *pToken, eExpressionTokenType eType)
{
	OperatorEnum *pOpEnum = GetOperatorEnum(eType);
	if (pOpEnum)
		pToken->SetOperator(pOpEnum);
	else
	{
		FunctionEnum *pFuncEnum = GetFunctionEnum(eType);
		if (pFuncEnum)
			pToken->SetOperator(pFuncEnum);
	}
}


FunctionEnum* CExpressionParser::GetFunctionEnum(eExpressionTokenType eType)
{
	for (FUNCTIONITR it = m_pDictionary->FunctionsEnum.begin(); it != m_pDictionary->FunctionsEnum.end(); it++)
		if ((*it)->m_eOperatorType == eType)
			return *it;
	return NULL;
}

OperatorEnum* CExpressionParser::GetOperatorEnum(eExpressionTokenType eType)
{
	for (OPERATORITR it = m_pDictionary->OperatorsEnum.begin(); it != m_pDictionary->OperatorsEnum.end(); it++)
		if ((*it)->m_eOperatorType == eType)
			return *it;
	return NULL;
}

void CExpressionParser::DeleteToken(CExpressionToken *pToken)
{
	if (pToken->ParentsCount() == 0)
	{
		bool bCanDelete = false;
		if (pToken->GetType() == ETT_VARIABLE || pToken->GetType() == ETT_MODELLINK)
		{
			VariableEnum *pEnum = m_Variables.Find(pToken->GetTextValue());
			bCanDelete = false;
		}
		else
			bCanDelete = true;

		if (bCanDelete)
		{
			TOKENSETITR it = m_TokenPool.find(pToken);
			if (it != m_TokenPool.end())
			{
				_ASSERTE(pToken->ChildrenCount() == 0);
				m_TokenPool.erase(it);
				delete pToken;
				SetChanged();
			}
		}
	}
}


CExpressionToken* CExpressionParser::CloneToken(CExpressionToken* pToken)
{
	CExpressionToken *pCloneToken = NewExpressionToken(pToken);
	CExpressionToken *pRetToken = pCloneToken;


	PARSERSTACK Stack, CloneStack;
	Stack.push(pToken);
	CloneStack.push(pCloneToken);

	while (!Stack.empty())
	{
		CExpressionToken *pStackToken = Stack.top();
		pCloneToken = CloneStack.top();
		Stack.pop();
		CloneStack.pop();

		for (TOKENITR it = pStackToken->ChildrenBegin(); it != pStackToken->ChildrenEnd(); it++)
		{
			CExpressionToken *pChildToken = *it;
			CExpressionToken *pNewCloneToken = NewExpressionToken(pChildToken);
			pCloneToken->AddChild(pNewCloneToken);
			Stack.push(pChildToken);
			CloneStack.push(pNewCloneToken);
		}
	}
	
	return pRetToken;
}

void CExpressionParser::SetChanged()
{
	m_bChanged = true;
}

bool CExpressionParser::GetChanged() const
{
	return m_bChanged;
}

bool CExpressionParser::AddRule(const _TCHAR* cszSource, const _TCHAR* cszDestination)
{
	CExpressionRule *pNewRule = new CExpressionRule(cszSource, cszDestination, this);
	//m_Rules.push_back(pNewRule);
	return true;
}

void CExpressionParser::CleanTree()
{
	TOKENSETITR it = m_TokenPool.begin();

	while (it != m_TokenPool.end())
	{
		bool bDeleted = false;
		switch ((*it)->GetType())
		{
		case ETT_LB:
		case ETT_RB:
		case ETT_EOF:
		case ETT_COMMA:
			delete *it;
			it = m_TokenPool.erase(it);
			bDeleted = true;
			break;
		}
		if (!bDeleted)
			it++;
	}
}

void CExpressionParser::RemoveRoot()
{
	CExpressionToken *pToken = m_ResultStack.top();
	if (pToken->GetType() == ETT_ROOT)
	{
		m_ResultStack.pop();
		m_ResultStack.push(pToken->GetChild());
	}
}

void CExpressionParser::GetExpressionConstants()
{
	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		if (pToken->IsLeaf() && !pToken->GetTextValue())
			pToken->SetTextValue(GetText(pToken));
	}
}

void CExpressionParser::CheckLinks() const
{
	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		switch (pToken->GetType())
		{
		case ETT_ROOT:
		case ETT_VARIABLE:
		case ETT_MODELLINK:
			break;
		default:
			_ASSERTE(pToken->ParentsCount() > 0);
			break;
		}
	}
}

const _TCHAR* CExpressionParser::GetErrorDescription()
{
	for (TOKENSETITR it = m_TokenPool.begin(); it != m_TokenPool.end(); it++)
	{
		CExpressionToken *pToken = *it;
		if (pToken->IsError())
			return GetErrorDescription(pToken); 
	}

	m_szErrorDescription = _T('\0');

	return m_szErrorDescription.c_str();
}

// создание токена ссылки на модель по разобранному тексту
bool CExpressionParser::BuildModelLink(CExpressionTokenModelLink *pModelLink)
{
	bool bRes = true;
	// после приема текста можем находиться только в режиме ожидания букв
	_ASSERTE(pModelLink->nWait == CExpressionTokenModelLink::MLS_ALPHA_CONTINUE);
	pModelLink->AdvanceChild(m_nAdvanceNextSymbol);
	//для всех токенов, которые входя в ссылку создаем текстовые представления
	for (TOKENITR it = pModelLink->ChildrenBegin(); it != pModelLink->ChildrenEnd(); it++)
		(*it)->SetTextValue(GetText(*it));
	// и таже создаем текст для всей ссылки
	pModelLink->SetTextValue(GetText(pModelLink));		// сначала имя таблицы
	pModelLink->EvaluateText();							// а потом ссылку полностью по дочерним элементам

	// для ссылки создаем переменную
	VariableEnum *pEnum = m_Variables.Find(pModelLink->GetTextValue());
	if (!pEnum)
	{
		// если ее еще нет
		m_Variables.Add(pModelLink->GetTextValue(), pModelLink);
		m_TokenPool.insert(pModelLink);
	}
	else
	{
		// а если есть, то заменяем токен обрабатываемой ссылки на токен переменной, так как они указывают на один и тот же объект модели
		m_pCurrentToken = pEnum->m_pToken;
		delete pModelLink;
	}
	m_bContinueToken = false;
	return bRes;
}

bool CExpressionParser::Process(const _TCHAR *cszExpression)
{
	bool bRes = Parse(cszExpression);
	bRes = bRes && Simplify();
	bRes = bRes && Expand();
	bRes = bRes && Simplify();

#ifdef _DEBUG
	wstring str;
	bRes = bRes && Infix(str);
#endif

	return bRes;
}

VariableEnum* CExpressionParser::CreateInternalVariable(const _TCHAR* cszVarName)
{
	VariableEnum *pInternalVar = m_Variables.Find(cszVarName);
	if (pInternalVar)
	{
		pInternalVar->m_eVarType = eCVT_INTERNAL;
		return pInternalVar;
	}

	CExpressionToken *pToken = new CExpressionToken(this, ETT_VARIABLE, 0, 0);
	pToken->SetTextValue(cszVarName);
	m_TokenPool.insert(pToken);
	m_Variables.Add(cszVarName, pToken);
	pInternalVar = m_Variables.Find(cszVarName);
	pInternalVar->m_eVarType = eCVT_INTERNAL;
	return pInternalVar;
}


CParserVariables::~CParserVariables()
{
	Clear();
}

void CParserVariables::Clear()
{
	m_Variables.clear();
}

bool CParserVariables::ChangeToEquationNames()
{
	set<CExpressionParser*> Parsers;

	for (VARIABLEITR it = m_Variables.begin(); it != m_Variables.end(); it++)
	{
		wstring compname;
		it->second.m_pToken->GetEquationOperand(it->second.m_pToken->m_pEquation, compname);
		it->second.m_pToken->SetTextValue(compname.c_str());
		Parsers.insert(it->second.m_pToken->m_pParser);
	}

	for (set<CExpressionParser*>::iterator pit = Parsers.begin(); pit != Parsers.end(); pit++)
	{
		CExpressionParser *pParser = *pit;
		wstring Infix;
		pParser->Infix(Infix);
	}
	return true;
}

// ищем переменную по имени и возвращаем найденное инфо
VariableEnum *CParserVariables::Find(const _TCHAR* cszVarName)
{
	VariableEnum *pEnum = NULL;
	VARIABLEITR it = m_Variables.find(cszVarName);
	if (it != m_Variables.end())
		pEnum = &it->second;
	return pEnum;
}

// изменение имени переменной, например BASE <- #Table[Key].Prop
bool CParserVariables::Rename(const _TCHAR *cszVarName, const _TCHAR *cszNewVarName)
{
	bool bRes = false;
	VariableEnum *pVarEnum = Find(cszVarName);
	if (pVarEnum)
	{
		// находим по заданному имени переменную,
		// сохраяем информацию по переменной
		VariableEnum Tmp = *pVarEnum;
		// ищем информацию по новой переменной
		VariableEnum *pCheckEnum = Find(cszNewVarName);
		if (pCheckEnum)
		{
			// если информация найдена - удаляем старую переменную
			if (m_Variables.erase(cszVarName))
			{
				// и обновляем найденную информацию по старой
				pCheckEnum->m_eVarType = Tmp.m_eVarType;
				bRes = pCheckEnum->m_pToken->Join(Tmp.m_pToken);
				Tmp.m_pToken->Delete();
			}
		}
		else
		{
			// если информации по новой переменной нет 
			// просто меняем имя переменной в токене
			// и вставляем новое имя в список переменных
			pVarEnum->m_pToken->SetTextValue(cszNewVarName);
			if (m_Variables.erase(cszVarName))
				bRes = m_Variables.insert(make_pair(cszNewVarName, Tmp)).second;
		}
	}
	return bRes;
}

// поиск переменной по имени
VariableEnum *CParserVariables::Find(const wstring& strVarName)
{
	return Find(strVarName.c_str());
}

// поиск перемеменной по уравнению
VariableEnum *CParserVariables::Find(const CCompilerEquation *pEquation)
{
	for (VARIABLEITR it = m_Variables.begin(); it != m_Variables.end(); it++)
	{
		if (it->second.m_pToken->m_pEquation == pEquation)
			return &it->second;
	}
	return NULL;
}

// добавляем новую перменную с токеном
void CParserVariables::Add(const _TCHAR* cszVarName, CExpressionToken* pToken)
{
	m_Variables.insert(make_pair(cszVarName, VariableEnum(pToken)));
	// если токен - ссылка на модель - ставим тип переменной "внешняя"
	if (pToken->GetType() == ETT_MODELLINK)
		m_Variables.find(cszVarName)->second.m_eVarType = eCVT_EXTERNAL;

}

void CParserVariables::Add(const wstring& strVarName, CExpressionToken* pToken)
{
	Add(strVarName.c_str(), pToken);
}


const CParserDefaultDictionary CExpressionParser::DefaultDictionary;
const CExpressionRulesDefault CExpressionParserRules::DefaultRules;
