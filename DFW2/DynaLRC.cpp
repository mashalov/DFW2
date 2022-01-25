#include "stdafx.h"
#include "DynaLRC.h"
#include "DynaModel.h"
#include <fstream>

using namespace DFW2;

void CDynaLRC::SetNpcs(ptrdiff_t nPcsP, ptrdiff_t  nPcsQ)
{
	m_P.SetSize(nPcsP);
	m_Q.SetSize(nPcsQ);
}

bool CDynaLRC::Check()
{
	return m_P.Check(this) && m_Q.Check(this);
}

eDEVICEFUNCTIONSTATUS CDynaLRC::Init(CDynaModel* pDynaModel)
{
	eDEVICEFUNCTIONSTATUS Status{ eDEVICEFUNCTIONSTATUS::DFS_FAILED };

	if (Check() && 
		m_P.CollectConstantData(this) && 
		m_Q.CollectConstantData(this))
			Status = eDEVICEFUNCTIONSTATUS::DFS_OK;
	return Status;
	
}

void CDynaLRC::DeviceProperties(CDeviceContainerProperties& props)
{
	props.SetType(DEVTYPE_LRC);
	props.SetClassName(CDeviceContainerProperties::m_cszNameLRC, CDeviceContainerProperties::m_cszSysNameLRC);
	props.DeviceFactory = std::make_unique<CDeviceFactory<CDynaLRC>>();
}

void CDynaLRC::TestDump(const char* cszPathName)
{
	std::ofstream dump(stringutils::utf8_decode(cszPathName));
	if (dump.is_open())
	{
		double dP{ 0.0 }, dQ{ 0.0 }, dV{ 0.05 };
		for (double v = 0.0; v < 1.5; v += 0.01)
		{
			double P{ m_P.GetBoth(v, dP, dV) };
			double Q{ m_Q.GetBoth(v, dQ, dV) };
			dump << fmt::format("{};{};{};{};{}", v, P, dP, Q, dQ) << std::endl;
		}
	}
}


template<class T>
class CSerializerLRCPolynom : public CSerializerDataSourceVector<T>
{
	using CSerializerDataSourceVector<T>::CSerializerDataSourceVector;
public:
	void UpdateSerializer(CSerializerBase* pSerializer) override
	{
		T& DataItem(CSerializerDataSourceVector<T>::GetItem());
		CSerializerDataSourceVector<T>::UpdateSerializer(pSerializer);
		pSerializer->AddProperty("v", DataItem.V, eVARUNITS::VARUNIT_PU);
		pSerializer->AddProperty("freq", DataItem.Freq, eVARUNITS::VARUNIT_PU);
		pSerializer->AddProperty("a0", DataItem.a0);
		pSerializer->AddProperty("a1", DataItem.a1);
		pSerializer->AddProperty("a2", DataItem.a2);
	}
};

void CDynaLRC::UpdateSerializer(CSerializerBase* Serializer)
{
	CDevice::UpdateSerializer(Serializer);
	Serializer->AddProperty("LRCId", TypedSerializedValue::eValueType::VT_ID);
	Serializer->AddSerializer("P", new CSerializerBase(new CSerializerLRCPolynom<CLRCData>(m_P.P)));
	Serializer->AddSerializer("Q", new CSerializerBase(new CSerializerLRCPolynom<CLRCData>(m_Q.P)));
}


