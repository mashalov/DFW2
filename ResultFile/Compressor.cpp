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
* https://www.elibrary.ru/item.asp?id=32269427 (C) Евгений Машалов, 2017
*
*/

#include "stdafx.h"
#include "Compressor.h"

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#include <cpuid.h>
#endif 

// чтение double и его декодирование по предиктору
eFCResult CCompressorBase::ReadDouble(double& Value, double& Predictor, CBitStream& Input)
{
	// количество нулевых бит
	BITWORD nz(0);
	// обнуляем double
	Value = 0.0;
	// готовим буфер для чтения количества нулевых бит
	CBitStream NzCount(&nz, &nz + 1, 0);
	// читаем количетсво нулевых бит, деленное на 4 (4 бита)
	eFCResult fcResult{ NzCount.WriteBits(Input, 4) };

	if (fcResult == eFCResult::FC_OK)
	{
		// готовим буфер для чтения битов 
		BITWORD* pd{ static_cast<BITWORD*>(static_cast<void*>(&Value)) };
		CBitStream Dbl(pd, pd + sizeof(double) / sizeof(BITWORD), 0);
		// читаем ненулевые биты
		fcResult = Dbl.WriteBits(Input, sizeof(double) * 8 - nz * 4);
		// если нет ошибок - декодируем double
		if (fcResult == eFCResult::FC_OK)
			Xor(Value, Predictor);
	}
	return fcResult;
}

// проверка доступности __lzcnt
bool IsLzcntAvailable()
{
#ifdef _MSC_VER	
	int cpufeats[4];
	__cpuid(cpufeats, 0x80000001);
	return cpufeats[2] & 0x20;
#else
	unsigned int eax(0), ebx(0), ecx(0), edx(0);
	__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
	return ecx & 0x20;
#endif	
}

CCompressorBase::fnWriteDoublePtr CCompressorBase::AssignDoubleWriter()
{
#ifdef _WIN64
	if(IsLzcntAvailable())
		return CCompressorBase::WriteDoubleLZcnt64;		// если доступна __lzcnt64 - используем более быструю функцию сжатия
	return CCompressorBase::WriteDoublePlain;			// иначе - платформонезависимую
#else
	return CCompressorBase::WriteDoublePlain;
#endif
}

CCompressorBase::fnCountZeros32Ptr CCompressorBase::AssignZeroCounter()
{
	if (IsLzcntAvailable())
		return CCompressorBase::CLZ_LZcnt32;			// если lzcnt доступна - используем ее для подсчета нулевых битов
	return CCompressorBase::CLZ1;						// иначе считаем нулевые биты по таблице
}

const CCompressorBase::fnWriteDoublePtr CCompressorBase::pFnWriteDouble = CCompressorBase::AssignDoubleWriter();
const CCompressorBase::fnCountZeros32Ptr CCompressorBase::pFnCountZeros32 = CCompressorBase::AssignZeroCounter();

eFCResult CCompressorBase::WriteDouble(double& Value, double& Predictor, CBitStream& Output)
{
	// вызываем функцию сжатия по указателю, который инициализируем в рантайме в зависимости от платформы
	return (*pFnWriteDouble)(Value, Predictor, Output);
}

void CCompressorBase::Xor(double& Value, double& Predictor)
{
	uint64_t* pv{ static_cast<uint64_t*>(static_cast<void*>(&Value)) };
	uint64_t* pd{ static_cast<uint64_t*>(static_cast<void*>(&Predictor)) };
	*pv ^= *pd;
}

eFCResult CCompressorBase::WriteLEB(uint64_t Value, CBitStream& Output)
{
	eFCResult fcResult{ Output.AlignTo(8) };
	if (fcResult == eFCResult::FC_OK)
	{
		do
		{
			unsigned char low( Value & 0x7f );
			Value >>= 7;
			if (Value != 0)
				low |= 0x80;
			fcResult = Output.WriteByte(low);
			if (fcResult != eFCResult::FC_OK)
				break;
		} while (Value != 0);
	}
	return fcResult;
}

eFCResult CCompressorBase::ReadLEB(uint64_t& Value, CBitStream& Input)
{
	eFCResult fcResult{ Input.AlignTo(8) };
	if (fcResult == eFCResult::FC_OK)
	{
		Value = 0;
		ptrdiff_t shift{ 0 };
		unsigned char low;
		do
		{
			// limit byte count to 8
			if (shift > 63)
			{
				fcResult = eFCResult::FC_ERROR;
				break;
			}

			fcResult = Input.ReadByte(low);
			if (fcResult != eFCResult::FC_OK)
				break;
			Value |= ((static_cast<uint64_t>(low)& 0x7f) << shift);
			shift += 7;
		} while (low & 0x80);
	}
	return eFCResult::FC_OK;
}


