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

