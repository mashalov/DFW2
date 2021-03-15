#pragma once
#include "vector"

template<class TInterface, class TObject>
class CGenericCollection : public TInterface
{
protected:
	typedef std::vector< CComObject<TObject> *> OBJVECTOR;
	OBJVECTOR m_ObjVector;

public:
	STDMETHOD(Item)(LONG Index, VARIANT *Item)
	{
		HRESULT hRes = E_INVALIDARG;

		if (SUCCEEDED(VariantClear(Item)))
		{
			if (Index >= 0 && Index < static_cast<LONG>(m_ObjVector.size()))
			{
				Item->vt = VT_DISPATCH;
				Item->pdispVal = m_ObjVector[Index];
				m_ObjVector[Index]->AddRef();
				hRes = S_OK;
			}
		}
		return hRes;
	}
	STDMETHOD(get_Count)(LONG *Count)
	{
		*Count = static_cast<LONG>(m_ObjVector.size());
		return S_OK;
	}

	STDMETHOD(get__NewEnum)(LPUNKNOWN *pVal)
	{
		size_t size = m_ObjVector.size(); 
		std::unique_ptr<VARIANT[]> pVar = std::make_unique<VARIANT[]>(size);
		for (size_t i = 0; i < size; i++)
		{
			pVar[i].vt = VT_DISPATCH;
			pVar[i].pdispVal = m_ObjVector[i];
		}
		typedef CComObject<CComEnum< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy< VARIANT> > > enumVar;
		enumVar *pEnumVar = new enumVar;
		pEnumVar->Init(&pVar[0], &pVar[size], NULL, AtlFlagCopy);
		return pEnumVar->QueryInterface(IID_IUnknown, (void**)pVal);
	}

	bool Add(CComObject<TObject> *pObject)
	{
		m_ObjVector.push_back(pObject);
		return true;
	}

	void CollectionFinalRelease()
	{
		for (auto&& it : m_ObjVector)
			it->Release();
	}

};