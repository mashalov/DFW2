﻿#include "stdafx.h"
#include "CSVWriter.h"


CCSVWriter::CCSVWriter(CResultFileReader& ResultFileReader) : m_ResultFileReader(ResultFileReader)
{
	pChannelLink = nullptr;
	loc = setlocale(LC_ALL, "RU-ru");
}


CCSVWriter::~CCSVWriter()
{
	CSVFile.close();
	CSVOut.close();
	setlocale(LC_ALL, loc.c_str());
}


void CCSVWriter::IndexChannels()
{
	const CResultFileReader::DEVTYPESET &devtypeset = m_ResultFileReader.GetTypesSet();
	CResultFileReader::DeviceTypeInfo devtypefind;
	pChannelLink = std::make_unique<ChannelLink[]>(nChannelsCount);
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;

	ChannelLink *pC = pChannelLink.get();

	while (pChannel < pChannelsEnd)
	{
		devtypefind.eDeviceType = pChannel->eDeviceType;
		CResultFileReader::DEVTYPEITRCONST dit = devtypeset.find(&devtypefind);

		pC->pDevice = nullptr;
		pC->pVariable = nullptr;

		if (dit != devtypeset.end())
		{
			CResultFileReader::DeviceTypeInfo *pDevType = *dit;
			CResultFileReader::DeviceInstanceInfo *pDev = pDevType->m_pDeviceInstances.get();
			CResultFileReader::DeviceInstanceInfo *pDevEnd = pDevType->m_pDeviceInstances.get() + pDevType->DevicesCount;
			while (pDev < pDevEnd)
			{
				if (pDev->GetId(0) == pChannel->DeviceId && pDev->m_pDevType)
				{
					pC->pDevice = pDev;
					CResultFileReader::VARTYPEITRCONST vit = pDevType->m_VarTypes.begin();
					while (vit != pDevType->m_VarTypes.end())
					{
						if (vit->nIndex == pChannel->DeviceVarIndex)
						{
							pC->pVariable = &*vit;
							break;
						}
						vit++;
					}
					break;
				}
				pDev++;
			}
		}
		pC++;
		pChannel++;
	}
}

void CCSVWriter::WriteDeviceTypes()
{
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	const ChannelLink *pC = pChannelLink.get();

	while (pChannel < pChannelsEnd)
	{
		if (pC->pDevice)
			WriteField(pC, fmt::format("%s", pC->pDevice->m_pDevType->strDevTypeName));
		else
			WriteField(pC, cszUnknonwn);

		pC++;
		pChannel++;
	}

	WriteCRLF2();
}

void CCSVWriter::WriteDeviceIds()
{
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	const ChannelLink *pC = pChannelLink.get();

	while (pChannel < pChannelsEnd)
	{
		if (pC->pDevice)
			WriteField(pC, fmt::format("%d", pC->pDevice->GetId(0)));
		else
			WriteField(pC, cszUnknonwn);
		pC++;
		pChannel++;
	}

	WriteCRLF2();
}

void CCSVWriter::WriteDeviceNames()
{
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	const ChannelLink *pC = pChannelLink.get();

	while (pChannel < pChannelsEnd)
	{
		if (pC->pDevice)
			WriteField(pC, fmt::format("%s", pC->pDevice->Name));
		else
			WriteField(pC, cszUnknonwn);

		pC++;
		pChannel++;
	}

	WriteCRLF2();
}

void CCSVWriter::WriteVariableNames()
{
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	const ChannelLink *pC = pChannelLink.get();

	while (pChannel < pChannelsEnd)
	{
		if (pC->pVariable)
			WriteField(pC, fmt::format("%s", pC->pVariable->Name));
		else
			WriteField(pC, cszUnknonwn);
		pC++;
		pChannel++;
	}

	WriteCRLF();
}


