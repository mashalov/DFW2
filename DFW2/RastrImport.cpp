﻿#include "stdafx.h"
#include "stringutils.h"
#include "RastrImport.h"
#include "DynaGeneratorMustang.h"
#include "DynaGeneratorInfBus.h"
#include "DynaExciterMustang.h"
#include "DynaDECMustang.h"
#include "DynaExcConMustang.h"
#include "SVC.h"
#include "BranchMeasures.h"

using namespace DFW2;
//cl /LD /EHsc -DUNICODE -D_UNICODE customdevice.cpp dllmain.cpp


CRastrImport::CRastrImport(IRastrPtr spRastr)
{
	m_spRastr = spRastr;
	InitRastrWin();
}

bool GetConstFromField(const CConstVarIndex& VarInfo )
{
	bool bRes = false;
	if (VarInfo.DevVarType_ == eDEVICEVARIABLETYPE::eDVT_CONSTSOURCE)
		bRes = true;
	return bRes;
}

bool CRastrImport::GetCustomDeviceData(CDynaModel& Network, IRastrPtr spRastr, CustomDeviceConnectInfo& ConnectInfo, CCustomDeviceCPPContainer& CustomDeviceContainer)
{
	bool bRes(false);
	if (spRastr)
	{
		try
		{
			ITablePtr spSourceTable = spRastr->Tables->Item(ConnectInfo.m_TableName.c_str());
			IColsPtr spSourceCols = spSourceTable->Cols;

			using COLVECTOR = std::vector<std::pair<IColPtr, ptrdiff_t>>;
			COLVECTOR Cols;
			Cols.reserve(CustomDeviceContainer.ContainerProps().ConstVarMap_.size());
			IColPtr spColId = spSourceCols->Item(L"Id");
			IColPtr spColName = spSourceCols->Item(L"Name");
			IColPtr spColState = spSourceCols->Item(L"sta");
			for (const auto& col : CustomDeviceContainer.ContainerProps().ConstVarMap_)
				if (GetConstFromField(col.second))
					Cols.push_back(std::make_pair(spSourceCols->Item(col.first.c_str()), col.second.Index_));

			// count model types in storage

			IColPtr spModelType = spSourceCols->Item(ConnectInfo.m_ModelTypeField.c_str());
			long nTableIndex = 0;
			long nTableSize = spSourceTable->GetSize();
			long nModelsCount = 0;

			for (; nTableIndex < nTableSize; nTableIndex++)
			{
				if (spModelType->GetZ(nTableIndex).lVal == ConnectInfo.m_nModelType)
					nModelsCount++;
			}

			if (nModelsCount)
			{
				// create models for count given
				CCustomDeviceCPP* pCustomDevices = CustomDeviceContainer.CreateDevices<CCustomDeviceCPP>(nModelsCount);
				CustomDeviceContainer.BuildStructure();

				// put constants to each model
				long nModelIndex = 0;
				for (nTableIndex = 0; nTableIndex < nTableSize; nTableIndex++)
				{
					if (spModelType->GetZ(nTableIndex).lVal == ConnectInfo.m_nModelType)
					{
						if (nModelIndex >= nModelsCount)
						{
							Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszDLLBadBlocks, 
								stringutils::utf8_encode(CustomDeviceContainer.DLL()->GetModuleFilePath().c_str())));
							break;
						}

						CCustomDeviceCPP* pDevice = pCustomDevices + nModelIndex;
						pDevice->SetConstsDefaultValues();
						pDevice->SetId(spColId->GetZ(nTableIndex).lVal);
						pDevice->SetName(static_cast<const char*>(spColId->GetZS(nTableIndex)));
						pDevice->SetState(spColState->GetZ(nTableIndex).boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
						for (const auto& col : Cols)
							pDevice->SetSourceConstant(col.second, col.first->GetZ(nTableIndex).dblVal);
						nModelIndex++;
					}
				}
			}
		}
		catch (const _com_error& err)
		{
			Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszTableNotFoundForCustomDevice,
				stringutils::utf8_encode(CustomDeviceContainer.DLL()->GetModuleFilePath().c_str()),
				static_cast<const char*>(err.Description())));
		}
	}
	return bRes;
}

// читаем в сериализатор строку с заднным номером из таблицы RastrWin
// полагая что в сериализаторе устройство
void CRastrImport::ReadRastrRow(SerializerPtr& Serializer, long Row)
{
	// ставим устройству в сериализаторе индекс в БД и идентификатор
	Serializer->GetDevice()->SetDBIndex(Row);
	Serializer->GetDevice()->SetId(Row); // если идентификатора нет или он сложный - ставим порядковый номер в качестве идентификатора
	ReadRastrRowData(Serializer, Row);
}