CCompressorSingle::CCompressorSingle() : CCompressorBase(), PredictorOrder_(0) { }

void CCompressorSingle::UpdatePredictor(double y)
{
	if (PredictorOrder_ >= PREDICTOR_ORDER)
		ys[PREDICTOR_ORDER - 1] = y;
	else
	{
		ys[PredictorOrder_] = y;
		PredictorOrder_++;
	}
}

void CCompressorSingle::UpdatePredictor(double& y, double Tolerance)
{
	if (PredictorOrder_ > 0 && std::abs(y - ys[PredictorOrder_ - 1]) < Tolerance)
		y = ys[PredictorOrder_ - 1];

	UpdatePredictor(y);
}

void CCompressorParallel::ResetPredictor(ptrdiff_t PredictorOrder)
{
	if (PredictorOrder >= PREDICTOR_ORDER)
		ys[0] = ys[PREDICTOR_ORDER - 1];
	else
		ys[0] = ys[PredictorOrder - 1];
}

void CCompressorParallel::UpdatePredictor(double y, ptrdiff_t PredictorOrder)
{
	if (PredictorOrder >= PREDICTOR_ORDER)
		ys[PREDICTOR_ORDER - 1] = y;
	else
		ys[PredictorOrder] = y;
}

void CCompressorParallel::UpdatePredictor(double& y, ptrdiff_t PredictorOrder, double Tolerance)
{
	if (PredictorOrder > 0 && std::abs(y - ys[PredictorOrder - 1]) < Tolerance)
		y = ys[PredictorOrder - 1];

	UpdatePredictor(y, PredictorOrder);
}

#ifdef _WIN64
// Запись сжатого double на системе с доступной командой __lzcnt64
eFCResult CCompressorBase::WriteDoubleLZcnt64(double& Value, double& Predictor, CBitStream& Output)
{
	eFCResult Result{ eFCResult::FC_OK };
	Xor(Value, Predictor);
	uint64_t* pv{ static_cast<uint64_t*>(static_cast<void*>(&Value)) };
	// формируем буфер для записи количества нулевых битов
	uint64_t nZ4count(0);
	BITWORD *pZ4(static_cast<BITWORD*>(static_cast<void*>(&nZ4count)));
	CBitStream Source(pZ4, pZ4 + sizeof(BITWORD), 0);

	// считаем количество нулевых младших бит и делим на 4
	// lzcnt считает количество старших битов - то есть начиная от знака и экспоненты
	// double при этом расчете представлено от старших к младшим
	nZ4count = __lzcnt64(*pv) >> 2;
	// если все 64 бита нулевые 
	// это не подойдет для формата
	// делаем 60 бит, записываем 0xF0
	if (nZ4count == 16)
		nZ4count--;
	// считаем количество нулевых битов, округленное до 4
	size_t ZeroBitsCount{ nZ4count << 2 };
	
	// если в буфере осталось место для записи ненулевых битов
	// 64 + 4 = 68 - максимальное количество бит
	if (Output.BitsLeft() >= 68 - ZeroBitsCount)
	{
		// записываем количество нулевых битов деленное на 4
		Result = Output.WriteBits(Source, 4);
		// если записалось нормально
		if (Result == eFCResult::FC_OK)
		{
			// формируем буфер для записи double
			BITWORD* pvb{ static_cast<BITWORD*>(static_cast<void*>(pv)) };
			BITWORD* pve{ static_cast<BITWORD*>(static_cast<void*>(pv + 1)) };
			CBitStream SourceDbl(pvb, pve, 0);
			// и записываем только ненулевые биты
			// double в памяти лежит от младших битов к старшим, так что записываем ненулевые младшие
			Result = Output.WriteBits(SourceDbl, sizeof(uint64_t) * 8 - ZeroBitsCount);
		}
		// если количество битов не записалось - возвращаем результат (ошибка или переполенение)
	}
	else
	{
		// если в буфере не было достаточно места для
		// записи восстанавливаем исходный double и сбрасываем буфер
		Xor(Value, Predictor);
		Result = eFCResult::FC_BUFFEROVERFLOW;
	}
	return Result;
}
#endif