HRESULT CCSVWriter::WriteCSV(std::string_view FilePath)
{
	HRESULT hRes = E_FAIL;

	nPointsCount = m_ResultFileReader.GetPointsCount();
	nChannelsCount = m_ResultFileReader.GetChannelsCount() - 2;

	/*
	
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	FILE *pw = NULL;
	fopen_s(&pw, "c:\\tmp\\binarydata", "wb+");

	for (int c = 0; c < nChannelsCount; c++)
	{
		double *pData = m_ResultFileReader.ReadChannel(pChannel->eDeviceType, pChannel->DeviceId, pChannel->DeviceVarIndex);
		fwrite(pData, sizeof(double), nPointsCount, pw);
		pChannel++;
	}
	fclose(pw);
	*/

	CSVOut.open(FilePath, std::ios_base::out);

	wchar_t TempPath[MAX_PATH];
	if (!GetTempPath(MAX_PATH, TempPath))
		throw CFileWriteException();

	wchar_t TempFileName[MAX_PATH];
	if(!GetTempFileName(TempPath, L"csv", 0, TempFileName))
		throw CFileWriteException();

	strFilePath = stringutils::utf8_encode(TempFileName);

	CSVFile.open(strFilePath.c_str(), std::ios_base::out);
	CSVFile << "t;h";

	IndexChannels();
	WriteDeviceTypes();
	WriteDeviceIds();
	WriteDeviceNames();
	WriteVariableNames();
	WriteData();
	CSVFile.close();

	CSVFile.open(strFilePath.c_str(), std::ios_base::in);

	CSVOut << CSVFile.rdbuf();

	CSVOut.close();
	CSVFile.close();

	DeleteFile(TempFileName);
	
	hRes = S_OK;
	return hRes;
}

void CCSVWriter::WriteColumn(const double *pData, size_t nColumn, bool bLastColumn)
{
	const double *pDataEnd = pData + nPointsCount;
	const double *pDataStart = pData;

	while (pData < pDataEnd)
	{
		uint64_t nOffs = nRowStep * (pData - pDataStart) + nColumn * MinFieldWidth + nOffset;

		CSVFile.seekg(nOffs, std::ios_base::beg);
		
		if (bLastColumn)
			CSVFile << *pData << "\n";
		else
			CSVFile << *pData;

		pData++;
	}
}


void CCSVWriter::WriteData()
{
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	const ChannelLink *pC = pChannelLink.get();

	CSVFile.close();

	CSVFile.open(strFilePath.c_str(), std::ios_base::in| std::ios_base::out|std::ios_base::binary);

	/*
	if (_tfopen_s(&pCSVFile, strFilePath.c_str(), "rb+R"))
		throw CFileWriteException(NULL);
	*/

	CSVFile.seekg(0, std::ios_base::end);

	nOffset = CSVFile.tellg();

	pChannel = m_ResultFileReader.GetChannelHeaders();
	pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	nRowStep = (pChannelsEnd - pChannel + 2) * MinFieldWidth;

	std::unique_ptr<double[]> pData;
	pData = std::make_unique<double[]>(nPointsCount);
	m_ResultFileReader.GetTimeScale(pData.get(), nPointsCount);
	WriteColumn(pData.get(), 0, false);
	m_ResultFileReader.GetStep(pData.get(), nPointsCount);
	WriteColumn(pData.get(), 1, false);

	while (pChannel < pChannelsEnd)
	{
		pData = std::move(m_ResultFileReader.ReadChannel(pChannel->eDeviceType, static_cast<int>(pChannel->DeviceId), pChannel->DeviceVarIndex));
		if (pData)
			WriteColumn(pData.get(), pC - pChannelLink.get() + 2, pChannel == pChannelsEnd - 1);
		pC++;
		pChannel++;
	}
}

void CCSVWriter::WriteField(const ChannelLink *pLink, std::string_view Field)
{
	std::string str(Field);
	std::replace(str.begin(), str.end(), ';', ':');
	std::replace(str.begin(), str.end(), '\"', '\'');
	str += "\";";
	str.insert(str.begin(), L'\"');
	CSVFile << str.c_str();
}

void CCSVWriter::WriteCRLF2()
{
	CSVFile << cszCRLF2;
}

void CCSVWriter::WriteCRLF()
{
	CSVFile << cszCRLF;
}

const char* CCSVWriter::cszUnknonwn		= "???";
const char* CCSVWriter::cszCRLF2		= "\n;;";
const char* CCSVWriter::cszCRLF			= "\n";
const size_t CCSVWriter::MinFieldWidth	= 30;
