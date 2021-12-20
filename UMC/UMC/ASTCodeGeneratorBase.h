#pragma once
#include<iostream>
#include<fstream>
#include<filesystem>
#include "ASTNodes.h"
#include "../CryptoPP/gzip.h"
#include "../CryptoPP/base64.h"
#define NOMINMAX
#ifdef _MSC_VER
#include "windows.h"
#endif

using StringViewList = std::list<std::string_view>;

class CASTCodeGeneratorBase
{
protected:
	CASTTreeBase* pTree;
	std::ofstream OutputStream;
	size_t Indent = 0;
	size_t DefaultIndent = 15;
	size_t MaxLineLentgh = 80;
	std::string SourceText;
	std::list<CASTHostBlockBase*> HostBlocksList;
public:
	CASTCodeGeneratorBase(CASTTreeBase* pASTTree, 
						  std::string_view Source) : pTree(pASTTree), 
												     SourceText(Source)
	{

	}

	void GenerateGetDeviceProperties()
	{
		// объявление функции
		EmitLine("void GetDeviceProperties(CDeviceContainerPropertiesBase & DeviceProps) override");
		EmitLine("{");
		Indent++;

		// тип устройства
		EmitLine("DeviceProps.SetType(DEVTYPE_AUTOMATIC);");
		// вербальное имя класса устройства
		EmitLine("DeviceProps.SetClassName(\"Automatic & scenario\", \"Automatic\");");
		// описание связей и их режимов
		EmitLine("DeviceProps.AddLinkFrom(DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER);");

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
		GenerateVariables("DeviceProps.m_VarMap      = { ", stateVars, " };", true);
		GenerateVariables("DeviceProps.m_ExtVarMap   = { ", extVars, " };", true);
		GenerateVariables("DeviceProps.m_ConstVarMap = { ", constVars, " };", true);
		// формируем количество уравнений
		EmitLine("DeviceProps.nEquationsCount = DeviceProps.m_VarMap.size();");
		Indent--;
		EmitLine("}");
	}

	void GenerateHostBlocksParameters()
	{
		EmitLine("const DOUBLEVECTOR& GetBlockParameters(ptrdiff_t nBlockIndex) override");
		EmitLine("{");
		Indent++;
		EmitLine("m_BlockParameters.clear();");
		if (!HostBlocksList.empty())
		{
			EmitLine("switch(nBlockIndex)");
			EmitLine("{");
			Indent++;
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
				EmitLine(fmt::format("m_BlockParameters = DOUBLEVECTOR{{ {} }};", fmt::join(Consts, ", ")));
				Indent--;
				EmitLine("break;");
			}
			Indent--;
			EmitLine("}");
		}
		EmitLine("return m_BlockParameters;");
		Indent--;
		EmitLine("}");
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
		EmitLine("#include <math.h>");
		EmitLine("namespace DFW2\n{");
		// отступ вправо
		Indent++;
		// генерируем класс
		GenerateClass();

		// возврат отступа
		Indent--;
		EmitLine("}");
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
		GenerateVariables("VariableIndexRefVec m_StateVariables = { ", Vars, " };", true);
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