eFCResult CCompressorBase::WriteDoublePlain(double& Value, double& Predictor, CBitStream& Output)
{
	eFCResult Result{ eFCResult::FC_OK };
	Xor(Value, Predictor);
	unsigned int* ppv{ static_cast<unsigned int*>(static_cast<void*>(&Value)) + 1 };
	unsigned int pv{ *ppv };
	BITWORD nZ4count{ 0 };
	BITWORD *pZ4(&nZ4count);
	CBitStream Source(pZ4, pZ4 + sizeof(BITWORD), 0);

	// если отличается только бит знака - была идея обнулять разницу. Но это приводит
	// к сбою предиктора при чтении - предиктор делается отрицательный и симметрично уводит в минус
	// записанные значения, которые исходно были положительные
	// поэтому обнуление разницы убрано - будут записываться все 80 бит, но так как только старший 
	// единица а все остальные нули - их сожмет RLE

	if (!pv)
	{
		if (*(ppv - 1))
		{
			// bits in the lower int only
			//nZ4count = 8;

			pv = *static_cast<int*>(static_cast<void*>(&Value));

			//nZ4count = 8 + (CLZ1(pv) >> 2);
			nZ4count = 8 + ((CCompressorBase::pFnCountZeros32)(pv) >> 2);

			if (Output.BitsLeft() >= 68 - nZ4count * 4)
			{
				Result = Output.WriteBits(Source, 4);
				if (Result == eFCResult::FC_OK)
				{
					CBitStream SourceDbl2(static_cast<BITWORD*>(static_cast<void*>(ppv - 1)), static_cast<BITWORD*>(static_cast<void*>(ppv)), 0);
					Result = Output.WriteBits(SourceDbl2, sizeof(int) * 8 - (nZ4count - 8) * 4);
				}
			}
			else
			{
				Xor(Value, Predictor);
				Result = eFCResult::FC_BUFFEROVERFLOW;
			}
		}
		else
		{
			nZ4count = 15;
			if (Output.BitsLeft() >= 68 - nZ4count * 4)
			{
				Result = Output.WriteBits(Source, 4);
				if (Result == eFCResult::FC_OK)
				{
					CBitStream SourceDbl2(static_cast<BITWORD*>(static_cast<void*>(ppv - 1)), static_cast<BITWORD*>(static_cast<void*>(ppv)), 0);
					Result = Output.WriteBits(SourceDbl2, 4);
				}
			}
			else
			{
				Xor(Value, Predictor);
				Result = eFCResult::FC_BUFFEROVERFLOW;
			}
		}
	}
	else
	{

		//nZ4count = CLZ1(pv) >> 2;
		nZ4count = (CCompressorBase::pFnCountZeros32)(pv) >> 2;

		if (Output.BitsLeft() >= 68 - nZ4count * 4)
		{
			Result = Output.WriteBits(Source, 4);
			if (Result == eFCResult::FC_OK)
			{
				CBitStream SourceDbl1(static_cast<BITWORD*>(static_cast<void*>(ppv - 1)), static_cast<BITWORD*>(static_cast<void*>(ppv)), 0);
				Result = Output.WriteBits(SourceDbl1, sizeof(int) * 8);
				if (Result == eFCResult::FC_OK)
				{
					CBitStream SourceDbl2(static_cast<BITWORD*>(static_cast<void*>(ppv)), static_cast<BITWORD*>(static_cast<void*>(ppv + 1)), 0);
					Result = Output.WriteBits(SourceDbl2, sizeof(int) * 8 - nZ4count * 4);
				}
			}
		}
		else
		{
			Xor(Value, Predictor);
			Result = eFCResult::FC_BUFFEROVERFLOW;
		}
	}

	return Result;
}

uint32_t CCompressorBase::CLZ_LZcnt32(uint32_t x)
{
#ifdef _MSC_VER
	return __lzcnt(x);
#else
#ifdef __x86_64__
	return __builtin_clz(x); 
#else
	return __lzcnt32(x);
#endif
#endif
}


// https://embeddedgurus.com/state-space/2014/09/fast-deterministic-and-portable-counting-leading-zeros/

