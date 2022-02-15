#include "stdafx.h"
#include "RLECompressor.h"


bool CRLECompressor::OutSkip(const unsigned char *pBuffer, const unsigned char *pInput)
{
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
	bool bRes = OutCompressed(static_cast<unsigned char>(pInput - pBuffer)|0x80);
	bRes = bRes && OutCompressed(*pBuffer) ;
	return bRes;
}


bool CRLECompressor::OutCompressed(const unsigned char Byte)
{
	if (pCompr_ < pWriteBufferEnd_)
	{
		*pCompr_ = Byte;
		pCompr_++;
		return true;
	}
	return false;
}

bool CRLECompressor::OutDecompressed(const unsigned char Byte)
{
	if (pDecompr_ < pWriteBufferEnd_)
	{
		*pDecompr_= Byte;
		pDecompr_++;
		return true;
	}
	return false;
}

bool CRLECompressor::Compress(const unsigned char* pBuffer, size_t Size, unsigned char *pCompressedBuffer, size_t& ComprSize, bool& AllBytesEqual)
{
	bool bRes{ true };

	const unsigned char* pInput{ pBuffer }, *pInputEnd{ pBuffer + Size };
	pCompr_ = pCompressedBuffer;
	pWriteBufferEnd_ = pCompressedBuffer + ComprSize;
	AllBytesEqual = true;

	// сжимаем блок от заданного указателя до конца исходного буфера
	// или до обнаружения ошибки
	while (pInput < pInputEnd && bRes)
	{
		const unsigned char* pScan{ pInput };
		unsigned char Last{ *pInput };
		const unsigned char* pRepeatStart{ nullptr };
		pScan++;	// просматриваем следующий байт до тех пор, пока не дошли до конца буфера
		while (pScan < pInputEnd)
		{
			if (*pScan == Last)
			{
				// если следующий байт равен предыдущему
				// и не был обозначен старт повтора - ставим старт повтора на предыдущий байт
				// и продолжаем просмотр
				if (!pRepeatStart)
					pRepeatStart = pScan - 1;
			}
			else
			{
				// если следующий байт не равен предыдущему
				// и был обозначен старт повтора - выходим: нам надо записать повтор
				if (pRepeatStart)
					break;
			}
			// перемещаемся на следующий байт
			Last = *pScan;
			pScan++;

			// проверяем не превышает ли количество повторов или количество неповторяющихся байт
			// допустимого значения счетчика. Если превышает - выходим
			if (pRepeatStart)
			{
				if (static_cast<size_t>(pScan - pRepeatStart) >= MaxBlockSize_)
					break;
			}
			else
			{
				if (static_cast<size_t>(pScan - pInput) >= MaxBlockSize_)
					break;
			}
		}

		if (pRepeatStart)
		{
			// если обозначен счетчик повторов
			// и счетчик повторов 
			if (pRepeatStart > pInput)
			{
				bRes = bRes && OutSkip(pInput, pRepeatStart);
				AllBytesEqual = false; // есть неповторяющиеся байты
			}
			// записываем повторы
			bRes = bRes && OutRepeat(pRepeatStart, pScan);
		}
		else
		{
			bRes = bRes && OutSkip(pInput, pScan);	// если повторов нет - записываем последовательность без повторов
			AllBytesEqual = false; // есть неповторяющиеся байты
		}
		pInput = pScan;
	}
	ComprSize = pCompr_ - pCompressedBuffer;
	return bRes;
}


bool CRLECompressor::Decompress(const unsigned char *pBuffer, size_t Size, unsigned char *pDecompressedBuffer, size_t& DecomprSize)
{
	bool bRes{ true };
	const unsigned char* pInput{ pBuffer }, * pInputEnd{ pBuffer + Size };
	pDecompr_ = pDecompressedBuffer;
	pWriteBufferEnd_ = pDecompressedBuffer + DecomprSize;

	while (pInput < pInputEnd && bRes)
	{
		unsigned char pByte{ *pInput };
		if (pByte & 0x80)
		{
			size_t nCount( pByte & 0x7f );
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

	DecomprSize = pDecompr_ - pDecompressedBuffer;
	return bRes;
}

const size_t CRLECompressor::MaxBlockSize_ = 127;