#include "CompilerBase.h"
#include "../../DFW2/dfw2exception.h"
#include "../../DFW2/Messages.h"

bool CompilerBase::SetProperty(std::string_view PropertyName, std::string_view Value)
{
	if (auto it = Properties.find(PropertyName); it != Properties.end())
		it->second = Value;
    return true;
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

// сохраняет исходный текст в файл
void CompilerBase::SaveSource(std::string_view SourceToCompile, std::filesystem::path& pathSourceOutput)
{
	// создаем каталог для вывода исходного текста
    std::filesystem::path OutputPath(pathSourceOutput);
	OutputPath.remove_filename();
	std::filesystem::create_directories(OutputPath);
	// UTF-8 filename ?
	std::ofstream OutputStream(pathSourceOutput);
	if (!OutputStream.is_open())
        throw dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszUserModelCannotSaveFile, utf8_encode(OutputPath.c_str())));
    // выводим исходный текст в файл
	OutputStream << SourceToCompile;
	OutputStream.close();
}

// возвращает true, если повторная рекомпиляция модуля таргета не требуется
bool CompilerBase::NoRecompileNeeded(std::string_view SourceToCompile, std::filesystem::path& pathDLLOutput)
{
	bool bRes(false);
    // проверяем режим ребилда, если он задан - игнорируем 
    // исходный текст в модуле и требуем рекомпиляции
	if (!GetProperty(PropertyMap::szPropRebuild).empty())
		return bRes;

	// достаем исходный текст из модуля
    if (auto source(GetSource(pathDLLOutput)); source.has_value())
	{
        // если исходный текст в модуле есть, разжимаем его из base64 и потом из gzip
		std::string Gzip;
		CryptoPP::StringSource d64(source.value(), true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(Gzip)));
		source.value().clear();
		CryptoPP::StringSource gzip(Gzip, true, new CryptoPP::Gunzip(new CryptoPP::StringSink(source.value()), 1));
        // сравниваем компилируемый исходный текст с тем, который извлечен из модуля, 
        // если они совпадают - отказываемся от рекомпиляции
		if (source.value() == SourceToCompile)
			bRes = true;
	}

    static const char* m_cszUserModelAlreadyCompiled;
    static const char* m_cszUserModelShouldBeCompiled;

    // формируем текст сообщения о результате проверки рекомпиляции
	if (bRes)
		pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelAlreadyCompiled, utf8_encode(pathDLLOutput.c_str())));
	else
		pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelShouldBeCompiled, utf8_encode(pathDLLOutput.c_str())));

	return bRes;
}


bool CompilerBase::Compile(std::filesystem::path FilePath)
{
    std::ifstream strm;
    strm.open(FilePath);

    if (!strm.is_open())
    {
        const auto GLE{ dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszUserModelFailedToOpenSource, stringutils::utf8_encode(FilePath.c_str()))) };
        pTree->Message(GLE.what());
        return false;
    }

    return Compile(strm);
}

bool CompilerBase::Compile(std::istream& SourceStream)
{
    bool bRes{ false };
    try
    {
        std::filesystem::path pathSourceOutput;
        // получаем стрим в строку
        std::string Source((std::istreambuf_iterator<char>(SourceStream)), std::istreambuf_iterator<char>());
        // создаем свойства AST-дерева
        pTree = std::make_unique<CASTTreeBase>(Properties);
        // устанавливаем обработчики ошибок парсера
        pTree->SetMessageCallBacks(messageCallBacks);
        // к пути из свойств компилятора
        compiledModulePath = pTree->GetPropertyPath(PropertyMap::szPropDllLibraryPath);
        // добавляем имя проекта
        compiledModulePath.append(Properties[PropertyMap::szPropProjectName]);

        // формируем расширение таргета в зависимости от платформы
#ifdef _MSC_VER
        compiledModulePath.replace_extension(".dll");
#else
        compiledModulePath.replace_extension(".so");
#endif

        // формируем путь для сохранения исходного текста из стрима
        pathSourceOutput = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
        pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);
        pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);

        // сохраняем исходный текст из стрима
        SaveSource(Source, pathSourceOutput);

        // проверяем, нужна повторная компиляция модуля таргета
        if (NoRecompileNeeded(Source, compiledModulePath))
            return true;

        // создаем дерево правил
        auto pRuleTree = std::make_unique<CASTTreeBase>(Properties);
        // выполняем лексер и парсер
        antlr4::ANTLRInputStream input(Source);
        EquationsLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        EquationsParser parser(&tokens);
        auto errorListener = std::make_unique<AntlrASTErrorListener>(pTree.get(), input.toString());
        // ставим обработчики ошибок в лексер и парсер
        parser.removeErrorListeners();
        parser.addErrorListener(errorListener.get());
        lexer.removeErrorListeners();
        lexer.addErrorListener(errorListener.get());
        antlr4::tree::ParseTree* tree = parser.input();
        // создаем правила преобразований AST-дерева
        pRuleTree->CreateRules();
        // обходим дерево в режиме visitor
        AntlrASTVisitor visitor(pTree.get());
        visitor.visit(tree);
        // выполняем проверку после обработки visitor
        pTree->PostParseCheck();
        if (!pTree->ErrorCount())
        {
            // если не было ошибок, делаем flatten/collect 
            // с промежуточными сортировками

            pTree->PrintInfix();
            pTree->Flatten();
            pTree->Sort();
            pTree->Collect();
            pTree->Sort();
            // применяем правила
            //pTree->Match(pRuleTree->GetRule());
            pTree->PrintInfix();
            // создаем уравнения
            pTree->GenerateEquations();
            pTree->PrintInfix();
        }
        parser.removeErrorListeners();
        lexer.removeErrorListeners();
        // генерируем код на C++
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
            std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive,
            ec);
        if (ec)
        {
            throw dfw2errorGLE(fmt::format(DFW2::CDFW2Messages::m_cszCouldNotCopyUserModelReference,
                pathRefDir.string(),
                pathOutDir.string()));
        }
        // построить модуль с помощью выбранного компилятора
        BuildWithCompiler();

        if (pTree->ErrorCount() == 0)
        {
            pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelCompiled, stringutils::utf8_encode(compiledModulePath.c_str())));
            bRes = true;
        }
        else
            pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelFailedToCompile, stringutils::utf8_encode(compiledModulePath.c_str())));
    }
    catch (const dfw2error& err)
    {
        pTree->Message(err.what());
        bRes = false;
    }

    return bRes;
}