#pragma once

#include "resource.h"       // main symbols

#ifdef _MSC_VER
#include "ResultFile_i.h"
#endif

#include "ResultFileReader.h"

using namespace DFW2;

class CCSVWriter
{
protected:

	CStreamedFile CSVFile;
	CStreamedFile CSVOut;

	struct ChannelLink
	{
		const DeviceInstanceInfo* pDevice = nullptr;
		const VariableTypeInfo *pVariable = nullptr;
	};

	std::unique_ptr<ChannelLink[]> pChannelLink;
	CResultFileReader& ResultFileReader_;
	size_t nPointsCount;
	size_t nChannelsCount;
	uint64_t nRowStep;
	uint64_t nOffset;

	void IndexChannels();
	void WriteDeviceTypes();
	void WriteDeviceIds();
	void WriteDeviceNames();
	void WriteVariableNames();
	void WriteData();
	std::string loc;

	void WriteField(const ChannelLink *pLink, std::string_view Field);
	void WriteCRLF2();
	void WriteCRLF();
	void WriteColumn(const double *pData, size_t nColumn, bool bLastColumn);


	std::string strFilePath;
	
public:
	bool WriteCSV(std::string_view FilePath);
	CCSVWriter(CResultFileReader& ResultFileReader);
	~CCSVWriter();

	static const char* cszUnknonwn;
	static const char* cszCRLF2;
	static const char* cszCRLF;
	static const size_t MinFieldWidth;

};

