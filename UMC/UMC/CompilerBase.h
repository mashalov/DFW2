#pragma once
#include "AntlrASTVisitor.h"
#include "ASTCodeGeneratorBase.h"
#include "ICompiler.h"

class CompilerBase : public ICompiler
{
protected:
    PropertyMap Properties =
    {
        {PropertyMap::szPropOutputPath, "c:\\tmp\\"},
        {PropertyMap::szPropProjectName, "AutoDLL"},
        {PropertyMap::szPropExportSource, "yes"},
        {PropertyMap::szPropReferenceSources, "C:\\Users\\masha\\source\\repos\\DFW2\\ReferenceCustomModel\\"},
        {PropertyMap::szPropDllLibraryPath, "C:\\tmp\\CustomModels\\dll\\"},
        {PropertyMap::szPropConfiguration, "Debug"},
        {PropertyMap::szPropPlatform, "Win32"},
        {PropertyMap::szPropRebuild, ""}
    };

    std::unique_ptr<CASTTreeBase> pTree;
    void CompileWithMSBuild();
    MessageCallBacks messageCallBacks;
public:

    void SetMessageCallBacks(const MessageCallBacks& MessageCallBackFunctions) override
    {
        messageCallBacks = MessageCallBackFunctions;
    }

    bool Compile(std::string_view FilePath) override
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

    bool Compile(std::istream& SourceStream) override
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
            CompileWithMSBuild();
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

    bool SetProperty(std::string_view PropertyName, std::string_view Value) override;
    std::string GetProperty(std::string_view PropertyName) override;
    void Destroy() override { delete this; }
    bool NoRecompileNeeded(std::string_view SourceToCompile, std::filesystem::path& pathDLLOutput);
    void SaveSource(std::string_view SourceToCompile, std::filesystem::path& pathSourceOutput);
};

