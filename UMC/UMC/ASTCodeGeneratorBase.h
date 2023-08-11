#pragma once
#include<iostream>
#include<fstream>
#include<filesystem>
#include "ASTNodes.h"
#include "../CryptoPP/gzip.h"
#include "../CryptoPP/base64.h"
#include "../CryptoPP/base32.h"
#include "../CryptoPP/sha.h"
#include "../CryptoPP/filters.h"
#define NOMINMAX
#ifdef _MSC_VER
#include "windows.h"
#endif
#include "../../DFW2/PackageVersion.h"

using StringViewList = std::list<std::string_view>;

class CASTCodeGeneratorBase;
class BlockEmit
{
	CASTCodeGeneratorBase& CodeGenerator_;
public:
	BlockEmit(CASTCodeGeneratorBase& CodeGenerator);
	virtual ~BlockEmit();
};



class CASTCodeGeneratorBase
{
protected:
	CASTTreeBase* pTree;
	std::ofstream OutputStream;
	size_t DefaultIndent = 15;
	size_t MaxLineLentgh = 80;
	std::string SourceText;
	std::list<CASTHostBlockBase*> HostBlocksList;
public:
	size_t Indent = 0;
	CASTCodeGeneratorBase(CASTTreeBase* pASTTree, 
						  std::string_view Source) : pTree(pASTTree), 
												     SourceText(Source)
	{

	}

	void GenerateGetDeviceProperties()
	{
		// объявление функции
		EmitLine("void GetDeviceProperties(CDeviceContainerPropertiesBase & DeviceProps) override");
		BlockEmit block(*this);

		// тип устройства
		EmitLine(fmt::format("DeviceProps.SetType({});", pTree->GetProperties().at(PropertyMap::szPropDeviceType)));
		// вербальное имя класса устройства
		EmitLine(fmt::format("DeviceProps.SetClassName(\"{}\", \"{}\");",
			pTree->GetProperties().at(PropertyMap::szPropDeviceTypeNameVerbal),
			pTree->GetProperties().at(PropertyMap::szPropDeviceTypeNameSystem)));

		// описание связей и их режимов
		//EmitLine(fmt::format("DeviceProps.AddLinkFrom(DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER);",
		EmitLine(fmt::format("DeviceProps.AddLinkFrom({}, {}, {});",
			pTree->GetProperties().at(PropertyMap::szPropLinkDeviceType),
			pTree->GetProperties().at(PropertyMap::szPropLinkDeviceMode),
			pTree->GetProperties().at(PropertyMap::szPropLinkDeviceDependency)));

		size_t nConstVarIndex(0), nExtVarIndex(0), nStateVarIndex(0);
		std::list<std::string> stateVars, extVars, constVars;

		// обходим переменные AST-дерева
		for (auto& v : pTree->GetVariables())
		{
			// если ссылок на переменную с данными именем нет, то переменную не вводим в список
			// (она, видимо, была удалена в процессе манипуляций)
			// также не выводим переменные помеченные Internal. Они используются для инициализации
			if (v.second.InitSection) continue;

			if (v.second.NamedConstant)
			{
				if(v.second.VarInstances.size() == 0)
					pTree->Warning(fmt::format("Переменная \"{}\" объявлена как константа, но не используется в модели", v.first));
				else
				{
					_ASSERTE(v.second.IsNamedConstant());
					// добавляем константу в список констант
					constVars.push_back(fmt::format("{{ \"{}\", {{ {}, eDVT_CONSTSOURCE }} }}", v.first, nConstVarIndex));
				}
				nConstVarIndex++;
			}
			else if (v.second.External)
			{
				if (v.second.VarInstances.size() == 0)
					pTree->Warning(fmt::format("Переменная \"{}\" = \"{}\" объявлена как внешняя, но не используется в модели", 
						v.first,
						v.second.ModelLink));
				else
				{
					_ASSERTE(v.second.IsExternal());
					// добавляем внешнюю перменную в список внешних переменных
					extVars.push_back(fmt::format("{{ \"{}\", {{ {}, DEVTYPE_MODEL }} /* {} */}}", 
						v.second.ModelLink,		// символическая ссылка
						nExtVarIndex,			// индекс внешней переменной в m_ExternalVariables
						v.first));				// комментарий - имя переменной в исходном тексте
				}
				nExtVarIndex++;
			}
			else
			{
				if (v.second.IsMainStateVariable())
					// добавляем переменную состояния в список переменных состояния
					stateVars.push_back(fmt::format("{{ \"{}\", {{ {}, VARUNIT_NOTSET }} }}", v.first, nStateVarIndex++));
			}
		}

		// генерируем векторы переменных состояния, внешних и констант
		GenerateVariables("DeviceProps.VarMap_      = { ", stateVars, " };", true);
		GenerateVariables("DeviceProps.ExtVarMap_   = { ", extVars, " };", true);
		GenerateVariables("DeviceProps.ConstVarMap_ = { ", constVars, " };", true);
		// формируем количество уравнений
		EmitLine("DeviceProps.EquationsCount = DeviceProps.VarMap_.size();");
	}

