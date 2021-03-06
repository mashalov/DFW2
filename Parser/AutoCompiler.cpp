#include "stdafx.h"
#include "AutoCompiler.h"

CAutoItem::CAutoItem(ptrdiff_t eType, ptrdiff_t nId, const _TCHAR* cszName, const _TCHAR *cszFormula) :
																															m_eType(eType),
																															m_nId(nId),
																															m_strName(cszName),
																															m_strFormula(cszFormula)
{ 
	trim(m_strFormula);
}

CAutoStarterItem::CAutoStarterItem(CAutoItem& AutoItem, CCompilerModelLink& ModelLink) : 
																															CAutoItem(AutoItem),
																															m_ModelLink(ModelLink)
{

}


CAutoLogicItem::CAutoLogicItem(CAutoItem& AutoItem, const _TCHAR* cszActions, const _TCHAR* cszDelay, int nOutputMode) :
																															CAutoItem(AutoItem),
																															m_strActions(cszActions),
																															m_strDelay(cszDelay),
																															m_nOutputMode(nOutputMode)
{
	trim(m_strDelay);
	trim(m_strActions);
}

CAutoActionItem::CAutoActionItem(CAutoItem& AutoItem, CCompilerModelLink& ModelLink, ptrdiff_t nActionGroup, int nOutputMode, int nRunsCount) :

																															CAutoStarterItem(AutoItem, ModelLink),
																															m_nActionGroup(nActionGroup),
																															m_nOutputMode(nOutputMode),
																															m_nRunsCount(nRunsCount)
{

}


CAutoCompilerItem::CAutoCompilerItem(CCompiler& Compiler) : CCompilerItem(Compiler)
{

}

bool CAutoCompilerItem::InsertEquations(CCompilerEquations& Equations)
{
	bool bRes = true;

	bRes = bRes && m_pParser->InsertEquations(Equations);

	return bRes;
}

bool CAutoCompilerItemBaseV::Process(CAutoStarterItem *pStarterItem)
{
	m_pParser = new CExpressionParserRules(m_Compiler.m_Variables);

	wstring strFormula = pStarterItem->GetFormula();
	if (strFormula.empty())
		strFormula = cszV;

	wstring EquationExpression = Cex(_T("%s"), strFormula.c_str());

	bool bRes = m_pParser->Process(EquationExpression.c_str());

	if (!bRes)
	{
		m_Compiler.m_Logger.Log(Cex(CAutoCompilerMessages::cszCompileError, EquationExpression.c_str(), m_pParser->GetErrorDescription()));
		return false;
	}
	
	CParserVariables& vars = m_Compiler.m_Variables;
	VariableEnum *pVarEnum = vars.Find(cszV);
	wstring strLink = pStarterItem->GetModelLink().ToString();
	if (pVarEnum)
	{
		pVarEnum->m_eVarType = eCVT_EXTERNAL;
		bRes = vars.Rename(cszV, strLink.c_str());
		if (!bRes)
			m_Compiler.m_Logger.Log(Cex(CAutoCompilerMessages::cszCannotSetVariable, strLink.c_str()));
		else
			m_Compiler.AddExternalVariable(strLink.c_str(), m_Compiler.m_Variables.Find(strLink.c_str()));
	}

	if (bRes)
	{
		pVarEnum = vars.Find(cszBase);
		if (pVarEnum)
		{
			pVarEnum->m_eVarType = eCVT_EXTERNALSETPOINT;
			strLink.insert(strLink.begin(), _T('#'));
			bRes = vars.Rename(cszBase, strLink.c_str());
			if (!bRes)
				m_Compiler.m_Logger.Log(Cex(CAutoCompilerMessages::cszCannotSetVariable, strLink.c_str()));
			else
				m_Compiler.AddSetPoint(strLink.c_str(), m_Compiler.m_Variables.Find(strLink.c_str()));
		}
	}

	wstring str;
	m_pParser->Infix(str);
	_tcprintf(_T("\n%s"), str.c_str());

	return bRes;
}

