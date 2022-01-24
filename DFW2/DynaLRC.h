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

		double Get(double VdivVnom) const
		{
			VdivVnom = (std::max)(VdivVnom, 0.0);
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}

		double GetBoth(double VdivVnom, double &dLRC) const
		{
			VdivVnom = (std::max)(VdivVnom, 0.0);
			dLRC = a1 + a2 * 2.0 * VdivVnom;
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}

		bool operator < (const CLRCData& other) const
		{
			return V < other.V;
		}
	};

	using LRCDATA = std::vector<CLRCData>;
	using LRCDATASET = std::set<CLRCData>;

	class CDynaLRCChannel
	{
	public:
		LRCDATA P;
		LRCDATASET Ps;
		CDynaLRCChannel(const char* Type) : cszType(Type) {}
		double Get(double VdivVnom, double dVicinity) const;
		double GetBoth(double VdivVnom, double& dP, double dVicinity) const;
		bool Check(const CDevice* pDevice);
		void SetSize(size_t nSize);
		bool CollectConstantData();
	protected:
		double GetBothInterpolatedHermite(const CLRCData* const pBase, ptrdiff_t nCount, double VdivVnom, double dVicinity, double& dLRC) const;
		const char* cszType;
	};

	class CDynaLRC : public CDevice
	{
	public:
		using CDevice::CDevice;
		void SetNpcs(ptrdiff_t nPcsP, ptrdiff_t  nPcsQ);
		bool Check();
		static void DeviceProperties(CDeviceContainerProperties& properties);
		void TestDump(const char* cszPathName = "c:\\tmp\\lrctest.csv");
		const CDynaLRCChannel* P() const { return &m_P; }
		const CDynaLRCChannel* Q() const { return &m_Q; }
		CDynaLRCChannel* P() { return &m_P; }
		CDynaLRCChannel* Q() { return &m_Q; }
	protected:
		//friend class CDynaLRCContainer;
		CDynaLRCChannel m_P{ CDynaLRC::m_cszP }, m_Q{ CDynaLRC::m_cszQ };
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		void UpdateSerializer(CSerializerBase* Serializer) override;
		static constexpr const char* m_cszP = "P";
		static constexpr const char* m_cszQ = "Q";
	};



	class CDynaLRCContainer : public CDeviceContainer
	{
	protected:
		static bool IsLRCEmpty(const LRCRawData& lrc);
		static bool CompareLRCs(const LRCRawData& lhs, const LRCRawData& rhs);
	public:
		using CDeviceContainer::CDeviceContainer;
		virtual ~CDynaLRCContainer() = default;
		void CreateFromSerialized();
	};
}
