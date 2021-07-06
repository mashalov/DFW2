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
	unsigned char* m_pCompr = nullptr, *m_pDecompr = nullptr, *m_pWriteBufferEnd = nullptr;
public:
	bool Compress(const unsigned char* pBuffer, size_t nSize, unsigned char *pCompressedBuffer, size_t& nComprSize, bool& bAllBytesEqual);
	bool Decompress(const unsigned char *pBuffer, size_t nSize, unsigned char *pDecompressedBuffer, size_t& nDecomprSize);
	static const size_t m_nMaxBlockSize;
};