	void GenerateHostBlocksParameters()
	{
		EmitLine("const DOUBLEVECTOR& GetBlockParameters(ptrdiff_t BlockIndex) override");
		BlockEmit block(*this);
		EmitLine("BlockParameters_.clear();");
		if (!HostBlocksList.empty())
		{
			EmitLine("switch(BlockIndex)");
			BlockEmit block(*this);
			for (auto& h : HostBlocksList)
			{
				std::list<std::string> Consts;
				for (auto& cst : h->ConstantArguments)
				{
					if (cst.first->CheckType(ASTNodeType::Equation) && cst.first->ChildNodes().size() == 2)
						Consts.push_back(ctrim(cst.first->ChildNodes().back()->GetInfix()));
					else
						EXCEPTIONMSG("Wrong constant argument type");
				}

				EmitLine(fmt::format("case {}:", h->GetHostBlockIndex()));
				Indent++;
				EmitLine(fmt::format("BlockParameters_ = DOUBLEVECTOR{{ {} }};", fmt::join(Consts, ", ")));
				Indent--;
				EmitLine("break;");
			}
		}
		EmitLine("return BlockParameters_;");
	}

	void GenerateVersions()
	{
		EmitLine("static constexpr VersionInfo modelVersion = { { 1, 0, 0, 0 } };");
		EmitLine(fmt::format("static constexpr VersionInfo compilerVersion = {{ {} }};", fmt::join(compilerVersion, ", ")));
	}

	void GenerateEncodedSource()
	{
		std::string Gzipped, Base64Encoded;
		if (!pTree->GetProperties().at(PropertyMap::szPropExportSource).empty())
		{
			CryptoPP::StringSource ss(SourceText, true, new CryptoPP::Gzip(new CryptoPP::StringSink(Gzipped)));
			CryptoPP::StringSource b64(Gzipped, true, new CryptoPP::Base64Encoder(new CryptoPP::StringSink(Base64Encoded), false));
		}
		std::string Line("static constexpr const char* zipSource = ");
		size_t counter = Base64Encoded.size();
		size_t varsinline = 0;
		size_t indent = Line.length();
		while (1)
		{
			size_t Available = std::min(Base64Encoded.size(), MaxLineLentgh);
			Line += '\"' + Base64Encoded.substr(0, Available) + '\"';
			Base64Encoded.erase(0, Available);
			if (Base64Encoded.empty())
			{
				Line += ';';
				EmitLine(Line);
				break;
			}
			EmitLine(Line);
			Line = std::string(indent, ' ');
		}
	}

	// вывесьт строку исходного текста C++ с текущим отступом
	void EmitLine(std::string_view Line)
	{		
		OutputStream << std::string(Indent * 4, ' ') << Line << std::endl;
	}