// читаем в сериализатор строку с заднным номером из таблицы RastrWin
// полагая что в сериализаторе просто поля
// разделение функций нужно для того чтобы читать из RastrWin
// данные, не отнесенные к устрйоствам
void CRastrImport::ReadRastrRowData(SerializerPtr& Serializer, long Row)
{

	// проходим по значениям сериализатора
	for (auto&& sv : *Serializer)
	{
		MetaSerializedValue& mv = *sv.second;
		// пропускаем переменные состояния
		if (mv.bState)
			continue;
		// если поле не найден (нет адаптера) - пропускаем поле
		// ошибку надо было выдавать до ReadRastrRowData при формировании адаптеров
		if (!mv.pAux)
			continue;
		//получаем вариант со значением из столбца таблицы, привязанного к сериализатору
		const auto& dataProp{static_cast<CSerializedValueAuxDataRastr*>(mv.pAux.get())};
		variant_t vt { dataProp->m_spCol->GetZ(Row) };
		// конвертируем вариант из таблицы в нужный тип значения сериализатора
		// с необходимыми преобразованиями по метаданным
		switch (mv.ValueType)
		{
		case TypedSerializedValue::eValueType::VT_DBL:
			vt.ChangeType(VT_R8);
			// для вещественного поля учитываем множитель
			mv.SetDouble(vt.dblVal);
			break;
		case TypedSerializedValue::eValueType::VT_INT:
			vt.ChangeType(VT_I4);
			mv.SetInt(vt.lVal);
			break;
		case TypedSerializedValue::eValueType::VT_BOOL:
			vt.ChangeType(VT_BOOL);
			mv.SetBool(vt.boolVal);
			break;
		case TypedSerializedValue::eValueType::VT_NAME:
			vt.ChangeType(VT_BSTR);
			Serializer->GetDevice()->SetName(stringutils::utf8_encode(vt.bstrVal));
			break;
		case TypedSerializedValue::eValueType::VT_STATE:
			vt.ChangeType(VT_BOOL);
			// состояние RastrWin конвертируем в состояние устройства
			Serializer->GetDevice()->SetState(vt.boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
			break;
		case TypedSerializedValue::eValueType::VT_ID:
			vt.ChangeType(VT_I4);
			// для установки идентификатора используем функцию устройства вместо присваивания
			Serializer->GetDevice()->SetId(vt.lVal);
			break;
		case TypedSerializedValue::eValueType::VT_ADAPTER:
			// если у поля есть трансформатор, используем его
			// для преобразования варианта
			if (dataProp->m_pTransformer)
				vt = dataProp->m_pTransformer->Transform(vt);
			vt.ChangeType(VT_I4);
			// если поле привязано к адаптеру, даем адаптеру int
			// и если нужно адаптер должен перекодировать int во что-то нужное
			// в данном типе поля
			mv.Adapter->SetInt(vt.lVal);
			break;
		case TypedSerializedValue::eValueType::VT_STRING:
			vt.ChangeType(VT_BSTR);
			// если поле привязано к адаптеру, даем адаптеру int (недоделано)
			mv.SetString(stringutils::utf8_encode(vt.bstrVal));
			break;
		default:
			throw dfw2error(fmt::format("CRastrImport::ReadRastrRow wrong serializer type {}", mv.ValueType));
		}
	}
}

void CRastrImport::InitRastrWin()
{
	if(!m_spRastr)
	if (FAILED(m_spRastr.CreateInstance(CLSID_Rastr)))
		throw dfw2error(CDFW2Messages::m_cszCannotUseRastrWin3);

	struct RastrWinHandle
	{
		HKEY handle = NULL;
		RastrWinHandle(const wchar_t* regPath)
		{
			if (RegOpenKey(HKEY_CURRENT_USER, regPath, &handle) != ERROR_SUCCESS)
				throw dfw2error(CDFW2Messages::m_cszNoRastrWin3FoundInRegistry);
		}
		~RastrWinHandle()
		{
			if (handle)
				RegCloseKey(handle);
		}

		operator HKEY& ()
		{
			return handle;
		}
	};

	std::filesystem::path templatePath;
	const auto cszUserFolder = L"UserFolder";

	RastrWinHandle regRastrWin3(L"Software\\RastrWin3");
	DWORD size(0);
	if (RegQueryValueEx(regRastrWin3, cszUserFolder, NULL, NULL, NULL, &size) != ERROR_SUCCESS)
		throw dfw2error(CDFW2Messages::m_cszNoRastrWin3FoundInRegistry);

	auto buffer = std::make_unique<wchar_t[]>(size * sizeof(wchar_t) / sizeof(BYTE));
	if (RegQueryValueEx(regRastrWin3, cszUserFolder, NULL, NULL, reinterpret_cast<BYTE*>(buffer.get()), &size) == ERROR_SUCCESS)
		templatePath = buffer.get();
	else
		throw dfw2error(CDFW2Messages::m_cszNoRastrWin3FoundInRegistry);

	templatePath.append(L"shablon");

	rstPath = templatePath;
	dfwPath = templatePath;
	scnPath = templatePath;

	rstPath.append(wcszDynamicRST);
	dfwPath.append(L"автоматика.dfw");
	scnPath.append(L"сценарий.scn");
}

void CRastrImport::LoadFile(std::filesystem::path FilePath)
{
	LoadFile(FilePath, "");
}

void CRastrImport::LoadFile(std::filesystem::path FilePath, const std::filesystem::path& TemplatePath)
{
	m_spRastr->Load(RG_REPL, 
		stringutils::utf8_decode(FilePath.string()).c_str(), TemplatePath.c_str());
	LoadedFiles.push_back(FilePath);
}

void CRastrImport::GetFileData(CDynaModel& Network)
{
	
	// Тесты СО
	//LoadFile("e:\\temp\\sztest\\deep\\test9");				// исходный режим
	//LoadFile("e:\\temp\\sztest\\РМ_mdp_debug_1_111_027440");		// режим с нарушением устойчивости(сценарии 102, 109)
	//LoadFile("e:\\temp\\sztest\\РМ_mdp_debug_1_111_m004560");		// последний режим с нарушением устойчивости, перед предельным(сценарии 109)
	//LoadFile("e:\\temp\\sztest\\РМ_mdp_debug_1_111_m005560");		// режим без нарушения устойчивости, предельный


	//LoadFile("e:\\downloads\\starters_with_formulas\\mdp_debug_1_19"); 
	//LoadFile("e:\\downloads\\starters_with_formulas\\k_33_0_48312_changed");
	//LoadFile("D:\\Documents\\Raiden\\ModelDebugFolder\\model-00009");
	//LoadFile("e:\\downloads\\тестирование_4_19_006499_109\\тестирование_4_19_006499_109.os");
	//
	//LoadFile("d:/Documents/RastrWin3/test-rastr/RUSTab/FACTS/УШР/test9_dec.rst");
	LoadFile("e:/downloads/bugggs/модель.rst");
	m_spRastr->NewFile(dfwPath.c_str()); 
	LoadFile("d:/Documents/RastrWin3/test-rastr/RUSTab/FACTS/УШР/test9_p0q0.scn", scnPath.c_str());

	//LoadFile("E:/Downloads/mdp_debug_1_400014 ошибка", rstPath);
	//m_spRastr->NewFile(dfwPath.c_str()); 
	//LoadFile("E:/Downloads/mdp_debug_1_400014 ошибка", scnPath);
	
	

	//LoadFile("e:/downloads/ПРМ Московское РДУ/result.os");
	// 
	//LoadFile("D:\\source\\repos\\MatPowerImport\\x64\\Release\\case9all");

	//LoadFile("e:/downloads/ПРМ Московское РДУ/result.os");
	
	//LoadFile("D:\\source\\repos\\DFW2\\tests\\case39.rst", rstPath.c_str());
	//LoadFile("D:\\source\\repos\\DFW2\\tests\\case39_sc5.scn", scnPath.c_str());
	//m_spRastr->NewFile(dfwPath.c_str()); 

	//LoadFile("e:\\temp\\sztest\\102_1ф.КЗ с УРОВ на КАЭС (откл. КАЭС - Княжегубская №1).dfw", dfwPath.c_str());
	//LoadFile("e:\\temp\\sztest\\109_ 1ф. КЗ с УРОВ на КАЭС с отключением ВЛ 330 кВ Кольская АЭС - Мончегорск №2.dfw", dfwPath);
	//LoadFile("e:\\temp\\sztest\\109_ 1ф. КЗ с УРОВ на КАЭС с отключением ВЛ 330 кВ Кольская АЭС - Мончегорск №2.scn", scnPath);
	//LoadFile("e:\\temp\\sztest\\106_БЕЗ ШУНТА ПС 330 кВ Петрозаводск (с откл.Кондопога-Петрозаводск).dfw", dfwPath.c_str());
	//LoadFile("e:\\temp\\sztest\\deep\\102_1ф.КЗ с УРОВ на КАЭС (откл. КАЭС - Княжегубская №1).dfw ", dfwPath.c_str());
	//LoadFile("e:\\temp\\sztest\\deep\\КЗ-откл-нагрузка.dfw ", dfwPath.c_str());

	//LoadFile("e:\\downloads\\srv-smzu1-sz__mdp_debug_1_111_1\\ur78scn");
	//LoadFile("e:\\downloads\\srv-smzu1-sz__mdp_debug_1_111_1\\1.dfw", dfwPath.c_str());




	
	// Уват
	/*
	m_spRastr->NewFile(dfwPath.c_str());
	m_spRastr->NewFile(scnPath.c_str());
	m_spRastr->Load(RG_REPL, stringutils::utf8_decode("D:\\Downloads\\Уват 55\\Уватнефть_корр35_ver15_корр_быстр5_nopms.rst").c_str(), rstPath.c_str());
	*/

	// СМЗУ Северо-Запад
	//LoadFile("d:\\source\\repos\\DFW2\\tests\\mdp_debug_1"); 
	//LoadFile("E:\\Downloads\\matpower-master\\matpower-master\\data\\case13659pegase.rst");
	
	// Единая схема
	//LoadFile("e:\\downloads\\Mashalov\\Mashalov\\edin\\2.rst");


	// Test-9
	//LoadFile("d:\\source\\repos\\DFW2\\tests\\test92.rst", rstPath.c_str());
	//m_spRastr->NewFile(dfwPath.c_str());
	//LoadFile("C:\\Users\\masha\\source\\repos\\DFW2\\tests\\test9_park_noexc.rst");
	//LoadFile("C:\\Users\\masha\\source\\repos\\DFW2\\tests\\test9_park3c.rst");
	//LoadFile("C:\\Users\\masha\\source\\repos\\DFW2\\tests\\test9_park4c.rst");
	//m_spRastr->NewFile(dfwPath.c_str());

	// СМЗУ Сибирь
	//LoadFile("d:\\downloads\\4_18122019_14-00_simple_v7_clean_nosvc_fixpunom - утяж УР.os", rstPath.c_str());
	//LoadFile("C:\\Users\\masha\\source\\repos\\DFW2\\tests\\Siberia\\K1 уров.dfw", dfwPath.c_str());
	//LoadFile("d:\\downloads\\last_version_mdp_debug_1_New_calc2\\last_version_mdp_debug_1_New_calc2");
	//LoadFile("d:\\downloads\\5_check_mdp_debug_1\\5_check_mdp_debug_4");
	//LoadFile("d:\\downloads\\8_check_mdp_debug_1\\8_check_mdp_debug_1");
	//LoadFile("d:\\downloads\\схемы\\10_check_mdp_debug_1_stable");
	//LoadFile("d:\\downloads\\схемы\\9_check_mdp_debug_1_unstable");
	//LoadFile("d:\\downloads\\d1\\d1");



	//LoadFile("d:\\downloads\\!упрощ кор 16122020_01-00_itog+Урал_откл СЭ (нов) зам.УШР.rst", rstPath.c_str());
	//LoadFile("d:\\downloads\\mustang", rstPath.c_str());
	//LoadFile("C:\\Users\\masha\\source\\repos\\DFW2\\tests\\Siberia\\18122019_14-00_simple_v7_clean_nosvc_fixpunom.rst", rstPath.c_str());
	//LoadFile("C:\\Users\\masha\\source\\repos\\DFW2\\tests\\Siberia\\кз.dfw", dfwPath.c_str());


	// СМЗУ Сибирь 16.11.2021
	//LoadFile("c:\\Users\\masha\\source\\repos\\DFW2\\tests\\Siberia_20211116\\16.11.2021 to_dynamic_check");
	//LoadFile("c:\\Users\\masha\\source\\repos\\DFW2\\tests\\Siberia_20211116\\1. 2ф КЗ на землю СМЗУ.dfw", dfwPath.c_str());
	//LoadFile("c:\\Users\\masha\\source\\repos\\DFW2\\tests\\Siberia_20211116\\2. 1ф КЗ на землю+УРОВ СМЗУ.dfw", dfwPath.c_str());



	//m_spRastr->Load(RG_REPL, L"D:\\temp\\1", L"");
	//m_spRastr->Load(RG_REPL, L"..\\tests\\original.dfw", dfwPath.c_str());
	//m_spRastr->NewFile(dfwPath.c_str());
	//m_spRastr->Load(RG_REPL, dfwPath.c_str());
	//m_spRastr->Load(RG_REPL, rstPath.c_str()); 
	//spRastr->Load(RG_REPL, L"..\\tests\\mdp_debug_5", "");

	for (const auto& file : LoadedFiles)
		Network.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLoadingModelFormat, "RastrWin3", file.string()));

	GetData(Network);
}


void CRastrImport::StoreResults(const CDynaModel& Network)
{
	ITablesPtr spTables{ m_spRastr->Tables };
	constexpr const char* szRaidenResults{ "RaidenResults" };
	constexpr const char* szTimeComputed{ "TimeComputed" };
	constexpr const char* szMessage{ "Message" };
	constexpr const char* szSyncLossCause{ "SyncLossCause" };
	constexpr const char* szResultFilePath{ "ResultFilePath" };

	long TableIndex{ spTables->GetFind(szRaidenResults) };
	if (TableIndex < 0)
		spTables->Add(szRaidenResults);
	ITablePtr spResults{ spTables->Item(szRaidenResults) };
	IColsPtr spResCols{ spResults->Cols };
	spResults->DelRowS();
	spResults->AddRow();
	
	long TimeComputedIndex{ spResCols->GetFind(szTimeComputed) };
	if (TimeComputedIndex < 0)
		spResCols->Add(szTimeComputed, PropTT::PR_REAL);

	IColPtr spTimeComputed{ spResCols->Item(szTimeComputed) };
	spTimeComputed->PutZ(0, Network.GetCurrentIntegrationTime());

	long MessageIndex{ spResCols->GetFind(szMessage) };
	if (MessageIndex < 0)
		spResCols->Add(szMessage, PropTT::PR_STRING);

	IColPtr spMessage{ spResCols->Item(szMessage) };
	spMessage->PutZS(0, stringutils::utf8_decode(Network.FinalMessage()).c_str());

	long SyncLossCauseIndex{ spResCols->GetFind(szSyncLossCause) };
	if (SyncLossCauseIndex < 0)
		spResCols->Add(szSyncLossCause, PropTT::PR_ENUM);

	IColPtr spSyncLossCause{ spResCols->Item(szSyncLossCause) };
	spSyncLossCause->PutProp(FL_NAMEREF, stringutils::COM_encode("Нет|АР ветви|АР генератора|Автомат скорости|Отказ метода").c_str());
	spSyncLossCause->PutZ(0, static_cast<long>(Network.SyncLossCause()));

	long ResultFilePathIndex{ spResCols->GetFind(szResultFilePath) };
	if (ResultFilePathIndex < 0)
		spResCols->Add(szResultFilePath, PropTT::PR_STRING);

	IColPtr spResultFilePath{ spResCols->Item(szResultFilePath) };
	spResultFilePath->PutZS(0, Network.ResultFilePath().c_str());
}

