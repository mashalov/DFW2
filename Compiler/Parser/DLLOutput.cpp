#include "..\stdafx.h"
#include "..\..\DFW2\cex.h"
#include "DLLOutput.h"
#include "BaseCompiler.h"


CDLLOutput::CDLLOutput(CCompiler& Compiler) :	 m_bStatus(true),
												 m_pFile(NULL),
												 m_Compiler(Compiler)
{
}

void CDLLOutput::CloseSourceFile()
{
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

CDLLOutput::~CDLLOutput()
{
	CloseSourceFile();
}

bool CDLLOutput::CreateSourceFile(const _TCHAR *cszFile)
{
	m_strFilePath = cszFile;

	try
	{
		std::wstring strDir = GetDirectory(cszFile);
		if(!CreateAllDirectories(strDir.c_str()))
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileOpenError, m_strFilePath.c_str()));

		//if (_tfopen_s(&m_pFile, cszFile, _T("w+,ccs=UTF-8")))
		if (_tfopen_s(&m_pFile, cszFile, _T("w+")))
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileOpenError, m_strFilePath.c_str()));

		if (_ftprintf_s(m_pFile, _T("\n#include \"..\\Headers\\DeviceDLL.h\"\n\n") ) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		/*if (_ftprintf_s(m_pFile, _T("\n#include \"stdafx.h\"\n#include \"..\\DeviceDLL\\DeviceDLL.h\"\n\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));*/

	}
	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}