	void GenerateHeader()
	{
		// обертка для класса
		EmitLine("#pragma once");
		EmitLine("#include\"DFW2/ICustomDevice.h\"");
		EmitLine("#include\"DFW2/version.h\"");
		EmitLine("#include <math.h>");
		EmitLine("namespace DFW2");
		BlockEmit block(*this);
		// генерируем класс
		GenerateClass();
	}

	// генерирут строку вектора ссылок на переменные
	template<typename T>
	void GenerateVariables(std::string_view Pre, T& VarList, std::string_view Post, bool UsePrePost = false)
	{
		std::string Line(Pre);	// результирующая строка начинается с пролога
		size_t counter = VarList.size();
		size_t varsinline = 0;
		size_t indent = Line.length();

		// проходим по списку переменных
		for (auto& var : VarList)
		{
			// добавляем переменную к строке
			Line += var;
			// считаем количество переменных в строке
			varsinline++;
			// если переменная не последняя, вводим разделитель ","
			if(counter > 1 && counter <= VarList.size()) Line += ", ";

			// контролируем длину строки исходника, если 
			// превышено ограничение - переносим на следующую
			// строку
			if (Line.length() > MaxLineLentgh)
			{
				// отмечаем, что было разбиение строки
				UsePrePost = false;
				// если вывели все переменные - добавляем эпилог
				if (counter == 1)
					Line += Post;	
				// выводим строку
				EmitLine(Line);
				// следующая строка начинается с отступа, 
				// размер которого равен прологу
				Line = std::string(indent, ' ');
				// все переменные их текущей строки выведены
				varsinline = 0;
			}
			counter--;
		}

		// если остались еще не обработанные 
		// переменные в строке или форсирован эпилог -
		// добавляем строку в исходник
		if (varsinline > 0 || UsePrePost)
		{
			Line += Post;
			EmitLine(Line);
		}
	}

	void GenerateStateVariables()
	{
		StringViewList Vars;
		for (auto& v : pTree->GetVariables())
			if (v.second.IsMainStateVariable() && !v.second.External)
				Vars.push_back(v.first);

		GenerateVariables("VariableIndex ", Vars, ";");
		GenerateVariables("VariableIndexRefVec StateVariables_ = { ", Vars, " };", true);
	}

	void GenerateConstVariables()
	{
		StringViewList Vars;
		// в список констант модели
		// включаем все константы
		for (auto& v : pTree->GetVariables())
			if (v.second.IsConstant())
				Vars.push_back(v.first);

		GenerateVariables("double ", Vars, ";");

		// список именованных констант доступных
		// снаружи может отличаться от списка всех констант

		Vars.clear();
		for (auto& v : pTree->GetVariables())
			if (v.second.IsNamedConstant())
				Vars.push_back(v.first);

		GenerateVariables("std::vector<std::reference_wrapper<double>> ConstantVariables_ = { ", Vars, " };", true);
	}

	void GenerateExternalVariables()
	{
		StringViewList Vars;
		for (auto& v : pTree->GetVariables())
			if (v.second.IsExternal())
				Vars.push_back(v.first);

		GenerateVariables("VariableIndexExternal ", Vars,";");
		GenerateVariables("VariableIndexExternalRefVec ExternalVariables_ = { ", Vars, " };", true);
	}

	void GenerateVariablesDeclaration()
	{
		GenerateStateVariables();
		GenerateConstVariables();
		GenerateExternalVariables();
	}

