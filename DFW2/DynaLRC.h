#pragma once
#include "Device.h"

namespace DFW2
{
	struct CLRCData
	{
		double V;
		double a0;
		double a1;
		double a2;
		/*
		double a3;
		double a4;
		double kf;
		*/

		CLRCData *pPrev;
		CLRCData *pNext;

		double dMaxRadius;

		double Get(double VdivVnom)
		{
			VdivVnom = max(VdivVnom, 0.0);
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}

		double GetBoth(double VdivVnom, double &dLRC)
		{
			VdivVnom = max(VdivVnom, 0.0);
			dLRC = a1 + a2 * 2.0 * VdivVnom;
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}
	};

	class CDynaLRC : public CDevice
	{
	public:
		CDynaLRC();
		virtual ~CDynaLRC();
		bool SetNpcs(ptrdiff_t nPcsP, ptrdiff_t  nPcsQ);
		bool Check();
		double GetP(double VdivVnom, double dVicinity);
		double GetQ(double VdivVnom, double dVicinity);
		double GetPdP(double VdivVnom, double &dP, double dVicinity);
		double GetQdQ(double VdivVnom, double &dQ, double dVicinity);

		static const CDeviceContainerProperties DeviceProperties();

		ptrdiff_t m_NpcsP, m_NpcsQ;
		CLRCData *P;
		CLRCData *Q;
		void TestDump(const _TCHAR *cszPathName = _T("c:\\tmp\\lrctest.csv"));
	protected:
		bool CollectConstantData(CLRCData *pBase, ptrdiff_t nCount);
		static int CDynaLRC::CompSLCPoly(const void* V1, const void* V2);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		bool CheckPtr(CLRCData *pBase, ptrdiff_t nCount);
		double GetBothInterpolatedHermite(CLRCData *pBase, ptrdiff_t nCount, double VdivVnom, double dVicinity, double &dLRC);
	};
}