void CRastrImport::GetData(CDynaModel& Network)
{
	
	ITablesPtr spTables{ m_spRastr->Tables };

	auto ps{ Network.GetParametersSerializer() };

	// пытаемся прочитать параметры Raiden из таблицы RastrWin
	if (long nTableIndex{ spTables->GetFind(cszRaidenParameters_) }; nTableIndex >= 0)
	{
		ITablePtr spRaidenParameters{ spTables->Item(nTableIndex) };
		IColsPtr spCols{ spRaidenParameters->Cols };
		// заполняем адаптеры полей RastrWin по названиям полей
		// если нужного поля в таблице RastrWin нет - молча пропускаем
		// и оставляем значение по умолчанию
		for (auto&& field : *ps)
			if (long index = spCols->GetFind(field.first.c_str()); index >= 0)
				field.second->pAux = std::make_unique<CSerializedValueAuxDataRastr>(spCols->Item(index), nullptr);
		// читаем данные из собранных адаптеров
		ReadRastrRowData(ps, 0);
	}


	
	// принимаем основные параметры от RUSTab
	ITablePtr spParameters{ spTables->Item("com_dynamics") };
	IColsPtr spParCols{ spParameters->Cols };
	IColPtr spDuration{ spParCols->Item(L"Tras") };
	IColPtr spLTC2Y{ spParCols->Item(L"frSXNtoY") };
	IColPtr spFreqT{ spParCols->Item(L"Tf") };
	IColPtr spParkParams{ spParCols->Item(L"corrT") };
	IColPtr spDamping{ spParCols->Item(L"IsDemp") };
	IColPtr spMaxResultFiles{ spParCols->Item(L"MaxResultFiles") };


	Network.DeserializeParameters(Network.Platform().Root() / "config.json");

	m_rastrSynonyms
		.AddRastrSynonym(CDeviceContainerProperties::cszSysNameLRC_, "polin")
		.AddFieldSynonyms("LRCId", "nsx")
		.AddFieldSynonyms("Umin", "umin")
		.AddFieldSynonyms("Freq", "frec");


	m_rastrSynonyms
		.AddRastrSynonym("Node", cszAliasNode_)
		.AddFieldSynonyms("LRCLFId", "nsx")
		.AddFieldSynonyms("LRCTransId", "dnsx")
		.AddFieldTransformer("tip", new CRastrNodeTypeEnumTransformer()); // задаем трансформатор типа узла

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameBranch_, cszAliasBranch_);
	m_rastrSynonyms
		.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGeneratorInfPower_, cszAliasGenerator_)
		.AddFieldSynonyms("Kgen", "NumBrand")
		.AddFieldSynonyms("Kdemp", "Demp")
		.AddFieldSynonyms("cosPhinom", "cosFi")
		.AddFieldSynonyms("Unom", "Ugnom");

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameReactor_, "Reactors")
		.AddFieldSynonyms("HeadNode", "Id1")
		.AddFieldSynonyms("TailNode", "Id2")
		.AddFieldSynonyms("ParallelBranch", "Id3")
		.AddFieldSynonyms(CDynaNodeBase::cszb_, "B")
		.AddFieldSynonyms(CDynaNodeBase::cszg_, "G")
		.AddFieldSynonyms(CSerializerBase::m_cszType, "tip")
		.AddFieldSynonyms("Placement", "Pr_vikl");

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameSVC_, "DFWFACTS")
		.AddFieldSynonyms("Kgen", "")
		.AddFieldSynonyms(CDynaPowerInjector::cszP_, "")
		.AddFieldSynonyms(CDynaPowerInjector::cszQ_, "Qout")
		//.AddFieldSynonyms(CDynaPowerInjector::m_cszQmin, "Min")
		//.AddFieldSynonyms(CDynaPowerInjector::m_cszQmax, "Max")
		//.AddFieldSynonyms(CDynaPowerInjector::m_cszQnom, CDynaPowerInjector::m_cszSnom)
		//.AddFieldSynonyms(CDynaPowerInjector::cszVref_, "Ref1")
		.AddFieldSynonyms(CDynaSVCDEC::cszDroop_, "XSL");

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameSVCDEC_, "DFWFACTS");

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGeneratorMotion_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGenerator1C_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGenerator2C_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGenerator3C_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGeneratorMustang_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGeneratorPark3C_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGeneratorPark4C_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameGeneratorPowerInjector_, cszAliasGenerator_);
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameExciterMustang_, "Exciter");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameDECMustang_, "Forcer");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::cszSysNameExcConMustang_, "ExcControl");

	ITablePtr VGSVCTable{ spTables->Item(L"USHR") };
	ITablePtr SVCTable{ spTables->Item(L"DFWFACTS") };
	IColsPtr VGSVCTableCols{ VGSVCTable->Cols };
	IColsPtr SVCTableCols{ SVCTable->Cols };

	IColPtr VGSVCId{ VGSVCTableCols->Item(L"Id") };
	IColPtr VGSVCName{ VGSVCTableCols->Item(L"Name") };
	IColPtr VGSVCSta{ VGSVCTableCols->Item(L"sta") };
	IColPtr VGSVCNodeId{ VGSVCTableCols->Item(L"NodeId") };
	IColPtr VGSVCQnom{ VGSVCTableCols->Item(L"Snom") };
	IColPtr VGSVCUnom{ VGSVCTableCols->Item(L"Unom") };
	IColPtr VGSVCType{ VGSVCTableCols->Item(L"Type") };
	IColPtr VGSVCTypeRef1{ VGSVCTableCols->Item(L"tref1") };
	IColPtr VGSVCRef1{ VGSVCTableCols->Item(L"Ref1") };
	IColPtr VGSVCRangeType{ VGSVCTableCols->Item(L"izm") };
	IColPtr VGSVCMin{ VGSVCTableCols->Item(L"Min") };
	IColPtr VGSVCMax{ VGSVCTableCols->Item(L"Max") };
	IColPtr VGSVCKct{ VGSVCTableCols->Item(L"Kct") };
	IColPtr VGSVCMode{ VGSVCTableCols->Item(L"mode") };
	IColPtr VGSVCQ{ VGSVCTableCols->Item(L"Qr") };

	IColPtr SVCId{ SVCTableCols->Item(L"Id") };
	IColPtr SVCName{ SVCTableCols->Item(L"Name") };
	IColPtr SVCSta{ SVCTableCols->Item(L"sta") };
	IColPtr SVCModelType{ SVCTableCols->Item(L"ModelType") };
	IColPtr SVCNodeId{ SVCTableCols->Item(L"NodeId") };
	IColPtr SVCQnom{ SVCTableCols->Item(L"Qnom") };
	IColPtr SVCUnom{ SVCTableCols->Item(L"Unom") };
	IColPtr SVCQmin{ SVCTableCols->Item(L"Qmin") };
	IColPtr SVCQmax{ SVCTableCols->Item(L"Qmax") };
	IColPtr SVCXSL{ SVCTableCols->Item(L"XSL") };
	IColPtr SVCEnfSta{ SVCTableCols->Item(L"EnfSta") };
	IColPtr SVCKienf{ SVCTableCols->Item(L"Kienf") };
	IColPtr SVCKidef{ SVCTableCols->Item(L"Kidef") };
	IColPtr SVCKenf{ SVCTableCols->Item(L"Kenf") };
	IColPtr SVCKdef{ SVCTableCols->Item(L"Kdef") };
	IColPtr SVCTcsmooth{ SVCTableCols->Item(L"Tcsmooth") };
	IColPtr SVCTfir{ SVCTableCols->Item(L"Tfir") };
	IColPtr SVCVref{ SVCTableCols->Item(L"Vref") };

	for (long index{ 0 }; index < VGSVCTable->GetSize(); index++)
	{
		const long Id{ VGSVCId->GetZ(index).lVal };
		const std::string SVCTypeString{ stringutils::utf8_encode(VGSVCType->GetZS(index)) };
		const std::string SVCTypeRefString{ stringutils::utf8_encode(VGSVCTypeRef1->GetZS(index)) };
		if (VGSVCType->GetZ(index).lVal != 0 || VGSVCTypeRef1->GetZ(index).lVal != 0)
		{
			Network.Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(
				CDFW2Messages::m_cszRastrWinSVCModelIsNotSupported,
				Id,
				SVCTypeString,
				SVCTypeRefString));
			continue;
		}
		

		double Qmin{ VGSVCMin->GetZN(index) }, Qmax{ VGSVCMax->GetZN(index) };
		const double Qnom{ VGSVCQnom->GetZN(index) }, Unom{ VGSVCUnom->GetZN(index) };
		switch (VGSVCRangeType->GetZ(index).lVal)
		{
		case 0: // Q
			break;
		case 1: // %Qnom
			Qmin *= Qnom * 1e-2; Qmax *= Qnom * 1e-2;
			break;
		case 2: // B
			Qmin *= Unom * Unom * 1e-6; Qmax *= Unom * Unom * 1e-6;
			break;
		case 3: // X
			Qmin = Unom * Unom / Qmin; Qmax = Unom * Unom / Qmax;
			break;
		case 4: // I
			Qmin *= Unom * 1e-3 * Consts::sqrt3;
			Qmax *= Unom * 1e-3 * Consts::sqrt3;
			break;
		default:
			continue;
		}

		SVCTable->SetSel(_bstr_t(stringutils::utf8_decode(fmt::format("Id={}", Id)).c_str()));
		long SVCindex{ SVCTable->GetFindNextSel(-1) };
		if (SVCindex < 0)
		{
			SVCindex = SVCTable->GetSize();
			SVCTable->AddRow();
			SVCModelType->PutZ(SVCindex, 2);
		}
		SVCId->PutZS(SVCindex, VGSVCId->GetZS(index));
		SVCName->PutZS(SVCindex, VGSVCName->GetZS(index));
		SVCSta->PutZN(SVCindex, VGSVCSta->GetZN(index));
		SVCQnom->PutZN(SVCindex, Qnom);
		SVCUnom->PutZN(SVCindex, Unom);
		SVCQmin->PutZN(SVCindex, Qmin);
		SVCQmax->PutZN(SVCindex, Qmax);
		SVCXSL->PutZN(SVCindex, VGSVCKct->GetZN(index));
		SVCVref->PutZN(SVCindex, VGSVCRef1->GetZN(index));
		SVCNodeId->PutZN(SVCindex, VGSVCNodeId->GetZN(index));
	}

	for (long index{ 0 }; index < SVCTable->GetSize(); index++)
	{
		const long Id{ SVCId->GetZ(index).lVal };
		VGSVCTable->SetSel(_bstr_t(stringutils::utf8_decode(fmt::format("Id={}", Id)).c_str()));
		if (const long VGSVCindex{ VGSVCTable->GetFindNextSel(-1) }; VGSVCindex < 0)
			SVCSta->PutZ(index, true);
	}

	ReadLRCs(static_cast<CDynaLRCContainer&>(Network.LRCs));
	ReadTable(Network.Reactors);
	ReadTable(Network.Nodes);
	ReadTable(Network.Branches);
	ReadTable(Network.GeneratorsInfBus, "ModelType=2");
	ReadTable(Network.GeneratorsMotion, "ModelType=3");
	ReadTable(Network.Generators1C, "ModelType=4");
	ReadTable(Network.Generators3C, "ModelType=5");
	ReadTable(Network.GeneratorsMustang, "ModelType=6");
	ReadTable(Network.GeneratorsPark3C, "ModelType=7");
	ReadTable(Network.GeneratorsPark4C, "ModelType=8");
	ReadTable(Network.Generators2C, "ModelType=10");
	ReadTable(Network.GeneratorsPowerInjector, "ModelType=0");
	ReadTable(Network.ExcitersMustang);
	ReadTable(Network.DECsMustang);
	ReadTable(Network.ExcConMustang);
	ReadTable(Network.SVCs, "ModelType=2");
	ReadTable(Network.SVCDECs, "ModelType=5");

	ReadAutomatic(Network);

	//Network.CustomDeviceCPP.ConnectDLL("CustomDeviceCPP.dll");
	//CustomDeviceConnectInfo ci("ExcControl",2);
	//ITablePtr spExAddXTable{ m_spRastr->Tables->Item(L"ExcControl") };
	//IColsPtr spExAddCols{ spExAddXTable->Cols };
	//const _bstr_t cszXcomp{ L"Xcomp" };
	//if(spExAddCols->GetFind(cszXcomp) < 0)
	//	spExAddCols->Add(cszXcomp, PR_REAL);

	//GetCustomDeviceData(Network, m_spRastr, ci, Network.CustomDevice);
	//GetCustomDeviceData(Network, m_spRastr, ci, Network.CustomDeviceCPP);
}

