#include "stdafx.h"
#include "DynaLRC.h"

using namespace DFW2;

CDynaLRC::CDynaLRC() : CDevice(), 
					   P(nullptr),
					   Q(nullptr)
					   {}

bool CDynaLRC::SetNpcs(ptrdiff_t nPcsP, ptrdiff_t  nPcsQ)
{
	bool bRes = false;
	if (nPcsP >= 0 && nPcsQ >= 0)
	{
		m_NpcsP = nPcsP;
		m_NpcsQ = nPcsQ;
		if (P) delete P;
		if (Q) delete Q;
		P = new CLRCData[m_NpcsP];
		Q = new CLRCData[m_NpcsQ];
		bRes = true;
	}
	return bRes;
}

int CDynaLRC::CompSLCPoly(const void* V1, const void* V2)
{
	const CLRCData *v1 = static_cast<const CLRCData*>(V1);
	const CLRCData *v2 = static_cast<const CLRCData*>(V2);

	double v = v1->V - v2->V;

	if (Equal(v, 0.0))
		return 0;
	else
		return v > 0.0 ? 1 : -1;
}

double CDynaLRC::GetP(double VdivVnom, double dVicinity)
{
	CLRCData *v = P;

	if (m_NpcsP == 1)
	{
		return v->Get(VdivVnom);
	}
	double dP = 0.0;
	return GetBothInterpolatedHermite(P, m_NpcsP, VdivVnom, dVicinity, dP);
}

double CDynaLRC::GetBothInterpolatedHermite(CLRCData *pBase, ptrdiff_t nCount, double VdivVnom, double dVicinity, double &dLRC)
{
	// По умолчанию считаем что напряжение находится в последнем сегменте
	CLRCData *pHitV = pBase + nCount - 1;

	CLRCData *v = pBase;
	VdivVnom = max(0.0, VdivVnom);

	// ищем сегмент у которого напряжение больше заданного
	while (v < pBase + nCount)
	{
		if (v->V > VdivVnom)
		{
			pHitV = v - 1;
			break;
		}
		v++;
	}

	bool bLeft = false;
	bool bRight = false;


	if (pHitV->pPrev)
	{
		// если у найденного сегмента есть предыдущий сегмент
		// и напряжение с учетом радиуса сглаживания в него попадает
		// отмечаем что есть сегмент слева
		if (VdivVnom - dVicinity < pHitV->V)
			bLeft = true;
	}

	if (pHitV->pNext)
	{
		// если у найденного сегмента есть следующий сегмент
		// и напряжение с учетом радиуса сглаживания в него попадает
		// отмечаем что есть сегмент справа
		if (VdivVnom + dVicinity > pHitV->pNext->V)
			bRight = true;

		if (bLeft)
		{
			// если есть и левый и правый сегменты (и это в пределах радиуса)
			// выбрасываем тот который дальше от текущего напряжения
			if (VdivVnom - pHitV->V > pHitV->pNext->V - VdivVnom)
				bLeft = false;
			else
				bRight = false;
		}
	}


	//bLeft = bRight = false;

	if (bLeft || bRight)
	{
		_ASSERTE(!(bLeft && bRight));

		double x1, x2, y1, y2, k1 = 0.0, k2 = 0.0;

		// https://en.wikipedia.org/wiki/Spline_interpolation
		

		if (bLeft)
		{
			dVicinity = min(dVicinity, pHitV->dMaxRadius);
			x1 = pHitV->V - dVicinity;
			y1 = pHitV->pPrev->GetBoth(x1, k1);
			x2 = pHitV->V + dVicinity;
			y2 = pHitV->GetBoth(x2, k2);
		}
		else
		{
			dVicinity = min(dVicinity, pHitV->pNext->dMaxRadius);
			x1 = pHitV->pNext->V - dVicinity;
			y1 = pHitV->GetBoth(x1, k1);
			x2 = pHitV->pNext->V + dVicinity;
			y2 = pHitV->pNext->GetBoth(x2, k2);
		}

		double x2x1 = x2 - x1;
		double y2y1 = y2 - y1;

		double a = k1 * x2x1- y2y1;
		double b = -k2 * x2x1 + y2y1;
		double t = (VdivVnom - x1) / x2x1;
		if (t >= 0 && t <= 1.0)
		{
			double P = (1.0 - t) * y1 + t * y2 + t * (1.0 - t) * (a* (1.0 - t) + b * t);
			//dLRC = ((y2 - y1) + (1.0 - 2 * t) * (a * (1.0 - t) + b *t) + t * (1.0 - t) * (b - a)) / x2x1;
			dLRC = (a - y1 + y2 - (4.0 * a - 2.0 * b - 3.0 * t * (a - b)) * t) / x2x1;
			return P;
		}
	}
	return pHitV->GetBoth(VdivVnom, dLRC);
}

