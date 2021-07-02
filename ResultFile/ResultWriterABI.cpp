#include "stdafx.h"
#include "ResultFileWriter.h"

using namespace DFW2;
/*
void CResultWriterABI::CreateResultFile(const std::filesystem::path FilePath)
{
	writer.CreateResultFile(FilePath.string());
}

void CResultWriterABI::SetNoChangeTolerance(double Tolerance)
{
	writer.SetNoChangeTolerance(Tolerance);
}

void CResultWriterABI::SetComment(const std::string_view Comment)
{
	writer.SetComment(Comment);
}

void CResultWriterABI::Close()
{
	writer.CloseFile();
}

void CResultWriterABI::FinishWriteHeader()
{
	writer.WriteHeader();
}

void CResultWriterABI::WriteResults(double Time, double Step)
{
	writer.WriteResults(Time, Step);
}

void CResultWriterABI::AddVariableUnit(ptrdiff_t nUnitType, const std::string_view UnitName)
{
	writer.AddVariableUnit(nUnitType, UnitName);
}

void CResultWriterABI::SetChannel(ptrdiff_t nDeviceId, ptrdiff_t nDeviceType, ptrdiff_t nDeviceVarIndex, const double* pVariable, ptrdiff_t nVariableIndex)
{
	writer.SetChannel(nDeviceId, nDeviceType, nDeviceVarIndex, pVariable, nVariableIndex);
}


IDeviceTypeABI* CResultWriterABI::AddDeviceType(ptrdiff_t nDeviceType, const std::string_view DeviceTypeName)
{
	return new CResultWriterDeviceTypeABI();
}

void CResultWriterDeviceTypeABI::SetDeviceTypeMetrics(ptrdiff_t DeviceIdsCount, ptrdiff_t ParentIdsCount, ptrdiff_t DevicesCount)
{
	if (m_pDeviceTypeInfo)
	{
		m_pDeviceTypeInfo->DeviceIdsCount = static_cast<int>(DeviceIdsCount);
		m_pDeviceTypeInfo->DevicesCount = static_cast<int>(DevicesCount);
		m_pDeviceTypeInfo->DeviceParentIdsCount = static_cast<int>(ParentIdsCount);
		m_pDeviceTypeInfo->AllocateData();
		m_pDeviceInstanceInfo = m_pDeviceTypeInfo->m_pDeviceInstances.get();
	}
	else
		throw dfw2error("CResultWriterDeviceTypeABI::SetDeviceTypeMetrics - m_pDeviceTypeInfo == nullptr");
}

void CResultWriterDeviceTypeABI::AddDeviceTypeVariable(const std::string_view VariableName, ptrdiff_t UnitsId, double Multiplier)
{
	if (m_pDeviceTypeInfo)
	{
		CResultFileReader::VariableTypeInfo VarTypeInfo;
		VarTypeInfo.eUnits = static_cast<int>(UnitsId);
		VarTypeInfo.Multiplier = Multiplier;
		VarTypeInfo.Name = VariableName;
		VarTypeInfo.nIndex = m_pDeviceTypeInfo->m_VarTypes.size();
		if (m_pDeviceTypeInfo->m_VarTypes.insert(VarTypeInfo).second)
			m_pDeviceTypeInfo->m_VarTypesList.push_back(VarTypeInfo);
		else
			throw dfw2error(fmt::format(CDFW2Messages::m_cszDuplicatedVariableName, VarTypeInfo.Name, m_pDeviceTypeInfo->strDevTypeName));
	}
	else
		throw dfw2error("CResultWriterDeviceTypeABI::AddDeviceTypeVariable - m_pDeviceTypeInfo == nullptr");
}

void CResultWriterDeviceTypeABI::AddDevice(const std::string_view DeviceName,
	const std::vector<ptrdiff_t>& DeviceIds,
	const std::vector<ptrdiff_t>& ParentIds,
	const std::vector<ptrdiff_t>& ParentTypes)
{
	if (m_pDeviceTypeInfo)
	{
		ptrdiff_t nIndex = m_pDeviceInstanceInfo - m_pDeviceTypeInfo->m_pDeviceInstances.get();

		if (nIndex < m_pDeviceTypeInfo->DevicesCount)
		{
			m_pDeviceInstanceInfo->nIndex = nIndex;
			m_pDeviceInstanceInfo->Name = DeviceName;

			for (int i = 0; i < (std::min)(m_pDeviceTypeInfo->DeviceIdsCount, static_cast<int>(DeviceIds.size())); i++)
				m_pDeviceInstanceInfo->SetId(i, DeviceIds[i]);

			for (int i = 0; i < (std::min)(m_pDeviceTypeInfo->DeviceParentIdsCount, (std::min)(static_cast<int>(ParentIds.size()), static_cast<int>(ParentTypes.size()))); i++)
				m_pDeviceInstanceInfo->SetParent(i, ParentTypes[i], ParentIds[i]);

			m_pDeviceInstanceInfo++;
		}
		else
			throw dfw2error(fmt::format(CDFW2Messages::m_cszTooMuchDevicesInResult,
					m_pDeviceInstanceInfo - m_pDeviceTypeInfo->m_pDeviceInstances.get(),
					m_pDeviceTypeInfo->DevicesCount));
	}
	else
		throw dfw2error("CResultWriterDeviceTypeABI::AddDevice - m_pDeviceTypeInfo == nullptr");
}
*/