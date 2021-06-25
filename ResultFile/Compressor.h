#pragma once
#include "../DFW2/Header.h"

enum eFCResult
{
	FC_OK,
	FC_ERROR,
	FC_BUFFEROVERFLOW
};

typedef unsigned int BITWORD;
#define PREDICTOR_ORDER 4

class CBitStream
{
protected:
	bool Check();
	// сколько бит осталось в слове
	inline size_t WordBitsLeft() const
	{
		return WordBitCount - m_nBitSeek;
	}
	eFCResult MoveBitCount(size_t BitCount);			// сдвинуть битовое смещение, выбрать новое слово, если нужно, проверить переполнение
	ptrdiff_t m_nTotalBitsWritten;						// количество записанных битов
	BITWORD *m_pWord;									// указатель на текущее слово
	BITWORD *m_pWordInitial;							// указатель на начало буфера
	BITWORD *m_pEnd;									// указатель на конец буфера
	ptrdiff_t m_nBitSeek;								// битовое смещение относительно слова
public:
	CBitStream(BITWORD *Buffer, BITWORD *BufferEnd, size_t BitSeek);
	CBitStream();
	void Init(BITWORD *Buffer, BITWORD *BufferEnd, size_t BitSeek);
	void Reset();										// сбросить смещение, слово и количество записанных битов, очистить буфер
	void Rewind();										// то же что Reset, но без очистки буфера

	eFCResult WriteBits(CBitStream& Source, size_t BitCount);
	eFCResult WriteByte(unsigned char Byte);
	eFCResult WriteDouble(double &dValue);
	eFCResult ReadByte(unsigned char& Byte);
	eFCResult AlignTo(size_t nAlignTo);					// выровнять битовое смещение кратно заданному
	size_t BytesWritten();								// количество записанных байт
	size_t BitsLeft();									// количество доступных бит до конца буфера на чтении или записи
	BITWORD* Buffer();									// указатель на слово начала буфера
	unsigned char* BytesBuffer();						// указатель на байт начала буфера
	static const size_t WordBitCount;
};

class CCompressorBase
{
protected:
	double ys[PREDICTOR_ORDER];
public:
	typedef eFCResult(*fnWriteDoublePtr)(double&, double&, CBitStream&);				// прототип функции записи double
	typedef uint32_t(*fnCountZeros32Ptr)(uint32_t);										// прототип функции подсчета нулевых битов
	eFCResult WriteDouble(double& dValue, double& dPredictor, CBitStream& Output);
	eFCResult ReadDouble(double& dValue, double& dPredictor, CBitStream& Input);
	eFCResult WriteLEB(uint64_t Value, CBitStream& Output);
	eFCResult ReadLEB(uint64_t& Value, CBitStream& Input);
	static void Xor(double& dValue, double& dPredictor);
	static const fnWriteDoublePtr pFnWriteDouble;
	static const fnCountZeros32Ptr pFnCountZeros32;
	// платформонезависимая запись сжатого double 
	static eFCResult WriteDoublePlain(double& dValue, double& dPredictor, CBitStream& Output);
	// подсчет нулевых битов по таблице
	static uint32_t CLZ1(uint32_t x);
	// подсчет нулевых битов инструкцией CPU
	static uint32_t CLZ_LZcnt32(uint32_t x);
	// выбор функции записи double в зависимости от платформы
	static fnWriteDoublePtr AssignDoubleWriter();
	// выбор функции подсчета нулевых битов в зависимости от платформы
	static fnCountZeros32Ptr AssignZeroCounter();
#ifdef _WIN64
	// запись double для x64-процессора с поддержкой __lzcnt64
	static eFCResult WriteDoubleLZcnt64(double& dValue, double& dPredictor, CBitStream& Output);
#endif
};


class CCompressorSingle : public CCompressorBase
{
protected:
	double ts[PREDICTOR_ORDER];
	ptrdiff_t m_nPredictorOrder;
public:
	CCompressorSingle();
	virtual double Predict(double t);
	void UpdatePredictor(double& y, double dTolerance);
	void UpdatePredictor(double y);
};


class CCompressorParallel : public CCompressorBase
{
public:
	virtual double Predict(double t, bool bPredictorReset, ptrdiff_t nPredictorOrder, const double * const pts);
	void UpdatePredictor(double y, ptrdiff_t nPredictorOrder);
	void UpdatePredictor(double& y, ptrdiff_t nPredictorOrder, double dTolerance);
	void ResetPredictor(ptrdiff_t nPredictorOrder);
};
