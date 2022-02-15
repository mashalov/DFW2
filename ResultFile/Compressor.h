/*
* Модуль сжатия результатов расчета электромеханических переходных процессов
* (С) 2018 - Н.В. Евгений Машалов
* Свидетельство о государственной регистрации программы для ЭВМ №2021663231
* https://fips.ru/EGD/c7c3d928-893a-4f47-a218-51f3c5686860
*
* Transient stability simulation results compression library
* (C) 2018 - present Eugene Mashalov
* pat. RU №2021663231
* 
* Реализует предиктивный энкодер для вещественных чисел 
* https://www.elibrary.ru/item.asp?id=32269427 (C) 2018 Евгений Машалов
* 
*/

#pragma once
#include "../DFW2/Header.h"

enum class eFCResult
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
		return WordBitCount - BitSeek_;
	}
	eFCResult MoveBitCount(size_t BitCount);			// сдвинуть битовое смещение, выбрать новое слово, если нужно, проверить переполнение
	ptrdiff_t TotalBitsWritten_;						// количество записанных битов
	BITWORD *pWord_;									// указатель на текущее слово
	BITWORD *pWordInitial_;								// указатель на начало буфера
	BITWORD *pEnd_;										// указатель на конец буфера
	ptrdiff_t BitSeek_;									// битовое смещение относительно слова
public:
	CBitStream(BITWORD *pBuffer, BITWORD *pBufferEnd, size_t BitSeek);
	CBitStream();
	void Init(BITWORD *pBuffer, BITWORD *pBufferEnd, size_t BitSeek);
	void Reset();										// сбросить смещение, слово и количество записанных битов, очистить буфер
	void Rewind();										// то же что Reset, но без очистки буфера

	eFCResult WriteBits(CBitStream& Source, size_t BitCount);
	eFCResult WriteByte(unsigned char Byte);
	eFCResult WriteDouble(double &Value);
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
	double ys[PREDICTOR_ORDER] = {};
public:
	typedef eFCResult(*fnWriteDoublePtr)(double&, double&, CBitStream&);				// прототип функции записи double
	typedef uint32_t(*fnCountZeros32Ptr)(uint32_t);										// прототип функции подсчета нулевых битов
	eFCResult WriteDouble(double& Value, double& Predictor, CBitStream& Output);
	eFCResult ReadDouble(double& Value, double& Predictor, CBitStream& Input);
	eFCResult WriteLEB(uint64_t Value, CBitStream& Output);
	eFCResult ReadLEB(uint64_t& Value, CBitStream& Input);
	static void Xor(double& Value, double& Predictor);
	static const fnWriteDoublePtr pFnWriteDouble;
	static const fnCountZeros32Ptr pFnCountZeros32;
	// платформонезависимая запись сжатого double 
	static eFCResult WriteDoublePlain(double& Value, double& Predictor, CBitStream& Output);
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
	static eFCResult WriteDoubleLZcnt64(double& Value, double& Predictor, CBitStream& Output);
#endif
};


class CCompressorSingle : public CCompressorBase
{
protected:
	double ts[PREDICTOR_ORDER] = {};
	ptrdiff_t PredictorOrder_;
public:
	CCompressorSingle();
	virtual double Predict(double t);
	void UpdatePredictor(double& y, double Tolerance);
	void UpdatePredictor(double y);
};


class CCompressorParallel : public CCompressorBase
{
public:
	virtual double Predict(double t, bool bPredictorReset, ptrdiff_t PredictorOrder, const double * const pts);
	void UpdatePredictor(double y, ptrdiff_t PredictorOrder);
	void UpdatePredictor(double& y, ptrdiff_t PredictorOrder, double Tolerance);
	void ResetPredictor(ptrdiff_t PredictorOrder);
};