	void GenerateHostBlocks()
	{
		std::string Line = "PRIMITIVEVECTOR Primitives_ = { ";
		auto& HostBlocks = pTree->GetHostBlocks();
		size_t indent = Line.length();

		// выбираем хост-блоки из main-секции в список, чтобы можно было отсортировать по индексам
		for (auto& h : HostBlocks)
		{
			// если хост-блок в секции Init - пропускаем
			if (h->GetParentEquation()->GetParentOfType(ASTNodeType::Init)) continue;
			HostBlocksList.push_back(h);
		}
		size_t counter = HostBlocksList.size();
		if (counter == 0)
			EmitLine(Line + " };");


		HostBlocksList.sort([&counter](const CASTHostBlockBase* lhs, const CASTHostBlockBase* rhs)
			{
				return lhs->GetHostBlockIndex() < rhs->GetHostBlockIndex();
			});

		for (auto& h : HostBlocksList)
		{

			// проверяем идут ли индексы последовательно от нуля
			if(h->GetHostBlockIndex() != HostBlocksList.size() - counter)
				EXCEPTIONMSG("Wrong host-blocks indexes");

			std::list<std::string> inputs, outputs;
			ptrdiff_t pin(0);

			for (auto& o : h->Outputs)
				outputs.push_back(fmt::format("{{ {}, {} }}", pin++, o->first));

			// если у примитива есть заданный идентификатор
			// ввводим его, если нет - используем имя выхода, если задано

			std::string PrimitiveDescription{ h->GetId() };
			if (PrimitiveDescription.empty() && !h->Outputs.empty())
				PrimitiveDescription = h->Outputs.front()->first;

			pin = 0;
			for (auto& i : h->ChildNodes())
			{
				if (CASTNodeBase::IsVariable(i))
					inputs.push_back(fmt::format("{{ {}, {} }}", pin++, i->GetText()));
				pin++;
			}
			 
			Line += fmt::format("{{ {}, \"{}\",  {{ {} }},  {{ {} }} }}",
				h->GetHostBlockInfo().PrimitiveCompilerName,
				PrimitiveDescription,
				fmt::join(outputs, ", "),
				fmt::join(inputs, ", "));

			if (counter > 1 && counter <= HostBlocks.size()) Line += ", ";
			counter--;
			if (counter == 0)
				Line += " };";
			EmitLine(Line);
			Line = std::string(indent, ' ');
		}

		EmitLine("DOUBLEVECTOR BlockParameters_;");
	}

	void GenerateGetVariablesAndPrimitives()
	{
		// переменные состояния
		EmitLine("const VariableIndexRefVec& GetVariables() override { return StateVariables_;}");
		// внешние переменные
		EmitLine("const VariableIndexExternalRefVec& GetExternalVariables() override { return ExternalVariables_;}");
		// примитивы (хост-блоки)
		EmitLine("const PRIMITIVEVECTOR& GetPrimitives() override { return Primitives_;}");
	}

	void GenerateSetSourceConstant()
	{
		EmitLine("bool SetSourceConstant(size_t Index, double Value) override");
		BlockEmit blockfunction(*this);
		EmitLine("if( Index < ConstantVariables_.size() )");
		{
			BlockEmit blockif(*this);
			EmitLine("ConstantVariables_[Index] = Value;");
			EmitLine("return true;");
		}
		EmitLine("else return false;");
	}

	void GenerateBuildRightHand()
	{
		EmitLine("void BuildRightHand(CCustomDeviceData& CustomDeviceData) override");
		BlockEmit block(*this);

		CASTEquationSystem* pMain = pTree->GetEquationSystem(ASTNodeType::Main);
		for (auto& eq : pMain->ChildNodes())
		{
			if (!eq->CheckType(ASTNodeType::Equation)) continue;
			CASTEquation* pEquation(static_cast<CASTEquation*>(eq));
			if (pEquation->pHostBlock == nullptr)
			{
				EmitLine(fmt::format("CustomDeviceData.SetFunction({}, {});",
					pEquation->itResolvedBy->first,
					ctrim(pEquation->ChildNodes().front()->GetInfix())));
			}
			else
				EmitLine(fmt::format("// {} is for host block ", ctrim(pEquation->ChildNodes().front()->GetInfix())));

		}
	}

