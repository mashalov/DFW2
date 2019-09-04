#include "stdafx.h"
#include "Compressor.h"
#include <intrin.h>

eFCResult CCompressorBase::ReadDouble(double& dValue, double& dPredictor, CBitStream& Input)
{
	unsigned int nz = 0;
	dValue = 0.0;
	CBitStream NzCount(&nz, &nz + 1, 0);
	eFCResult fcResult = NzCount.WriteBits(Input, 4);

	if (fcResult == FC_OK)
	{
		unsigned int *pd = static_cast<unsigned int*>(static_cast<void*>(&dValue));
		CBitStream Dbl(pd, pd + 2, 0);
		fcResult = Dbl.WriteBits(Input, sizeof(double) * 8 - nz * 4);
		if (fcResult == FC_OK)
			Xor(dValue, dPredictor);
	}
	return fcResult;
}

// проверка доступности __lzcnt
bool IsLzcntAvailable()
{
	int cpufeats[4];
	__cpuid(cpufeats, 0x80000001);
	return cpufeats[2] & 0x20;
}

CCompressorBase::fnWriteDoublePtr CCompressorBase::AssignDoubleWriter()
{
#ifdef _WIN64
	if(IsLzcntAvailable())
		return CCompressorBase::WriteDoubleLZcnt64;		// если доступна __lzcnt64 - используем более быструю функцию сжати€
	return CCompressorBase::WriteDoublePlain;			// иначе - платформонезависимую
#else
	return CCompressorBase::WriteDoublePlain;
#endif
}

CCompressorBase::fnCountZeros32Ptr CCompressorBase::AssignZeroCounter()
{
	if (IsLzcntAvailable())
		return CCompressorBase::CLZ_LZcnt32;			// если lzcnt доступна - используем ее дл€ подсчета нулевых битов
	return CCompressorBase::CLZ1;						// иначе считаем нулевые биты по таблице
}

const CCompressorBase::fnWriteDoublePtr CCompressorBase::pFnWriteDouble = CCompressorBase::AssignDoubleWriter();
const CCompressorBase::fnCountZeros32Ptr CCompressorBase::pFnCountZeros32 = CCompressorBase::AssignZeroCounter();

eFCResult CCompressorBase::WriteDouble(double& dValue, double& dPredictor, CBitStream& Output)
{
	// вызываем функцию сжати€ по указателю, который инициализируем в рантайме в зависимости от платформы
	return (*pFnWriteDouble)(dValue, dPredictor, Output);
}

void CCompressorBase::Xor(double& dValue, double& dPredictor)
{
	__int64 *pv = static_cast<__int64*>(static_cast<void*>(&dValue));
	__int64 *pd = static_cast<__int64*>(static_cast<void*>(&dPredictor));
	*pv ^= *pd;
}

eFCResult CCompressorBase::WriteLEB(unsigned __int64 Value, CBitStream& Output)
{
	eFCResult fcResult = Output.AlignTo(8);
	if (fcResult == FC_OK)
	{
		do
		{
			unsigned char low = Value & 0x7f;
			Value >>= 7;
			if (Value != 0)
				low |= 0x80;
			fcResult = Output.WriteByte(low);
			if (fcResult != FC_OK)
				break;
		} while (Value != 0);
	}
	return fcResult;
}

eFCResult CCompressorBase::ReadLEB(unsigned __int64& Value, CBitStream& Input)
{
	eFCResult fcResult = Input.AlignTo(8);
	if (fcResult == FC_OK)
	{
		Value = 0;
		ptrdiff_t shift = 0;
		unsigned char low;
		do
		{
			// limit byte count to 8
			if (shift > 63)
			{
				fcResult = FC_ERROR;
				break;
			}

			fcResult = Input.ReadByte(low);
			if (fcResult != FC_OK)
				break;
			Value |= ((static_cast<unsigned __int64>(low)& 0x7f) << shift);
			shift += 7;
		} while (low & 0x80);
	}
	return FC_OK;
}


CCompressorSingle::CCompressorSingle() : CCompressorBase(), m_nPredictorOrder(0) { }

void CCompressorSingle::UpdatePredictor(double y)
{
	if (m_nPredictorOrder >= PREDICTOR_ORDER)
		ys[PREDICTOR_ORDER - 1] = y;
	else
	{
		ys[m_nPredictorOrder] = y;
		m_nPredictorOrder++;
	}
}

void CCompressorSingle::UpdatePredictor(double& y, double dTolerance)
{
	if (m_nPredictorOrder > 0 && fabs(y - ys[m_nPredictorOrder - 1]) < dTolerance)
		y = ys[m_nPredictorOrder - 1];

	UpdatePredictor(y);
}