void CDynaLRCContainer::CreateFromSerialized()
{
	double Vmin{ GetModel()->GetLRCToShuntVmin() };

	struct DualLRC { std::array<std::list<CLRCData>,2> PQ; };
	using LRCConstructmMap = std::map<ptrdiff_t, DualLRC>;

	LRCConstructmMap constructMap;

	if (Vmin <= 0.0)
	{
		Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszLRCVminChanged, Vmin, 0.5));
		Vmin = 0.5;
	}

	// проходим по десериализованным СХН
	for (auto&& dev : m_DevVec)
	{
		auto lrc{ static_cast<CDynaLRC*>(dev) };

		// собираем сегменты СХН в карту по идентификаторам
		auto& pqFromId{ constructMap[dev->GetId()].PQ };

		// копируем непустые сериализованные СХН в настоящие СХН
		for (ptrdiff_t x = 0; x < 2; x++)
		{
			for (const auto& p : x == 0 ? lrc->P()->P : lrc->Q()->P)
			{
				if(!CDynaLRCContainer::IsLRCEmpty(p))
					pqFromId[x].push_back(p);
			}
		}
	}


	// добавляем типовые и служебные СХН
	// типовые СХН Rastr 1 и 2

	bool bForceStandardLRC{ !GetModel()->AllowUserToOverrideStandardLRC() };
	
	// удаляем все сегменты по p и q
	auto clearPQ = [](DualLRC& dualLRC)
	{
		dualLRC.PQ[0].clear(); dualLRC.PQ[1].clear();
	};

	auto CheckUserLRC = [this,&constructMap](ptrdiff_t Id, bool bForceStandardLRC) -> bool
	{
		bool bInsert{ true };
		auto itLRC{ constructMap.find(Id) };
		if (itLRC != constructMap.end())
		{
			// если СХН с данным Id есть сообщаем что она уже существует
			this->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszUserOverrideOfStandardLRC, Id));
			// и если задан флаг - разрешаем заменить на новую
			if (bForceStandardLRC)
				this->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszUserLRCChangedToStandard, Id));
			else
				bInsert = false;
		}
		return bInsert;
	};

	// проверяем и добавляем/заменяем стандартные СХН
	if (CheckUserLRC(1, bForceStandardLRC))
	{
		auto& lrc{ constructMap[1] };
		clearPQ(lrc);
		lrc.PQ[0].push_back({ 0.0,		1.0,	0.83,		-0.3,		0.47 });
		lrc.PQ[1].push_back({ 0.0,		1.0,	0.721,		0.15971,	0.0 });
		lrc.PQ[1].push_back({ 0.815,	1.0,	3.7,		-7.0,		4.3 });
		lrc.PQ[1].push_back({ 1.2,		1.0,	1.492,		0.0,		0.0 });
	}

	if (CheckUserLRC(2, bForceStandardLRC))
	{
		auto& lrc{ constructMap[2] };
		clearPQ(lrc);
		lrc.PQ[0].push_back({ 0.0,		1.0,	0.83,		-0.3,		0.47 });
		lrc.PQ[1].push_back({ 0.0,		1.0,	0.657,		0.159135,	0.0 });
		lrc.PQ[1].push_back({ 0.815,	1.0,	4.9,		-10.1,		6.2 });
		lrc.PQ[1].push_back({ 1.2,		1.0,	1.708,		0.0,		0.0 });
	}

	// СХН на шунт для динамики
	if (CheckUserLRC(0, true))
	{
		auto& lrcShunt{ constructMap[0] };
		clearPQ(lrcShunt);
		lrcShunt.PQ[0].push_back({ 0.0, 1.0, 0.0, 0.0, 1.0 });
		lrcShunt.PQ[1].push_back({ 0.0, 1.0, 0.0, 0.0, 1.0 });
	}

	// СХН на постоянную мощность для УР
	if (CheckUserLRC(-1, true))
	{
		auto& lrcConstP{ constructMap[-1] };
		clearPQ(lrcConstP);
		lrcConstP.PQ[0].push_back({ 0.0, 1.0, 1.0, 0.0, 0.0 });
		lrcConstP.PQ[1].push_back({ 0.0, 1.0, 1.0, 0.0, 0.0 });
	}

	// СХН на постоянный ток для динамики
	if (CheckUserLRC(-2, true))
	{
		auto& lrcConstI{ constructMap[-2] };
		clearPQ(lrcConstI);
		lrcConstI.PQ[0].push_back({ 0.0, 1.0, 0.0, 1.0, 0.0 });
		lrcConstI.PQ[1].push_back({ 0.0, 1.0, 0.0, 1.0, 0.0 });
	}

	
	auto SortLRC = [](const auto& lhs, const auto& rhs) { return lhs.V < rhs.V; };

	for (auto&& lrc : constructMap)
		for (auto&& pq : lrc.second.PQ)
		{
			// сортируем сегменты 
			pq.sort(SortLRC);
			if (pq.size())
			{
				// проверяем что СХН начинается с нуля
				if (double& Vbeg = pq.front().V; Vbeg > 0.0)
				{
					Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszLRCStartsNotFrom0, lrc.first, Vbeg));
					Vbeg = 0.0;
				}

				// проверяем наличие сегментов с одинаковым напряжением
				auto it{ pq.begin() };
				while (it != pq.end())
				{
					auto nextIt{ std::next(it) };

					if (nextIt != pq.end() && Equal(it->V, nextIt->V)) 
					{

						if(CDynaLRCContainer::CompareLRCs(*it,*nextIt))
							Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszAmbigousLRCSegment,
								lrc.first, 
								it->V, 
								it->a0, 
								it->a1,
								it->a2));
						// если нашли сегменты с одинаковым напряжением - убираем второй по счету
						nextIt = pq.erase(nextIt);
					}

					it = nextIt;
				}

				// вставляем сегмент от 0 до Vmin

				// ищем последний сегмент в отсортированном списке с напряжением меньше минимального
				auto rit{ std::find_if(pq.rbegin(), pq.rend(), [&Vmin](const CLRCData& lrc) { return lrc.V < Vmin; }) };
				if (rit != pq.rend())
				{
					// если нашли получаем значение при Vmin
					const double LRCatV{ rit->Get(Vmin) };
					// ставим найденному сегменту Vmin (обрезаем СХН до Vmin)
					rit->V = Vmin;
					// удаляем все сегменты до Vmin
					auto revRit{ std::prev(rit.base()) };
					
					pq.erase(pq.begin(), revRit);
					// добавляем шунтовой сегмент от нуля до Vmin в точку предыдущего сегмента
					pq.insert(pq.begin(), { 0.0,	1.0,	0.0,	0.0,	LRCatV / Vmin / Vmin } );
				}

				// удаляем из СХН сегменты с одинаковыми коэффициентами
				it = pq.begin();
				while (it != pq.end())
				{
					auto nextIt{ std::next(it) };
					if (nextIt != pq.end() && CDynaLRCContainer::CompareLRCs(*it, *nextIt))
						nextIt = pq.erase(nextIt);
					it = nextIt;
				}
			}
		}
	// все собрали и обработали в constructMap
	// теперь создаем настоящие СХН

	CDynaLRC* pLRC{ CreateDevices<CDynaLRC>(constructMap.size()) };

	for (auto&& lrc : constructMap)
	{
		std::copy(lrc.second.PQ[0].begin(), lrc.second.PQ[0].end(), std::back_inserter(pLRC->P()->P));
		std::copy(lrc.second.PQ[1].begin(), lrc.second.PQ[1].end(), std::back_inserter(pLRC->Q()->P));
		pLRC->SetId(lrc.first);

		/*
		std::cout << pLRC->GetId() << std::endl;
		for (const auto& l : pLRC->P)
			std::cout << "P " << l.V << " " << l.a0 << " " << l.a1 << " " << l.a2 << std::endl;
		for (const auto& l : pLRC->Q)
			std::cout << "Q " << l.V << " " << l.a0 << " " << l.a1 << " " << l.a2 << std::endl;
		*/

		pLRC++;
	}
}