void CRastrImport::ReadAutomatic(CDynaModel& Network)
{
	struct AutoLevelT
	{
		CAutomatic& Automatic;
		const _bstr_t Starters_;
		const _bstr_t Logics_;
		const _bstr_t Actions_;
	};

	const std::array<AutoLevelT, 2> AutoLevels =
	{
		{
			{Network.Automatic(), L"DFWAutoStarter", L"DFWAutoLogic", L"DFWAutoAction"},
			{Network.Scenario(), L"DFWAutoStarterScn", L"DFWAutoLogicScn", L"DFWAutoActionScn"},
		}
	};


	struct ActionCommandTypeArgsT
	{
		long Type;
		std::string_view Table;
		std::string_view Prop;
	};


	/*  Типы действий RUSTab

	0 Откл|
	1 Объект|
	2 Сост узла|
	3 Сост ветви|
	4 Узел Gш|
	5 Узел Bш|
	6 Узел Rш|
	7 Узел Xш|
	8 Узел Pn|
	9 Узел Qn|
	10 ЭГП|
	11 Якоби|
	12 Узел PnQn|
	13 Узел PnQn0|
	14 ГРАМ|
	15 РТ|
	16 Стоп|
	17 Ветвь КЗ|
	18 Протокол
	19 КЗ Uост
	20 КЗ Uост
	*/

	/* Типы стартеров RUSTab

	0 Откл|
	1 Объект|
	2 Время|
	3 Формула|
	4 Сост узла|
	5 Сост ветви|
	6 Ветвь Iнач|
	7 Ветвь Iкон|
	8 Ветвь Pнач|
	9 Ветвь Pкон|
	10 Ветвь Qнач|
	11 Ветвь Qкон|
	12 Ветвь PHIнач|
	13 Ветвь PHIкон

	*/


	const std::map<long, std::pair<const std::string_view, const std::string_view>>
		ActionCommandTypeArgs
	{
		{2 , {CDynaNodeBase::cszAliasNode_, CDevice::cszSta_ } },
		{3 , {CDynaNodeBase::cszAliasNode_, CDevice::cszSta_ } },
		{4 , {CDynaNodeBase::cszAliasNode_, CDynaNode::cszGsh_ } },
		{5 , {CDynaNodeBase::cszAliasNode_, CDynaNode::cszBsh_} },
		{6 , {CDynaNodeBase::cszAliasNode_, CDynaNodeBase::cszr_} },
		{7 , {CDynaNodeBase::cszAliasNode_, CDynaNodeBase::cszx_} },
		{8 , {CDynaNodeBase::cszAliasNode_, CDynaNode::cszPload_} },
		{9 , {CDynaNodeBase::cszAliasNode_, CDynaNode::cszQload_} },
		{13 , {CDynaNodeBase::cszAliasNode_, CDynaNode::cszPload_} },
		{19 , {CDynaNodeBase::cszAliasNode_, CDynaNode::cszV_} },
		{20 , {CDynaNodeBase::cszAliasNode_, CDynaNode::cszV_} },
	},
	StarterTypeArgs
	{
		{4 , {CDynaBranch::cszAliasBranch_, CDevice::cszSta_ } },
		{5 , {CDynaBranch::cszAliasBranch_, CDevice::cszSta_ } },
		{6 , {CDynaBranch::cszAliasBranch_, CDynaBranchMeasure::cszIb_ } },
		{7 , {CDynaBranch::cszAliasBranch_, CDynaBranchMeasure::cszIe_ } },
		{8 , {CDynaBranch::cszAliasBranch_, CDynaBranchMeasure::cszPb_ } },
		{9 , {CDynaBranch::cszAliasBranch_, CDynaBranchMeasure::cszPe_ } },
		{10 , {CDynaBranch::cszAliasBranch_, CDynaBranchMeasure::cszQb_ } },
		{11 , {CDynaBranch::cszAliasBranch_, CDynaBranchMeasure::cszQe_ } }
	};

	ITablesPtr spTables{ m_spRastr->Tables };

	const _bstr_t strId{ L"Id" };
	const _bstr_t strName{ L"Name" };
	const _bstr_t strType{ L"Type" };
	const _bstr_t strFormula{ L"Formula" };
	const _bstr_t strClass{ L"ObjectClass" };
	const _bstr_t strKey{ L"ObjectKey" };
	const _bstr_t strProp{ L"ObjectProp" };
	const _bstr_t strOutput{ L"OutputMode" };

	class AutoValue : public variant_t
	{
		using variant_t::variant_t;
		std::string String_;
		void ChangeToString()
		{
			ChangeType(VT_BSTR);
			String_ = stringutils::utf8_encode(bstrVal);
		}
	public:
		AutoValue(IColPtr& col, long index) : variant_t{ col->GetZ(index) } {};
		operator const std::string_view()
		{
			ChangeToString();
			return String_;
		}
		operator const char* ()
		{
			ChangeToString();
			return String_.c_str();
		}
	};

	for (auto&& Level : AutoLevels)
	{
		ITablePtr spAutoStarters{ spTables->Item(Level.Starters_) };
		IColsPtr spASCols{ spAutoStarters->Cols };
		IColPtr	spASId{ spASCols->Item(strId) },
			spASName{ spASCols->Item(strName) },
			spASType{ spASCols->Item(strType) },
			spASExpr{ spASCols->Item(strFormula) },
			spASObjClass{ spASCols->Item(strClass) },
			spASObjKey{ spASCols->Item(strKey) },
			spASObjProp{ spASCols->Item(strProp) };

		for (long i{ 0 }; i < spAutoStarters->GetSize(); i++)
		{

			std::string table{ static_cast<std::string_view>(AutoValue(spASObjClass, i)) };
			std::string prop{ static_cast<std::string_view>(AutoValue(spASObjProp, i)) };

			if (const auto type{ StarterTypeArgs.find(AutoValue(spASType, i)) }; type != StarterTypeArgs.end())
			{
				table = type->second.first;
				prop = type->second.second;
			}

			Level.Automatic.AddStarter(
				AutoValue(spASType, i),
				AutoValue(spASId, i),
				AutoValue(spASName, i),
				AutoValue(spASExpr, i),
				0,
				table,
				AutoValue(spASObjKey, i),
				prop);
		}

		ITablePtr spAutoLogic{ spTables->Item(Level.Logics_) };
		IColsPtr spALCols{ spAutoLogic->Cols };
		IColPtr spALId{ spALCols->Item(strId) },
			spALName{ spALCols->Item(strName) },
			spALType{ spALCols->Item(strType) },
			spALExpr{ spALCols->Item(strFormula) },
			spALActions{ spALCols->Item(L"Actions") },
			spALDelay{ spALCols->Item(L"Delay") },
			spALOutputMode{ spALCols->Item(strOutput) };

		for (int i{ 0 }; i < spAutoLogic->GetSize(); i++)
		{
			Level.Automatic.AddLogic(
				AutoValue(spALType, i),
				AutoValue(spALId, i),
				AutoValue(spALName, i),
				AutoValue(spALExpr, i),
				AutoValue(spALActions, i),
				AutoValue(spALDelay, i),
				AutoValue(spALOutputMode, i));
		}

		ITablePtr spAutoActions{ spTables->Item(Level.Actions_) };
		IColsPtr spAACols{ spAutoActions->Cols };
		IColPtr spAAId{ spAACols->Item(strId) },
			spAAName{ spAACols->Item(strName) },
			spAAType{ spAACols->Item(strType) },
			spAAExpr{ spAACols->Item(strFormula) },
			spAAObjClass{ spAACols->Item(strClass) },
			spAAObjKey{ spAACols->Item(strKey) },
			spAAObjProp{ spAACols->Item(strProp) },
			spAAOutputMode{ spAACols->Item(strOutput) },
			spAAActionGroup{ spAACols->Item(L"ParentId") },
			spAAORunsCount{ spAACols->Item(L"RunsCount") };

		for (long i{ 0 }; i < spAutoActions->GetSize(); i++)
		{
			std::string table{ static_cast<std::string_view>(AutoValue(spAAObjClass, i)) };
			std::string prop{ static_cast<std::string_view>(AutoValue(spAAObjProp, i)) };

			if (const auto type{ ActionCommandTypeArgs.find(AutoValue(spAAType, i)) }; type != ActionCommandTypeArgs.end())
			{
				table = type->second.first;
				prop = type->second.second;
			}

			Level.Automatic.AddAction(
				AutoValue(spAAType, i),
				AutoValue(spAAId, i),
				AutoValue(spAAName, i),
				AutoValue(spAAExpr, i),
				0,
				table,
				AutoValue(spAAObjKey, i),
				prop,
				AutoValue(spAAActionGroup, i),
				AutoValue(spAAOutputMode, i),
				AutoValue(spAAORunsCount, i));
		}
	}

	// в качестве имени модели используем имя файла

	const auto rstFile{ std::find_if(LoadedFiles.rbegin(), LoadedFiles.rend(), [](const auto& path) 
		{ 
			return path.extension() == ".rst" || path.extension().empty();
		}) };
	if(rstFile != LoadedFiles.rend())
		Network.SetModelName(stringutils::utf8_encode(rstFile->filename().c_str()));
}