double CCompressorSingle::Predict(double t)
{
	if (m_nPredictorOrder > 0 )
	{
		bool bReset = false;
		for (int j = 0; j < m_nPredictorOrder; j++)
		{
			if (Equal(t, ts[j]))
			{
				bReset = true;
				break;
			}
		}

		if(bReset)
		{
			// Reset predictor
			if (m_nPredictorOrder > 0)
			{
				if (m_nPredictorOrder >= PREDICTOR_ORDER)
					ys[0] = ys[PREDICTOR_ORDER - 1];
				else
					ys[0] = ys[m_nPredictorOrder - 1];
			}

			// если текущее и предыдущее времена одинаковые, сбрасываем предиктор
			m_nPredictorOrder = 0;

			ts[0] = t;
			// но вовзвращаем не ноль, а предыдущее значение
			return ys[0];
		}
	}

	if (m_nPredictorOrder >= PREDICTOR_ORDER)
	{
		// текущее и предыдущее времена разные
		double Pred = 0.0;
		bool bIdenctical = true;

		for (int j = 0; j < PREDICTOR_ORDER; j++)
		{
			double l = 1;
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

		memcpy(ts, ts + 1, sizeof(double) * (PREDICTOR_ORDER - 1));
		memcpy(ys, ys + 1, sizeof(double) * (PREDICTOR_ORDER - 1));
		ts[PREDICTOR_ORDER - 1] = t;
		return Pred;
	}
	else
	{
		// пор€док предиктора не достиг заданного
		ts[m_nPredictorOrder] = t;

		// если еще ничего не предсказывали, возвращаем ноль
		// иначе - предыдущее значение
		if (!m_nPredictorOrder)
			return 0.0;
		else
			return ys[m_nPredictorOrder - 1];
	}
}


void CCompressorParallel::ResetPredictor(ptrdiff_t nPredictorOrder)
{
	if (nPredictorOrder >= PREDICTOR_ORDER)
		ys[0] = ys[PREDICTOR_ORDER - 1];
	else
		ys[0] = ys[nPredictorOrder - 1];
}

void CCompressorParallel::UpdatePredictor(double y, ptrdiff_t nPredictorOrder)
{
	if (nPredictorOrder >= PREDICTOR_ORDER)
		ys[PREDICTOR_ORDER - 1] = y;
	else
		ys[nPredictorOrder] = y;
}

void CCompressorParallel::UpdatePredictor(double& y, ptrdiff_t nPredictorOrder, double dTolerance)
{
	if (nPredictorOrder > 0 && fabs(y - ys[nPredictorOrder - 1]) < dTolerance)
		y = ys[nPredictorOrder - 1];

	UpdatePredictor(y, nPredictorOrder);
}

double CCompressorParallel::Predict(double t, bool bPredictorReset, ptrdiff_t nPredictorOrder, const double * const pts)
{
	if (nPredictorOrder >= PREDICTOR_ORDER)
	{
		double Pred = 0.0;
		double dErr = 0.0;

		bool bIdenctical = true;

		for (int j = 0; j < PREDICTOR_ORDER; j++)
		{
			Pred += ys[j] * pts[j];

			// try use modified lagrange or newton 
			if(j < PREDICTOR_ORDER - 1 && bIdenctical)
			if (memcmp(ys + j, ys + j + 1, sizeof(double)))
				bIdenctical = false;
		}

		if (bIdenctical)
			Pred = *ys;

		memcpy(ys, ys + 1, sizeof(double) * (PREDICTOR_ORDER - 1));
		return Pred;
	}
	else
	{
		// если пор€док предиктора 0, но взведен флаг, 
		// используем дл€ предиктора запомненное значение, а не ноль.
		if (bPredictorReset)
			return ys[0];

		// флага нет, значит сброса не было, и пор€док предиктора действительно 0
		// предыдущих значений нет, и возвращаем ноль
		if (!nPredictorOrder)
			return 0.0;
		else
			return ys[nPredictorOrder - 1];
	}
}

#ifdef _WIN64
eFCResult CCompressorBase::WriteDoubleLZcnt64(double& dValue, double& dPredictor, CBitStream& Output)
{
	eFCResult Result = FC_OK;
	Xor(dValue, dPredictor);
	unsigned __int64 *pv = static_cast<unsigned __int64*>(static_cast<void*>(&dValue));
	ptrdiff_t nZ4count = 0;
	unsigned int *pZ4 = static_cast<unsigned int*>(static_cast<void*>(&nZ4count));
	CBitStream Source(pZ4, pZ4 + sizeof(ptrdiff_t), 0);
	nZ4count = __lzcnt64(*pv) >> 2;

	if (nZ4count == 16)
		nZ4count--;

	ptrdiff_t ZeroBitsCount = nZ4count << 2;
	

	if (Output.BitsLeft() >= 68 - ZeroBitsCount)
	{
		Result = Output.WriteBits(Source, 4);
		if (Result == FC_OK)
		{
			BITWORD *pvb = static_cast<BITWORD*>(static_cast<void*>(pv));
			BITWORD *pve = static_cast<BITWORD*>(static_cast<void*>(pv+1));
			CBitStream SourceDbl(pvb, pve, 0);
			Result = Output.WriteBits(SourceDbl, sizeof(__int64) * 8 - ZeroBitsCount);
		}
	}
	else
	{
		Xor(dValue, dPredictor);
		Result = FC_BUFFEROVERFLOW;
	}

	return Result;
}
#endif

eFCResult CCompressorBase::WriteDoublePlain(double& dValue, double& dPredictor, CBitStream& Output)
{
	eFCResult Result = FC_OK;

	Xor(dValue, dPredictor);
	unsigned int *ppv = static_cast<unsigned int*>(static_cast<void*>(&dValue)) + 1;
	unsigned int pv = *ppv;
	ptrdiff_t nZ4count = 0;
	unsigned int *pZ4 = static_cast<unsigned int*>(static_cast<void*>(&nZ4count));
	CBitStream Source(pZ4, pZ4 + sizeof(ptrdiff_t), 0);

	// если отличаетс€ только бит знака - была иде€ обнул€ть разницу. Ќо это приводит
	// к сбою предиктора при чтении - предиктор делаетс€ отрицательный и симметрично уводит в минус
	// записанные значени€, которые исходно были положительные
	// поэтому обнуление разницы убрано - будут записыватьс€ все 80 бит, но так как только старший 
	// единица а все остальные нули - их сожмет RLE
	/*
	if (pv == 0x80000000 && *(ppv - 1) == 0)
		pv = 0x80000000;
	*/

	if (!pv)
	{
		if (*(ppv - 1))
		{
			// bits in the lower int only
			//nZ4count = 8;

			pv = *static_cast<int*>(static_cast<void*>(&dValue));

			/*
			mask = 0xf << 28;
			while (nZ4count < 16)
			{
				if (pv & mask)
					break;
				nZ4count++;
				mask >>= 4;
			}
			*/

			//nZ4count = 8 + (CLZ1(pv) >> 2);
			nZ4count = 8 + ((CCompressorBase::pFnCountZeros32)(pv) >> 2);

			if (Output.BitsLeft() >= 68 - nZ4count * 4)
			{
				Result = Output.WriteBits(Source, 4);
				if (Result == FC_OK)
				{
					CBitStream SourceDbl2(ppv - 1, ppv, 0);
					Result = Output.WriteBits(SourceDbl2, sizeof(int) * 8 - (nZ4count - 8) * 4);
				}
			}
			else
			{
				Xor(dValue, dPredictor);
				Result = FC_BUFFEROVERFLOW;
			}
		}
		else
		{
			nZ4count = 15;
			if (Output.BitsLeft() >= 68 - nZ4count * 4)
			{
				Result = Output.WriteBits(Source, 4);
				if (Result == FC_OK)
				{
					CBitStream SourceDbl2(ppv - 1, ppv, 0);
					Result = Output.WriteBits(SourceDbl2, 4);
				}
			}
			else
			{
				Xor(dValue, dPredictor);
				Result = FC_BUFFEROVERFLOW;
			}
		}
	}
	else
	{
		// bits in the higher int too
		/*
		while (nZ4count < 8)
		{
			if (pv & mask)
				break;
			nZ4count++;
			mask >>= 4;
		}
		*/

		//nZ4count = CLZ1(pv) >> 2;
		nZ4count = (CCompressorBase::pFnCountZeros32)(pv) >> 2;

		if (Output.BitsLeft() >= 68 - nZ4count * 4)
		{
			Result = Output.WriteBits(Source, 4);
			if (Result == FC_OK)
			{
				CBitStream SourceDbl1(ppv - 1, ppv, 0);
				Result = Output.WriteBits(SourceDbl1, sizeof(int) * 8);
				if (Result == FC_OK)
				{
					CBitStream SourceDbl2(ppv, ppv + 1, 0);
					Result = Output.WriteBits(SourceDbl2, sizeof(int) * 8 - nZ4count * 4);
				}
			}
		}
		else
		{
			Xor(dValue, dPredictor);
			Result = FC_BUFFEROVERFLOW;
		}
	}
	return Result;
}

uint32_t CCompressorBase::CLZ_LZcnt32(uint32_t x)
{
	return __lzcnt(x);
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