﻿#include "CompilerBase.h"

bool CompilerBase::SetProperty(std::string_view PropertyName, std::string_view Value)
{
	if (auto it = Properties.find(PropertyName); it != Properties.end())
	{
		it->second = Value;
		return true;
	}
	return false;
}

std::string CompilerBase::GetProperty(std::string_view PropertyName)
{
	if (auto it = Properties.find(PropertyName); it != Properties.end())
		return it->second;
	else
		return "";
}

void CompilerBase::SaveSource(std::string_view SourceToCompile, std::filesystem::path& pathSourceOutput)
{
	std::filesystem::path OutputPath(pathSourceOutput);
	OutputPath.remove_filename();
	std::filesystem::create_directories(OutputPath);
	// UTF-8 filename ?
	std::ofstream OutputStream(pathSourceOutput);
	if (!OutputStream.is_open())
    {
#ifdef _MSC_VER
        int ec(GetLastError());
#else
        int ec(errno);
#endif
        throw std::system_error(std::error_code(ec, std::system_category()), fmt::format("Невозможно открыть файл \"{}\"", OutputPath.string()));
    }
	OutputStream << SourceToCompile;
	OutputStream.close();
}

bool CompilerBase::NoRecompileNeeded(std::string_view SourceToCompile, std::filesystem::path& pathDLLOutput)
{
	bool bRes(false);
	if (!GetProperty(PropertyMap::szPropRebuild).empty())
		return bRes;

	if (auto source(GetSource(pathDLLOutput)); source.has_value())
	{
		std::string Gzip;
		CryptoPP::StringSource d64(source.value(), true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(Gzip)));
		source.value().clear();
		CryptoPP::StringSource gzip(Gzip, true, new CryptoPP::Gunzip(new CryptoPP::StringSink(source.value()), 1));
		if (source.value() == SourceToCompile)
			bRes = true;
	}

	if (bRes)
		pTree->Message(fmt::format("Модуль \"{}\" не нуждается в повторной компиляции", pathDLLOutput.string()));
	else
		pTree->Message(fmt::format("Необходима компиляция модуля \"{}\"", pathDLLOutput.string()));

	return bRes;
}


bool CompilerBase::Compile(std::string_view FilePath)
{
    bool bRes(false);
    std::ifstream strm;
    strm.open(std::string(FilePath));
    if (strm.is_open())
    {
        bRes = Compile(strm);
    }
    return bRes;
}

bool CompilerBase::Compile(std::istream& SourceStream)
{
    bool bRes(false);
    std::filesystem::path pathDLLOutput, pathSourceOutput;
    try
    {
        std::string Source((std::istreambuf_iterator<char>(SourceStream)), std::istreambuf_iterator<char>());
        pTree = std::make_unique<CASTTreeBase>(Properties);
        pTree->SetMessageCallBacks(messageCallBacks);
        pathDLLOutput = pTree->GetPropertyPath(PropertyMap::szPropDllLibraryPath);
        pathDLLOutput.append(Properties[PropertyMap::szPropProjectName]).replace_extension(".dll");

        pathSourceOutput = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
        pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);
        pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);

        SaveSource(Source, pathSourceOutput);


        if (NoRecompileNeeded(Source, pathDLLOutput))
            return true;

        auto pRuleTree = std::make_unique<CASTTreeBase>(Properties);
        antlr4::ANTLRInputStream input(Source);
        EquationsLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        EquationsParser parser(&tokens);
        auto errorListener = std::make_unique<AntlrASTErrorListener>(pTree.get(), input.toString());
        parser.removeErrorListeners();
        parser.addErrorListener(errorListener.get());
        lexer.removeErrorListeners();
        lexer.addErrorListener(errorListener.get());
        antlr4::tree::ParseTree* tree = parser.input();
        pRuleTree->CreateRules();
        AntlrASTVisitor visitor(pTree.get());
        visitor.visit(tree);
        pTree->PostParseCheck();
        if (!pTree->ErrorCount())
        {
            pTree->PrintInfix();
            pTree->Flatten();
            pTree->Sort();
            pTree->Collect();
            pTree->Sort();
            //pTree->Match(pRuleTree->GetRule());
            pTree->PrintInfix();
            pTree->GenerateEquations();
            pTree->PrintInfix();
        }
        parser.removeErrorListeners();
        lexer.removeErrorListeners();
        CASTCodeGeneratorBase codegen(pTree.get(), input.toString());
        codegen.Generate();
        // построить модуль с помощью выбранного компилятора
        BuildWithCompiler();
        bRes = true;
    }
    catch (std::exception& ex)
    {
        pTree->Error(fmt::format("Исключение {}", ex.what()));
    }

    if (pTree->ErrorCount() == 0)
        pTree->Message(fmt::format("Выполнена компиляция модуля \"{}\"", pathDLLOutput.filename().string()));
    else
        pTree->Error(fmt::format("Ошибка компиляции модуля \"{}\"", pathDLLOutput.filename().string()));

    return pTree->ErrorCount() == 0;
}