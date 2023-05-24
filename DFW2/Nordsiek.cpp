#include "stdafx.h"
#include "DynaModel.h"
#include <immintrin.h>

//#define _AVX2

using namespace DFW2;

void CDynaModel::InitDevicesNordsiek()
{
	ChangeOrder(1);
	for (auto&& it : DeviceContainers_)
		it->InitNordsieck(this);
}

void CDynaModel::InitNordsiek()
{
	InitDevicesNordsiek();

	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		InitNordsiekElement(pVectorBegin,Atol(),Rtol());

	sc.StepChanged();
	sc.OrderChanged();
	sc.SetNordsiekScaledForH(0.0);
	sc.SetNordsiekScaledForHSaved(0.0);
	sc.m_bNordsiekSaved = false;
}

// построение Nordsieck после того, как обнаружено что текущая история
// ненадежна
void CDynaModel::ReInitializeNordsiek()
{
	
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	if (sc.m_bNordsiekSaved)
	{
		// если есть сохраненный шаг
		for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		{
			// значение восстанавливаем
			pVectorBegin->Nordsiek[0] = pVectorBegin->SavedNordsiek[0];
			*pVectorBegin->pValue = pVectorBegin->Nordsiek[0];
			// а элемент первого порядка конструируем по разности между предыдущим и пред-предыдущим значениями
			// т.к. первый элемент это hy', а y' = (y(-1) - y(-2)) / h. Note - мы берем производную с пред-предыдущего шага

			//											y(-1)						y(-2)
			pVectorBegin->Nordsiek[1] = pVectorBegin->SavedNordsiek[0] - pVectorBegin->Tminus2Value;
			pVectorBegin->SavedError = 0.0;
		}
		// масшатабируем Nordsieck на заданный шаг
		sc.SetNordsiekScaledForH(sc.NordsiekScaledForHSaved());
		RescaleNordsiek();
		BuildDerivatives();
	}
	sc.OrderChanged();
	sc.StepChanged();
	sc.ResetStepsToFail();
}

// сохранение копии Nordsieck перед выполнением шага
void CDynaModel::SaveNordsiek()
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
#ifdef _AVX2
		_mm256_store_pd(pVectorBegin->Nordsiek, _mm256_load_pd(pVectorBegin->SavedNordsiek));
#else
		double *pN = pVectorBegin->Nordsiek;
		double *pS = pVectorBegin->SavedNordsiek;

		// сохраняем пред-предыдущее значение переменной состояния
		pVectorBegin->Tminus2Value = *pS;

		// запоминаем три элемента, не смотря на текущий порядок
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
		*pS = *pN; pS++; pN++;
#endif

		pVectorBegin->SavedError = pVectorBegin->Error;
	}
	StoreUsedH();
	sc.SetNordsiekScaledForHSaved(sc.NordsiekScaledForH());
	sc.SetNordsiekScaledForH(H());
	sc.m_bNordsiekSaved = true;
}

// масштабирование Nordsieck на заданный коэффициент изменения шага
void CDynaModel::RescaleNordsiek()
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };
	
	// расчет выполняется путем умножения текущего Nordsieck на диагональную матрицу C[q+1;q+1]
	// с элементами C[i,i] = r^(i-1) [Lsode 2.64]

	_ASSERTE(sc.NordsiekScaledForH() > 0.0);
	const double r{ H() / sc.NordsiekScaledForH() };
	if (r == 1.0)
		return;

	LogTime(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format("Nordsiek rescale {}->{}", 
		sc.NordsiekScaledForH(), 
		H()));

	const double crs[4] = { 1.0, r , (sc.q == 2) ? r * r : 1.0 , 1.0};
#ifdef _AVX2
	const __m256d rs = _mm256_load_pd(crs);
#endif
	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
	{
#ifdef _AVX2
		__m256d nord = _mm256_load_pd(pVectorBegin->Nordsiek);
		nord = _mm256_mul_pd(nord, rs);
		_mm256_store_pd(pVectorBegin->Nordsiek,nord);
#else
		for (ptrdiff_t j = 1; j < sc.q + 1; j++)
			pVectorBegin->Nordsiek[j] *= crs[j];
#endif
	}

	// вызываем функции обработки изменения шага и порядка
	// пока они блокируют дальнейшее увеличение шага на протяжении
	// заданного количества шагов
	sc.StepChanged();
	sc.OrderChanged();
	sc.SetNordsiekScaledForH(H());

	// рассчитываем коэффициент изменения шага
	const double dRefactorRatio{ H() / sc.m_dLastRefactorH};
	// если шаг изменился более в заданное количество раз - взводим флаг рефакторизации Якоби
	// sc.m_dLastRefactorH обновляется после рефакторизации
	if (dRefactorRatio > m_Parameters.m_dRefactorByHRatio || 1.0 / dRefactorRatio > m_Parameters.m_dRefactorByHRatio)
		sc.RefactorMatrix();
}

void CDynaModel::ConstructNordsiekOrder()
{
	const RightVector* const pVectorEnd{ pRightVector + klu.MatrixSize() };

	for (RightVector* pVectorBegin = pRightVector; pVectorBegin < pVectorEnd; pVectorBegin++)
		pVectorBegin->Nordsiek[2] = pVectorBegin->Error / 2.0;
}


void CDynaModel::InitNordsiekElement(struct RightVector *pVectorBegin, double Atol, double Rtol)
{
	std::fill(pVectorBegin->Nordsiek + 1, pVectorBegin->Nordsiek + 3, 0.0);
	std::fill(pVectorBegin->SavedNordsiek, pVectorBegin->SavedNordsiek + 3, 0.0);
	pVectorBegin->EquationType = DET_ALGEBRAIC;
	pVectorBegin->PrimitiveBlock = PBT_UNKNOWN;
	pVectorBegin->Atol = Atol;
	pVectorBegin->Rtol = Rtol;
	pVectorBegin->SavedError = pVectorBegin->Tminus2Value = 0.0;
	pVectorBegin->ErrorHits = 0;
	pVectorBegin->RingsCount = 0;
	pVectorBegin->RingsSuppress = 0;
}

void CDynaModel::PrepareNordsiekElement(struct RightVector *pVectorBegin)
{
	pVectorBegin->Error = 0.0;
	pVectorBegin->PhysicalEquationType = DET_ALGEBRAIC;
}