double CDynaLRC::GetPdP(double VdivVnom, double &dP, double dVicinity)
{
	CLRCData *v = P;
	if (m_NpcsP == 1)
	{
		return v->GetBoth(VdivVnom, dP);
	}
	return GetBothInterpolatedHermite(P, m_NpcsP, VdivVnom, dVicinity, dP);
}

double CDynaLRC::GetQdQ(double VdivVnom, double &dQ, double dVicinity)
{
	CLRCData *v = Q;
	if (m_NpcsQ == 1)
	{
		return v->GetBoth(VdivVnom, dQ);
	}
	return GetBothInterpolatedHermite(Q, m_NpcsQ, VdivVnom, dVicinity, dQ);
}

double CDynaLRC::GetQ(double VdivVnom, double dVicinity)
{
	CLRCData *v = Q;
	if (m_NpcsQ == 1)
	{
		return v->Get(VdivVnom);
	}
	double dQ = 0.0;
	return GetBothInterpolatedHermite(Q, m_NpcsQ, VdivVnom, dVicinity, dQ);
}

bool CDynaLRC::Check()
{
	bool bRes = true;
	qsort(P, m_NpcsP, sizeof(CLRCData), CompSLCPoly);
	qsort(Q, m_NpcsQ, sizeof(CLRCData), CompSLCPoly);
	return CheckPtr(P, m_NpcsP) && CheckPtr(Q, m_NpcsQ);
}


bool CDynaLRC::CheckPtr(CLRCData *pBase, ptrdiff_t nCount)
{
	ptrdiff_t Ncount = nCount;
	bool bRes = true;
	CLRCData *v = pBase;
	while (v < pBase + Ncount)
	{
		if (v > pBase)
		{
			double s = v->Get(v->V);
			double q = (v -1)->Get(v->V);
			if (!Equal(10.0 * DFW2_EPSILON * (s - q), 0.0))
			{
				Log(CDFW2Messages::DFW2LOG_ERROR, Cex(CDFW2Messages::m_cszLRCDiscontinuityAt, GetVerbalName(), *v, s, q));
				bRes = false;
			}
		}
		v ++;
	}
	return bRes;
}

CDynaLRC::~CDynaLRC()
{
	delete P;
	delete Q;
}

eDEVICEFUNCTIONSTATUS CDynaLRC::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status = DFS_FAILED;

	if (Check() && CollectConstantData(P, m_NpcsP) && CollectConstantData(Q,m_NpcsQ))
		Status = DFS_OK;
	return Status;
	
}

bool CDynaLRC::CollectConstantData(CLRCData *pBase, ptrdiff_t nCount)
{
	bool bRes = true;
	CLRCData *v = pBase;
	// строим связный список сегментов
	while (v < pBase + nCount)
	{
		v->pPrev = (v == pBase) ? NULL : v - 1;
		v->pNext = (v + 1 < pBase + nCount) ? v + 1 : NULL;
		v++;
	}

	v = pBase;
	// определяем максимальный радиус сглаживания
	// для каждого из сегментов
	// как половину от его ширины по напряжению

	while (v < pBase + nCount)
	{
		v->dMaxRadius = 100.0;

		if (v->pPrev)
			v->dMaxRadius = min(0.5 * (v->V - v->pPrev->V), v->dMaxRadius);
		if (v->pNext)
			v->dMaxRadius = min(0.5 * (v->pNext->V - v->V), v->dMaxRadius);

		v++;
	}
	return bRes;
}

