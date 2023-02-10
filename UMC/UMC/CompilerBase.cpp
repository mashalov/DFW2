#include "CompilerBase.h"
#include "../../DFW2/dfw2exception.h"
#include "../../DFW2/Messages.h"
#include "../../DFW2/FmtFormatters.h"

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


void CompilerBase::RemoveCachedModules(const std::filesystem::path& path, size_t AllowedFilesCount, size_t AllowedSizeInMbytes, long CheckPeriodInSecs)
{
    // если ограничения заданы нулями, ставим масимальные ограничения
    if (AllowedFilesCount == 0)
        AllowedFilesCount = std::numeric_limits<decltype(AllowedFilesCount)>::max();
    if(AllowedSizeInMbytes == 0)
        AllowedSizeInMbytes = std::numeric_limits<decltype(AllowedSizeInMbytes)>::max();

    // если фактически ограничений нет - просто выходим
    if (AllowedFilesCount == std::numeric_limits<decltype(AllowedFilesCount)>::max() &&
        AllowedSizeInMbytes == std::numeric_limits<decltype(AllowedSizeInMbytes)>::max())
        return;

    struct FileTime
    {
        const std::filesystem::path Path_;
        std::filesystem::file_time_type Time_;
        double Size_;
    };

    auto fnFileTimeCompare = [](const FileTime& lhs, const FileTime& rhs)
    {
        return std::tie(lhs.Time_, lhs.Size_, lhs.Path_) < std::tie(rhs.Time_, rhs.Size_, rhs.Path_);
    };

    // получаем текущее время
    const auto CurrentTime{ std::chrono::system_clock::now() };
    std::chrono::seconds CurrentTimeInSeconds{ std::chrono::duration_cast<std::chrono::seconds>(CurrentTime.time_since_epoch()) };
    std::chrono::seconds LastTimeInSeconds{ 0 };

    // пытаемся прочитать файл-маркер, в котором указано время последней очистки
    const auto MarkerFile{ path / "last_clean_marker" };
    std::ifstream  MarkerIn{ MarkerFile };
    if (MarkerIn.is_open())
    {
        // если файл есть, читаем время последней очистки
        MarkerIn.read(reinterpret_cast<char*>(&LastTimeInSeconds), sizeof(std::chrono::seconds));
        MarkerIn.close();
    }
    // если файла маркера нет - время последней очистки равно нулю
    // если с момента последней очистки прошло меньше заданного периода очистки ничего не делаем

    if (const auto SecondsLeft{ (CurrentTimeInSeconds - LastTimeInSeconds).count() - CheckPeriodInSecs }; SecondsLeft < 0)
    {
        pTree->Information(fmt::format(DFW2::CDFW2Messages::m_cszNextCacheClieanIn, stringutils::utf8_encode(path.c_str()), -SecondsLeft));
        return;
    }

    // пишем в файл маркера текущее время
    std::ofstream MarkerOut{ MarkerFile };
    if (MarkerOut.is_open())
    {
        MarkerOut.write(reinterpret_cast<const char*>(&CurrentTimeInSeconds), sizeof(std::chrono::seconds));
        MarkerOut.close();
    }

    std::set<FileTime, decltype(fnFileTimeCompare)> files{ fnFileTimeCompare };
    std::error_code ec;

    // собираем файлы из каталога в сет, сортируем по дате, размеру и пути
    double CachedFilesSize{ 0.0 };
    for (const auto& file : std::filesystem::directory_iterator(path))
        if (std::filesystem::is_regular_file(file, ec))
            // подсчитываем размер файлов в каталоге
            CachedFilesSize += files.insert({ file.path(),
                                              std::filesystem::last_write_time(file, ec), 
                                              static_cast<double>(file.file_size(ec)) / 1024 / 1024 
                                            }).first->Size_;


    // считаем количество файлов для удаления
    size_t FilesToDelete{ files.size() > AllowedFilesCount ? files.size() - AllowedFilesCount : 0 };

    // если подсчитанные количество файлов и размер меньше, чем заданные ограничения - выходим
    if (FilesToDelete == 0 && AllowedSizeInMbytes >= CachedFilesSize)
        return;

    constexpr const char* cszInf{ "∞" };

    pTree->Information(fmt::format(DFW2::CDFW2Messages::m_cszTooManyCachedModules,
        files.size(),
        CachedFilesSize,
        AllowedFilesCount == std::numeric_limits<decltype(AllowedFilesCount)>::max() ? cszInf: std::to_string(AllowedFilesCount),
        AllowedSizeInMbytes == std::numeric_limits<decltype(AllowedFilesCount)>::max() ? cszInf: std::to_string(AllowedSizeInMbytes),
        stringutils::utf8_encode(path.c_str())));

    for (const auto& file : files)
    {
        // работаем в цикле до тех пор, пока есть файлы для удаления
        // и размер файлов не снизился до ограничения
        if(FilesToDelete == 0 && CachedFilesSize < AllowedSizeInMbytes)
            break;

        // не удаляем файл маркера
        if (file.Path_ == MarkerFile)
            continue;

        // удаляем файл
        std::filesystem::remove(file.Path_, ec);
        if (!ec)
        {
            pTree->Information(fmt::format(DFW2::CDFW2Messages::m_cszRemovingExcessCachedModule,
                stringutils::utf8_encode(file.Path_.c_str())));
            // уменьшаем суммарный размер файлов на размер удаленного файла
            CachedFilesSize -= file.Size_;
            if (FilesToDelete > 0)
                FilesToDelete--;
        }
    }
}

