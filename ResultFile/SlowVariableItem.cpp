#include "stdafx.h"
#include "SlowVariableItem.h"

CSlowVariableItem::CSlowVariableItem(ptrdiff_t DeviceTypeId, const ResultIds& DeviceIds, std::string_view VarName) :
								m_DeviceTypeId(DeviceTypeId),
								m_DeviceIds(DeviceIds),
								m_strVarName(VarName)
{
}


CSlowVariableItem::~CSlowVariableItem()
{
}

void CSlowVariableItem::AddGraphPoint(double Time, double Value, double PreviousValue, std::string_view ChangeDescription)
{
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
}



void CSlowVariablesSet::Add(ptrdiff_t DeviceTypeId, 
							const ResultIds& DeviceIds, 
							std::string_view VarName, 
							double Time, 
							double Value, 
							double PreviousValue, 
							std::string_view ChangeDescription)
{
	CSlowVariableItem *pItem = new CSlowVariableItem(DeviceTypeId, DeviceIds, VarName);
	std::pair<ITERATOR, bool> its = insert(pItem);
	if (!its.second)
	{
		delete pItem;
		pItem = *its.first;
	}
	pItem->AddGraphPoint(Time, Value, PreviousValue, ChangeDescription);
}
