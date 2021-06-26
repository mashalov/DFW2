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
		EmitLine("void GetDeviceProperties(CDeviceContainerPropertiesBase & DeviceProps) override");
		EmitLine("{");
		Indent++;

		EmitLine("DeviceProps.SetType(DEVTYPE_AUTOMATIC);");
		EmitLine("DeviceProps.SetClassName(\"Automatic & scenario\", \"Automatic\");");
		EmitLine("DeviceProps.AddLinkFrom(DEVTYPE_MODEL, DLM_SINGLE, DPD_MASTER);");

		size_t nConstVarIndex(0), nExtVarIndex(0), nStateVarIndex(0);
		std::list<std::string> stateVars, extVars, constVars;

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
					constVars.push_back(fmt::format("{{ \"{}\", {{ {}, eDVT_CONSTSOURCE }} }}", v.first, nConstVarIndex));
				}
				nConstVarIndex++;
			}
			else if (v.second.External)
			{
				if (v.second.VarInstances.size() == 0)
					pTree->Warning(fmt::format("Переменная \"{}\" объявлена как внешняя, но не используется в модели", v.first));
				else
				{
					_ASSERTE(v.second.IsExternal());
					extVars.push_back(fmt::format("{{ \"{}\", {{ {}, DEVTYPE_MODEL }} }}", v.second.ModelLink, nExtVarIndex));
				}
				nExtVarIndex++;
			}
			else
			{
				if (v.second.IsMainStateVariable())
					stateVars.push_back(fmt::format("{{ \"{}\", {{ {}, VARUNIT_NOTSET }} }}", v.first, nStateVarIndex++));
			}
		}

		GenerateVariables("DeviceProps.m_VarMap      = { ", stateVars, " };", true);
		GenerateVariables("DeviceProps.m_ExtVarMap   = { ", extVars, " };", true);
		GenerateVariables("DeviceProps.m_ConstVarMap = { ", constVars, " };", true);
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

	void EmitLine(std::string_view Line)
	{		
		OutputStream << std::string(Indent * 4, ' ') << Line << std::endl;
	}

	void GenerateHeader()
	{
		EmitLine("#pragma once");
		EmitLine("#include\"../DFW2/ICustomDevice.h\"");
		EmitLine("namespace DFW2\n{");
		Indent++;
		GenerateClass();

		//GenerateDLLFunctions();
		Indent--;
		EmitLine("}");
	}

	template<typename T>
	void GenerateVariables(std::string_view Pre, T& VarList, std::string_view Post, bool UsePrePost = false)
	{
		std::string Line(Pre);
		size_t counter = VarList.size();
		size_t varsinline = 0;
		size_t indent = Line.length();

		for (auto& var : VarList)
		{
			Line += var;
			varsinline++;
			if(counter > 1 && counter <= VarList.size()) Line += ", ";

			if (Line.length() > MaxLineLentgh)
			{
				UsePrePost = false;
				if (counter == 1)
					Line += Post;
				EmitLine(Line);
				Line = std::string(indent, ' ');
				varsinline = 0;
			}
			counter--;
		}

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
		EmitLine("const VariableIndexRefVec& GetVariables() override { return m_StateVariables;}");
		EmitLine("const VariableIndexExternalRefVec& GetExternalVariables() override { return m_ExternalVariables;}");
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
		EmitLine("void BuildEquations(CCustomDeviceData& CustomDeviceData) override");
		EmitLine("{");
		Indent++;

		
		for (auto& v : pTree->GetJacobian()->ChildNodes())
		{
			CASTJacobiElement* pJe = static_cast<CASTJacobiElement*>(v);
			EmitLine(fmt::format("CustomDeviceData.SetElement({}, {}, {});   // {} by {}",
				pJe->pequation->itResolvedBy->first,
				pJe->var->first,
				ctrim(pJe->ChildNodes().front()->GetInfix()),
				ctrim(pJe->pequation->GetInfix()),
				pJe->var->first));
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
		EmitLine("class CCustomDevice : public ICustomDevice");
		EmitLine("{");
		Indent++;
		EmitLine("protected:");
		GenerateVariablesDeclaration();
		GenerateHostBlocks();
		EmitLine("public:");
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
		Indent--;
		EmitLine("};");
	}

	void Generate()
	{
		if (pTree->ErrorCount() != 0) return;

		std::filesystem::path OutputPath = pTree->GetPropertyPath(PropertyMap::szPropOutputPath);
		OutputPath.append(pTree->GetProperties().at(PropertyMap::szPropProjectName));

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
		GenerateHeader();
		OutputStream.close();
		pTree->PrintErrorsWarnings();
	}

	static inline const std::string CustomDeviceHeader = "CustomDevice.h";
};