void CRastrImport::ReadLRCs(CDynaLRCContainer& container)
{
	ITablePtr spLRC = m_spRastr->Tables->Item("polin");
	IColsPtr spCols = spLRC->Cols;
	IColPtr Id = spCols->Item("nsx");
	IColPtr v = spCols->Item("umin");
	IColPtr f = spCols->Item("frec");

	using lrcpolynom = std::array<IColPtr, 3>;
	lrcpolynom pcols { spCols->Item("p0"), spCols->Item("p1"), spCols->Item("p2") },
			   qcols { spCols->Item("q0"), spCols->Item("q1"), spCols->Item("q2") };

	// считаем сколько у нас уникальных СХН
	std::set<ptrdiff_t> uniques;
	for (long index = 0; index < spLRC->GetSize(); index++)
		uniques.insert(Id->GetZ(index));

	auto lrcs = container.CreateDevices<CDynaLRC>(uniques.size());
	for (const auto& id : uniques)
	{
		lrcs->SetId(id);
		lrcs++;
	}


	for (long index = 0; index < spLRC->GetSize(); index++)
	{
		auto lrc = static_cast<CDynaLRC*>(container.GetDevice(Id->GetZ(index)));
		if (!lrc) continue;

		struct polynoms
		{
			LRCRAWDATA* target;
			lrcpolynom& source;
		};

		std::array<polynoms, 2> pq{ { { &lrc->P()->P, pcols }, { &lrc->Q()->P, qcols }  } };

		for (auto&& power : pq)
		{
			power.target->push_back(
				{ 
				  v->GetZ(index), 
				  f->GetZ(index), 
				  power.source[0]->GetZ(index), 
				  power.source[1]->GetZ(index), 
				  power.source[2]->GetZ(index) 
				});
		}
	}

}

void CRastrImport::GenerateRastrWinFile(CDynaModel& Network, const std::filesystem::path& Path)
{
	Network.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Генерация файла для RastrWin : {}", stringutils::utf8_encode(rstPath.c_str())));
	m_spRastr->Load(RG_KOD::RG_REPL, Path.c_str(), L"");
	UpdateRastrWinFile(Network, rstPath.filename().string());
	m_spRastr->Save(Path.c_str(), L"");
	Network.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Сохранен файл для RastrWin : {}", stringutils::utf8_encode(rstPath.c_str())));

}

void CRastrImport::GenerateRastrWinTemplate(CDynaModel& Network, const std::filesystem::path& Path)
{
	if (!Path.empty())
		rstPath = Path;
	Network.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Генерация шаблона для RastrWin : {}", stringutils::utf8_encode(rstPath.c_str())));
	m_spRastr->NewFile(rstPath.c_str());
	UpdateRastrWinFile(Network, Path.filename().string());
	m_spRastr->Save("", rstPath.c_str());
	Network.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Сохранен шаблон для RastrWin : {}", stringutils::utf8_encode(rstPath.c_str())));

}