bool CDynaLRCContainer::IsLRCEmpty(const LRCRawData& lrc)
{
	return Equal(lrc.a0, 0.0) && Equal(lrc.a1, 0.0) && Equal(lrc.a2, 0.0);
}

bool CDynaLRCContainer::CompareLRCs(const LRCRawData& lhs, const LRCRawData& rhs)
{
	return Equal(lhs.a0, rhs.a0) && Equal(lhs.a1, rhs.a1) && Equal(lhs.a2, rhs.a2);
}


void CDynaLRCChannel::SetSize(size_t nSize)
{
	if (nSize >= 0)
		P.resize(nSize);
	else
		throw dfw2error("CDynaLRCChannel::SetSize - size is zero");
}


double CDynaLRCChannel::Get(double VdivVnom, double dVicinity) const
{
	const CLRCData* const v{ &P.front() };
	if (P.size() == 1)
		return v->Get(VdivVnom);
	double dP{ 0.0 };
	return GetBothInterpolatedHermite(v, P.size(), VdivVnom, dVicinity, dP);
}

double CDynaLRCChannel::GetBoth(double VdivVnom, double& dP, double dVicinity) const
{
	/*
	if (!Ps.empty())
	{
		auto itSegment{ std::prev(std::upper_bound(Ps.begin(), Ps.end(), CLRCDataInterpolated(VdivVnom))) };
		return itSegment->GetBoth(VdivVnom, dP);
	}
	*/

	const CLRCData* const v{ &P.front() };
	if (P.size() == 1)
		return v->GetBoth(VdivVnom, dP);

	return GetBothInterpolatedHermite(v, P.size(), VdivVnom, dVicinity, dP);
}