uint32_t CCompressorBase::CLZ1(uint32_t x) 
{
	static uint8_t const clz_lkup[] = { 
		32U, 31U, 30U, 30U, 29U, 29U, 29U, 29U,
		28U, 28U, 28U, 28U, 28U, 28U, 28U, 28U,
		27U, 27U, 27U, 27U, 27U, 27U, 27U, 27U,
		27U, 27U, 27U, 27U, 27U, 27U, 27U, 27U,
		26U, 26U, 26U, 26U, 26U, 26U, 26U, 26U,
		26U, 26U, 26U, 26U, 26U, 26U, 26U, 26U,
		26U, 26U, 26U, 26U, 26U, 26U, 26U, 26U,
		26U, 26U, 26U, 26U, 26U, 26U, 26U, 26U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		25U, 25U, 25U, 25U, 25U, 25U, 25U, 25U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U,
		24U, 24U, 24U, 24U, 24U, 24U, 24U, 24U
	};
	uint32_t n;
	if (x >= (1U << 16)) {
		if (x >= (1U << 24)) {
			n = 24U;
		}
		else {
			n = 16U;
		}
	}
	else {
		if (x >= (1U << 8)) {
			n = 8U;
		}
		else {
			n = 0U;
		}
	}
	return (uint32_t)clz_lkup[x >> n] - n;
}


#ifdef __GNUC__
#pragma GCC push options 
#pragma GCC optimize ("O0")
#endif

#ifdef _MSC_VER
#pragma optimize("", off)
#endif

double CCompressorSingle::Predict(double t)
{
	if (PredictorOrder_ > 0)
	{
		bool bReset{ false };
		for (int j = 0; j < PredictorOrder_; j++)
		{
			if (DFW2::Equal(t, ts[j]))
			{
				bReset = true;
				break;
			}
		}

		if (bReset)
		{
			// Reset predictor
			if (PredictorOrder_ > 0)
			{
				if (PredictorOrder_ >= PREDICTOR_ORDER)
					ys[0] = ys[PREDICTOR_ORDER - 1];
				else
					ys[0] = ys[PredictorOrder_ - 1];
			}

			// если текущее и предыдущее времена одинаковые, сбрасываем предиктор
			PredictorOrder_ = 0;

			ts[0] = t;
			// но вовзвращаем не ноль, а предыдущее значение
			return ys[0];
		}
	}

	if (PredictorOrder_ >= PREDICTOR_ORDER)
	{
		// текущее и предыдущее времена разные
		double Pred{ 0.0 };
		bool bIdenctical{ true };

		for (int j = 0; j < PREDICTOR_ORDER; j++)
		{
			double l{ 1 };
			for (int m = 0; m < PREDICTOR_ORDER; m++)
			{
				if (m != j)
					l *= (t - ts[m]) / (ts[j] - ts[m]);
				_CheckNumber(l);
			}

			Pred += ys[j] * l;

			// try use modified lagrange or newton 
			if (j < PREDICTOR_ORDER - 1 && bIdenctical)
				if (memcmp(ys + j, ys + j + 1, sizeof(double)))
					bIdenctical = false;
		}

		if (bIdenctical)
			Pred = *ys;

		std::copy(ts + 1, ts + PREDICTOR_ORDER, ts);
		std::copy(ys + 1, ys + PREDICTOR_ORDER, ys);
		ts[PREDICTOR_ORDER - 1] = t;
		return Pred;
	}
	else
	{
		// порядок предиктора не достиг заданного
		ts[PredictorOrder_] = t;

		// если еще ничего не предсказывали, возвращаем ноль
		// иначе - предыдущее значение
		if (!PredictorOrder_)
			return 0.0;
		else
			return ys[PredictorOrder_ - 1];
	}
	}

double CCompressorParallel::Predict(double t, bool bPredictorReset, ptrdiff_t PredictorOrder, const double* const pts)
{
	if (PredictorOrder >= PREDICTOR_ORDER)
	{
		double Pred{ 0.0 }, dErr{ 0.0 };
		bool bIdenctical{ true };

		for (int j = 0; j < PREDICTOR_ORDER; j++)
		{
			Pred += ys[j] * pts[j];

			// try use modified lagrange or newton 
			if (j < PREDICTOR_ORDER - 1 && bIdenctical)
				if (memcmp(ys + j, ys + j + 1, sizeof(double)))
					bIdenctical = false;
		}

		if (bIdenctical)
			Pred = *ys;

		std::copy(ys + 1, ys + PREDICTOR_ORDER, ys);
		return Pred;
	}
	else
	{
		// если порядок предиктора 0, но взведен флаг, 
		// используем для предиктора запомненное значение, а не ноль.
		if (bPredictorReset)
			return ys[0];

		// флага нет, значит сброса не было, и порядок предиктора действительно 0
		// предыдущих значений нет, и возвращаем ноль
		if (!PredictorOrder)
			return 0.0;
		else
			return ys[PredictorOrder - 1];
	}
}

#ifdef __GNUC__
#pragma GCC pop options
#endif

#ifdef _MSC_VER
#pragma optimize("", on)
#endif
