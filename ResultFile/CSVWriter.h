#pragma once

#include "resource.h"       // main symbols
#include "ResultFile_i.h"
#include "ResultFileReader.h"

using namespace DFW2;

class CCSVWriter
{
protected:

	FILE *pCSVFile;
	FILE *pCSVOut;
	struct ChannelLink
	{
		const CResultFileReader::DeviceInstanceInfo *pDevice;
		const CResultFileReader::VariableTypeInfo *pVariable;
	};

	std::unique_ptr<ChannelLink[]> pChannelLink;
	CResultFileReader& m_ResultFileReader;
	size_t nPointsCount;
	size_t nChannelsCount;
	__int64 nRowStep;
	__int64 nOffset;

	void IndexChannels();
	void WriteDeviceTypes();
	void WriteDeviceIds();
	void WriteDeviceNames();
	void WriteVariableNames();
	void WriteData();
	std::string loc;

	void WriteField(const ChannelLink *pLink, std::wstring_view Field);
	void WriteCRLF2();
	void WriteCRLF();
	void WriteColumn(const double *pData, size_t nColumn, bool bLastColumn);


	std::wstring strFilePath;
	
public:
	HRESULT WriteCSV(const _TCHAR *cszFilePath);
	CCSVWriter(CResultFileReader& ResultFileReader);
	~CCSVWriter();

	static const _TCHAR *cszUnknonwn;
	static const _TCHAR *cszCRLF2;
	static const _TCHAR *cszCRLF;
	static const size_t MinFieldWidth;

};