bool CDLLOutput::EmitDeviceTypeName(const _TCHAR *cszTypeName)
{
	if (!m_bStatus) return m_bStatus;
	if (_ftprintf_s(m_pFile, _T("\n%s const _TCHAR* GetDeviceTypeName(void)\n{\n\treturn _T(\"%s\");\n}\n"), cszDLLExportDecl, cszTypeName) < 0)
	{
		CDFW2GetLastErrorException ex(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}
	return m_bStatus;
}

bool CDLLOutput::EmitVarCount(const _TCHAR *cszFunctionName, size_t nCount)
{
	if (!m_bStatus) return m_bStatus;

	if (_ftprintf_s(m_pFile, _T("\n%s long %s(void)\n{\n\treturn %td;\n}\n"), cszDLLExportDecl, cszFunctionName, nCount) < 0)
	{
		m_bStatus = false;
		throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
	}

	return m_bStatus;
}

bool CDLLOutput::EmitDestroy()
{
	if (!m_bStatus) return m_bStatus;

	if (_ftprintf_s(m_pFile, _T("\n%s long Destroy(void)\n{\n\treturn 1;\n}\n"), cszDLLExportDecl) < 0)
	{
		CDFW2GetLastErrorException ex(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}


bool CDLLOutput::EmitVariableCounts()
{
	if (!m_bStatus) return m_bStatus;

	const CCompilerEquations& Equations = m_Compiler.GetEquations();

	try
	{
		EmitVarCount(_T("GetInputsCount"), Equations.GetExternalVars().size());
		EmitVarCount(_T("GetOutputsCount"), Equations.GetOutputVars().size());
		EmitVarCount(_T("GetSetPointsCount"), Equations.GetSetpointVars().size());
		EmitVarCount(_T("GetConstantsCount"), Equations.GetConstVars().size());
		EmitVarCount(_T("GetBlocksCount"), Equations.GetBlocks().size());

		size_t nCount = 0;

		for (EQUATIONSCONSTITR it = Equations.EquationsBegin(); it != Equations.EquationsEnd(); it++)
		{
			const CCompilerEquation* pEquation = *it;
			const CExpressionToken *pToken = pEquation->m_pToken;
			if (!pToken->IsHostBlock())
				nCount++;
		}

		EmitVarCount(_T("GetInternalsCount"), nCount);
	}

	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}

bool CDLLOutput::EmitBlockDescriptions()
{
	if (!m_bStatus) return m_bStatus;
	try
	{

		// GetBlockDescriptions
		if (_ftprintf_s(m_pFile, _T("\n%s long GetBlocksDescriptions(BlockDescriptions *pBlockDescriptions)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		const TOKENLIST& Blocks = m_Compiler.GetEquations().GetBlocks();

		size_t nCount = 0;
		const _TCHAR* strBlockName = _T("Us");

		for (CONSTTOKENITR bit = Blocks.begin(); bit != Blocks.end() && m_bStatus; bit++, nCount++)
		{
			const CExpressionToken *pToken = *bit;

			if (_ftprintf(m_pFile, _T("\tSetBlockDescription(pBlockDescriptions + %5zu, %-20s, %5zu, _T(\"%s\") );\n"), nCount, pToken->GetBlockType(), nCount, pToken->GetBlockName()) < 0)
				throw CDFW2GetLastErrorException (Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		// GetBlockPinsCount
		if (_ftprintf(m_pFile, _T("\n\treturn GetBlocksCount();\n}\n\n%s long GetBlockPinsCount(long nBlockIndex)\n{\n"), cszDLLExportDecl) < 0 )
			throw CDFW2GetLastErrorException (Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (Blocks.empty())
		{
			if (_ftprintf(m_pFile, _T("\treturn -1;\n}\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}
		else
		{
			if (_ftprintf(m_pFile, _T("\tconst long PinsCount[%zu] = { "), Blocks.size()) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

			for (CONSTTOKENITR bit = Blocks.begin(); bit != Blocks.end() && m_bStatus; bit++)
			{
				const CExpressionToken *pToken = *bit;

				if (bit != Blocks.begin())
					if (_ftprintf(m_pFile, _T(", ")) < 0)
						throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				if (_ftprintf(m_pFile, _T("%d"), pToken->GetPinsCount()) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}

			if (_ftprintf(m_pFile, _T(" };\n\n\tif(nBlockIndex >= 0 && nBlockIndex < %td)\n\t\treturn PinsCount[nBlockIndex];\n\n\treturn -1;\n}\n\n"), Blocks.size()) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		// GetBlockPinIndexes
		if (_ftprintf(m_pFile, _T("%s long GetBlockPinsIndexes(long nBlockIndex, BLOCKPININDEX* pBlockPins)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (Blocks.empty())
		{
			if (_ftprintf(m_pFile, _T("\treturn -1;\n}\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}
		else
		{
			if (_ftprintf(m_pFile, _T("\tswitch(nBlockIndex)\n\t{\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

			nCount = 0;

			for (CONSTTOKENITR bit = Blocks.begin(); bit != Blocks.end() && m_bStatus; bit++, nCount++)
			{
				const CExpressionToken *pToken = *bit;

				if (_ftprintf(m_pFile, _T("\tcase %td:\n"), nCount) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				size_t nPinCount = 0;

				if (_ftprintf(m_pFile, cszSetPinInfoInternal, nPinCount++, pToken->m_pEquation->m_nIndex) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				for (CONSTRTOKENITR pit = pToken->FirstChild(); pit != pToken->LastChild() && nPinCount < static_cast<size_t>(pToken->GetPinsCount()); pit++, nPinCount++)
				{
					const CExpressionToken *pChildToken = *pit;
					const CCompilerEquation *pEquation = pChildToken->m_pEquation;

					if (!pEquation)
						if (pChildToken->IsVariable())
						{
							VariableEnum *pVarEnum = m_Compiler.m_Variables.Find(pChildToken->GetTextValue());
							if (pVarEnum)
								pEquation = pVarEnum->m_pToken->m_pEquation;
						}

					if (pEquation)
					{
						if (_ftprintf(m_pFile, cszSetPinInfoInternal, nPinCount, pEquation->m_nIndex) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
					}
					else
					{
						VariableEnum *pVarEnum = m_Compiler.m_Variables.Find(pChildToken->GetTextValue());
						_ASSERTE(pVarEnum->m_eVarType == eCVT_EXTERNAL);
						if (_ftprintf(m_pFile, cszSetPinInfoExternal, nPinCount, pVarEnum->GetIndex(), pChildToken->GetTextValue()) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
					}
				}

				if (_ftprintf(m_pFile, _T("\t\tbreak;\n\n")) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}

			if (_ftprintf(m_pFile, _T("\t}\n\treturn GetBlockPinsCount(nBlockIndex);\n}\n\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		// GetBlockParamtersCount
		if (_ftprintf(m_pFile, _T("%s long GetBlockParametersCount(long nBlockIndex)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (Blocks.empty())
		{
			if (_ftprintf(m_pFile, _T("\treturn -1;\n}\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}
		else
		{
			if (_ftprintf(m_pFile, _T("\tconst long ParametersCount[%zd] = { "), Blocks.size()) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

			for (CONSTTOKENITR bit = Blocks.begin(); bit != Blocks.end() && m_bStatus; bit++)
			{
				const CExpressionToken *pToken = *bit;

				if (bit != Blocks.begin())
					if (_ftprintf(m_pFile, _T(", ")) < 0)
						throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));


				ptrdiff_t nBlockParametersCount = pToken->ChildrenCount() - pToken->GetPinsCount() + 1;

				if (_ftprintf(m_pFile, _T("%td"), nBlockParametersCount) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}

			if (_ftprintf(m_pFile, _T(" };\n\n\tif(nBlockIndex >= 0 && nBlockIndex < %zd)\n\t\treturn ParametersCount[nBlockIndex];\n\n\treturn -1;\n}\n\n"), Blocks.size()) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}
	}

	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}

bool CDLLOutput::EmitVariablesInfo()
{
	if (!m_bStatus) return m_bStatus;

	try
	{
		const CCompilerEquations& Equations = m_Compiler.GetEquations();

		// GetSetPointsInfos
		if (_ftprintf_s(m_pFile, _T("\n%s long GetSetPointsInfos(VarsInfo *pVarsInfo)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		const COMPILERVARS& SetPoints = Equations.GetSetpointVars();

		size_t nCount = 0;
		for (COMPILERVARSCONSTITR it = SetPoints.begin(); it != SetPoints.end(); it++, nCount++)
		{
			const CExpressionToken *pToken = it->second->m_pToken;
			if (_ftprintf(m_pFile, _T("\tSetVarInfo(pVarsInfo + %5zd, %5zd, _T(\"%s\"), VARUNIT_NOTSET, 0);\n"), 
                                                                                                                    nCount, 
                                                                                                                    it->second->GetIndex(), 
                                                                                                                    pToken->GetTextValue()) < 0)

				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetSetPointsCount();\n}\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));


        // GetInputsInfos
		if (_ftprintf_s(m_pFile, _T("\n%s long GetInputsInfos(InputVarsInfo *pVarsInfo)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		const COMPILERVARS& Inputs = Equations.GetExternalVars();

		nCount = 0;

		for (COMPILERVARSCONSTITR it = Inputs.begin(); it != Inputs.end(); it++, nCount++)
		{
			const CExpressionToken *pToken = it->second->m_pToken;
			if (_ftprintf(m_pFile, _T("\tSetInputVarsInfo(pVarsInfo + %5td, _T(\"%s\"), DEVTYPE_MODEL, _T(\"%s\") );\n"),
				it->second->GetIndex(),
				pToken->GetTextValue(),
                _T("") ) < 0)

				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetInputsCount();\n}\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		// GetConstantsInfos
		if (_ftprintf_s(m_pFile, _T("\n%s long GetConstantsInfos(ConstVarsInfo *pVarsInfo)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		const COMPILERVARS& Consts = Equations.GetConstVars();

		nCount = 0;

		for (COMPILERVARSCONSTITR it = Consts.begin(); it != Consts.end(); it++, nCount++)
		{
			const CExpressionToken *pToken = it->second->m_pToken;
			double dMin = 0.0;
			double dMax = 0.0;
			double dDefault = 0.0;
			std::wstring strDeviceType = _T("DEVTYPE_UNKNOWN");
			std::wstring strConstFlags = _T("0");
			if (_ftprintf(m_pFile, _T("\tSetConstantVarInfo(pVarsInfo + %5zd, _T(\"%s\"), %10g, %10g, %10g, %s, %s);\n"),
				nCount,
				pToken->GetTextValue(),
				dMin,
                dMax,
                dDefault,
				strDeviceType.c_str(),
				strConstFlags.c_str() ) < 0)

				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetConstantsCount();\n}\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		// GetOutputsInfos
		if (_ftprintf_s(m_pFile, _T("\n%s long GetOutputsInfos(VarsInfo *pVarsInfo)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		const COMPILERVARS& Outputs = Equations.GetOutputVars();

		nCount = 0;

		for (COMPILERVARSCONSTITR it = Outputs.begin(); it != Outputs.end(); it++, nCount++)
		{
			const CExpressionToken *pToken = it->second->m_pToken;

			if (_ftprintf(m_pFile, _T("\tSetVarInfo(pVarsInfo + %5zd, %5td, _T(\"%s\"), VARUNIT_NOTSET, 1);\n"),
				nCount,
				it->second->GetIndex(),
				pToken->GetTextValue()) < 0)

				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetOutputsCount();\n}\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));


		// GetInternalsInfos
		if (_ftprintf_s(m_pFile, _T("\n%s long GetInternalsInfos(VarsInfo *pVarsInfo)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		nCount = 0;

		for (EQUATIONSCONSTITR it = Equations.EquationsBegin(); it != Equations.EquationsEnd(); it++)
		{
    		const CCompilerEquation* pEquation = *it;
			const CExpressionToken *pToken = pEquation->m_pToken;

			if (!pToken->IsHostBlock())
			{
				VariableEnum* pEnum = m_Compiler.m_Variables.Find(pEquation);
				const COMPILERVARS& OutVars = m_Compiler.GetEquations().GetOutputVars();

				COMPILERVARSCONSTITR oit = OutVars.begin();

				for (; oit != OutVars.end(); oit++)
				{
					const CExpressionToken *pToken = oit->second->m_pToken;
					if (pToken && pToken->m_pEquation == pEquation)
						break;
				}

				if (_ftprintf(m_pFile, _T("\tSetVarInfo(pVarsInfo + %5zd, %5td, _T(\"%s\"), VARUNIT_NOTSET, %d);\n"),
					nCount,
					pEquation->m_nIndex,
					pEnum ? pEnum->m_pToken->GetTextValue() : pToken->GetTextValue(),
					oit == OutVars.end()  ? 0 : 1) < 0)

					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				nCount++;
			}
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetInternalsCount();\n}\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
	}

	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}


bool CDLLOutput::EmitBuildRightHand()
{
	if (!m_bStatus) return m_bStatus;

	const CCompilerEquations& Equations = m_Compiler.GetEquations();

	try
	{
		// BuildRightHand
		if (_ftprintf_s(m_pFile, _T("\n%s long BuildRightHand(BuildEquationsArgs *pArgs)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (_ftprintf_s(m_pFile, _T("\tBuildEquationsObjects *pm = &pArgs->BuildObjects;\n\tMDLSETFUNCTION SetFunction = pArgs->pFnSetFunction;\n\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		for (EQUATIONSCONSTITR it = Equations.EquationsBegin(); it != Equations.EquationsEnd(); it++)
		{
			const CCompilerEquation* pEquation = *it;
			const CExpressionToken *pToken = pEquation->m_pToken;

			if (!pEquation->m_pToken->IsHostBlock())
			{
				std::wstring str = pEquation->Generate();

				if (_ftprintf(m_pFile, _T("\t// %s\n"), pEquation->m_pToken->GetTextValue() ) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				if (_ftprintf(m_pFile, _T("\t(SetFunction)(pm, %5td, pArgs->pEquations[%td].Value - (%s));\n"),
                        pEquation->m_nIndex,
						pEquation->m_nIndex,
						str.c_str() ) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}
			else
			{
				if (_ftprintf(m_pFile, _T("\t// %td %s in host block \n"), pEquation->m_nIndex, pEquation->m_pToken->GetTextValue()) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetInternalsCount();\n}\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

	}
	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}


bool CDLLOutput::EmitBuildEquations()
{
	if (!m_bStatus) return m_bStatus;

	const CCompilerEquations& Equations = m_Compiler.GetEquations();

	try
	{
		// GetSetPointsInfos
		if (_ftprintf_s(m_pFile, _T("\n%s long BuildEquations(BuildEquationsArgs *pArgs)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (_ftprintf_s(m_pFile, _T("\tBuildEquationsObjects *pm = &pArgs->BuildObjects;\n\tMDLSETELEMENT SetElement = pArgs->pFnSetElement;\n\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		for (EQUATIONSCONSTITR it = Equations.EquationsBegin(); it != Equations.EquationsEnd(); it++)
		{
			const CCompilerEquation *pEquation = *it;
			CExpressionToken *pToken = pEquation->m_pToken;

			if (!pToken->IsHostBlock())
			{
				std::wstring Diagonal, Diff, strCol;

				if (pToken->IsVariable() && !pToken->IsConst())
				{
					
					bool bExternalGuard = pToken->GetEquationOperandIndex(pEquation, strCol);
					// если производная по внешней переменной, то вставляем проверку на индекс
					// внешней переменной равный DFW2_NON_STATE_INDEX. В случае, если индекс имеет
					// такое значение, не вставляем элемент матрицы на столбец с этой переменной

					Diff = _T("1");

					if (_ftprintf(m_pFile, _T("\t// External variable %s\n"), pToken->GetTextValue() ) < 0)
						throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

					if (bExternalGuard)
					{
						// проверка внешней переменной на индекс DFW2_NON_STATE_INDEX
						if (_ftprintf(m_pFile, _T("\tif(%s != DFW2_NON_STATE_INDEX)\n\t(SetElement)(pm, %5td, %-40s, -(%s), 0 );\n"),
							strCol.c_str(),
							pEquation->m_nIndex,
							strCol.c_str(),
							Diff.c_str()) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
					}
					else
					{
						if (_ftprintf(m_pFile, _T("\t(SetElement)(pm, %5td, %-40s, -(%s), 0 );\n"),
							pEquation->m_nIndex,
							strCol.c_str(),
							Diff.c_str()) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
					}
				}
				else
				{
					struct CommentedRow
					{
						std::wstring m_Value;
						std::wstring m_Comment;
						bool bExternalGuard;
						CommentedRow(std::wstring& Value, std::wstring& Comment, bool bExtGuard) : m_Value(Value),
																						 m_Comment(Comment), 
																						 bExternalGuard(bExtGuard) {};
					};

					typedef std::map<std::wstring, CommentedRow> MATRIXROW;
					MATRIXROW MatrixRow;

					for (TOKENLIST::reverse_iterator tkit = pToken->m_Children.rbegin(); tkit != pToken->m_Children.rend(); tkit++)
					{
						CExpressionToken *pChildToken = *tkit;
						bool bExternalGuard = false;
						if (Equations.GetDerivative(pToken, pChildToken, Diff, strCol, bExternalGuard))
						{
							// если производная по внешней переменной, то вставляем проверку на индекс
							// внешней переменной равный DFW2_NON_STATE_INDEX. В случае, если индекс имеет
							// такое значение, не вставляем элемент матрицы на столбец с этой переменной

							std::wstring DiagIndex = Cex(_T("%d"), pEquation->m_nIndex);
							if (DiagIndex == strCol)
							{
								Diagonal = Diff;
								if (Diff == _T("1"))
								{
									m_Compiler.m_Logger.Log(Cex(CAutoCompilerMessages::cszSingularMatrix));
									m_bStatus = false;
								}
							}
							else
							{
								std::wstring Comment = Cex(_T("%s by %s"), pToken->GetTextValue(), pChildToken->GetTextValue());
								MATRIXROW::iterator mit = MatrixRow.find(strCol);
								if (mit == MatrixRow.end())
									MatrixRow.insert(make_pair(strCol, CommentedRow(Diff, Comment, bExternalGuard)));
								else
								{
									mit->second.m_Value   += _T(" + ") + Diff;
									mit->second.m_Comment += _T(" plus ") + Comment;
									_ASSERTE(mit->second.bExternalGuard != bExternalGuard);
								}
							}
						}
					}
					 
					if (m_bStatus)
					{
						for (MATRIXROW::iterator mit = MatrixRow.begin(); mit != MatrixRow.end(); mit++)
						{
							if (_ftprintf(m_pFile, _T("\t// %s\n"), mit->second.m_Comment.c_str()) < 0)
								throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

							if (mit->second.bExternalGuard)
							{
								// проверка внешней переменной на индекс DFW2_NON_STATE_INDEX
								if (_ftprintf(m_pFile, _T("\tif(%s != DFW2_NON_STATE_INDEX)\n\t(SetElement)(pm, %5td, %-40s, -(%s), 0 );\n"),
									mit->first.c_str(),
									pEquation->m_nIndex,
									mit->first.c_str(),
									mit->second.m_Value.c_str()) < 0)
									throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
							}
							else
							{
								if (_ftprintf(m_pFile, _T("\t(SetElement)(pm, %5td, %-40s, -(%s), 0 );\n"),
									pEquation->m_nIndex,
									mit->first.c_str(),
									mit->second.m_Value.c_str()) < 0)
									throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
							}
						}
					}
				}

				if (m_bStatus)
				{
					if (_ftprintf(m_pFile, _T("\t// %s / %s;\n"), pToken->GetTextValue(), pToken->GetTextValue()) < 0)
						throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
                        
                    if (Diagonal.empty())
					{
						if (_ftprintf(m_pFile, _T("\t(SetElement)(pm, %5td, %-40td, 1.0, 0);\n"),
							pEquation->m_nIndex,
							pEquation->m_nIndex ) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
					}
					else
					{
						if (_ftprintf(m_pFile, _T("\t(SetElement)(pm, %5td, %-40td, 1.0 - (%s), 0);\n"),
							pEquation->m_nIndex,
							pEquation->m_nIndex,
							Diagonal.c_str()) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
					}
				}
			}
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetInternalsCount();\n}\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

	}
	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}

bool CDLLOutput::EmitInits()
{
	if (!m_bStatus) return m_bStatus;

	const CCompilerEquations& Equations = m_Compiler.GetEquations();
	const TOKENLIST& Blocks = m_Compiler.GetEquations().GetBlocks();

	try
	{

		// GetBlockParamtersValues
		if (_ftprintf(m_pFile, _T("\n%s long GetBlockParametersValues(long nBlockIndex, BuildEquationsArgs *pArgs, double* pBlockParameters)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (Blocks.empty())
		{
			if (_ftprintf(m_pFile, _T("\n\treturn -1;\n}\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}
		else
		{
			if (_ftprintf(m_pFile, _T("\tswitch(nBlockIndex)\n\t{\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

			size_t nCount = 0;

			for (CONSTTOKENITR bit = Blocks.begin(); bit != Blocks.end() && m_bStatus; bit++, nCount++)
			{
				const CExpressionToken *pToken = *bit;

				size_t nPinCount = 0;
				size_t nPins = pToken->GetPinsCount() - 1;

				bool bCaseOut = true;

				for (CONSTRTOKENITR pit = pToken->FirstChild(); pit != pToken->LastChild(); pit++, nPinCount++)
				{
					const CExpressionToken *pChildToken = *pit;
					if (nPinCount >= nPins)
					{
						if (bCaseOut)
						{
							if (_ftprintf(m_pFile, _T("\tcase %td:\n"), nCount) < 0)
								throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

							bCaseOut = false;
						}

						CCompilerEquation *pEquation = pChildToken->m_pEquation;
						size_t nParameterIndex = nPinCount - nPins;
						std::wstring Operand = pChildToken->GetTextValue();
						if (_ftprintf(m_pFile, cszBlockParameter, nParameterIndex, Operand.c_str()) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
					}
				}

				if (!bCaseOut)
					if (_ftprintf(m_pFile, _T("\t\tbreak;\n\n")) < 0)
						throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}

			if (_ftprintf(m_pFile, _T("\t}\n\n\treturn GetBlockParametersCount(nBlockIndex);\n}\n\n")) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		// DeviceInit
		if (_ftprintf(m_pFile, _T("\n%s long DeviceInit(BuildEquationsArgs *pArgs)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (_ftprintf_s(m_pFile, _T("\tBuildEquationsObjects *pm = &pArgs->BuildObjects;\n\tMDLINITBLOCK InitBlock = pArgs->pFnInitBlock;\n\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		for (VARIABLECONSTITR vspit = m_Compiler.m_Variables.Begin(); vspit != m_Compiler.m_Variables.End(); vspit++)
		{
			if (vspit->second.m_eVarType == eCVT_EXTERNALSETPOINT)
			{
				CBaseToExternalLink *pLink = static_cast<CBaseToExternalLink*>(vspit->second.m_pVarExtendedInfo);
				if (pLink)
				{
					if (_ftprintf_s(m_pFile, _T("\t%s = %s;\n"), 
						vspit->second.m_pToken->GetTextValue(), 
						pLink->m_pEnum->m_pToken->GetTextValue()) < 0)
							throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
				}
			}
		}

		for (EQUATIONSCONSTITR it = Equations.EquationsBegin(); it != Equations.EquationsEnd(); it++)
		{
			const CCompilerEquation* pEquation = *it;
			const CExpressionToken *pToken = pEquation->m_pToken;

			std::wstring str = pEquation->Generate(true);

			if (pToken->IsHostBlock())
			{

				int nCount = Equations.GetBlockIndex(pToken);

				if (_ftprintf(m_pFile, _T("\t// %s %d"), pToken->GetBlockType(), nCount) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));


				if (_ftprintf(m_pFile, _T("\t// pArgs->pEquations[%td].Value = %s;\n"), 
					pEquation->m_nIndex,
					str.c_str()) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				if (_ftprintf(m_pFile, _T("\t(*InitBlock)(pm,%d);\n"), nCount) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}
			else
			{
				if (_ftprintf(m_pFile, _T("\t// %s\n"), pEquation->m_pToken->GetTextValue()) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				if (_ftprintf(m_pFile, _T("\tpArgs->pEquations[%td].Value = %s;\n"),
					pEquation->m_nIndex,
					str.c_str()) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
			}
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetInternalsCount();\n}\n\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

	}
	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}

bool CDLLOutput::EmitTypes()
{
	if (!m_bStatus) return m_bStatus;
	try
	{
		// GetTypesCount
		if (_ftprintf(m_pFile, _T("\n%s long GetTypesCount(void)\n{\n\treturn 1;\n}\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		//GetTypes
		if (_ftprintf(m_pFile, _T("\n%s long GetTypes(long* pTypes)\n{\n\tpTypes[0] = DEVTYPE_AUTOMATIC;\n\treturn GetTypesCount();\n}\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		//GetLinksCount
		if (_ftprintf(m_pFile, _T("\n%s long GetLinksCount(void)\n{\n\treturn 1;\n}\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (_ftprintf(m_pFile, _T("\n%s long GetLinks(LinkType* pLink)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (_ftprintf(m_pFile, _T("\tSetLink(pLink + 0, eLINK_FROM, DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER, _T(\"\"));\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (_ftprintf(m_pFile, _T("\treturn GetLinksCount();\n}\n") ) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

	}
	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}


bool CDLLOutput::EmitBuildDerivatives()
{
	if (!m_bStatus) return m_bStatus;
	try
	{
		// BuildDerivatives
		if (_ftprintf(m_pFile, _T("\n%s long BuildDerivatives(BuildEquationsArgs* pArgs)\n{\n\treturn GetInternalsCount();\n}\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
	}
	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}

bool CDLLOutput::EmitProcessDiscontinuity()
{
	if (!m_bStatus) return m_bStatus;

	const CCompilerEquations& Equations = m_Compiler.GetEquations();
	const TOKENLIST& Blocks = m_Compiler.GetEquations().GetBlocks();

	try
	{
		// ProcessDiscontinuity
		if (_ftprintf(m_pFile, _T("\n%s long ProcessDiscontinuity(BuildEquationsArgs *pArgs)\n{\n"), cszDLLExportDecl) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		if (_ftprintf_s(m_pFile, _T("\tBuildEquationsObjects *pm = &pArgs->BuildObjects;\n\tMDLPROCESSBLOCKDISCONTINUITY ProcessBlockDiscontinuity = pArgs->pFnProcessBlockDiscontinuity;\n\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

		for (EQUATIONSCONSTITR it = Equations.EquationsBegin(); it != Equations.EquationsEnd(); it++)
		{
			const CCompilerEquation* pEquation = *it;
			const CExpressionToken *pToken = pEquation->m_pToken;

			if (pToken->IsHostBlock())
			{
				int nCount = Equations.GetBlockIndex(pToken);

				if (_ftprintf(m_pFile, _T("\t// %s %d\n"), pToken->GetBlockType(), nCount) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

				if (_ftprintf(m_pFile, _T("\t(*ProcessBlockDiscontinuity)(pm,%d);\n"), nCount) < 0)
					throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
				
				continue;
			}

			std::wstring str = pEquation->Generate(true);

			if (_ftprintf(m_pFile, _T("\t// %s\n"), pEquation->m_pToken->GetTextValue()) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

			if (_ftprintf(m_pFile, _T("\tpArgs->pEquations[%td].Value = %s;\n"),
				pEquation->m_nIndex,
				str.c_str()) < 0)
				throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));
		}

		if (_ftprintf(m_pFile, _T("\n\treturn GetInternalsCount();\n}\n\n")) < 0)
			throw CDFW2GetLastErrorException(Cex(CAutoCompilerMessages::cszFileWriteError, m_strFilePath.c_str()));

	}
	catch (CDFW2GetLastErrorException& ex)
	{
		m_Compiler.m_Logger.Log(ex.Message());
		m_bStatus = false;
	}

	return m_bStatus;
}

const _TCHAR *CDLLOutput::cszDLLExportDecl = _T("__declspec(dllexport)");
const _TCHAR *CDLLOutput::cszSetPinInfoInternal = _T("\t\tSetPinInfo(pBlockPins + %5d, %5d, eVL_INTERNAL);\n");
const _TCHAR *CDLLOutput::cszSetPinInfoExternal = _T("\t\tSetPinInfo(pBlockPins + %5d, %5d, eVL_EXTERNAL); // %s\n");
const _TCHAR *CDLLOutput::cszBlockParameter = _T("\t\tpBlockParameters[%d] = %s;\n");