// возвращает путь к файлу, в котором уже есть скомпилированный текст модели, определяемый по хэшу
std::filesystem::path CompilerBase::CachedModulePath(const std::string_view& SourceToCompile, const std::filesystem::path& pathDLLOutput)
{
    std::filesystem::path OutputPath(pathDLLOutput);
    const auto extension{ pathDLLOutput.extension() };
    OutputPath.remove_filename();
    // строим строку хэша от исходника
    std::string SourceHash;
    CryptoPP::SHA256 hash;
    CryptoPP::StringSource SourceHashCompute(reinterpret_cast<const CryptoPP::byte*>(SourceToCompile.data()), SourceToCompile.size(), true,
        new CryptoPP::HashFilter(hash,
            new CryptoPP::Base32Encoder(
                new CryptoPP::StringSink(SourceHash)
            )
        )
    );

    // формируем имя файла по полученному хэшу с исходным расширением
    OutputPath.append(SourceHash);
    OutputPath.replace_extension(extension);
    return OutputPath;
}

// возвращает true, если повторная рекомпиляция модуля таргета не требуется
bool CompilerBase::NoRecompileNeeded(std::string_view SourceToCompile, const std::filesystem::path& pathDLLOutput)
{
    bool bRes{ false };
    // проверяем режим ребилда, если он задан - игнорируем 
    // исходный текст в модуле и требуем рекомпиляции
	if (!GetProperty(PropertyMap::szPropRebuild).empty())
		return bRes;

    // формируем начало сообщения о проверке пользовательской модели на необходимость рекомпиляции
    auto ModuleName { fmt::format(DFW2::CDFW2Messages::m_cszCustomModelModule, utf8_encode(pathDLLOutput.c_str())) };
    // готовимся к тому, что в модуле нет метаданных
    std::string condition{ DFW2::CDFW2Messages::m_cszCustomModelHasNoMetadata };

	// достаем метаданные из модуля
    if (auto metadataoption(GetMetaData(pathDLLOutput)); metadataoption.has_value())
	{
        auto& metadata{ metadataoption.value() };
        // проверяем версию компилятора в модели
        if (CompilerBase::VersionsEquals(metadata.compilerVersion, CASTCodeGeneratorBase::compilerVersion))
        {
            // версия компилятора модели и данного компилятора совпадают
            // если исходный текст в модуле есть, разжимаем его из base64 и потом из gzip
            std::string Gzip;
            CryptoPP::StringSource d64(metadata.Source, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(Gzip)));
            metadata.Source.clear();
            CryptoPP::StringSource gzip(Gzip, true, new CryptoPP::Gunzip(new CryptoPP::StringSink(metadata.Source), false));
            // сравниваем компилируемый исходный текст с тем, который извлечен из модуля, 
            // если они совпадают - отказываемся от рекомпиляции
            if (metadata.Source == SourceToCompile)
                bRes = true;
            else
                condition = DFW2::CDFW2Messages::m_cszCustomModelChanged;   // тексты не совпадают
        }
        else
            condition = fmt::format(DFW2::CDFW2Messages::m_cszCompilerVersionMismatch,  // версии компилятора модели и данного компилятора не совпадают
                metadata.compilerVersion,
                CASTCodeGeneratorBase::compilerVersion);
	}
    
    // формируем полный текст сообщения о результате проверки рекомпиляции
	if (bRes)
		pTree->Message(fmt::format("{}. {}", ModuleName, DFW2::CDFW2Messages::m_cszUserModelAlreadyCompiled));
	else
		pTree->Message(fmt::format("{} {}. {}", ModuleName, condition, DFW2::CDFW2Messages::m_cszUserModelShouldBeCompiled));

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
        const std::filesystem::path DllLibraryPath{ pTree->GetPropertyPath(PropertyMap::szPropDllLibraryPath) };
        compiledModulePath = DllLibraryPath;
        // добавляем имя проекта
        compiledModulePath.append(Properties[PropertyMap::szPropProjectName]);
        compiledModulePath.replace_extension(CompilerBase::szModuleExtension);

        // формируем путь для сохранения исходного текста из стрима
        pathSourceOutput = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
        pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);
        pathSourceOutput.append(Properties[PropertyMap::szPropProjectName]);

        // сохраняем исходный текст из стрима
        SaveSource(Source, pathSourceOutput);

        // получаем из параметров ограничения выходного каталога по количеству и объему файлов
        size_t AllowedCachedFilesCount{ 0 }, AllowedCachedFilesSize{ 0 };

        if (const auto CacheSizeString{ GetProperty(PropertyMap::szPropCachedModulesCount) }; !CacheSizeString.empty())
            AllowedCachedFilesCount = std::stoul(CacheSizeString);

        if (const auto CacheSizeString{ GetProperty(PropertyMap::szPropCachedModulesSize) }; !CacheSizeString.empty())
            AllowedCachedFilesSize = std::stoul(CacheSizeString);
     
        // выполняем очистку каталога по заданным ограничениями
        RemoveCachedModules(DllLibraryPath, AllowedCachedFilesCount, AllowedCachedFilesSize);
         
        // получаем путь к файлу, имя которого построено по хэшу
        // исходника модели
        const auto cachedModulePath{ CachedModulePath(Source, compiledModulePath) };
        if (std::filesystem::exists(cachedModulePath))
        {
            // если файл есть, проверяем, нужна ли повторная компиляция модуля таргета
            pTree->Message(fmt::format("{}. {}", 
                stringutils::utf8_encode(cachedModulePath.c_str()),
                DFW2::CDFW2Messages::m_cszCachedUserModelFound));

            if (NoRecompileNeeded(Source, cachedModulePath))
            {
                // если исходник внутри точно такой же,
                // принимаем найденный по хэшу модуль как готовый к работе
                compiledModulePath = cachedModulePath;
                return true;
            }
        } // если по хэшу ничего не нашли, используем исходное имя модуля
        else if (NoRecompileNeeded(Source, compiledModulePath))
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
            pTree->ApplyHostBlocks();
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
                stringutils::utf8_encode(pathRefDir.c_str()),
                stringutils::utf8_encode(pathOutDir.c_str())));
        }

        // построить модуль с помощью выбранного компилятора
        BuildWithCompiler();

        std::filesystem::copy(compiledModulePath, cachedModulePath, ec);
        if(ec)
            pTree->Warning(fmt::format(DFW2::CDFW2Messages::m_cszFileCopyError, 
                stringutils::utf8_encode(compiledModulePath.c_str()),
                stringutils::utf8_encode(cachedModulePath.c_str())));

        if (pTree->ErrorCount() == 0)
        {
            pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelCompiled, stringutils::utf8_encode(compiledModulePath.c_str())));
            bRes = true;
        }
        else
            pTree->Message(fmt::format(DFW2::CDFW2Messages::m_cszUserModelFailedToCompile, stringutils::utf8_encode(compiledModulePath.c_str())));
    }
    catch (const std::exception& err) 
    {
        pTree->Message(err.what());
        bRes = false;
    }

    return bRes;
}

const DFW2::VersionInfo& CompilerBase::Version() const
{
    return CASTCodeGeneratorBase::compilerVersion;
}