bool CCompilerAutoStarterItem::Process()
{
	return CAutoCompilerItemBaseV::Process(this); 
}

bool CCompilerAutoStarterItem::InsertEquations(CCompilerEquations& Equations)
{
	bool bRes = CAutoCompilerItem::InsertEquations(Equations);

	if (bRes)
	{
		wstring NewVarName(Cex(_T("S%d"), GetId()));
		_ASSERTE(m_pParser->m_ResultStack.size() > 0 && m_pParser->m_ResultStack.top()->ChildrenCount() > 0);
		CExpressionToken *pRootEquation = m_pParser->m_ResultStack.top()->GetChild();
		_ASSERTE(pRootEquation->m_pEquation);
		VariableEnum *pS = m_Compiler.m_Variables.Find(NewVarName);

		if (pS)
		{
			_ASSERTE(!pS->m_pToken->m_pEquation);
			//if (pS->m_pToken->m_pEquation)
				//Equations.ReplaceTemporalEquation(pS->m_pToken->m_pEquation, pRootEquation->m_pEquation);
			pS->m_pToken->m_pEquation = pRootEquation->m_pEquation;
		}
		else
		{
			pS = m_pParser->CreateInternalVariable(NewVarName.c_str());
			pS->m_pToken->m_pEquation = pRootEquation->m_pEquation;
		}

		m_Compiler.AddOutputVariable(NewVarName.c_str(), m_Compiler.m_Variables.Find(NewVarName.c_str()));

	}

	return bRes;
}


bool CCompilerAutoLogicItem::InsertEquations(CCompilerEquations& Equations)
{
	bool bRes = CAutoCompilerItem::InsertEquations(Equations);

	if (bRes)
	{
		wstring NewVarName(Cex(_T("LT%d"), GetId()));
		wstring NewVarLName(Cex(_T("L%d"), GetId()));

		_ASSERTE(m_pParser->m_ResultStack.size() > 0 && m_pParser->m_ResultStack.top()->ChildrenCount() > 0);
		CExpressionToken *pRelay = m_pParser->m_ResultStack.top()->GetChild();
		if (pRelay && pRelay->GetType() == ETT_RELAY && pRelay->ChildrenCount() > 0)
		{
			CExpressionToken *pRelayInput = *pRelay->FirstChild();

			_ASSERTE(pRelay->m_pEquation);
			_ASSERTE(pRelayInput->m_pEquation);

			VariableEnum *pRelayEqVar = m_Compiler.m_Variables.Find(NewVarName);
			VariableEnum *pRelayInputEqVar = m_Compiler.m_Variables.Find(NewVarLName);
			if (pRelayEqVar)
			{
				_ASSERTE(!pRelayEqVar->m_pToken->m_pEquation);
				pRelayEqVar->m_pToken->m_pEquation = pRelay->m_pEquation;
			}
			else
			{
				pRelayEqVar = m_pParser->CreateInternalVariable(NewVarName.c_str());
				pRelayEqVar->m_pToken->m_pEquation = pRelay->m_pEquation;
			}

			m_Compiler.AddOutputVariable(NewVarName.c_str(), m_Compiler.m_Variables.Find(NewVarName.c_str()));

			if (pRelayInputEqVar)
			{
				_ASSERTE(!pRelayInputEqVar->m_pToken->m_pEquation);
				pRelayInputEqVar->m_pToken->m_pEquation = pRelayInput->m_pEquation;
			}
			else
			{
				pRelayInputEqVar = m_pParser->CreateInternalVariable(NewVarLName.c_str());
				pRelayInputEqVar->m_pToken->m_pEquation = pRelayInput->m_pEquation;
			}

			m_Compiler.AddOutputVariable(NewVarLName.c_str(), m_Compiler.m_Variables.Find(NewVarLName.c_str()));
		}
		else
		{
			m_Compiler.m_Logger.Log(Cex(CAutoCompilerMessages::cszWrongLogicRelay, GetVerbalName().c_str()));
			bRes = false;
		}
	}

	return bRes;
}