void CRastrImport::UpdateRastrWinFile(CDynaModel& Network, std::string_view templatename)
{
	auto ps{ Network.GetParametersSerializer() };
	ITablesPtr spTables{ m_spRastr->Tables };

	size_t MissedDescriptions{ 0 };

	if (long nTableIndex{ spTables->GetFind(cszRaidenParameters_) }; nTableIndex >= 0)
		spTables->Remove(nTableIndex);

	ITablePtr spRaidenParameters{ spTables->Add(cszRaidenParameters_) };
	IColsPtr spCols{ spRaidenParameters->Cols };
	spRaidenParameters->PutTemplateName(std::string(templatename).c_str());
	IColPtr spGoRaiden{ spCols->Add(L"GoRaiden", PR_BOOL) };
	spGoRaiden->PutProp(FL_DESC, L"Выполнять расчет ЭМПП с помощью Raiden");
	spGoRaiden->PutProp(FL_ZAG, spGoRaiden->Name);
	spRaidenParameters->AddRow();
	spRaidenParameters->PutTipT(1L);

	Network.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("Генерация таблицы \"{}\"", cszRaidenParameters_));

	std::map<std::string, std::string> Descriptions =
	{
		{CDynaModel::Parameters::m_cszAdamsDampingAlpha, "Коэффициент подавления рингинга Адамса"},
		{CDynaModel::Parameters::m_cszLFImbalance, "УР: Допустимый небаланс"},
		{CDynaModel::Parameters::m_cszLFFlat, "УР: Плоский старт"},
		{CDynaModel::Parameters::m_cszLFStartup, "УР: Стартовый метод"},
		{CDynaModel::Parameters::m_cszLFSeidellIterations, "УР: Максимальное количество итераций стартового метода"},
		{CDynaModel::Parameters::m_cszLFSeidellStep, "УР: Коэффициент ускорения метода Зейделя"},
		{CDynaModel::Parameters::m_cszLFEnableSwitchIteration, "УР: Разрешить переключать типы узлов с данной итерации"},
		{CDynaModel::Parameters::m_cszLFMaxIterations, "УР: Максимальное количество итераций метода Ньютона"},
		{CDynaModel::Parameters::m_cszLFNewtonMaxVoltageStep, "УР: Максимальное приращение шага Ньютона по напряжению"},
		{CDynaModel::Parameters::m_cszLFNewtonMaxNodeAngleStep, "УР: Максимальное приращение шага Ньютона по углу узла"},
		{CDynaModel::Parameters::m_cszLFNewtonMaxBranchAngleStep, "УР: Максимальное приращение шага Ньютона по углу связи"},
		{CDynaModel::Parameters::m_cszLFForceSwitchLambda, "УР: Шаг Ньютона, меньше которого бэктрэк выключается и выполняется переключение типов узлов"},
		{CDynaModel::Parameters::m_cszLFFormulation, "УР: Система уравнений метода Ньютона"},
		{CDynaModel::Parameters::m_cszLFAllowNegativeLRC, "УР: Разрешить учет СХН для узлов с отрицательной нагрузкой"},
		{CDynaModel::Parameters::m_cszLFLRCMinSlope, "УР: Минимальная крутизна СХН"},
		{CDynaModel::Parameters::m_cszLFLRCMaxSlope, "УР: Максимальная крутизна СХН"},
		{CDynaModel::Parameters::m_cszFrequencyTimeConstant, "Постоянная времени сглаживания частоты в узле"},
		{CDynaModel::Parameters::m_cszLRCToShuntVmin, "Напряжение перехода СХН на шунт"},
		{CDynaModel::Parameters::m_cszConsiderDampingEquation, "Учитывать демпфирование в уравнении движения для моделей с демпферными контурами"},
		{CDynaModel::Parameters::cszZeroCrossingTolerance, "Точность определения дискретных изменений"},
		{CDynaModel::Parameters::m_cszProcessDuration, "Длительность ЭМПП"},
		{CDynaModel::Parameters::m_cszOutStep, "Минимальный шаг вывода результатов"},
		{CDynaModel::Parameters::m_cszAtol, "Абсолютная точность интегрирования"},
		{CDynaModel::Parameters::m_cszRtol, "Относительная точность интегрирования"},
		{CDynaModel::Parameters::m_cszRefactorByHRatio, "Выполнять рефакторизацию Якоби при изменении шага превышающем"},
		{CDynaModel::Parameters::m_cszMustangDerivativeTimeConstant, "Постоянная времени сглаживания производных в АРВ Мустанг"},
		{CDynaModel::Parameters::m_cszAdamsIndividualSuppressionCycles, "Количество перемен знака переменной для обнаружения рингинга"},
		{CDynaModel::Parameters::m_cszAdamsGlobalSuppressionStep, "Номер шага, на кратном которому работает глобальное подавление рингинга"},
		{CDynaModel::Parameters::m_cszAdamsIndividualSuppressStepsRange, "Количество шагов, на протяжении которого работает индивидуальное подавление рингинга переменной"},
		{CDynaModel::Parameters::m_cszUseRefactor, "Использовать быструю рефакторизацию KLU"},
		{CDynaModel::Parameters::m_cszDisableResultsWriter, "Отключить запись результатов"},
		{CDynaModel::Parameters::m_cszMinimumStepFailures, "Допустимое количество ошибок на минимальном шаге"},
		{CDynaModel::Parameters::m_cszZeroBranchImpedance, "Минимальное сопротивление ветви"},
		{CDynaModel::Parameters::m_cszAllowUserOverrideStandardLRC, "Разрешить замену стандартных СХН пользовательскими"},
		{CDynaModel::Parameters::m_cszAllowDecayDetector, "Разрешить завершение расчета при затухании ЭМПП"},
		{CDynaModel::Parameters::m_cszDecayDetectorCycles, "Контролировать затухание ЭМПП на протяжении количества циклов колебаний"},
		{CDynaModel::Parameters::m_cszStopOnBranchOOS, "Завершать расчет при фиксации асинхронного режима по связи"},
		{CDynaModel::Parameters::m_cszStopOnGeneratorOOS, "Завершать расчет при фиксации асинхронного режима в генераторе"},
		{CDynaModel::Parameters::m_cszWorkingFolder, "Рабочий каталог"},
		{CDynaModel::Parameters::m_cszResultsFolder, "Каталог результатов"},
		{CDynaModel::Parameters::m_cszAdamsRingingSuppressionMode, "Метод подавления рингинга"},
		{CDynaModel::Parameters::m_cszFreqDampingType, "Расчет скольжения для демпфирования в уравнении движения"},
		{CDynaModel::Parameters::m_cszFileLogLevel, "Уровень подробности протокола в файл"},
		{CDynaModel::Parameters::m_cszConsoleLogLevel, "Уровень подробности протокола в консоль"},
		{CDynaModel::Parameters::m_cszParkParametersDetermination, "Метод расчета параметров моделей Парка"},
		{CDynaModel::Parameters::m_cszGeneratorLessLRC, "Вид СХН для учета генераторных узлов УР без генераторов"},
		{CDynaModel::Parameters::m_cszAdamsDampingSteps, "Количество шагов демпфирования Адамса"},
		{CDynaModel::Parameters::cszBusFrequencyEstimation_, "Метод оценки частоты на шине"},
		{CDynaModel::Parameters::cszChangeActionsAreCumulative, "Кумулятивные изменения параметров действиями автоматики и сценария"},
		{CDynaModel::Parameters::cszDebugModelNameTemplate, "Шаблон имени вывода отладочных файлов"},
		{CDynaModel::Parameters::cszDerlagToleranceMultiplier, "Множитель точности дифференцирующего звена"},
		{CDynaModel::Parameters::m_cszDontCheckTolOnMinStep, "Не проверять сходимость на минимальном шаге"},
		{CDynaModel::Parameters::m_cszHmax, "Максимальный шаг интегрирования"},
		{CDynaModel::Parameters::cszHysteresisAtol, "Абсолютная точность гистерезиса пороговых элементов"},
		{CDynaModel::Parameters::cszHysteresisRtol, "Относительная точность гистерезиса пороговых элементов"},
		{CDynaModel::Parameters::m_cszLRCSmoothingRange, "Диапазон сглаживания СХН"},
		{CDynaModel::Parameters::m_cszMaxPVPQSwitches, "УР: Допустимое количество переключений PV-PQ"},
		{CDynaModel::Parameters::cszMaxResultFilesCount, "Максимальное количество файлов результатов в каталоге"},
		{CDynaModel::Parameters::cszMaxResultFilesSize, "Максимальный суммарный размер файлов результатов в каталоге"},
		{CDynaModel::Parameters::m_cszNewtonMaxNorm, "Допустимая норма контроля сходимости метода Ньютона"},
		{CDynaModel::Parameters::m_cszPVPQSwitchPerIt, "УР: Максимальное количество переключений PV-PQ на итерации"},
		{CDynaModel::Parameters::m_cszSecuritySpinReference, "Уставка автомата безопасности СГ"},
		{CDynaModel::Parameters::cszShowAbsoluteAngles, "Сохранять абсолютные углы в режиме COI"},
		{CDynaModel::Parameters::cszStepsToOrderChange, "Количество шагов перед сменой порядка метода интегрирования"},
		{CDynaModel::Parameters::cszStepsToStepChange, "Количество шагов перед сменой шага метода интегрирования"},
		{CDynaModel::Parameters::cszUseCOI, "Использовать систему координат синхронной зоны"},
		{CDynaModel::Parameters::cszVarSearchStackDepth, "Глубина стека поиска переменных"},
		{CDynaModel::Parameters::cszDiffEquationType, "Метод для дифференциальных уравнений"},
		{CDynaModel::Parameters::cszShortCircuitShuntMethod_, "Метод расчета шунта КЗ для Uост"}
	};

	IColPtr spNewCol;
	for (const auto& field : *ps)
	{
		const auto& mv{ *field.second };
		_bstr_t name{ field.first.c_str() };
		switch (field.second->ValueType)
		{
		case TypedSerializedValue::eValueType::VT_BOOL:
		case TypedSerializedValue::eValueType::VT_STATE:
			spNewCol = spCols->Add(name, PR_BOOL);
			spNewCol->PutZ(0, *mv.Value.pBool);
			break;
		case TypedSerializedValue::eValueType::VT_DBL:
			spNewCol = spCols->Add(name, PR_REAL);
			spNewCol->PutProp(FL_MASH, mv.Multiplier);
			spNewCol->PutProp(FL_PREC, 6);
			spNewCol->PutZ(0, *mv.Value.pDbl);
		break;
		case TypedSerializedValue::eValueType::VT_ID:
		case TypedSerializedValue::eValueType::VT_INT:
			spNewCol = spCols->Add(name, PR_INT);
			spNewCol->PutZ(0, *mv.Value.pInt);
			break;
		case TypedSerializedValue::eValueType::VT_NAME:
		case TypedSerializedValue::eValueType::VT_STRING:
			spNewCol = spCols->Add(name, PR_STRING);
			spNewCol->PutZ(0, mv.Value.pStr->c_str());
			break;
		case TypedSerializedValue::eValueType::VT_ADAPTER:
			{
				const auto& adapter { static_cast<CSerializerAdapterBase*>(mv.Adapter.get()) };
				STRINGLIST EnumStrings;
				const ptrdiff_t OriginalIndex{ adapter->GetInt() };
				try
				{
					ptrdiff_t index{ 0 };
					while (true)
					{
						adapter->SetInt(index++);
						EnumStrings.emplace_back(adapter->GetString());
					}
				}
				catch (const dfw2error&) { }
				adapter->SetInt(OriginalIndex);

				spNewCol = spCols->Add(name, PR_ENUM);
				spNewCol->PutProp(FL_NAMEREF, fmt::format("{}", fmt::join(EnumStrings, "|")).c_str());
				spNewCol->PutZ(0, OriginalIndex);
			}
			break;
		default:
			throw dfw2error(fmt::format("CRastrImport::ReadRastrRow wrong serializer type {}", mv.ValueType));
		}
	
		if (spNewCol)
		{
			spNewCol->PutProp(FL_ZAG, name);

			if (auto it{ Descriptions.find(field.first) }; it != Descriptions.end())
				spNewCol->PutProp(FL_DESC, stringutils::utf8_decode(it->second).c_str());
			else
			{
				Network.Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("No description for \"{}\"", field.first));
				MissedDescriptions++;
			}

			Network.Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format("\"{}\" = {} // {}",
				stringutils::utf8_encode(spNewCol->Name),
				stringutils::utf8_encode(spNewCol->GetZS(0)),
				stringutils::utf8_encode(spNewCol->GetProp(FL_DESC).bstrVal)));
		}
	}

	if (MissedDescriptions > 0)
		MessageBox(NULL,
			stringutils::utf8_decode(fmt::format("There are {} missed descriptions", MissedDescriptions)).c_str(),
			stringutils::utf8_decode(CDFW2Messages::m_cszProjectName).c_str(),
			MB_ICONEXCLAMATION | MB_OK);
}


std::string CRastrImport::COMErrorDescription(const _com_error& error)
{
	_bstr_t desc{ error.Description() };
	if (desc.length() == 0)
		desc = fmt::format("HRESULT {:0x}", static_cast<unsigned long>(error.Error())).c_str();
	return stringutils::utf8_encode(std::wstring(desc));
}

