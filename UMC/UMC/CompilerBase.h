#pragma once
#include "AntlrASTVisitor.h"
#include "ASTCodeGeneratorBase.h"
#include "ICompiler.h"
#include <optional>
#include "UniqueHandle.h"

class CompilerBase : public ICompiler
{
protected:

    std::filesystem::path compiledModulePath;
    static constexpr const char* cszUMCFailed = "Невозможна компиляция пользовательского устройства";
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
    // функция выполняет сборку модуля с помощью выбранного компилятора
    virtual void BuildWithCompiler() = 0;
    // возвращает исходный текст из модуля по заданному пути
    virtual std::optional<std::string> GetSource(const std::filesystem::path& pathDLLOutput) = 0;
    MessageCallBacks messageCallBacks;

    std::filesystem::path pathOutDir;
    std::filesystem::path pathRefDir;
public:

    void SetMessageCallBacks(const MessageCallBacks& MessageCallBackFunctions) override
    {
        messageCallBacks = MessageCallBackFunctions;
    }

    // компиляция исходного текста из файла
    void Compile(std::filesystem::path FilePath) override;
    // компиляция исходого текста из потока
    void Compile(std::istream& SourceStream) override;
    // задать свойство по имени
    void SetProperty(std::string_view PropertyName, std::string_view Value) override;
    // получить свойство по имени
    std::string GetProperty(std::string_view PropertyName) override;
    // удаление компилятора
    void Destroy() override { delete this; }
    // возвращает true, если модуль на заданном пути содержит исходник, идентичный заданному тексту
    bool NoRecompileNeeded(std::string_view SourceToCompile, std::filesystem::path& pathDLLOutput);
    // сохраняет исходный текст в файл с заданным путем
    void SaveSource(std::string_view SourceToCompile, std::filesystem::path& pathSourceOutput);
    // возвращает полный путь к файлу скомпилированного модуля
    const std::filesystem::path& CompiledModulePath() const override;

};

