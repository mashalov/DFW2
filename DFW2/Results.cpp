#include "stdafx.h"
#include "DynaModel.h"
#include "TaggedPath.h"
#include "FolderClean.h"
using namespace DFW2;


std::filesystem::path CDynaModel::CreateResultFilePath(std::string_view FileNameMask, const std::filesystem::path& Path)
{
	long Counter{ 0 };
	while (true)
	{
		std::filesystem::path CheckPath{ Path };
		CheckPath.append(fmt::format(FileNameMask, Counter));
		if (!std::filesystem::exists(CheckPath))
			return CheckPath;
		else
			Counter++;
	}
}

void CDynaModel::WriteResultsHeader()
{
	if (m_Parameters.m_bDisableResultsWriter)
	{
		Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszResultWriterDisabled);
		return;
	}

	auto resultPath(Platform().Results());
	std::filesystem::path parametersResultPath(stringutils::utf8_decode(m_Parameters.ResultsFolder_));

	// если в параметрах задан абосолютный путь к каталогу
	// результатов - используем его как есть,
	// если путь относительный - достраиваем его к стандартному каталогу

	if (parametersResultPath.is_absolute())
		resultPath = parametersResultPath;
	else
		resultPath.append(stringutils::utf8_decode(m_Parameters.ResultsFolder_));

	if (!(resultPath.has_filename() || resultPath.has_extension()))
	{
		// задан путь без имени файла результатов, генерируем имя
		//resultPath = CreateResultFilePath("Raiden_{:05d}.sna", resultPath);
		resultPath.append("binresultCOM.rst");
	}

	// путь к файлу и сам файл создаем
	// с помощью пути с тегами
	TaggedPath resultFilePath{ resultPath };
	resultFilePath.Create().close();
	ResultFilePath_ = stringutils::utf8_decode(resultFilePath.PathString());
	CFolderClean FolderClean(ResultFilePath_, m_Parameters.MaxResultFilesCount_, m_Parameters.MaxResultFilesSize_);
	FolderClean.SetReportFunction([this](const std::string_view& Message) { Log(DFW2MessageStatus::DFW2LOG_INFO, Message); });
	FolderClean.Clean();

	CResultsWriterBase::ResultsInfo resultsInfo { 0.0 * Atol(), ModelName() };
	m_ResultsWriter.CreateFile(ResultFilePath_, resultsInfo);
	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszResultFileCreated, resultFilePath.PathString()));

	// добавляем описание единиц измерения переменных
	CDFW2Messages vars;
	for (const auto& vn : vars.VarNameMap())
		m_ResultsWriter.AddVariableUnit(vn.first, vn.second);


	for (const auto& container : DeviceContainers_)
	{
		// проверяем, нужно ли записывать данные для такого типа контейнера
		if (!ApproveContainerToWriteResults(container)) continue;
		// если записывать надо - добавляем тип устройства контейнера
		m_ResultsWriter.AddDeviceType(*container);
	}

	m_ResultsWriter.FinishWriteHeader();

	long nIndex = 0;

	for (const auto& container : DeviceContainers_)
	{
		// собираем углы генераторов для детектора затухания колебаний
		if (m_Parameters.m_bAllowDecayDetector && container->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_MOTION))
		{
			ptrdiff_t deltaIndex(container->GetVariableIndex(CDynaNodeBase::cszDelta_));
			if (deltaIndex >= 0)
				for (const auto& device : *container)
					m_OscDetector.add_value_pointer(device->GetVariableConstPtr(deltaIndex));
		}

		// устанавливаем адреса, откуда ResultWrite будет забирать значения
		// записываемых переменных

		// проверяем нужно ли писать данные от этого контейнера
		if (!ApproveContainerToWriteResults(container)) continue;

		for (const auto& device : *container)
		{
			if (device->IsPermanentOff())
				continue;

			long nVarIndex(0);
			for (const auto& variable : container->ContainerProps().VarMap_)
			{
				if (variable.second.Output_)
				{
					m_ResultsWriter.SetChannel(device->GetId(),
						device->GetType(),
						nVarIndex++,
						device->GetVariablePtr(variable.second.Index_),
						nIndex++);
				}
			}

			for (const auto& variable : container->ContainerProps().ConstVarMap_)
			{
				if (variable.second.Output_)
				{
					m_ResultsWriter.SetChannel(device->GetId(),
						device->GetType(),
						nVarIndex++,
						device->GetConstVariablePtr(variable.second.Index_),
						nIndex++);
				}
			}
		}
	}

	sc.m_MaxBranchAngle.Reset();
	sc.m_MaxGeneratorAngle.Reset();

	TimeWritten_ = -m_Parameters.m_dOutStep - Hmin();
;
}

void CDynaModel::WriteResults()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	const double CurrentTime{ GetCurrentIntegrationTime() };
	if (sc.m_bEnforceOut || CurrentTime >= TimeWritten_ + m_Parameters.m_dOutStep)
	{
		// пытаемся сделать начальное t0 = 0.0 если оно CurrentTime пришло с небольшой погрешностью
		const auto TimeToWrite{ std::abs(CurrentTime) <= Hmin() ? 0.0 : (std::max)(CurrentTime, TimeWritten_)};
		m_ResultsWriter.WriteResults({ TimeToWrite, H() });
		TimeWritten_ = TimeToWrite;
		sc.m_bEnforceOut = false;
	}
}

void CDynaModel::FinishWriteResults()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	m_ResultsWriter.FinishWriteResults();
}

void CDynaModel::WriteSlowVariable(ptrdiff_t nDeviceType,
	const ResultIds& DeviceIds,
	const std::string_view VariableName,
	double Value,
	double PreviousValue,
	const std::string_view ChangeDescription)
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	m_ResultsWriter.AddSlowVariable(nDeviceType,
		DeviceIds,
		VariableName,
		GetCurrentIntegrationTime(),
		Value,
		PreviousValue,
		ChangeDescription);
}