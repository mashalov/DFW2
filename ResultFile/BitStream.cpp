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

#include "stdafx.h"
#include "Compressor.h"

CBitStream::CBitStream(BITWORD *pBuffer, BITWORD *pBufferEnd, size_t BitSeek)
{
	Init(pBuffer, pBufferEnd, BitSeek);
}

CBitStream::CBitStream()
{
	Init(0, 0, 0);
}

void CBitStream::Init(BITWORD *pBuffer, BITWORD *pBufferEnd, size_t BitSeek)
{
	pWordInitial_ = pWord_ = pBuffer;
	pEnd_ = pBufferEnd;
	BitSeek_ = BitSeek;
	TotalBitsWritten_ = 0;
}

void CBitStream::Rewind()
{
	pWord_ = pWordInitial_;
	BitSeek_ = 0;
	TotalBitsWritten_ = 0;
}

void CBitStream::Reset()
{
	Rewind();
	BITWORD *p = pWordInitial_;
	while (p < pEnd_)
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

	eFCResult fcResult{ eFCResult::FC_OK };
	// пока не записали все биты
	while (BitCount)
	{
		// берем слова приемника и источника
		auto pDest{ pWord_ };
		auto pSource{ Source.pWord_ };
		
		const size_t nBitsToWriteFromCurrentWord{ Source.WordBitsLeft() };	// сколько бит осталось записать из слова источника
		const size_t nBitsToWriteToCurrentWord{ WordBitsLeft() };			// сколько бит осталось записать в слово приемника
		size_t nBitsToWrite{ BitCount };									// по умолчанию хотим записать все запрошенные биты

		// если количество битов в слове источника меньше требуемого - ограничиваем требуемое
		if (nBitsToWrite > nBitsToWriteFromCurrentWord)			
			nBitsToWrite = nBitsToWriteFromCurrentWord;
		// если количество битов в слове приемника меньше требуемого - ограничиваем требуемое
		if (nBitsToWrite > nBitsToWriteToCurrentWord)
			nBitsToWrite = nBitsToWriteToCurrentWord;

		// берем слово из источника
		auto wSource{ *pSource };
		// формируем маску для записываемого количества бит
		// если записываемых бит меньше, чем доступно всего в слове - маску делаем сдвигом, если больше - маска все единицы слова
		BITWORD mask = (nBitsToWrite < WordBitCount) ? (((uint64_t)1) << nBitsToWrite) - 1 : ~static_cast<BITWORD>(0);
		// смещаем маску на битовое смещение источника
		mask <<= Source.BitSeek_;
		// маскируем нужное количество бит в слове источника
		wSource &= mask;
		// двумя сдвигами подгоняем блок битов к смещению приемника
		wSource <<= BitSeek_;													
		wSource >>= Source.BitSeek_;
		// дописываем биты в приемник
		*pDest |= wSource;
		// осталось дописать меньше битов
		BitCount -= nBitsToWrite;												
		// смещаем приемник, проверяем все ли в порядке, если нет - выходим с результатом
		fcResult = MoveBitCount(nBitsToWrite);									
		if (fcResult != eFCResult::FC_OK)
			break;
		// смещаем источник, проверяем все ли в порядке, если нет - выходим с результатом
		fcResult = Source.MoveBitCount(nBitsToWrite);							
		if (fcResult != eFCResult::FC_OK)
			break;
	}
	return fcResult;
}

bool CBitStream::Check()
{
	return true;
}

size_t CBitStream::BytesWritten()
{
	return TotalBitsWritten_ / 8 + ((TotalBitsWritten_ % 8) ? 1 : 0);
}

size_t CBitStream::BitsLeft()
{
	return (pEnd_ - pWordInitial_) * sizeof(BITWORD) * 8 - TotalBitsWritten_;
}

eFCResult CBitStream::AlignTo(size_t nAlignTo)
{
	_ASSERTE(!(nAlignTo % 8));
	return MoveBitCount(BitSeek_ % nAlignTo);
}

eFCResult CBitStream::WriteDouble(double &Value)
{
	_ASSERTE(!(BitSeek_ % 8));
	if (BitsLeft() >= sizeof(double) * 8)
	{
		*static_cast<double*>(static_cast<void*>(pWord_)) = Value;
		return MoveBitCount(sizeof(double) * 8);
	}
	else
		return eFCResult::FC_BUFFEROVERFLOW;
}

eFCResult CBitStream::WriteByte(unsigned char Byte)
{
	_ASSERTE(!(BitSeek_ % 8));
	auto pDest{ pWord_ };
	auto Src{ Byte };
	Src <<= BitSeek_;
	*pDest |= Src;
	return MoveBitCount(8);
}

eFCResult CBitStream::ReadByte(unsigned char& Byte)
{
	_ASSERTE(!(BitSeek_ % 8));
	auto wSource{ *pWord_ };
	Byte = (wSource >> BitSeek_) & 0xff;
	return MoveBitCount(8);
}


// перемещаем счетчик битов на заданное количество
eFCResult CBitStream::MoveBitCount(size_t BitCount)
{
	// добавляем к смещению битов заданное количество
	BitSeek_ += BitCount;
	// корректируем общее количество записанных бит
	TotalBitsWritten_ += BitCount;
	// считаем смещение указателя на слово
	ptrdiff_t nMovePtr = BitSeek_ / WordBitCount;
	// если BitSeek_ оказался больше WordBitCount то
	// указатель на слово смещается
	pWord_ += nMovePtr;
	// проверяем не достигнут ли конец буфера записи
	if (pWord_ > pEnd_)
	{
		// если да - то ограничиваем указатель на слово
		// размерами буфера
		pWord_ = pEnd_ - 1;
		BitSeek_ = 0;
		// информируем о переполении
		return eFCResult::FC_BUFFEROVERFLOW;
	}
	// если переполнения нет - считаем смещение битов
	BitSeek_ %= WordBitCount;
	return eFCResult::FC_OK;
}

unsigned char* CBitStream::BytesBuffer()
{
	return static_cast<unsigned char*>(static_cast<void*>(pWordInitial_));
}

BITWORD* CBitStream::Buffer()
{
	return pWordInitial_;
}

const size_t CBitStream::WordBitCount = (sizeof(BITWORD) * 8);