bool CCompilerAutoLogicItem::Process()
{
	m_pParser = new CExpressionParserRules(m_Compiler.m_Variables);

	if (m_strFormula.empty())
		m_strFormula = _T("0");

	if (m_strDelay.empty())
		m_strDelay = _T("0");

	wstring EquationExpression = Cex(_T("relay(%s,0,%s)"), m_strFormula.c_str(), m_strDelay.c_str());

	bool bRes = m_pParser->Process(EquationExpression.c_str());

	if (!bRes)
	{
		m_Compiler.m_Logger.Log(Cex(CAutoCompilerMessages::cszCompileError, EquationExpression.c_str(), m_pParser->GetErrorDescription()));
		return false;
	}

	wstring str;
	m_pParser->Infix(str);
	_tcprintf(_T("\n%s"), str.c_str());

	return bRes;
}


bool CAutoCompiler::AddStarter(CAutoStarterItem& Starter)
{
	if (!m_bStatus) return m_bStatus;

	if (!m_Starters.insert(make_pair(Starter.GetId(), CCompilerAutoStarterItem(Starter,*this))).second)
	{
		m_Logger.Log(Cex(CAutoCompilerMessages::cszDuplicatedStarter, Starter.GetVerbalName().c_str()));
		m_bStatus = false;
	}

	return m_bStatus;
}


bool CAutoCompiler::AddLogic(CAutoLogicItem& Logic)
{
	if (!m_bStatus) return m_bStatus;

	if (!m_Logics.insert(make_pair(Logic.GetId(), CCompilerAutoLogicItem(Logic, *this))).second)
	{
		m_Logger.Log(Cex(CAutoCompilerMessages::cszDuplicatedLogic, Logic.GetVerbalName().c_str()));
		m_bStatus = false;
	}

	return m_bStatus;
}


bool CAutoCompiler::AddAction(CAutoActionItem& Action)
{
	if (!m_bStatus) return m_bStatus;

	if (!m_Actions.insert(make_pair(Action.GetId(), CCompilerAutoActionItem(Action, *this))).second)
	{
		m_Logger.Log(Cex(CAutoCompilerMessages::cszDuplicatedAction, Action.GetVerbalName().c_str()));
		m_bStatus = false;
	}

	return m_bStatus;
}


