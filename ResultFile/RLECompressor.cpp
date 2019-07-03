#include "stdafx.h"
#include "RLECompressor.h"


CRLECompressor::CRLECompressor()
{
}


CRLECompressor::~CRLECompressor()
{

}

bool CRLECompressor::OutSkip(const unsigned char *pBuffer, const unsigned char *pInput)
{
	//_tcprintf(_T("\n---SKIP---"));
	bool bRes = OutCompressed(static_cast<unsigned char>(pInput - pBuffer));
	while (pBuffer < pInput && bRes)
	{
		bRes = OutCompressed(*pBuffer);
		pBuffer++;
	}
	return bRes;
}

bool CRLECompressor::OutRepeat(const unsigned char *pBuffer, const unsigned char *pInput)
{
	//_tcprintf(_T("\n---REPEAT---"));
	bool bRes = OutCompressed(static_cast<unsigned char>(pInput - pBuffer)|0x80);
	bRes = bRes && OutCompressed(*pBuffer) ;
	return bRes;
}


bool CRLECompressor::OutCompressed(const unsigned char Byte)
{
	//_tcprintf(_T("\n%02x %c"), Byte, Byte);
	if (m_pCompr < m_pWriteBufferEnd)
	{
		*m_pCompr = Byte;
		m_pCompr++;
		return true;
	}
	return false;
}

bool CRLECompressor::OutDecompressed(const unsigned char Byte)
{
	//_tcprintf(_T("%c"), Byte, Byte);
	if (m_pDecompr < m_pWriteBufferEnd)
	{
		*m_pDecompr = Byte;
		m_pDecompr++;
		return true;
	}
	return false;
}

bool CRLECompressor::Compress(const unsigned char* pBuffer, size_t nSize, unsigned char *pCompressedBuffer, size_t& nComprSize)
{
	bool bRes = true;

	const unsigned char *pInput = pBuffer;
	const unsigned char *pInputEnd = pBuffer + nSize;
	m_pCompr = pCompressedBuffer;
	m_pWriteBufferEnd = pCompressedBuffer + nComprSize;

	while (pInput < pInputEnd && bRes)
	{
		const unsigned char *pScan = pInput;
		unsigned char Last = *pInput;
		const unsigned char *pRepeatStart = NULL;
		pScan++;
		while (pScan < pInputEnd)
		{
			if (*pScan == Last)
			{
				if (!pRepeatStart)
					pRepeatStart = pScan - 1;
			}
			else
			{
				if (pRepeatStart)
					break;
			}
			Last = *pScan;
			pScan++;

			if (pRepeatStart)
			{
				if (static_cast<size_t>(pScan - pRepeatStart) >= m_nMaxBlockSize)
					break;
			}
			else
			{
				if (static_cast<size_t>(pScan - pInput) >= m_nMaxBlockSize)
					break;
			}
		}

		if (pRepeatStart)
		{
			if (pRepeatStart > pInput)
				bRes = bRes && OutSkip(pInput, pRepeatStart);
			bRes = bRes && OutRepeat(pRepeatStart, pScan);
		}
		else
			bRes = bRes && OutSkip(pInput, pScan);

		pInput = pScan;

	}

	nComprSize = m_pCompr - pCompressedBuffer;
	return bRes;
}


bool CRLECompressor::Decompress(const unsigned char *pBuffer, size_t nSize, unsigned char *pDecompressedBuffer, size_t& nDecomprSize)
{
	bool bRes = true;
	const unsigned char *pInput = pBuffer;
	const unsigned char *pInputEnd = pBuffer + nSize;
	m_pDecompr = pDecompressedBuffer;
	m_pWriteBufferEnd = pDecompressedBuffer + nDecomprSize;

	while (pInput < pInputEnd && bRes)
	{
		unsigned char pByte = *pInput;
		if (pByte & 0x80)
		{
			size_t nCount = pByte & 0x7f;
			pInput++;
			pByte = *pInput;
			while (nCount && bRes)
			{
				bRes = bRes && OutDecompressed(pByte);
				nCount--;
			}
			pInput++;
		}
		else
		{
			pInput++;
			while (pByte && bRes)
			{
				bRes = bRes && OutDecompressed(*pInput);
				pByte--;
				pInput++;
			}
		}
	}

	nDecomprSize = m_pDecompr - pDecompressedBuffer;
	return bRes;
}

const size_t CRLECompressor::m_nMaxBlockSize = 127;