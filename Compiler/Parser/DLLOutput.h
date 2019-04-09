#pragma once

#include "AutoCompilerMessages.h"

class CCompiler;

class CDLLOutput
{
protected:
	bool m_bStatus;
	FILE *m_pFile;
	CCompiler& m_Compiler;
	wstring m_strFilePath;
	bool EmitVarCount(const _TCHAR *cszFunctionName, size_t nCount);
public:
	CDLLOutput(CCompiler& Logger);
	bool CreateSourceFile(const _TCHAR *cszPath);
	bool EmitDeviceTypeName(const _TCHAR *cszTypeName);
	bool EmitVariableCounts();
	bool EmitBlockDescriptions();
	bool EmitDestroy();
	bool EmitBuildRightHand();
	bool EmitBuildEquations();
	bool EmitBuildDerivatives();
	bool EmitVariablesInfo();
	bool EmitInits();
	bool EmitProcessDiscontinuity();
	bool EnitBuildDerivatives();
	bool EmitTypes();

	void CloseSourceFile();

	~CDLLOutput();


	static const _TCHAR *cszDLLExportDecl;
	static const _TCHAR *cszSetPinInfoInternal;
	static const _TCHAR *cszSetPinInfoExternal;
	static const _TCHAR *cszBlockParameter;
};

