#include "stdafx.h"
#include "SlowVariableItem.h"


CSlowVariableItem::CSlowVariableItem(long DeviceTypeId, const LONGVECTOR& DeviceIds, std::string_view VarName) :
								m_DeviceTypeId(DeviceTypeId),
								m_DeviceIds(DeviceIds),
								m_strVarName(VarName)
{
}


CSlowVariableItem::~CSlowVariableItem()
{

}

bool CSlowVariableItem::AddGraphPoint(double Time, double Value, double PreviousValue, std::string_view ChangeDescription)
{
	bool bRes = true;
	if (m_Graph.empty())
	{
		CSlowVariableGraphItem test(-0.1, PreviousValue, ChangeDescription);
		m_Graph.insert(test);
	}

	CSlowVariableGraphItem test(Time, Value, ChangeDescription);
	if (test.m_dTime >= 0)
	{
		std::pair<SLOWGRAPHSET::iterator, bool> its = m_Graph.insert(test);
		if (!its.second)
			its.first->m_dValue = Value;
	}

	return bRes;
}



bool CSlowVariablesSet::Add(long DeviceTypeId, 
							const LONGVECTOR& DeviceIds, 
							std::string_view VarName, 
							double Time, 
							double Value, 
							double PreviousValue, 
							std::string_view ChangeDescription)
{
	bool bRes = true;
	CSlowVariableItem *pItem = new CSlowVariableItem(DeviceTypeId, DeviceIds, VarName);
	std::pair<ITERATOR, bool> its = insert(pItem);
	if (!its.second)
	{
		delete pItem;
		pItem = *its.first;
	}
	bRes = pItem->AddGraphPoint(Time, Value, PreviousValue, ChangeDescription);
	return bRes;
}

bool CSlowVariablesSet::VariantToIds(VARIANT* pVar, LONGVECTOR& vec) const
{
	bool bRes = false;
	vec.clear();
	if (pVar->vt & VT_ARRAY && pVar->vt & VT_I4)
	{
		if (SafeArrayGetDim(pVar->parray) == 1)
		{
			long LBound, UBound;
			if (SUCCEEDED(SafeArrayGetLBound(pVar->parray, 1, &LBound)) && SUCCEEDED(SafeArrayGetUBound(pVar->parray, 1, &UBound)))
			{
				long *pData;
				if (SUCCEEDED(SafeArrayAccessData(pVar->parray, (void**)&pData)))
				{
					for (; LBound <= UBound; LBound++, pData++)
						vec.push_back(*pData);

					if (SUCCEEDED(SafeArrayUnaccessData(pVar->parray)))
						bRes = true;
				}
			}
		}
	}
	else
	{
		if (SUCCEEDED(VariantChangeType(pVar, pVar, 0, VT_I4)))
		{
			vec.push_back(pVar->lVal);
			bRes = true;
		}
	}
	return bRes;
}