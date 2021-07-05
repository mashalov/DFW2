#include "CompilerBase.h"
#include "../../DFW2/dfw2exception.h"
#include "../../DFW2/Messages.h"

void CompilerBase::SetProperty(std::string_view PropertyName, std::string_view Value)
{
	if (auto it = Properties.find(PropertyName); it != Properties.end())
		it->second = Value;
}

std::string CompilerBase::GetProperty(std::string_view PropertyName)
{
	if (auto it = Properties.find(PropertyName); it != Properties.end())
		return it->second;

	else
		return "";
}

const std::filesystem::path& CompilerBase::CompiledModulePath() const
{
    return compiledModulePath;
}

void CompilerBase::SaveSource(std::string_view SourceToCompile, std::filesystem::path& pathSourceOutput)
{
	std::filesystem::path OutputPath(pathSourceOutput);
	OutputPath.remove_filename();
	std::filesystem::create_directories(OutputPath);
	// UTF-8 filename ?
	std::ofstream OutputStream(pathSourceOutput);
	if (!OutputStream.is_open())
        throw dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszUserModelCannotSaveFile, utf8_encode(OutputPath.c_str())));

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

    static const char* m_cszUserModelAlreadyCompiled;
    static const char* m_cszUserModelShouldBeCompiled;

	if (bRes)
		pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelAlreadyCompiled, utf8_encode(pathDLLOutput.c_str())));
	else
		pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelShouldBeCompiled, utf8_encode(pathDLLOutput.c_str())));

	return bRes;
}


void CompilerBase::Compile(std::filesystem::path FilePath)
{
    std::ifstream strm;
    strm.open(FilePath);

    if (!strm.is_open())
        throw dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszUserModelFailedToOpenSource, stringutils::utf8_encode(FilePath.c_str())));

    Compile(strm);
}

void CompilerBase::Compile(std::istream& SourceStream)
{
    std::filesystem::path pathSourceOutput;
    std::string Source((std::istreambuf_iterator<char>(SourceStream)), std::istreambuf_iterator<char>());
    pTree = std::make_unique<CASTTreeBase>(Properties);
    pTree->SetMessageCallBacks(messageCallBacks);
    compiledModulePath = pTree->GetPropertyPath(PropertyMap::szPropDllLibraryPath);
    compiledModulePath.append(Properties[PropertyMap::szPropProjectName]);

#ifdef _MSC_VER
    compiledModulePath.replace_extension(".dll");
#else
    compiledModulePath.replace_extension(".so");
#endif

    pathSourceOutput = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
    pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);
    pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);

    SaveSource(Source, pathSourceOutput);


    if (NoRecompileNeeded(Source, compiledModulePath))
        return;

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

    // берем из параметров путь для вывода
    pathOutDir = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
    // добавляем к пути для вывода имя проекта
    pathOutDir.append(Properties[PropertyMap::szPropProjectName]);
    // формируем путь до CustomDevice.h, который должен быть сгенерирован в каталоге сборки
    std::filesystem::path pathCustomDeviceHeader = pathOutDir;
    pathCustomDeviceHeader.append("CustomDevice.h");
    // проверяем, есть ли CustomDevice.h в каталоге сборки

    if (!std::filesystem::exists(pathCustomDeviceHeader))
        throw dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszNoUserModelInFolder, pathOutDir.string(), CASTCodeGeneratorBase::CustomDeviceHeader));

    // берем путь к референсному каталогу
    pathRefDir = pTree->GetPropertyPath(PropertyMap::szPropReferenceSources);
    // проверяем есть ли он
    if (!std::filesystem::exists(pathRefDir))
        throw dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszNoUserModelReferenceFolder, pathRefDir.string()));

    // если каталог есть - копируем референсные не *.h файлы в каталог сборки 
    std::error_code ec;
    std::filesystem::copy(std::filesystem::path(pathRefDir).append("Source"),
        pathOutDir, 
        std::filesystem::copy_options::overwrite_existing|std::filesystem::copy_options::recursive,
        ec);
    if(ec)
    {
        throw dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszCouldNotCopyUserModelReference,
            pathRefDir.string(),
            pathOutDir.string()));
    }
    // построить модуль с помощью выбранного компилятора
    BuildWithCompiler();

    if (pTree->ErrorCount() == 0)
        pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelCompiled, stringutils::utf8_encode(compiledModulePath.c_str())));
    else
        throw dfw2error(fmt::format(DFW2::CDFW2Messages::m_cszUserModelFailedToCompile, stringutils::utf8_encode(compiledModulePath.c_str())));
}