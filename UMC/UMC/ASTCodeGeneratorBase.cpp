#include "ASTCodeGeneratorBase.h"

BlockEmit::BlockEmit(CASTCodeGeneratorBase& CodeGenerator) : CodeGenerator_(CodeGenerator)
{
	CodeGenerator.EmitLine("{");
	CodeGenerator.Indent++;
}

BlockEmit::~BlockEmit()
{
	CodeGenerator_.Indent--;
	CodeGenerator_.EmitLine("}");
}