		GenerateVariables("std::vector<std::reference_wrapper<double>> m_ConstantVariables = { ", Vars, " };", true);
	}

	void GenerateExternalVariables()
	{
		StringViewList Vars;
		for (auto& v : pTree->GetVariables())
			if (v.second.IsExternal())
				Vars.push_back(v.first);

		GenerateVariables("VariableIndexExternal ", Vars,";");
		GenerateVariables("VariableIndexExternalRefVec m_ExternalVariables = { ", Vars, " };", true);
	}

	void GenerateVariablesDeclaration()
	{
		GenerateStateVariables();
		GenerateConstVariables();
		GenerateExternalVariables();
	}

	void GenerateHostBlocks()
	{
		std::string Line = "PRIMITIVEVECTOR m_Primitives = { ";
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

			pin = 0;
			for (auto& i : h->ChildNodes())
			{
				if (CASTNodeBase::IsVariable(i))
					inputs.push_back(fmt::format("{{ {}, {} }}", pin++, i->GetText()));
				pin++;
			}
			 
			Line += fmt::format("{{ {}, \"{}\",  {{ {} }},  {{ {} }} }}",
				h->GetHostBlockInfo().PrimitiveCompilerName,
				h->GetId(),
				fmt::join(outputs, ", "),
				fmt::join(inputs, ", "));

			if (counter > 1 && counter <= HostBlocks.size()) Line += ", ";
			counter--;
			if (counter == 0)
				Line += " };";
			EmitLine(Line);
			Line = std::string(indent, ' ');
		}

		EmitLine("DOUBLEVECTOR m_BlockParameters;");
	}

	void GenerateGetVariablesAndPrimitives()
	{
		// переменные состояния
		EmitLine("const VariableIndexRefVec& GetVariables() override { return m_StateVariables;}");
		// внешние переменные
		EmitLine("const VariableIndexExternalRefVec& GetExternalVariables() override { return m_ExternalVariables;}");
		// примитивы (хост-блоки)
		EmitLine("const PRIMITIVEVECTOR& GetPrimitives() override { return m_Primitives;}");
	}

	void GenerateSetSourceConstant()
	{
		EmitLine("bool SetSourceConstant(size_t Index, double Value) override");
		EmitLine("{");
		Indent++;
		EmitLine("if( Index < m_ConstantVariables.size() )");
		EmitLine("{");
		Indent++;
		EmitLine("m_ConstantVariables[Index] = Value;");
		EmitLine("return true;");
		Indent--;
		EmitLine("}");
		EmitLine("else");
		Indent++;
		EmitLine("return false;");
		Indent--;
		Indent--;
		EmitLine("}");
	}

	void GenerateBuildRightHand()
	{
		EmitLine("void BuildRightHand(CCustomDeviceData& CustomDeviceData) override");
		EmitLine("{");
		Indent++;

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
		Indent--;
		EmitLine("}");
	}

	void GenerateInit()
	{
		EmitLine("eDEVICEFUNCTIONSTATUS Init(CCustomDeviceData& CustomDeviceData) override");
		EmitLine("{");
		Indent++;

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
		Indent--;
		EmitLine("}");
	}

	void GenerateProcessDiscontinuity()
	{
		EmitLine("eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CCustomDeviceData& CustomDeviceData) override");
		EmitLine("{");
		Indent++;

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
		Indent--;
		EmitLine("}");
	}

	void GenerateBuildEquations()
	{
		// функция построения матрицы Якоби
		EmitLine("void BuildEquations(CCustomDeviceData& CustomDeviceData) override");
		EmitLine("{");
		Indent++;

		// проходим по якобиану дерева
		for (auto& v : pTree->GetJacobian()->ChildNodes())
		{
			// для каждого элемента якобиана
			CASTJacobiElement* pJe = static_cast<CASTJacobiElement*>(v);
			// добавляем SetElement
			EmitLine(fmt::format("CustomDeviceData.SetElement({}, {}, {});   // {} by {}",
				pJe->pequation->itResolvedBy->first,			// строка элемента якобиана
				pJe->var->first,								// столбец элемента якобина
				ctrim(pJe->ChildNodes().front()->GetInfix()),	// функция вычисления элемента якобиана
				ctrim(pJe->pequation->GetInfix()),				// комментарий: уравнение, от которого берется частная производная
				pJe->var->first));								// комментарие: переменная, по которой берется частная производная
		}
		Indent--;
		EmitLine("}");
	}

	void GenerateDestroy()
	{
		EmitLine("void Destroy() override { delete this; }");
	}

	void GenerateBuildDerivatives()
	{
		EmitLine("void BuildDerivatives(CCustomDeviceData& CustomDeviceData) override");
		EmitLine("{");
		EmitLine("}");
	}

	void GenerateSetConstsDefaultValues()
	{
		EmitLine("void SetConstsDefaultValues() override");
		EmitLine("{");
		EmitLine("}");
	}

	void GenerateDLLFunctions()
	{
		EmitLine("extern \"C\" __declspec(dllexport) ICustomDevice * __cdecl CustomDeviceFactory()");
		EmitLine("{");
		Indent++;
		EmitLine("return new CCustomDevice();");
		Indent--;
		EmitLine("}");
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
};

