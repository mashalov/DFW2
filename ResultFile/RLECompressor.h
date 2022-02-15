/*
* Модуль сжатия результатов расчета электромеханических переходных процессов
* (С) 2018 - Н.В. Евгений Машалов
* Свидетельство о государственной регистрации программы для ЭВМ №2021663231
* https://fips.ru/EGD/c7c3d928-893a-4f47-a218-51f3c5686860
*
* Transient stability simulation results compression library
* (C) 2018 - present Eugene Mashalov
* pat. RU №2021663231
*/

#pragma once
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#include <stddef.h>

class CRLECompressor
{
protected:
	bool OutSkip(const unsigned char *pBuffer, const unsigned char *pInput);
	bool OutRepeat(const unsigned char *pBuffer, const unsigned char *pInput);
	bool OutCompressed(const unsigned char Byte);
	bool OutDecompressed(const unsigned char Byte);
	unsigned char* pCompr_ = nullptr, *pDecompr_ = nullptr, *pWriteBufferEnd_ = nullptr;
public:
	bool Compress(const unsigned char* pBuffer, size_t Size, unsigned char *pCompressedBuffer, size_t& ComprSize, bool& AllBytesEqual);
	bool Decompress(const unsigned char *pBuffer, size_t Size, unsigned char *pDecompressedBuffer, size_t& DecomprSize);
	static const size_t MaxBlockSize_;
};

