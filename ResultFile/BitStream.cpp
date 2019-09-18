#include "stdafx.h"
#include "Compressor.h"

CBitStream::CBitStream(BITWORD *Buffer, BITWORD *BufferEnd, size_t BitSeek)
{
	Init(Buffer, BufferEnd, BitSeek);
}

CBitStream::CBitStream()
{
	Init(0, 0, 0);
}

void CBitStream::Init(BITWORD *Buffer, BITWORD *BufferEnd, size_t BitSeek)
{
	m_pWordInitial = m_pWord = Buffer;
	m_pEnd = BufferEnd;
	m_nBitSeek = BitSeek;
	m_nTotalBitsWritten = 0;
}

void CBitStream::Rewind()
{
	m_pWord = m_pWordInitial;
	m_nBitSeek = 0;
	m_nTotalBitsWritten = 0;
}

void CBitStream::Reset()
{
	Rewind();
	BITWORD *p = m_pWordInitial;
	while (p < m_pEnd)
	{
		*p = 0;
		p++;
	}
}

eFCResult CBitStream::WriteBits(CBitStream& Source, size_t BitCount)
{
	// !!!!!!!!!!!!!!!!!!!!!!!     обновляем указатели слов источника и приемника (кажется, это не нужно)   !!!!!!!!!!!!!!!!!!!!!
	//Source.MoveBitCount(0);
	//MoveBitCount(0);

	eFCResult fcResult = FC_OK;
	// пока не записали все биты
	while (BitCount)
	{
		// берем слова приемника и источника
		BITWORD *pDest = m_pWord;
		BITWORD *pSource = Source.m_pWord;
		
		
		size_t nBitsToWriteFromCurrentWord = Source.WordBitsLeft();	// сколько бит осталось записать из слова источника
		size_t nBitsToWriteToCurrentWord = WordBitsLeft();			// сколько бит осталось записать в слово приемника
		size_t nBitsToWrite = BitCount;								// по умолчанию хотим записать все запрошенные биты

		// если количество битов в слове источника меньше требуемого - ограничиваем требуемое
		if (nBitsToWrite > nBitsToWriteFromCurrentWord)			
			nBitsToWrite = nBitsToWriteFromCurrentWord;
		// если количество битов в слове приемника меньше требуемого - ограничиваем требуемое
		if (nBitsToWrite > nBitsToWriteToCurrentWord)
			nBitsToWrite = nBitsToWriteToCurrentWord;

		// берем слово из источника
		BITWORD wSource = *pSource;
		// формируем маску для записываемого количества бит
		// если записываемых бит меньше, чем доступно всего в слове - маску делаем сдвигом, если больше - маска все единицы слова
		BITWORD mask = (nBitsToWrite < WordBitCount) ? (1ui64 << nBitsToWrite) - 1 : ~static_cast<BITWORD>(0);
		// смещаем маску на битовое смещение источника
		mask <<= Source.m_nBitSeek;
		// маскируем нужное количество бит в слове источника
		wSource &= mask;
		// двумя сдвигами подгоняем блок битов к смещению приемника
		wSource <<= m_nBitSeek;													
		wSource >>= Source.m_nBitSeek;
		// дописываем биты в приемник
		*pDest |= wSource;
		// осталось дописать меньше битов
		BitCount -= nBitsToWrite;												
		// смещаем приемник, проверяем все ли в порядке, если нет - выходим с результатом
		fcResult = MoveBitCount(nBitsToWrite);									
		if (fcResult != FC_OK)													
			break;
		// смещаем источник, проверяем все ли в порядке, если нет - выходим с результатом
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

size_t CBitStream::WordBitsLeft()
{
	return WordBitCount - m_nBitSeek;
}

size_t CBitStream::BytesWritten()
{
	return m_nTotalBitsWritten / 8 + ((m_nTotalBitsWritten % 8) ? 1 : 0);
}

size_t CBitStream::BitsLeft()
{
	return (m_pEnd - m_pWordInitial) * sizeof(BITWORD) * 8 - m_nTotalBitsWritten;
}

eFCResult CBitStream::AlignTo(size_t nAlignTo)
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


// перемещаем счетчик битов на заданное количество
eFCResult CBitStream::MoveBitCount(size_t BitCount)
{
	// добавляем к смещению битов заданное количество
	m_nBitSeek += BitCount;
	// корректируем общее количество записанных бит
	m_nTotalBitsWritten += BitCount;
	// считаем смещение указателя на слово
	ptrdiff_t nMovePtr = m_nBitSeek / WordBitCount;
	// если m_nBitSeek оказался больше WordBitCount то
	// указатель на слово смещается
	m_pWord += nMovePtr;
	// проверяем не достигнут ли конец буфера записи
	if (m_pWord > m_pEnd)
	{
		// если да - то ограничиваем указатель на слово
		// размерами буфера
		m_pWord = m_pEnd - 1;
		m_nBitSeek = 0;
		// информируем о переполении
		return FC_BUFFEROVERFLOW;
	}
	// если переполнения нет - считаем смещение битов
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

const size_t CBitStream::WordBitCount = (sizeof(BITWORD) * 8);