double CDynaLRCChannel::GetBothInterpolatedHermite(const CLRCData* const pBase, ptrdiff_t nCount, double VdivVnom, double dVicinity, double& dLRC) const
{
	// По умолчанию считаем что напряжение находится в последнем сегменте
	const CLRCData* pHitV{ pBase + nCount - 1 }, * v{ pBase };
	VdivVnom = (std::max)(0.0, VdivVnom);

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

	bool bLeft{ false }, bRight{ false };

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

		double x1, x2, y1, y2, k1{ 0.0 }, k2{ 0.0 };

		// https://en.wikipedia.org/wiki/Spline_interpolation


		if (bLeft)
		{
			dVicinity = (std::min)(dVicinity, pHitV->dMaxRadius);
			x1 = pHitV->V - dVicinity;
			y1 = pHitV->pPrev->GetBoth(x1, k1);
			x2 = pHitV->V + dVicinity;
			y2 = pHitV->GetBoth(x2, k2);
		}
		else
		{
			dVicinity = (std::min)(dVicinity, pHitV->pNext->dMaxRadius);
			x1 = pHitV->pNext->V - dVicinity;
			y1 = pHitV->GetBoth(x1, k1);
			x2 = pHitV->pNext->V + dVicinity;
			y2 = pHitV->pNext->GetBoth(x2, k2);
		}

		const double x2x1{ x2 - x1 }, t{ (VdivVnom - x1) / x2x1 };

		if (t >= 0 && t <= 1.0)
		{
			const double t1{ 1.0 - t }, y2y1{ y2 - y1 }, a{ k1 * x2x1 - y2y1 }, b{ -k2 * x2x1 + y2y1 };

			const double P{ t1 * y1 + t * y2 + t * t1 * (a * t1 + b * t) };
			//dLRC = ((y2 - y1) + (1.0 - 2 * t) * (a * (1.0 - t) + b *t) + t * (1.0 - t) * (b - a)) / x2x1;
			dLRC = (a - y1 + y2 - (4.0 * a - 2.0 * b - 3.0 * t * (a - b)) * t) / x2x1;
			//_ASSERTE(Equal(dLRC,((y2 - y1) + (1.0 - 2 * t) * (a * (1.0 - t) + b * t) + t * (1.0 - t) * (b - a)) / x2x1));
			return P;
		}
	}

	return pHitV->GetBoth(VdivVnom, dLRC);
}


