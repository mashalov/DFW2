#include "stdafx.h"
#include "CSVWriter.h"


CCSVWriter::CCSVWriter(CResultFileReader& ResultFileReader) : ResultFileReader_(ResultFileReader)
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
	const DEVTYPESET& devtypeset{ ResultFileReader_.GetTypesSet() };
	DeviceTypeInfo devtypefind;
	pChannelLink = std::make_unique<ChannelLink[]>(nChannelsCount);

	auto ChannelsRange{ ResultFileReader_.GetChannelHeadersRange() };

	ChannelLink* pC{ pChannelLink.get() };

	while (ChannelsRange.begin < ChannelsRange.end)
	{
		devtypefind.eDeviceType = ChannelsRange.begin->eDeviceType;

		auto dit{ devtypeset.find(&devtypefind) };

		pC->pDevice = nullptr;
		pC->pVariable = nullptr;

		if (dit != devtypeset.end())
		{
			auto pDevType{ *dit };
			DeviceInstanceInfo* pDev{ pDevType->pDeviceInstances_.get() };
			const DeviceInstanceInfo* const pDevEnd{ pDevType->pDeviceInstances_.get() + pDevType->DevicesCount };
			while (pDev < pDevEnd)
			{
				if (pDev->GetId(0) == ChannelsRange.begin->DeviceId && pDev->pDevType_)
				{
					pC->pDevice = pDev;
					auto vit{ pDevType->VarTypes_.begin() };
					while (vit != pDevType->VarTypes_.end())
					{
						if (vit->Index == ChannelsRange.begin->DeviceVarIndex)
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
		ChannelsRange.begin++;
	}
}

void CCSVWriter::WriteDeviceTypes()
{
	auto ChannelsRange{ ResultFileReader_.GetChannelHeadersRange() };
	const ChannelLink* pC{ pChannelLink.get() };

	while (ChannelsRange.begin < ChannelsRange.end)
	{
		if (pC->pDevice)
			WriteField(pC, fmt::format("%s", pC->pDevice->pDevType_->DevTypeName_));
		else
			WriteField(pC, cszUnknonwn);

		pC++;
		ChannelsRange.begin++;
	}

	WriteCRLF2();
}

void CCSVWriter::WriteDeviceIds()
{
	auto ChannelsRange{ ResultFileReader_.GetChannelHeadersRange() };

	const ChannelLink* pC{ pChannelLink.get() };

	while (ChannelsRange.begin < ChannelsRange.end)
	{
		if (pC->pDevice)
			WriteField(pC, fmt::format("%d", pC->pDevice->GetId(0)));
		else
			WriteField(pC, cszUnknonwn);
		pC++;
		ChannelsRange.begin++;
	}

	WriteCRLF2();
}

void CCSVWriter::WriteDeviceNames()
{
	auto ChannelsRange{ ResultFileReader_.GetChannelHeadersRange() };

	const ChannelLink* pC{ pChannelLink.get() };

	while (ChannelsRange.begin < ChannelsRange.end)
	{
		if (pC->pDevice)
			WriteField(pC, fmt::format("%s", pC->pDevice->Name));
		else
			WriteField(pC, cszUnknonwn);

		pC++;
		ChannelsRange.begin++;
	}

	WriteCRLF2();
}

void CCSVWriter::WriteVariableNames()
{
	auto ChannelsRange{ ResultFileReader_.GetChannelHeadersRange() };

	const ChannelLink* pC{ pChannelLink.get() };

	while (ChannelsRange.begin < ChannelsRange.end)
	{
		if (pC->pVariable)
			WriteField(pC, fmt::format("%s", pC->pVariable->Name));
		else
			WriteField(pC, cszUnknonwn);
		pC++;
		ChannelsRange.begin++;
	}

	WriteCRLF();
}


bool CCSVWriter::WriteCSV(std::string_view FilePath)
{
	bool bRes(false);

	nPointsCount = ResultFileReader_.GetPointsCount();
	nChannelsCount = ResultFileReader_.GetChannelsCount() - 2;

	CSVOut.open(FilePath, std::ios_base::out);
	std::filesystem::path TempPath(std::filesystem::temp_directory_path());
#ifdef _MSC_VER
	const auto tmpname{ std::make_unique<char[]>(MAX_PATH) };
	tmpnam_s(tmpname.get(), MAX_PATH);
	TempPath.append(tmpname.get());
#else
	TempPath.append(tmpnam(nullptr));
#endif
	CSVFile.open(TempPath.string(), std::ios_base::out);

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
	std::filesystem::remove(TempPath);

	bRes = true;
	return bRes;
}

void CCSVWriter::WriteColumn(const double *pData, size_t nColumn, bool bLastColumn)
{
	const double* pDataEnd{ pData + nPointsCount };
	const double* pDataStart{ pData };

	while (pData < pDataEnd)
	{
		uint64_t nOffs{ nRowStep * (pData - pDataStart) + nColumn * MinFieldWidth + nOffset };

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
	auto ChannelsRange{ ResultFileReader_.GetChannelHeadersRange() };
	const ChannelLink* pC{ pChannelLink.get() };

	CSVFile.close();

	CSVFile.open(strFilePath.c_str(), std::ios_base::in| std::ios_base::out|std::ios_base::binary);

	/*
	if (_tfopen_s(&pCSVFile, strFilePath.c_str(), "rb+R"))
		throw CFileWriteException(NULL);
	*/

	CSVFile.seekg(0, std::ios_base::end);

	nOffset = CSVFile.tellg();

	nRowStep = (ChannelsRange.end - ChannelsRange.begin + 2) * MinFieldWidth;

	std::unique_ptr<double[]> pData;
	pData = std::make_unique<double[]>(nPointsCount);
	ResultFileReader_.GetTimeScale(pData.get(), nPointsCount);
	WriteColumn(pData.get(), 0, false);
	ResultFileReader_.GetStep(pData.get(), nPointsCount);
	WriteColumn(pData.get(), 1, false);

	while (ChannelsRange.begin < ChannelsRange.end)
	{
		pData = std::move(ResultFileReader_.ReadChannel(
			ChannelsRange.begin->eDeviceType, 
			static_cast<int>(ChannelsRange.begin->DeviceId), 
			ChannelsRange.begin->DeviceVarIndex)
		);

		if (pData)
			WriteColumn(pData.get(), pC - pChannelLink.get() + 2, ChannelsRange.begin == ChannelsRange.end - 1);
		pC++;
		ChannelsRange.begin++;
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
