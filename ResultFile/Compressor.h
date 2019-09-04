#pragma once
#include "..\dfw2\Header.h"

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
	ptrdiff_t WordBitsLeft();
	eFCResult MoveBitCount(ptrdiff_t BitCount);
	ptrdiff_t m_nTotalBitsWritten;
	BITWORD *m_pWord;
	BITWORD *m_pWordInitial;
	BITWORD *m_pEnd;
	ptrdiff_t m_nBitSeek;
public:
	CBitStream(BITWORD *Buffer, BITWORD *BufferEnd, ptrdiff_t BitSeek);
	CBitStream();
	void Init(BITWORD *Buffer, BITWORD *BufferEnd, ptrdiff_t BitSeek);
	void Reset();
	eFCResult WriteBits(CBitStream& Source, ptrdiff_t BitCount);
	eFCResult WriteByte(unsigned char Byte);
	eFCResult WriteDouble(double &dValue);
	eFCResult ReadByte(unsigned char& Byte);
	eFCResult AlignTo(ptrdiff_t nAlignTo);
	ptrdiff_t BytesWritten();
	ptrdiff_t BitsLeft();
	BITWORD* Buffer();
	unsigned char* BytesBuffer();
	static const ptrdiff_t WordBitCount;
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
	eFCResult WriteLEB(unsigned __int64 Value, CBitStream& Output);
	eFCResult ReadLEB(unsigned __int64& Value, CBitStream& Input);
	static void Xor(double& dValue, double& dPredictor);
	static fnWriteDoublePtr pFnWriteDouble;
	static fnCountZeros32Ptr pFnCountZeros32;
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