/*
* Archived source for reference
* 
ITablePtr spGen = spTables->Item("Generator");
	IColsPtr spGenCols = spGen->Cols;

	IColPtr spGenId = spGenCols->Item("Id");
	IColPtr spGenName = spGenCols->Item("Name");
	IColPtr spGenSta = spGenCols->Item("sta");

	IColPtr spGenUnom = spGenCols->Item("Ugnom");
	IColPtr spGenPnom = spGenCols->Item("Pnom");
	IColPtr spGenCos  = spGenCols->Item("cosFi");
	IColPtr spGenNode = spGenCols->Item("Node");
	IColPtr spGenKgen = spGenCols->Item("NumBrand");
	IColPtr spGenModelType = spGenCols->Item("ModelType");

	IColPtr spGenDemp = spGenCols->Item("Demp");
	IColPtr spGenMj   = spGenCols->Item("Mj");
	IColPtr spGenXd = spGenCols->Item("xd");
	IColPtr spGenXd1  = spGenCols->Item("xd1");
	IColPtr spGenXq   = spGenCols->Item("xq");
	IColPtr spGenTd01 = spGenCols->Item("td01");
	IColPtr spExciterId = spGenCols->Item("ExciterId");

	IColPtr spGenP = spGenCols->Item("P");
	IColPtr spGenQ = spGenCols->Item("Q");

	IColPtr spGenQmin = spGenCols->Item("Qmin");
	IColPtr spGenQmax = spGenCols->Item("Qmax");

	IColPtr spGenXd2 = spGenCols->Item("xd2");
	IColPtr spGenXq1 = spGenCols->Item("xq1");
	IColPtr spGenXq2 = spGenCols->Item("xq2");
	IColPtr spGenTd0ss = spGenCols->Item("td02");
	IColPtr spGenTq0ss = spGenCols->Item("tq02");


	size_t nGen1CCount = 0;
	size_t nGen3CCount = 0;
	size_t nGenMustangCount = 0;
	size_t nGenMotCount = 0;
	size_t nGenInfBusCount = 0;

	for (int i = 0; i < spGen->Size; i++)
	{
		switch (spGenModelType->GetZ(i).lVal)
		{
		case 6:
			nGenMustangCount++;
			break;
		case 5:
			nGen3CCount++;
			break;
		case 4:
			nGen1CCount++;
			break;
		case 3:
			nGenMotCount++;
			break;
		case 2:
			nGenInfBusCount++;
			break;
		}
	}


	CDynaGenerator3C* pGens3C =			Network.Generators3C.CreateDevices<CDynaGenerator3C>(nGen3CCount);
	CDynaGeneratorMustang* pGensMu =	Network.GeneratorsMustang.CreateDevices<CDynaGeneratorMustang>(nGenMustangCount);
	CDynaGenerator1C* pGens1C =			Network.Generators1C.CreateDevices<CDynaGenerator1C>(nGen1CCount);
	CDynaGeneratorMotion* pGensMot =	Network.GeneratorsMotion.CreateDevices<CDynaGeneratorMotion>(nGenMotCount);
	CDynaGeneratorInfBus* pGensInf =	Network.GeneratorsInfBus.CreateDevices<CDynaGeneratorInfBus>(nGenInfBusCount);

	for (int i = 0; i < spGen->Size; i++)
	{
		switch (spGenModelType->GetZ(i).lVal)
		{
		case 6:
			pGensMu->SetId(spGenId->GetZ(i));
			pGensMu->SetName(stringutils::utf8_encode(spGenName->GetZ(i).bstrVal));
			pGensMu->SetState(spGenSta->GetZ(i).boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
			pGensMu->NodeId = spGenNode->GetZ(i);
			pGensMu->Kdemp = spGenDemp->GetZ(i);
			pGensMu->Kgen = spGenKgen->GetZ(i);
			pGensMu->P = spGenP->GetZ(i);
			pGensMu->Q = spGenQ->GetZ(i);
			pGensMu->cosPhinom = spGenCos->GetZ(i);
			pGensMu->Unom = spGenUnom->GetZ(i);
			pGensMu->Mj = spGenMj->GetZ(i);
			pGensMu->xd1 = spGenXd1->GetZ(i);
			pGensMu->xq = spGenXq->GetZ(i);
			pGensMu->xd = spGenXd->GetZ(i);
			pGensMu->Td01 = spGenTd01->GetZ(i);
			pGensMu->Pnom = spGenPnom->GetZ(i);
			pGensMu->m_ExciterId = spExciterId->GetZ(i);
			pGensMu->xd2 = spGenXd2->GetZ(i);
			pGensMu->xq1 = spGenXq1->GetZ(i);
			pGensMu->xq2 = spGenXq2->GetZ(i);
			pGensMu->Td0ss = spGenTd0ss->GetZ(i);
			pGensMu->Tq0ss = spGenTq0ss->GetZ(i);
			pGensMu->LFQmax = spGenQmax->GetZ(i);
			pGensMu->LFQmin = spGenQmin->GetZ(i);
			pGensMu++
			break;
		case 5:
			pGens3C->SetId(spGenId->GetZ(i));
			pGens3C->SetName(stringutils::utf8_encode(spGenName->GetZ(i).bstrVal));
			pGens3C->SetState(spGenSta->GetZ(i).boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
			pGens3C->NodeId = spGenNode->GetZ(i);
			pGens3C->Kdemp = spGenDemp->GetZ(i);
			pGens3C->Kgen = spGenKgen->GetZ(i);
			pGens3C->P = spGenP->GetZ(i);
			pGens3C->Q = spGenQ->GetZ(i);
			pGens3C->cosPhinom = spGenCos->GetZ(i);
			pGens3C->Unom = spGenUnom->GetZ(i);
			pGens3C->Mj = spGenMj->GetZ(i);
			pGens3C->xd1 = spGenXd1->GetZ(i);
			pGens3C->xq = spGenXq->GetZ(i);
			pGens3C->xd = spGenXd->GetZ(i);
			pGens3C->Td01 = spGenTd01->GetZ(i);
			pGens3C->Pnom = spGenPnom->GetZ(i);
			pGens3C->m_ExciterId = spExciterId->GetZ(i);
			pGens3C->xd2 = spGenXd2->GetZ(i);
			pGens3C->xq1 = spGenXq1->GetZ(i);
			pGens3C->xq2 = spGenXq2->GetZ(i);
			pGens3C->Td0ss = spGenTd0ss->GetZ(i);
			pGens3C->Tq0ss = spGenTq0ss->GetZ(i);
			pGens3C->LFQmax = spGenQmax->GetZ(i);
			pGens3C->LFQmin = spGenQmin->GetZ(i);
			pGens3C++;
			break;
		case 4:
			pGens1C->SetId(spGenId->GetZ(i));
			pGens1C->SetName(stringutils::utf8_encode(spGenName->GetZ(i).bstrVal));
			pGens1C->SetState(spGenSta->GetZ(i).boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
			pGens1C->NodeId = spGenNode->GetZ(i);
			pGens1C->Kgen = spGenKgen->GetZ(i);
			pGens1C->Kdemp = spGenDemp->GetZ(i);
			pGens1C->P = spGenP->GetZ(i);
			pGens1C->Q = spGenQ->GetZ(i);
			pGens1C->cosPhinom = spGenCos->GetZ(i);
			pGens1C->Unom = spGenUnom->GetZ(i);
			pGens1C->Mj = spGenMj->GetZ(i);
			pGens1C->xd1 = spGenXd1->GetZ(i);
			pGens1C->xq = spGenXq->GetZ(i);
			pGens1C->xd = spGenXd->GetZ(i);
			pGens1C->Td01 = spGenTd01->GetZ(i);
			pGens1C->Pnom = spGenPnom->GetZ(i);
			pGens1C->m_ExciterId = spExciterId->GetZ(i);
			pGens1C->LFQmax = spGenQmax->GetZ(i);
			pGens1C->LFQmin = spGenQmin->GetZ(i);
			pGens1C++;
			break;
		case 2:
			pGensInf->SetId(spGenId->GetZ(i));
			pGensInf->SetName(stringutils::utf8_encode(spGenName->GetZ(i).bstrVal));
			pGensInf->SetState(spGenSta->GetZ(i).boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
			pGensInf->NodeId = spGenNode->GetZ(i);
			pGensInf->Kgen = spGenKgen->GetZ(i);
			pGensInf->P = spGenP->GetZ(i);
			pGensInf->Q = spGenQ->GetZ(i);
			pGensInf->xd1 = spGenXd1->GetZ(i);
			pGensInf->LFQmax = spGenQmax->GetZ(i);
			pGensInf->LFQmin = spGenQmin->GetZ(i);
			pGensInf++;
			break;
		case 3:
			pGensMot->SetId(spGenId->GetZ(i));
			pGensMot->SetName(stringutils::utf8_encode(spGenName->GetZ(i).bstrVal));
			pGensMot->SetState(spGenSta->GetZ(i).boolVal ? eDEVICESTATE::DS_OFF : eDEVICESTATE::DS_ON, eDEVICESTATECAUSE::DSC_EXTERNAL);
			pGensMot->NodeId = spGenNode->GetZ(i);
			pGensMot->Kgen = spGenKgen->GetZ(i);
			pGensMot->Kdemp = spGenDemp->GetZ(i);
			pGensMot->P = spGenP->GetZ(i);
			pGensMot->Q = spGenQ->GetZ(i);
			pGensMot->cosPhinom = spGenCos->GetZ(i);
			pGensMot->Unom = spGenUnom->GetZ(i);
			pGensMot->Mj = spGenMj->GetZ(i);
			pGensMot->xd1 = spGenXd1->GetZ(i);
			pGensMot->Pnom = spGenPnom->GetZ(i);
			pGensMot->LFQmax = spGenQmax->GetZ(i);
			pGensMot->LFQmin = spGenQmin->GetZ(i);
			pGensMot++;
		}
	}
*/


/*

ITablePtr spLRC = spTables->Item(L"polin");
IColsPtr spLRCCols = spLRC->Cols;

IColPtr spLCLCId = spLRCCols->Item(L"nsx");
IColPtr spP0 = spLRCCols->Item(L"p0");
IColPtr spP1 = spLRCCols->Item(L"p1");
IColPtr spP2 = spLRCCols->Item(L"p2");
IColPtr spP3 = spLRCCols->Item(L"p3");
IColPtr spP4 = spLRCCols->Item(L"p4");

IColPtr spQ0 = spLRCCols->Item(L"q0");
IColPtr spQ1 = spLRCCols->Item(L"q1");
IColPtr spQ2 = spLRCCols->Item(L"q2");
IColPtr spQ3 = spLRCCols->Item(L"q3");
IColPtr spQ4 = spLRCCols->Item(L"q4");

IColPtr spLCUmin = spLRCCols->Item(L"umin");
IColPtr spLCFreq = spLRCCols->Item(L"frec");

DBSLC *pLRCBuffer = new DBSLC[spLRC->GetSize()];
for (int i = 0; i < spLRC->GetSize(); i++)
{
	DBSLC *pSLC = pLRCBuffer + i;
	pSLC->m_Id = spLCLCId->GetZ(i).lVal;
	pSLC->Vmin = spLCUmin->GetZ(i).dblVal;
	pSLC->P0 = spP0->GetZ(i).dblVal;
	pSLC->P1 = spP1->GetZ(i).dblVal;
	pSLC->P2 = spP2->GetZ(i).dblVal;
	pSLC->Q0 = spQ0->GetZ(i).dblVal;
	pSLC->Q1 = spQ1->GetZ(i).dblVal;
	pSLC->Q2 = spQ2->GetZ(i).dblVal;
}

CreateLRCFromDBSLCS(Network, pLRCBuffer, spLRC->GetSize());
delete [] pLRCBuffer;

ITablePtr spNode = spTables->Item("node");
IColsPtr spNodeCols = spNode->Cols;

//CDynaNode *pNodes = new CDynaNode[spNode->Size];

IColPtr spLCIdLF = spNodeCols->Item("nsx");
IColPtr spLCId = spNodeCols->Item("dnsx");

Network.Nodes.CreateDevices(spNode->Size);




auto pNodes = Network.Nodes.begin();
auto pSerializer = (*pNodes)->GetSerializer();

for (auto&& sv : *pSerializer)
	if(!sv.second->bState)
		sv.second->pAux = std::make_unique<CSerializedValueAuxDataRastr>(spNodeCols->Item(sv.first.c_str()));

for (int i = 0; i < spNode->Size; i++)
{
	(*pNodes)->UpdateSerializer(pSerializer);
	ReadRastrRow(pSerializer, i);
	CDynaLRC *pDynLRC;
	if (Network.LRCs.GetDevice(spLCId->GetZ(i), pDynLRC))
	{
		static_cast<CDynaNode*>(*pNodes)->m_pLRC = pDynLRC;
		//if (!LcId && pNodes->V > 0.0)
		//	pNodes->m_dLRCKdef = (pNodes->Unom * pNodes->Unom / pNodes->V / pNodes->V);
	}

	ptrdiff_t nLRCLF = spLCIdLF->GetZ(i);
	if (nLRCLF > 0)
	{
		if (Network.LRCs.GetDevice(nLRCLF, pDynLRC))
			static_cast<CDynaNode*>(*pNodes)->m_pLRCLF = pDynLRC;
	}
	pNodes++;
}

*/