const CDeviceContainerProperties CDynaLRC::DeviceProperties()
{
	CDeviceContainerProperties props;
	props.SetType(DEVTYPE_LRC);
	return props;
}

/*
double CDynaLRC::GetBothInterpolatedHermite(double *pBase, ptrdiff_t nCount, double VdivVnom, double dVicinity, double &dP)
{
double *pNextV = NULL;
double *pHitV = NULL;

double *pP1  = NULL;
double *pP2  = NULL;
double *pMid = NULL;

double *v = pBase;
double Vdmin = VdivVnom - dVicinity;
double Vdmax = VdivVnom + dVicinity;

while (v < pBase + 4 * nCount)
{
if (*v > Vdmin && !pP1)
{
pP1 = v - 4;
}

if (*v > Vdmax && !pP2)
{
pP2 = v - 4;
break;
}

if (*v > VdivVnom && !pMid)
{
pMid = v - 4;
}

if (*v > VdivVnom && !pHitV)
{
pHitV = v - 4;
double dDistanceToNext = *v - VdivVnom;
double dDistanceToPrev = VdivVnom - *pHitV;

if (dDistanceToNext < dVicinity)
pNextV = v;

if (dDistanceToPrev < dVicinity &&
dDistanceToPrev < dDistanceToNext &&
pHitV - 4 > pBase)
{
pNextV = pHitV - 4;
}
break;
}
v += 4;
}

// if VdivVnom > max of V - then use last curve
if (!pHitV)
{
pHitV = v - 4;
if (VdivVnom - *pHitV < dVicinity && pHitV - 4 >= pBase)
pNextV = pHitV - 4;
}

if (pNextV)
{
double x1, y1, x2, y2, k1, k2;

if(pNextV > pHitV)
{
x1 = *pNextV - dVicinity;
x2 = x1 + 2.0 * dVicinity;
y1 = Get(pHitV, x1);
y2 = Get(pNextV, x2);
k1 = Getd(pHitV, x1);
k2 = Getd(pNextV, x2);
}
else
{
x1 = *pHitV - dVicinity;
x2 = x1 + 2.0 * dVicinity;
y1 = Get(pNextV, x1);
y2 = Get(pHitV, x2);
k1 = Getd(pNextV, x1);
k2 = Getd(pHitV, x2);
}

double a = k1 * (x2 - x1) - (y2 - y1);
double b = -k2 * (x2 - x1) + (y2 - y1);

double t = (VdivVnom - x1) / (x2 - x1);

double P = (1.0 - t) * y1 + t * y2 + t * (1.0 - t) * (a* (1.0 - t) + b * t);
dP = (y2 - y1) / (x2 - x1) + (1.0 - 2 * t) * (a * (1.0 - t) + b *t) / (x2 - x1) + t * (1.0 - t) * (b - a) / (x2 - x1);
return P;
}

return GetBoth(pHitV, VdivVnom, dP);
}
*/

void CDynaLRC::TestDump(const _TCHAR *cszPathName)
{
	FILE *flrc(NULL);
	setlocale(LC_ALL, "ru-ru");
	if (!_tfopen_s(&flrc, cszPathName, _T("w+")))
	{
		double dP(0.0), dQ(0.0), dV(0.3);
		for (double v = 0.0; v < 1.5; v += 0.01)
		{
			double P = GetPdP(v, dP, dV);
			double Q = GetQdQ(v, dQ, dV);
			_ftprintf(flrc, _T("%g;%g;%g;%g;%g\n"), v, P, dP, Q, dQ);
		}
		fclose(flrc);
	}
}
