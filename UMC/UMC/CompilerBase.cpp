#include "CompilerBase.h"

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
    {
#ifdef _MSC_VER
        int ec(GetLastError());
#else
        int ec(errno);
#endif
        throw std::system_error(std::error_code(ec, std::system_category()), fmt::format("Невозможно открыть файл \"{}\"", utf8_encode(OutputPath.c_str())));
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
		pTree->Message(fmt::format("Модуль \"{}\" не нуждается в повторной компиляции", utf8_encode(pathDLLOutput.c_str())));
	else
		pTree->Message(fmt::format("Необходима компиляция модуля \"{}\"", utf8_encode(pathDLLOutput.c_str())));

	return bRes;
}


bool CompilerBase::Compile(std::filesystem::path FilePath)
{
    bool bRes(false);
    std::ifstream strm;
    strm.open(FilePath);
    if (strm.is_open())
    {
        bRes = Compile(strm);
    }
    return bRes;
}

bool CompilerBase::Compile(std::istream& SourceStream)
{
    bool bRes(false);
    std::filesystem::path pathSourceOutput;
    try
    {
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

        // берем из параметров путь для вывода
        pathOutDir = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
        // добавляем к пути для вывода имя проекта
        pathOutDir.append(Properties[PropertyMap::szPropProjectName]);
        // формируем путь до CustomDevice.h, который должен быть сгенерирован в каталоге сборки
        std::filesystem::path pathCustomDeviceHeader = pathOutDir;
        pathCustomDeviceHeader.append("CustomDevice.h");
        // проверяем, есть ли CustomDevice.h в каталоге сборки

        if (!std::filesystem::exists(pathCustomDeviceHeader))
        {
            pTree->Error(fmt::format("В каталоге \"{}\" не найден файл скомпилированного пользовательского устройства \"{}\".",
                pathOutDir.string(),
                CASTCodeGeneratorBase::CustomDeviceHeader));
            throw std::runtime_error(cszUMCFailed);
        }

        // берем путь к референсному каталогу
        pathRefDir = pTree->GetPropertyPath(PropertyMap::szPropReferenceSources);
        // проверяем есть ли он
        if (!std::filesystem::exists(pathRefDir))
        {
            pTree->Error(fmt::format("Не найден каталог исходных файлов для сборки пользовательской модели \"{}\".",
                pathRefDir.string()));

            throw std::runtime_error(cszUMCFailed);
        }
        // если каталог есть - копируем референсные не *.h файлы в каталог сборки 
        std::error_code ec;
        std::filesystem::copy(std::filesystem::path(pathRefDir).append("Source"),
            pathOutDir, 
            std::filesystem::copy_options::overwrite_existing|std::filesystem::copy_options::recursive,
            ec);
        if(ec)
        {
            pTree->Error(fmt::format("Ошибка \"{}\" копирования файлов исходных текстов из \"{}\" в \"{}\"",
                std::error_code(errno, std::system_category()).message(),
                pathRefDir.string(),
                pathOutDir.string()
                ));
            throw std::runtime_error(cszUMCFailed);
        }
        // построить модуль с помощью выбранного компилятора
        BuildWithCompiler();
        bRes = true;
    }
    catch (std::exception& ex)
    {
        pTree->Error(fmt::format("Исключение {}", ex.what()));
    }

    if (pTree->ErrorCount() == 0)
        pTree->Message(fmt::format("Выполнена компиляция модуля \"{}\"", compiledModulePath.filename().generic_string()));
    else
        pTree->Error(fmt::format("Ошибка компиляции модуля \"{}\"", compiledModulePath.filename().generic_string()));

    return pTree->ErrorCount() == 0;
}