/*
bool CRastrImport::CreateLRCFromDBSLCS(CDynaModel& Network, DBSLC *pLRCBuffer, ptrdiff_t nLRCCount)
{
	bool bRes = true;
	CSLCLoader slcloader;
	SLCSITERATOR slit;

	double Vmin = Network.GetLRCToShuntVmin();

	for (ptrdiff_t i = 0; i < nLRCCount; i++)
	{
		DBSLC *pSLC = pLRCBuffer + i;
		if (pSLC->m_Id == 1 || pSLC->m_Id == 2)
		{
			Network.Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszLRC1And2Reserved,pSLC->m_Id));
			continue;
		}

		slit = slcloader.find(pSLC->m_Id);
		if (slit == slcloader.end())
			slit = slcloader.insert(std::make_pair(pSLC->m_Id, new CStorageSLC())).first;

		CSLCPolynom P(pSLC->Vmin, pSLC->P0, pSLC->P1, pSLC->P2);
		CSLCPolynom Q(pSLC->Vmin, pSLC->Q0, pSLC->Q1, pSLC->Q2);

		if (!P.IsEmpty())
			slit->second->P.push_back(P);
		if (!Q.IsEmpty())
			slit->second->Q.push_back(Q);
	}

	for (slit = slcloader.begin(); slit != slcloader.end();)
	{
		if (!slit->second->P.size() && !slit->second->Q.size())
		{
			delete slit->second;
			slcloader.erase(slit++);
		}
		else
			slit++;
	}

	SLCPOLYITR  itpoly;

	for (slit = slcloader.begin(); slit != slcloader.end(); slit++)
	{
		for (int pq = 0; pq < 2; pq++)
		{
			SLCPOLY& poly = pq ? slit->second->Q : slit->second->P;

			poly.sort();

			if (poly.size())
			{
				double Vbeg = poly.front().m_kV;
				if (Vbeg > 0.0)
				{
					Network.Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszLRCStartsNotFrom0, slit->first, Vbeg));
					poly.front().m_kV = 0.0;
				}
			}

			itpoly = poly.begin();
			bool bAmbiguityWarned = false;
			while (itpoly != poly.end())
			{
				SLCPOLYITR itpolyNext = itpoly;
				itpolyNext++;
				if (itpolyNext != poly.end())
				{
					if (Equal(itpoly->m_kV, itpolyNext->m_kV))
					{
						if (!bAmbiguityWarned)
						{
							if (!itpoly->CompareWith(*itpolyNext))
							{
								Network.Log(CDFW2Messages::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszAmbigousLRCSegment, slit->first,
																														itpoly->m_kV,
																														itpoly->m_k0,
																														itpoly->m_k1,
																														itpoly->m_k2));
								bAmbiguityWarned = true;
							}
						}
						poly.erase(itpolyNext);
					}
					itpoly++;
				}
				else
					break;
			}
		}
	}

	// типовые СХН Rastr 1 и 2
	if (slcloader.find(1) == slcloader.end())
	{
		slit = slcloader.insert(std::make_pair(1, new CStorageSLC())).first;
		slit->second->P.push_back(CSLCPolynom(0.0, 0.83, -0.3, 0.47));
		slit->second->Q.push_back(CSLCPolynom(0.0, 0.721, 0.15971, 0.0));
		slit->second->Q.push_back(CSLCPolynom(0.815, 3.7, -7.0, 4.3));
		slit->second->Q.push_back(CSLCPolynom(1.2, 1.492, 0.0, 0.0));
	}
	if (slcloader.find(2) == slcloader.end())
	{
		slit = slcloader.insert(std::make_pair(2, new CStorageSLC())).first;
		slit->second->P.push_back(CSLCPolynom(0.0, 0.83, -0.3, 0.47));
		slit->second->Q.push_back(CSLCPolynom(0.0, 0.657, 0.159135, 0.0));
		slit->second->Q.push_back(CSLCPolynom(0.815, 4.9, -10.1, 6.2));
		slit->second->Q.push_back(CSLCPolynom(1.2, 1.708, 0.0, 0.0));
	}

	// СХН шунт с Id=0
	slit = slcloader.insert(std::make_pair(0, new CStorageSLC())).first;
	slit->second->P.push_back(CSLCPolynom(0.0, 0.0, 0.0, 1.0));
	slit->second->Q.push_back(CSLCPolynom(0.0, 0.0, 0.0, 1.0));

	// СХН с постоянной мощностью с Id=-1
	slit = slcloader.insert(std::make_pair(-1, new CStorageSLC())).first;
	slit->second->P.push_back(CSLCPolynom(0.0, 1.0, 0.0, 0.0));
	slit->second->Q.push_back(CSLCPolynom(0.0, 1.0, 0.0, 0.0));

	// insert shunt LRC
	for (slit = slcloader.begin(); slit != slcloader.end(); slit++)
	{
		bRes = bRes && slit->second->P.InsertLRCToShuntVmin(Vmin);
		bRes = bRes && slit->second->Q.InsertLRCToShuntVmin(Vmin);
	}

	if (bRes)
	{
		// переписываем СХН из загрузчика в котейнер СХН

		CDynaLRC* pLRCs = Network.LRCs.CreateDevices<CDynaLRC>(slcloader.size());
		CDynaLRC* pLRC(pLRCs);

		for (slit = slcloader.begin(); slit != slcloader.end(); slit++, pLRC++)
		{
			pLRC->SetId(slit->first);
			pLRC->SetNpcs(slit->second->P.size(), slit->second->Q.size());

			CLRCData *pData = &pLRC->P[0];

			std::cout << pLRC->GetId() << std::endl;

			for (auto&& itpoly : slit->second->P)
			{
				pData->V	=	itpoly.m_kV;
				pData->a0   =	itpoly.m_k0;
				pData->a1	=	itpoly.m_k1;
				pData->a2	=	itpoly.m_k2;

				std::cout << "P " << pData->V << " " << pData->a0 << " " << pData->a1 << " " << pData->a2 << std::endl;

				pData++;
			}

			pData = &pLRC->Q[0];

			for (auto&& itpoly : slit->second->Q)
			{
				pData->V	=	itpoly.m_kV;
				pData->a0	=	itpoly.m_k0;
				pData->a1	=	itpoly.m_k1;
				pData->a2	=	itpoly.m_k2;

				std::cout << "Q " << pData->V << " " << pData->a0 << " " << pData->a1 << " " << pData->a2 << std::endl;

				pData++;
			}
		}
	}
	return bRes;
}

// вставляет в СХН сегмент шунта от нуля до Vmin
bool SLCPOLY::InsertLRCToShuntVmin(double Vmin)
{
	bool bRes = true;
	if (Vmin > 0.0)
	{
		// сортируем сегменты по напряжению
		sort();
		double LrcV = 0.0;
		bool bInsert = false;

		// обходим сегменты справа, для первого найденного с напряжением
		// меньше чем Vmin считаем мощность по этом сегменту от Vmin
		// и меняем ему напряжение на Vmin
		for (SLCPOLYRITR itrpoly = rbegin(); itrpoly != rend(); itrpoly++)
		{
			if (itrpoly->m_kV < Vmin)
			{
				LrcV = itrpoly->m_k0 + Vmin * itrpoly->m_k1 + Vmin * Vmin * itrpoly->m_k2;
				itrpoly->m_kV = Vmin;
				bInsert = true;
				break;
			}
		}
		if (bInsert)
		{
			// если нашли сегмент для Vmin удаляем все сегменты
			// с напряжением меньше чем Vmin
			while (size())
			{
				SLCPOLYITR itpoly = begin();
				if (itpoly->m_kV < Vmin)
					erase(itpoly);
				else
					break;
			}
			// вставляем новый сегмент шунта от нуля
			insert(begin(),CSLCPolynom(0.0, 0.0, 0.0, LrcV / Vmin / Vmin));
		}

		// снова сортируем по напряжению
		sort();
		SLCPOLYITR itpoly = begin();

		// ищем последовательные сегменты с одинаковыми коэффициентами
		// и удаляем их как изыбыточные
		while(itpoly != end())
		{
			SLCPOLYITR itpolyNext = itpoly;
			itpolyNext++;
			if (itpolyNext != end())
			{
				if (itpoly->CompareWith(*itpolyNext))
					erase(itpolyNext);
				itpoly++;
			}
			else
				break;
		}
	}
	return bRes;
}
*/