bool CAutoCompiler::Generate()
{
	if (!m_bStatus) return m_bStatus;

	STARTERMAPITR its;
	LOGICMAPITR itl;
	ACTIONMAPITR ita;

	for (its = m_Starters.begin(); its != m_Starters.end(); its++)
	{
		CCompilerAutoStarterItem& item = its->second;
		m_bStatus = m_bStatus && item.Process();
	}

	for (itl = m_Logics.begin(); itl != m_Logics.end(); itl++)
	{
		CCompilerAutoLogicItem& item = itl->second;
		m_bStatus = m_bStatus && item.Process();
	}

	for (ita = m_Actions.begin(); ita != m_Actions.end(); ita++)
	{
		CCompilerAutoActionItem& item = ita->second;
		m_bStatus = m_bStatus && item.Process();
	}

	_ASSERTE(m_bStatus);

	for (its = m_Starters.begin(); its != m_Starters.end(); its++)
	{
		CCompilerAutoStarterItem& item = its->second;
		m_bStatus = m_bStatus && item.InsertEquations(m_Equations);
	}
	
	for (itl = m_Logics.begin(); itl != m_Logics.end(); itl++)
	{
		CCompilerAutoLogicItem& item = itl->second;
		m_bStatus = m_bStatus && item.InsertEquations(m_Equations);
	}


	for (ita = m_Actions.begin(); ita != m_Actions.end(); ita++)
	{
		CCompilerAutoActionItem& item = ita->second;
		m_bStatus = m_bStatus && item.InsertEquations(m_Equations);
	}

	if (m_bStatus)
	{
		VariableEnum *pVar = m_Variables.Find(_T("t"));
		if (pVar)
		{
			pVar->m_eVarType = eCVT_HOST;
		}
	}

	if (m_bStatus)
	{
		for (VARIABLECONSTITR vit = m_Variables.Begin(); vit != m_Variables.End(); vit++)
		{
			const VariableEnum& VarEnum = vit->second;
			if (VarEnum.m_eVarType == eCVT_INTERNAL)
				if (VarEnum.m_pToken && VarEnum.m_pToken->m_pEquation == NULL)
				{
					m_Logger.Log(Cex(CAutoCompilerMessages::cszUnassignedVariable, vit->first.c_str()));
					m_bStatus = false;
				}
		}
	}


	//m_bStatus = m_bStatus && m_Equations.GenerateEquations();
	//m_bStatus = m_bStatus && m_Equations.GenerateMatrix();

	//C:\Users\mashalov\Documents\Visual Studio 2013\Projects\DFW2\AutoDLL\AutoDLL.cpp
	//C:\\Users\\Bug\\documents\\visual studio 2013\\Projects\\DFW2\\AutoDLL\\AutoDLL.cpp

	if (m_bStatus && m_DLLOutput.CreateSourceFile(_T("C:\\Users\\bug\\Documents\\Visual Studio 2013\\Projects\\DFW2\\AutoDLL\\AutoDLL.cpp")))
	{
		m_DLLOutput.EmitDeviceTypeName(CAutoCompilerMessages::cszAutomaticDLLTitle);
		m_DLLOutput.EmitDestroy();
		m_DLLOutput.EmitTypes();
		m_DLLOutput.EmitVariableCounts();
		m_DLLOutput.EmitBlockDescriptions();
		m_DLLOutput.EmitVariablesInfo();
		m_DLLOutput.EmitBuildEquations();
		m_DLLOutput.EmitBuildRightHand();
		m_DLLOutput.EmitBuildDerivatives();
		m_Variables.ChangeToEquationNames();
		m_DLLOutput.EmitInits();
		m_DLLOutput.EmitProcessDiscontinuity();
	}

	return m_bStatus;
}

bool CCompilerAutoActionItem::InsertEquations(CCompilerEquations& Equations)
{
	bool bRes = CAutoCompilerItem::InsertEquations(Equations);

	if (bRes)
	{
		wstring NewVarName(Cex(_T("A%d"), GetId()));
		_ASSERTE(m_pParser->m_ResultStack.size() > 0 && m_pParser->m_ResultStack.top()->ChildrenCount() > 0);
		CExpressionToken *pRootEquation = m_pParser->m_ResultStack.top()->GetChild();
		_ASSERTE(pRootEquation->m_pEquation);
		VariableEnum *pA = m_Compiler.m_Variables.Find(NewVarName);

		if (pA)
		{
			_ASSERTE(!pA->m_pToken->m_pEquation);
			pA->m_pToken->m_pEquation = pRootEquation->m_pEquation;
		}
		else
		{
			pA = m_pParser->CreateInternalVariable(NewVarName.c_str());
			pA->m_pToken->m_pEquation = pRootEquation->m_pEquation;
		}

		m_Compiler.AddOutputVariable(NewVarName.c_str(), m_Compiler.m_Variables.Find(NewVarName.c_str()));

	}

	return bRes;
}

bool CCompilerAutoActionItem::Process()
{
	return CAutoCompilerItemBaseV::Process(this);
}


const _TCHAR* CAutoCompilerItemBaseV::cszV = _T("V");
const _TCHAR* CAutoCompilerItemBaseV::cszBase = _T("BASE");
