#include "stdafx.h"
#include "CSVWriter.h"


CCSVWriter::CCSVWriter(CResultFileReader& ResultFileReader) : m_ResultFileReader(ResultFileReader)
{
	pChannelLink = NULL;
	loc = setlocale(LC_ALL, "RU-ru");
	pCSVFile = NULL;
	pCSVOut = NULL;
}


CCSVWriter::~CCSVWriter()
{
	if (pChannelLink)
		delete pChannelLink;

	if (pCSVFile)
		fclose(pCSVFile);

	if (pCSVOut)
		fclose(pCSVOut);

	setlocale(LC_ALL, loc.c_str());
}


void CCSVWriter::IndexChannels()
{
	const CResultFileReader::DEVTYPESET &devtypeset = m_ResultFileReader.GetTypesSet();
	CResultFileReader::DeviceTypeInfo devtypefind;
	pChannelLink = new ChannelLink[nChannelsCount];
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;

	ChannelLink *pC = pChannelLink;

	while (pChannel < pChannelsEnd)
	{
		devtypefind.eDeviceType = pChannel->eDeviceType;
		CResultFileReader::DEVTYPEITRCONST dit = devtypeset.find(&devtypefind);

		pC->pDevice = NULL;
		pC->pVariable = NULL;

		if (dit != devtypeset.end())
		{
			CResultFileReader::DeviceTypeInfo *pDevType = *dit;
			CResultFileReader::DeviceInstanceInfo *pDev = pDevType->m_pDeviceInstances;
			CResultFileReader::DeviceInstanceInfo *pDevEnd = pDevType->m_pDeviceInstances + pDevType->DevicesCount;
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
	ChannelLink *pC = pChannelLink;

	while (pChannel < pChannelsEnd)
	{
		if (pC->pDevice)
			WriteField(pC, Cex(_T("%s"), pC->pDevice->m_pDevType->strDevTypeName.c_str()));
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
	ChannelLink *pC = pChannelLink;

	while (pChannel < pChannelsEnd)
	{
		if (pC->pDevice)
			WriteField(pC, Cex(_T("%d"), pC->pDevice->GetId(0)));
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
	ChannelLink *pC = pChannelLink;

	while (pChannel < pChannelsEnd)
	{
		if (pC->pDevice)
			WriteField(pC, Cex(_T("%s"), pC->pDevice->Name.c_str()));
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
	ChannelLink *pC = pChannelLink;

	while (pChannel < pChannelsEnd)
	{
		if (pC->pVariable)
			WriteField(pC, Cex(_T("%s"), pC->pVariable->Name.c_str()));
		else
			WriteField(pC, cszUnknonwn);
		pC++;
		pChannel++;
	}

	WriteCRLF();
}


HRESULT CCSVWriter::WriteCSV(const _TCHAR *cszFilePath)
{
	HRESULT hRes = E_FAIL;

	nPointsCount = m_ResultFileReader.GetPointsCount();
	nChannelsCount = m_ResultFileReader.GetChannelsCount() - 2;

	/*
	
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	FILE *pw = NULL;
	_tfopen_s(&pw, _T("c:\\tmp\\binarydata"), _T("wb+"));

	for (int c = 0; c < nChannelsCount; c++)
	{
		double *pData = m_ResultFileReader.ReadChannel(pChannel->eDeviceType, pChannel->DeviceId, pChannel->DeviceVarIndex);
		fwrite(pData, sizeof(double), nPointsCount, pw);
		pChannel++;
	}
	fclose(pw);
	*/

	if (_tfopen_s(&pCSVOut, cszFilePath, _T("w+, ccs=UTF-8")))
		throw CFileWriteException(NULL);

	_TCHAR TempPath[MAX_PATH];
	if (!GetTempPath(MAX_PATH, TempPath))
		throw CFileWriteException(NULL);

	_TCHAR TempFileName[MAX_PATH];
	if(!GetTempFileName(TempPath, _T("csv"), 0, TempFileName))
		throw CFileWriteException(NULL);

	strFilePath = TempFileName;

	if (_tfopen_s(&pCSVFile, strFilePath.c_str(), _T("w+, ccs=UTF-8")))
		throw CFileWriteException(NULL);

	if (_ftprintf_s(pCSVFile, _T("t;h;")) < 0)
		throw CFileWriteException(pCSVFile);

	IndexChannels();
	WriteDeviceTypes();
	WriteDeviceIds();
	WriteDeviceNames();
	WriteVariableNames();
	WriteData();

	fclose(pCSVFile);
	pCSVFile = NULL;

	if (_tfopen_s(&pCSVFile, strFilePath.c_str(), _T("r, ccs=UTF-8")))
		throw CFileWriteException(NULL);

	wint_t symb;

	while ((symb = _fgettc(pCSVFile)) != WEOF)
	{
		if (symb != 0)
			_fputtc(symb, pCSVOut);
	}

	fclose(pCSVOut);
	pCSVOut = NULL;
	fclose(pCSVFile);
	pCSVFile = NULL;

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
		__int64 nOffs = nRowStep * (pData - pDataStart) + nColumn * MinFieldWidth + nOffset;

		if (_fseeki64(pCSVFile, nOffs, SEEK_SET))
			throw CFileWriteException(pCSVFile);

		if (bLastColumn)
		{
			if (fprintf_s(pCSVFile, "%g\n", *pData) < 0)
				throw CFileWriteException(pCSVFile);
		}
		else
		{
			if (fprintf_s(pCSVFile, "%g;", *pData) < 0)
				throw CFileWriteException(pCSVFile);
		}

		pData++;
	}
}


void CCSVWriter::WriteData()
{
	const CResultFileReader::ChannelHeaderInfo *pChannel = m_ResultFileReader.GetChannelHeaders();
	const CResultFileReader::ChannelHeaderInfo *pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	ChannelLink *pC = pChannelLink;

	fclose(pCSVFile);

	if (_tfopen_s(&pCSVFile, strFilePath.c_str(), _T("rb+R")))
		throw CFileWriteException(NULL);

	if (_fseeki64(pCSVFile, 0 , SEEK_END))
		throw CFileWriteException(pCSVFile);

	nOffset = _ftelli64(pCSVFile);

	pChannel = m_ResultFileReader.GetChannelHeaders();
	pChannelsEnd = m_ResultFileReader.GetChannelHeaders() + nChannelsCount;
	pC = pChannelLink;
	nRowStep = (pChannelsEnd - pChannel + 2) * MinFieldWidth;

	double *pData(nullptr);

	__try
	{
		pData = new double[nPointsCount];
		m_ResultFileReader.GetTimeScale(pData, nPointsCount);
		WriteColumn(pData, 0, false);
		m_ResultFileReader.GetStep(pData, nPointsCount);
		WriteColumn(pData, 1, false);
	}
	__finally
	{
		if (pData)
			delete pData;
	}

	while (pChannel < pChannelsEnd)
	{
		pData = m_ResultFileReader.ReadChannel(pChannel->eDeviceType, static_cast<int>(pChannel->DeviceId), pChannel->DeviceVarIndex);
		double *pDataStart = NULL;
		__try
		{
			pDataStart = pData;
			if (pData)
				WriteColumn(pData, pC - pChannelLink + 2, pChannel == pChannelsEnd - 1);
		}
		__finally
		{
			if (pDataStart)
				delete pDataStart;
		}
		pC++;
		pChannel++;
	}
}

void CCSVWriter::WriteField(ChannelLink *pLink, const _TCHAR *cszField)
{
	wstring str(cszField);
	std::replace(str.begin(), str.end(), ';', ':');
	std::replace(str.begin(), str.end(), '\"', '\'');
	str += _T("\";");
	str.insert(str.begin(), L'\"');

	if (_ftprintf_s(pCSVFile, str.c_str()) < 0)
		throw CFileWriteException(pCSVFile);
}

void CCSVWriter::WriteCRLF2()
{
	if (_ftprintf_s(pCSVFile, cszCRLF2) < 0)
		throw CFileWriteException(pCSVFile);
}

void CCSVWriter::WriteCRLF()
{
	if (_ftprintf_s(pCSVFile, cszCRLF) < 0)
		throw CFileWriteException(pCSVFile);
}



const _TCHAR *CCSVWriter::cszUnknonwn	= _T("???");
const _TCHAR *CCSVWriter::cszCRLF2		= _T("\n;;");
const _TCHAR *CCSVWriter::cszCRLF		= _T("\n");
const size_t CCSVWriter::MinFieldWidth	= 30;
