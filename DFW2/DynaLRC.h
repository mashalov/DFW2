#pragma once
#include "DeviceContainer.h"
#include <array>
#include <algorithm>

namespace DFW2
{
	struct LRCRawData
	{
		double V;
		double Freq;
		double a0;
		double a1;
		double a2;
		// double a3;
		// double a4;
	};

	struct CLRCData : public LRCRawData
	{
		CLRCData *pPrev;
		CLRCData *pNext;

		double dMaxRadius;

		double Get(double VdivVnom)
		{
			VdivVnom = (std::max)(VdivVnom, 0.0);
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}

		double GetBoth(double VdivVnom, double &dLRC)
		{
			VdivVnom = (std::max)(VdivVnom, 0.0);
			dLRC = a1 + a2 * 2.0 * VdivVnom;
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}
	};

	class CDummyLRC : public CDevice
	{
	public:
		using CDevice::CDevice;
		std::array<LRCRawData, 2> PQ;
		void UpdateSerializer(SerializerPtr& Serializer) override;
	};

	using LRCDATA = std::vector<CLRCData>;

	class CDynaLRC : public CDevice
	{
	public:
		using CDevice::CDevice;

		bool SetNpcs(ptrdiff_t nPcsP, ptrdiff_t  nPcsQ);
		bool Check();
		double GetP(double VdivVnom, double dVicinity);
		double GetQ(double VdivVnom, double dVicinity);
		double GetPdP(double VdivVnom, double &dP, double dVicinity);
		double GetQdQ(double VdivVnom, double &dQ, double dVicinity);
		static void DeviceProperties(CDeviceContainerProperties& properties);
		LRCDATA P, Q;
		void TestDump(const char* cszPathName = "c:\\tmp\\lrctest.csv");
	protected:
		bool CollectConstantData(LRCDATA& LRC);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		bool CheckPtr(LRCDATA& LRC);
		double GetBothInterpolatedHermite(CLRCData *pBase, ptrdiff_t nCount, double VdivVnom, double dVicinity, double &dLRC);
	};



	class CDynaLRCContainer : public CDeviceContainer
	{
	protected:
		static bool IsLRCEmpty(const LRCRawData& lrc);
	public:
		using CDeviceContainer::CDeviceContainer;
		virtual ~CDynaLRCContainer() {}
		void CreateFromSerialized();
	};
}