bool CDynaLRCChannel::Check(const CDevice *pDevice)
{
	bool bRes{ true };
	sort(P.begin(), P.end(), [](const CLRCData& lhs, const CLRCData& rhs) { return lhs.V < rhs.V;	});

	if (P.size() > 1)
	{
		// обходим со второго до последнего
		for (auto v = std::next(P.begin()); v != P.end(); ++v)
		{
			double s{ v->Get(v->V) };				// берем значение в начале следующего
			double q{ std::prev(v)->Get(v->V) };	// берем значение в конце предыдущего (в той же точке что и начало следующего)
			if (!Equal(10.0 * DFW2_EPSILON * (s - q), 0.0))
			{
				pDevice->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLRCDiscontinuityAt,
					pDevice->GetVerbalName(),
					cszType,
					v->V,
					s, q));

				bRes = false;
			}
		}
	}

	const auto& Parameters{ pDevice->GetModel()->Parameters() };
	double d{ 0.0 }, v{ GetBoth(1.0, d, 0.0) };

	// тут вопрос как контролировать равенство коэффициентов единице
	// пока решено использовать 1% от небаланса УР
	if (0.01 * std::abs(v - 1.0) > Parameters.m_Imb)
	{
		pDevice->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLRCNonUnity,
			pDevice->GetVerbalName(),
			cszType,
			v));

		bRes = false;
	}

	if (d > Parameters.m_dLRCMaxSlope || d < Parameters.m_dLRCMinSlope)
	{
		pDevice->Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format(CDFW2Messages::m_cszLRCSlopeViolated,
			pDevice->GetVerbalName(),
			cszType,
			Parameters.m_dLRCMinSlope,
			Parameters.m_dLRCMaxSlope,
			d));

		bRes = false;
	}

	return bRes;
}

bool CDynaLRCChannel::CollectConstantData(const CDevice* pDevice)
{
	// строим связный список сегментов
	for (auto&& v = P.begin(); v != P.end(); ++v)
	{
		v->pPrev = (v == P.begin()) ? nullptr : &*std::prev(v);
		auto next{ std::next(v) };
		v->pNext = (next == P.end()) ? nullptr : &*next;
	}

	// определяем максимальный радиус сглаживания
	// для каждого из сегментов
	// как половину от его ширины по напряжению

	const double dLRCVicinity{ pDevice->GetModel()->Parameters().m_dLRCSmoothingRange };
	double dMaxLRCVicinity{ dLRCVicinity };

	if (P.size() > 1)
	{
		for (auto&& v : P)
		{
			v.dMaxRadius = 100.0;
			if (v.pPrev)
				v.dMaxRadius = (std::min)(0.5 * (v.V - v.pPrev->V), v.dMaxRadius);
			if (v.pNext)
				v.dMaxRadius = (std::min)(0.5 * (v.pNext->V - v.V), v.dMaxRadius);

			dMaxLRCVicinity = (std::min)(dMaxLRCVicinity, v.dMaxRadius);
		}
	}

	if (dLRCVicinity > 0.0)
	{
		if(dLRCVicinity > dMaxLRCVicinity)
			pDevice->Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format(CDFW2Messages::m_cszLRCSmoothingTooBigFor,
				pDevice->GetVerbalName(),
				cszType,
				dLRCVicinity,
				dMaxLRCVicinity));

		for (auto it = P.begin(); it != P.end(); it++)
		{
			auto next{ std::next(it) };
			// вставляем текущий сегмент. Если он первый, вставляем как есть, если нет - сдвигаем его область
			// вправо на величину области сглаживания
			Ps.insert(CLRCDataInterpolated(it->V + (it == P.begin() ? 0.0 : dMaxLRCVicinity), *it));
			// если есть следующий сегмент - вставляем сегмент сглаживания между
			// текущим и следующим
			if (next != P.end())
				Ps.insert(CLRCDataInterpolated(*it, *next, dMaxLRCVicinity));
		}
	}

	return true;
}
