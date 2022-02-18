#include "stdafx.h"
#include "DynaModel.h"
using namespace DFW2;

void CDynaModel::WriteResultsHeader()
{
	if (m_Parameters.m_bDisableResultsWriter)
	{
		Log(DFW2MessageStatus::DFW2LOG_INFO, CDFW2Messages::m_cszResultWriterDisabled);
		return;
	}

	auto resultPath(Platform().Results());
	std::filesystem::path parametersResultPath(m_Parameters.m_strResultsFolder);

	// если в параметрах задан абосолютный путь к каталогу
	// результатов - используем его как есть,
	// если путь относительный - достраиваем его к стандартному каталогу

	if (parametersResultPath.is_absolute())
		resultPath = parametersResultPath;
	else
		resultPath.append(m_Parameters.m_strResultsFolder);

	// создаем каталог для вывода результатов
	Platform().CheckPath(resultPath);

	resultPath.append("binresultCOM.rst");

	CResultsWriterBase::ResultsInfo resultsInfo { 0.0 * GetAtol(), "Тестовая схема mdp_debug5 с КЗ"};
	m_ResultsWriter.CreateFile(resultPath, resultsInfo );
	Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszResultFileCreated, stringutils::utf8_encode(resultPath.c_str())));

	// добавляем описание единиц измерения переменных
	CDFW2Messages vars;
	for (const auto& vn : vars.VarNameMap())
		m_ResultsWriter.AddVariableUnit(vn.first, vn.second);


	for (const auto& container : m_DeviceContainers)
	{
		// проверяем, нужно ли записывать данные для такого типа контейнера
		if (!ApproveContainerToWriteResults(container)) continue;
		// если записывать надо - добавляем тип устройства контейнера
		m_ResultsWriter.AddDeviceType(*container);
	}

	m_ResultsWriter.FinishWriteHeader();

	long nIndex = 0;

	for (const auto& container : m_DeviceContainers)
	{
		// собираем углы генераторов для детектора затухания колебаний
		if (m_Parameters.m_bAllowDecayDetector && container->IsKindOfType(eDFW2DEVICETYPE::DEVTYPE_GEN_MOTION))
		{
			ptrdiff_t deltaIndex(container->GetVariableIndex(CDynaNodeBase::m_cszDelta));
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

	m_dTimeWritten = 0.0;
}

void CDynaModel::WriteResults()
{
	if (m_Parameters.m_bDisableResultsWriter)
		return;

	if (sc.m_bEnforceOut || GetCurrentTime() >= m_dTimeWritten)
	{
		m_ResultsWriter.WriteResults({ GetCurrentTime(), GetH() });
		m_dTimeWritten = GetCurrentTime() + m_Parameters.m_dOutStep;
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
		GetCurrentTime(),
		Value,
		PreviousValue,
		ChangeDescription);
}