	void GenerateInit()
	{
		EmitLine("eDEVICEFUNCTIONSTATUS Init(CCustomDeviceData& CustomDeviceData) override");
		BlockEmit block(*this);

		// в секции Init есть основная система и система присваивания базовых значений
		// внешних переменных. Основная система сортируется в порядке исполнения. Система
		// присваивания не требует сортировки, но должна располагаться до основной системы

		// вывод системы присваивания базовых значений
		CASTEquationSystem* pInitExternalBases = pTree->GetEquationSystem(ASTNodeType::InitExtVars);
		if (pInitExternalBases)
		{
			for (auto& eq : pInitExternalBases->ChildNodes())
			{
				if (!eq->CheckType(ASTNodeType::Equation)) continue;
				eq->CheckChildCount();
				if (!CASTNodeBase::IsVariable(eq->ChildNodes().back()))
					EXCEPTIONMSG("Right child should be variable");

				CASTVariable* pExtVar = static_cast<CASTVariable*>(eq->ChildNodes().back());
				VariableInfo& varInfo(pExtVar->Info());
				if (!varInfo.External)
					EXCEPTIONMSG("Right child should be external variable");

				EmitLine(fmt::format("{}; // {} ", ctrim(eq->GetInfix()), varInfo.ModelLink));
			}
		}

		// вывод основной системы
		CASTEquationSystem* pInit = pTree->GetEquationSystem(ASTNodeType::Init);
		for (auto& eq : pInit->ChildNodes())
		{
			if (!eq->CheckType(ASTNodeType::Equation)) continue;
			CASTEquation* pEquation(static_cast<CASTEquation*>(eq));
			if (pEquation->pHostBlock == nullptr)
			{
				EmitLine(fmt::format("{};", ctrim(pEquation->GetInfix())));
			}
			else
				EmitLine(fmt::format("CustomDeviceData.InitPrimitive({}); // {}",
					pEquation->pHostBlock->GetHostBlockIndex(),
					ctrim(pEquation->GetInfix())));
		}
		EmitLine("return eDEVICEFUNCTIONSTATUS::DFS_OK;");
	}

	void GenerateProcessDiscontinuity()
	{
		EmitLine("eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData) override");
		BlockEmit block(*this);

		CASTEquationSystem* pInit = pTree->GetEquationSystem(ASTNodeType::Init);
		for (auto& eq : pInit->ChildNodes())
		{
			if (!eq->CheckType(ASTNodeType::Equation)) continue;
			CASTEquation* pEquation(static_cast<CASTEquation*>(eq));
			if (pEquation->pHostBlock == nullptr)
			{
				EmitLine(fmt::format("{};", ctrim(pEquation->GetInfix())));
			}
			else
				EmitLine(fmt::format("CustomDeviceData.ProcessPrimitiveDisco({}); // {}", 
					pEquation->pHostBlock->GetHostBlockIndex(),
					ctrim(pEquation->GetInfix())));
		}
		EmitLine("return eDEVICEFUNCTIONSTATUS::DFS_OK;");
	}

