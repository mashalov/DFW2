#include "stdafx.h"
#include "Compressor.h"

CBitStream::CBitStream(BITWORD *Buffer, BITWORD *BufferEnd, ptrdiff_t BitSeek)
{
	Init(Buffer, BufferEnd, BitSeek);
}

CBitStream::CBitStream()
{
	Init(0, 0, 0);
}

void CBitStream::Init(BITWORD *Buffer, BITWORD *BufferEnd, ptrdiff_t BitSeek)
{
	m_pWordInitial = m_pWord = Buffer;
	m_pEnd = BufferEnd;
	m_nBitSeek = BitSeek;
	m_nTotalBitsWritten = 0;
}

void CBitStream::Reset()
{
	m_pWord = m_pWordInitial;
	m_nBitSeek = 0;
	m_nTotalBitsWritten = 0;
	BITWORD *p = m_pWordInitial;
	while (p < m_pEnd)
	{
		*p = 0;
		p++;
	}
}

eFCResult CBitStream::WriteBits(CBitStream& Source, ptrdiff_t BitCount)
{
	Source.MoveBitCount(0);
	MoveBitCount(0);
	eFCResult fcResult = FC_OK;
	while (BitCount)
	{
		BITWORD *pDest = m_pWord;
		BITWORD *pSource = Source.m_pWord;

		ptrdiff_t nBitsToWriteFromCurrentWord = Source.WordBitsLeft();
		ptrdiff_t nBitsToWriteToCurrentWord = WordBitsLeft();
		ptrdiff_t nBitsToWrite = BitCount;

		if (nBitsToWrite > nBitsToWriteFromCurrentWord)
			nBitsToWrite = nBitsToWriteFromCurrentWord;

		if (nBitsToWrite > nBitsToWriteToCurrentWord)
			nBitsToWrite = nBitsToWriteToCurrentWord;



		BITWORD wSource = *pSource;
		BITWORD mask = (nBitsToWrite < WordBitCount) ? (1 << nBitsToWrite) - 1 : ~static_cast<BITWORD>(0);
		mask <<= Source.m_nBitSeek;
		wSource &= mask;
		wSource <<= m_nBitSeek;
		wSource >>= Source.m_nBitSeek;
		*pDest |= wSource;

		BitCount -= nBitsToWrite;
		fcResult = MoveBitCount(nBitsToWrite);
		if (fcResult != FC_OK)
			break;
		fcResult = Source.MoveBitCount(nBitsToWrite);
		if (fcResult != FC_OK)
			break;
	}
	return fcResult;
}

bool CBitStream::Check()
{
	return true;
}

ptrdiff_t CBitStream::WordBitsLeft()
{
	return WordBitCount - m_nBitSeek;
}

ptrdiff_t CBitStream::BytesWritten()
{
	return m_nTotalBitsWritten / 8 + ((m_nTotalBitsWritten % 8) ? 1 : 0);
}

ptrdiff_t CBitStream::BitsLeft()
{
	return (m_pEnd - m_pWordInitial) * sizeof(BITWORD) * 8 - m_nTotalBitsWritten;
}

eFCResult CBitStream::AlignTo(ptrdiff_t nAlignTo)
{
	_ASSERTE(!(nAlignTo % 8));
	return MoveBitCount(m_nBitSeek % nAlignTo);
}

eFCResult CBitStream::WriteDouble(double &dValue)
{
	_ASSERTE(!(m_nBitSeek % 8));
	if (BitsLeft() >= sizeof(double) * 8)
	{
		*static_cast<double*>(static_cast<void*>(m_pWord)) = dValue;
		return MoveBitCount(sizeof(double) * 8);
	}
	else
		return FC_BUFFEROVERFLOW;
}

eFCResult CBitStream::WriteByte(unsigned char Byte)
{
	_ASSERTE(!(m_nBitSeek % 8));
	BITWORD *pDest = m_pWord;
	BITWORD Src = Byte;
	Src <<= m_nBitSeek;
	*pDest |= Src;
	return MoveBitCount(8);
}

eFCResult CBitStream::ReadByte(unsigned char& Byte)
{
	_ASSERTE(!(m_nBitSeek % 8));
	BITWORD wSource = *m_pWord;
	Byte = (wSource >> m_nBitSeek) & 0xff;
	return MoveBitCount(8);
}


eFCResult CBitStream::MoveBitCount(ptrdiff_t BitCount)
{
	m_nBitSeek += BitCount;
	m_nTotalBitsWritten += BitCount;
	ptrdiff_t nMovePtr = m_nBitSeek / WordBitCount;
	m_pWord += nMovePtr;
	if (m_pWord > m_pEnd)
	{
		m_pWord = m_pEnd - 1;
		m_nBitSeek = 0;
		return FC_BUFFEROVERFLOW;
	}
	m_nBitSeek %= WordBitCount;
	return FC_OK;
}


unsigned char* CBitStream::BytesBuffer()
{
	return static_cast<unsigned char*>(static_cast<void*>(m_pWordInitial));
}

BITWORD* CBitStream::Buffer()
{
	return m_pWordInitial;
}

bool CBitStream::EncodeRLE()
{
	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(m_pWordInitial));
	unsigned char *e = p + BytesWritten();
	while (p < e)
	{
		if (*p != 0xf)
			break;
		p++;
	}
	return p == e;
}

const ptrdiff_t CBitStream::WordBitCount = (sizeof(BITWORD) * 8);