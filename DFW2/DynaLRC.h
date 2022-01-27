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

		double Get(double VdivVnom) const
		{
			VdivVnom = (std::max)(VdivVnom, 0.0);
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}

		double GetBoth(double VdivVnom, double& dLRC) const
		{
			VdivVnom = (std::max)(VdivVnom, 0.0);
			dLRC = a1 + a2 * 2.0 * VdivVnom;
			return a0 + (a1 + a2 * VdivVnom) * VdivVnom;
		}
	};

	struct CLRCData : public LRCRawData
	{
		double dMaxRadius;
	};

	// структура для сегмента данных СХН с интерполяцией
	// содержит либо указатель на обычный сегмент СХН,
	// либо строит интерполяцию между сегментами с заданным радиусом dVicinity
	struct CLRCDataInterpolated 
	{
		// параметры интерполяции
		// https://en.wikipedia.org/wiki/Spline_interpolation
		double a, b, x2x1, y1, y2, y2y1;
		// указатель на обычный сегмент СХН
		const CLRCData * const m_pRawLRC = nullptr;
		// напряжение сегмента (может не совпадать с m_pRawLRC->V)
		double V;
		// конcтруктор для поиска в сете
		CLRCDataInterpolated(double VdivVnom) : V(VdivVnom) {}
		// конструктор для варианта с использованием обычного сегмента СХН
		CLRCDataInterpolated(double VdivVnom, const CLRCData& rawData) : V(VdivVnom), m_pRawLRC { &rawData } {}
		// конструктор интерполяции между двумя сегментами
		CLRCDataInterpolated(const CLRCData& left, const CLRCData& right, double dVicinity) : V(right.V - dVicinity)
		{
			double k1{ 0.0 }, k2{ 0.0 };
			// вместо x1 используем V
			y1 = left.GetBoth(V, k1);
			y2 = right.GetBoth(right.V + dVicinity, k2);
			x2x1 = 2 * dVicinity;
			y2y1 = y2 - y1;
			a = k1 * x2x1 - y2y1;
			b = -k2 * x2x1 + y2y1;
		}

		// оператор сортировки
		bool operator < (const CLRCDataInterpolated& other) const
		{
			return V < other.V;
		}

		double Get(double VdivVnom) const
		{
			// если в сегменте обычная СХН - возвращаем от нее
			if (m_pRawLRC)
				return m_pRawLRC->Get(VdivVnom);
			else
			{
				// если интерполяция - рассчитываем 
				VdivVnom = (std::max)(VdivVnom, 0.0);
				const double t{ (VdivVnom - V) / x2x1 }, t1{ 1.0 - t };
				return  t1 * y1 + t * y2 + t * t1 * (a * t1 + b * t);
			}
		}

		double GetBoth(double VdivVnom, double& dLRC) const
		{
			// если в сегменте обычная СХН - возвращаем от нее
			if (m_pRawLRC)
				return m_pRawLRC->GetBoth(VdivVnom, dLRC);
			{
				// если интерполяция - рассчитываем 
				VdivVnom = (std::max)(VdivVnom, 0.0);
				const double t{ (VdivVnom - V) / x2x1 }, t1{ 1.0 - t };
				dLRC = (a - y1 + y2 - (4.0 * a - 2.0 * b - 3.0 * t * (a - b)) * t) / x2x1;
				return  t1 * y1 + t * y2 + t * t1 * (a * t1 + b * t) ;
			}
		}
	};

	using LRCRAWDATA = std::vector<CLRCData>;
	using LRCDATASET = std::set<CLRCDataInterpolated>;

	class CDynaLRCChannel
	{
	public:
		LRCRAWDATA P;
		LRCDATASET Ps;
		CDynaLRCChannel(const char* Type) : cszType(Type) {}
		double Get(double VdivVnom, double dVicinity) const;
		double GetBoth(double VdivVnom, double& dP, double dVicinity) const;
		bool Check(const CDevice* pDevice);
		void SetSize(size_t nSize);
		bool BuildLRCSet(const CDevice* pDevice, const double dVicinity);
	protected:
		double GetBothInterpolatedHermite(double VdivVnom, double dVicinity, double& dLRC) const;
		const char* cszType;
	};

	class CDynaLRC : public CDevice
	{
	public:
		using CDevice::CDevice;
		void SetNpcs(ptrdiff_t nPcsP, ptrdiff_t  nPcsQ);
		bool Check();
		bool UpdateSet();
		static void DeviceProperties(CDeviceContainerProperties& properties);
		void TestDump(const char* cszPathName = "c:\\tmp\\lrctest.csv");
		inline const CDynaLRCChannel* P() const { return &m_P; }
		inline const CDynaLRCChannel* Q() const { return &m_Q; }
		inline CDynaLRCChannel* P() { return &m_P; }
		inline CDynaLRCChannel* Q() { return &m_Q; }
	protected:
		CDynaLRCChannel m_P{ CDynaLRC::m_cszP }, m_Q{ CDynaLRC::m_cszQ };
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