	void GenerateBuildEquations()
	{
		// функция построения матрицы Якоби
		EmitLine("void BuildEquations(CCustomDeviceData& CustomDeviceData) override");
		BlockEmit block(*this);

		std::map<std::string, ASTNodeList> ExternalVarsDerivatives;

		auto fnEmitDerivative = [this](const CASTJacobiElement* const pJe) -> void
		{
			EmitLine(fmt::format("CustomDeviceData.SetElement({}, {}, {});   // {} by {}",
				pJe->pequation->itResolvedBy->first,			// строка элемента якобиана
				pJe->var->first,								// столбец элемента якобина
				ctrim(pJe->ChildNodes().front()->GetInfix()),	// функция вычисления элемента якобиана
				ctrim(pJe->pequation->GetInfix()),				// комментарий: уравнение, от которого берется частная производная
				pJe->var->first));								// комментарие: переменная, по которой берется частная производная
		};

		// проходим по якобиану дерева
		for (const auto& v : pTree->GetJacobian()->ChildNodes())
		{
			// для каждого элемента якобиана
			const CASTJacobiElement* pJe{ static_cast<CASTJacobiElement*>(v) };

			if (pJe->var->second.External)
				// внешняя переменная, которая может быть константой,
				// ограждаем условием по наличию индекса
				ExternalVarsDerivatives[pJe->var->first].emplace_back(v);
			else
				// обычная переменная - добавляем SetElement
				(fnEmitDerivative)(pJe);
		}

		for (const auto& v : ExternalVarsDerivatives)
		{
			// вставляем проверку является ли переменная индексированной
			// или это константа с фиктивным вектором Нордсика
			EmitLine(fmt::format("if(CustomDeviceData.IndexedVariable({}))", v.first));
			std::unique_ptr<BlockEmit> block;
			if (v.second.size() > 1)
				block = std::make_unique<BlockEmit>(*this);
			else
				Indent++;

			for (const auto& pv : v.second)
			{
				const CASTJacobiElement* pJe{ static_cast<CASTJacobiElement*>(pv) };
				(fnEmitDerivative)(pJe);
			}

			if (v.second.size() <= 1)
				Indent--;
		}
	}

	void GenerateDestroy()
	{
		EmitLine("void Destroy() override { delete this; }");
	}

	void GenerateBuildDerivatives()
	{
		EmitLine("void BuildDerivatives(CCustomDeviceData& CustomDeviceData) override");
		BlockEmit block(*this);
	}

	void GenerateSetConstsDefaultValues()
	{
		EmitLine("void SetConstsDefaultValues() override");
		BlockEmit block(*this);
	}

	void GenerateDLLFunctions()
	{
		EmitLine("extern \"C\" __declspec(dllexport) ICustomDevice * __cdecl CustomDeviceFactory()");
		BlockEmit block(*this);
		EmitLine("return new CCustomDevice();");
	}

	void GenerateClass()
	{
		// объявление класса
		EmitLine("class CCustomDevice : public ICustomDevice");
		EmitLine("{");
		// отступ вправо
		Indent++;
		// переменные и хост-блоки
		EmitLine("protected:");
		GenerateVariablesDeclaration();
		GenerateHostBlocks();
		EmitLine("public:");
		// функции класса
		GenerateGetVariablesAndPrimitives();
		GenerateGetDeviceProperties();
		GenerateSetSourceConstant();
		GenerateSetConstsDefaultValues();
		GenerateHostBlocksParameters();
		GenerateBuildRightHand();
		GenerateBuildDerivatives();
		GenerateBuildEquations();
		GenerateInit();
		GenerateProcessDiscontinuity();
		GenerateDestroy();
		GenerateEncodedSource();
		GenerateVersions();
		// возврат отступа
		Indent--;
		EmitLine("};");
	}

	void Generate()
	{
		// если в дереве есть ошибки - отказываемся генерировать код
		if (pTree->ErrorCount() != 0) return;
		// формируем имя файла с исходником на c++
		std::filesystem::path OutputPath = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
		OutputPath.append(pTree->GetProperties().at(PropertyMap::szPropProjectName));
		// создаем выходной каталог
		std::filesystem::create_directories(OutputPath);
		std::filesystem::path HeaderPath = OutputPath;
		HeaderPath.append(CustomDeviceHeader);
		OutputStream.open(HeaderPath);
		if (!OutputStream.is_open())
		{
#ifdef _MSC_VER		
			const int ec(GetLastError());
#else
			const int ec(errno);
#endif 	
			throw std::system_error(std::error_code(ec,std::system_category()), fmt::format("Невозможно открыть файл \"{}\"", OutputPath.string()));
		}		
		// генерируем текст на C++ в виде 
		// единственного h-файла
		GenerateHeader();
		OutputStream.close();
		pTree->PrintErrorsWarnings();
	}

	static inline const std::string CustomDeviceHeader = "CustomDevice.h";
	static constexpr DFW2::VersionInfo compilerVersion = PackageVersion;
};


