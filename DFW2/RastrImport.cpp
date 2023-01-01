#include "stdafx.h"
#include "stringutils.h"
#include "RastrImport.h"
#include "DynaGeneratorMustang.h"
#include "DynaGeneratorInfBus.h"
#include "DynaExciterMustang.h"
#include "DynaDECMustang.h"
#include "DynaExcConMustang.h"
#include "DynaBranch.h"

using namespace DFW2;
//cl /LD /EHsc -DUNICODE -D_UNICODE customdevice.cpp dllmain.cpp


CRastrImport::CRastrImport(IRastrPtr spRastr)
{
	m_spRastr = spRastr;
	InitRastrWin();
}

bool GetConstFromField(const ConstVarsInfo& VarsInfo)
{
	bool bRes = false;
	if (!(VarsInfo.VarFlags & (CVF_INTERNALCONST)) && VarsInfo.eDeviceType == DEVTYPE_UNKNOWN)
		bRes = true;
	return bRes;
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
		catch (_com_error& err)
		{
			Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszTableNotFoundForCustomDevice,
				stringutils::utf8_encode(CustomDeviceContainer.DLL()->GetModuleFilePath().c_str()),
				static_cast<const char*>(err.Description())));
		}
	}
	return bRes;
}


bool CRastrImport::GetCustomDeviceData(CDynaModel& Network, IRastrPtr spRastr, CustomDeviceConnectInfo& ConnectInfo, CCustomDeviceContainer& CustomDeviceContainer)
{
	bool bRes = false;
	if (spRastr)
	{
		try
		{
			ITablePtr spSourceTable = spRastr->Tables->Item(ConnectInfo.m_TableName.c_str());
			IColsPtr spSourceCols = spSourceTable->Cols;
			const DFW2::CCustomDeviceDLL& DLL = CustomDeviceContainer.DLL();

			// get and check constants fields from storage
			size_t nConstsCount = DLL.GetConstsInfo().size();
			typedef std::vector<std::pair<IColPtr,ptrdiff_t> > COLVECTOR;
			typedef COLVECTOR::iterator COLITR;
			COLVECTOR Cols;
			Cols.reserve(nConstsCount);

			IColPtr spColId		= spSourceCols->Item(L"Id");
			IColPtr spColName	= spSourceCols->Item(L"Name");


			ptrdiff_t ConstIndex(0);
			for (const auto& it : DLL.GetConstsInfo())
				if (GetConstFromField(it))
					Cols.push_back(std::make_pair(spSourceCols->Item(it.VarInfo.Name), ConstIndex++));


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
				
				
				CCustomDevice *pCustomDevices = Network.CustomDevice.CreateDevices<CCustomDevice>(nModelsCount);
				Network.CustomDevice.BuildStructure();

				// put constants to each model
				long nModelIndex = 0;
				for (nTableIndex = 0 ; nTableIndex < nTableSize; nTableIndex++)
				{
					if (spModelType->GetZ(nTableIndex).lVal == ConnectInfo.m_nModelType)
					{
						if (nModelIndex >= nModelsCount)
						{
							Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszDLLBadBlocks, CustomDeviceContainer.DLL().GetModuleFilePath()));
							break;
						}

						CCustomDevice *pDevice = pCustomDevices + nModelIndex;
						if (pDevice->SetConstDefaultValues())
						{
							pDevice->SetId(spColId->GetZ(nTableIndex).lVal);
							pDevice->SetName(static_cast<const char*>(spColId->GetZS(nTableIndex)));
							for (COLITR cit = Cols.begin(); cit != Cols.end(); cit++)
							{
								if (!pDevice->SetConstValue(cit->second, cit->first->GetZ(nTableIndex).dblVal))
								{
									Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszDLLBadBlocks, CustomDeviceContainer.DLL().GetModuleFilePath()));
									break;
								}
							}
						}
						else
						{
							Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszDLLBadBlocks, CustomDeviceContainer.DLL().GetModuleFilePath()));
							break;
						}
						nModelIndex++;
					}
				}
			}
		}
		catch (_com_error &err)
		{
			Network.Log(DFW2::DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszTableNotFoundForCustomDevice,
																CustomDeviceContainer.DLL().GetModuleFilePath(), 
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
		variant_t vt = { dataProp->m_spCol->GetZ(Row) };
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

	rstPath.append(L"динамика.rst");
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
	//LoadFile("e:\\temp\\sztest\\deep\\test9.rst", rstPath.c_str());				// исходный режим
	//m_spRastr->NewFile(dfwPath.c_str());
	//m_spRastr->NewFile(scnPath.c_str());
	//LoadFile("e:\\temp\\sztest\\РМ_mdp_debug_1_111_027440");		// режим с нарушением устойчивости(сценарии 102, 109)
	//LoadFile("e:\\temp\\sztest\\РМ_mdp_debug_1_111_m004560");		// последний режим с нарушением устойчивости, перед предельным(сценарии 109)
	LoadFile("e:\\temp\\sztest\\РМ_mdp_debug_1_111_m005560_noarv");		// режим без нарушения устойчивости, предельный

	m_spRastr->NewFile(dfwPath.c_str());
	m_spRastr->NewFile(scnPath.c_str());

	//LoadFile("e:\\temp\\sztest\\102_1ф.КЗ с УРОВ на КАЭС (откл. КАЭС - Княжегубская №1).dfw", dfwPath.c_str());
	LoadFile("e:\\temp\\sztest\\109_ 1ф. КЗ с УРОВ на КАЭС с отключением ВЛ 330 кВ Кольская АЭС - Мончегорск №2.dfw", dfwPath.c_str());
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
	spTimeComputed->PutZ(0, Network.GetCurrentTime());

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
	if (long nTableIndex{ spTables->GetFind("RaidenParameters") }; nTableIndex >= 0)
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

	ps->at(CDynaModel::Parameters::m_cszProcessDuration)->SetDouble(spDuration->GetZ(0).dblVal);
	ps->at(CDynaModel::Parameters::m_cszLRCToShuntVmin)->SetDouble(spLTC2Y->GetZ(0).dblVal);
	ps->at(CDynaModel::Parameters::m_cszFrequencyTimeConstant)->SetDouble(spFreqT->GetZ(0).dblVal);
	ps->at(CDynaModel::Parameters::m_cszConsiderDampingEquation)->SetBool(spDamping->GetZ(0).lVal ? true : false);
	ps->at(CDynaModel::Parameters::m_cszParkParametersDetermination)->SetInt(spParkParams->GetZ(0).lVal ? 0 : 1);

	/*if (!Network.CustomDevice.ConnectDLL("DeviceDLL.dll"))
		return;
		*/

	m_rastrSynonyms
		.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameLRC, "polin")
		.AddFieldSynonyms("LRCId", "nsx")
		.AddFieldSynonyms("Umin", "umin")
		.AddFieldSynonyms("Freq", "frec");


	m_rastrSynonyms
		.AddRastrSynonym("Node", "node")
		.AddFieldSynonyms("LRCLFId", "nsx")
		.AddFieldSynonyms("LRCTransId", "dnsx")
		.AddFieldTransformer("tip", new CRastrNodeTypeEnumTransformer()); // задаем трансформатор типа узла

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameBranch, "vetv");
	m_rastrSynonyms
		.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGeneratorInfPower, "Generator")
		.AddFieldSynonyms("Kgen", "NumBrand")
		.AddFieldSynonyms("Kdemp", "Demp")
		.AddFieldSynonyms("cosPhinom", "cosFi")
		.AddFieldSynonyms("Unom", "Ugnom");

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameReactor, "Reactors")
		.AddFieldSynonyms("HeadNode", "Id1")
		.AddFieldSynonyms("TailNode", "Id2")
		.AddFieldSynonyms("ParallelBranch", "Id3")
		.AddFieldSynonyms(CSerializerBase::m_cszType, "tip")
		.AddFieldSynonyms("Placement", "Pr_vikl");

	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGeneratorMotion, "Generator");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGenerator1C, "Generator");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGenerator3C, "Generator");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGeneratorMustang, "Generator");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGeneratorPark3C, "Generator");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGeneratorPark4C, "Generator");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameGeneratorPowerInjector, "Generator");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameExciterMustang, "Exciter");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameDECMustang, "Forcer");
	m_rastrSynonyms.AddRastrSynonym(CDeviceContainerProperties::m_cszSysNameExcConMustang, "ExcControl");


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
	ReadTable(Network.GeneratorsPowerInjector, "ModelType=0");
	ReadTable(Network.ExcitersMustang);
	ReadTable(Network.DECsMustang);
	ReadTable(Network.ExcConMustang);
	

	ITablePtr spAutoStarters = spTables->Item("DFWAutoStarter");
	IColsPtr spASCols = spAutoStarters->Cols;
	IColPtr spASId			= spASCols->Item(L"Id");
	IColPtr spASName		= spASCols->Item(L"Name");
	IColPtr spASType		= spASCols->Item(L"Type");
	IColPtr spASExpr		= spASCols->Item(L"Formula");
	IColPtr spASObjClass	= spASCols->Item(L"ObjectClass");
	IColPtr spASObjKey		= spASCols->Item(L"ObjectKey");
	IColPtr spASObjProp		= spASCols->Item(L"ObjectProp");


	for (int i = 0; i < spAutoStarters->GetSize(); i++)
	{
		Network.Automatic().AddStarter(spASType->GetZ(i),
			spASId->GetZ(i).lVal,
			stringutils::utf8_encode(spASName->GetZ(i).bstrVal),
			stringutils::utf8_encode(spASExpr->GetZ(i).bstrVal),
			0,
			stringutils::utf8_encode(spASObjClass->GetZ(i).bstrVal),
			stringutils::utf8_encode(spASObjKey->GetZ(i).bstrVal),
			stringutils::utf8_encode(spASObjProp->GetZ(i).bstrVal));
	}

	ITablePtr spAutoLogic = spTables->Item("DFWAutoLogic");
	IColsPtr spALCols = spAutoLogic->Cols;
	IColPtr spALId			= spALCols->Item(L"Id");
	IColPtr spALName		= spALCols->Item(L"Name");
	IColPtr spALType		= spALCols->Item(L"Type");
	IColPtr spALExpr		= spALCols->Item(L"Formula");
	IColPtr spALActions		= spALCols->Item(L"Actions");
	IColPtr spALDelay		= spALCols->Item(L"Delay");
	IColPtr spALOutputMode	= spALCols->Item(L"OutputMode");

	for (int i = 0; i < spAutoLogic->GetSize(); i++)
	{
		Network.Automatic().AddLogic(spALType->GetZ(i),
			spALId->GetZ(i).lVal,
			stringutils::utf8_encode(spALName->GetZ(i).bstrVal),
			stringutils::utf8_encode(spALExpr->GetZ(i).bstrVal),
			stringutils::utf8_encode(spALActions->GetZ(i).bstrVal),
			stringutils::utf8_encode(spALDelay->GetZ(i).bstrVal),
			spALOutputMode->GetZ(i).lVal);
	}


	ITablePtr spAutoActions = spTables->Item("DFWAutoAction");
	IColsPtr spAACols = spAutoActions->Cols;
	IColPtr spAAId = spAACols->Item(L"Id");
	IColPtr spAAName = spAACols->Item(L"Name");
	IColPtr spAAType = spAACols->Item(L"Type");
	IColPtr spAAExpr = spAACols->Item(L"Formula");
	IColPtr spAAObjClass = spAACols->Item(L"ObjectClass");
	IColPtr spAAObjKey = spAACols->Item(L"ObjectKey");
	IColPtr spAAObjProp = spAACols->Item(L"ObjectProp");
	IColPtr spAAOutputMode = spAACols->Item(L"OutputMode");
	IColPtr spAAActionGroup = spAACols->Item(L"ParentId");
	IColPtr spAAORunsCount = spAACols->Item(L"RunsCount");
	
	for (int i = 0; i < spAutoActions->GetSize(); i++)
	{
		Network.Automatic().AddAction(spAAType->GetZ(i),
			spAAId->GetZ(i).lVal,
			stringutils::utf8_encode(spAAName->GetZ(i).bstrVal),
			stringutils::utf8_encode(spAAExpr->GetZ(i).bstrVal),
			0,
			stringutils::utf8_encode(spAAObjClass->GetZ(i).bstrVal),
			stringutils::utf8_encode(spAAObjKey->GetZ(i).bstrVal),
			stringutils::utf8_encode(spAAObjProp->GetZ(i).bstrVal),
			spAAActionGroup->GetZ(i).lVal,
			spAAOutputMode->GetZ(i).lVal,
			spAAORunsCount->GetZ(i).lVal);
	}

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