#pragma once
#include "Device.h"
#include "DeviceContainerProperties.h"

namespace DFW2
{
	class CDynaModel;

	typedef vector<CDevice*> DEVICEVECTOR;
	typedef DEVICEVECTOR::iterator DEVICEVECTORITR;

	typedef set<CDeviceId*, CDeviceIdComparator> DEVSEARCHSET;
	typedef DEVSEARCHSET::iterator DEVSEARCHSETITR;
	typedef DEVSEARCHSET::const_iterator DEVSEARCHSETCONSTITR;
	typedef set<CDevice*> DEVICEPTRSET;
	typedef DEVICEPTRSET::iterator DEVICEPTRSETITR;

	class CDeviceContainer;
	class CMultiLink
	{
	public:
		CDevice  **m_ppPointers;
		CDeviceContainer *m_pContainer;
		CLinkPtrCount *m_pLinkInfo;
		size_t	 m_nSize;
		size_t   m_nCount;
		CMultiLink(CDeviceContainer* pContainer, size_t nCount);
		bool Join(CMultiLink *pLink);


		virtual ~CMultiLink()
		{
			delete m_ppPointers;
			delete m_pLinkInfo;
		}
	};


	typedef vector<CMultiLink*> LINKSVEC;
	typedef LINKSVEC::iterator LINKSVECITR;

	class CDeviceContainer
	{
	protected:
		eDEVICEFUNCTIONSTATUS m_eDeviceFunctionStatus;
		DEVICEVECTOR m_DevVec;
		DEVSEARCHSET m_DevSet;
		bool SetUpSearch();
		CDevice *m_pControlledData;
		CDevice **m_ppSingleLinks;
		void CleanUp();
		CDynaModel *m_pDynaModel;
		void PrepareSingleLinks();

	public:

		CDeviceContainerProperties m_ContainerProps;
		ptrdiff_t EquationsCount();

		inline eDFW2DEVICETYPE GetType() { return m_ContainerProps.eDeviceType; }
		const _TCHAR* GetTypeName() { return m_ContainerProps.m_strClassName.c_str(); }

		CDeviceContainer(CDynaModel *pDynaModel);
		virtual ~CDeviceContainer();

		CDevice **m_ppDevicesAux;
		size_t   m_nVisitedCount;

		template<typename T> bool AddDevices(T* pDevice, size_t nCount)
		{
			CleanUp();
			m_pControlledData = pDevice;
			T* p = pDevice;
			bool bRes = true;
			m_DevVec.reserve(m_DevVec.size() + nCount);
			for (size_t i = 0; i < nCount; i++)
			{
				bRes = AddDevice(p++) && bRes;
			}
			return bRes;
		}
		template<typename T> bool GetDevice(ptrdiff_t nId, T* &pDevice)
		{
			pDevice = static_cast<T*>(GetDevice(nId));
			return pDevice ? true : false;
		}
		template<typename T> bool GetDevice(CDeviceId* pDeviceId, T* &pDevice)
		{
			pDevice = static_cast<T*>(GetDevice(pDeviceId));
			return pDevice ? true : false;
		}
		template<typename T> bool GetDeviceByIndex(ptrdiff_t nIndex, T* &pDevice)
		{
			pDevice = static_cast<T*>(GetDeviceByIndex(nIndex));
			return pDevice ? true : false;
		}
		bool RegisterVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits);
		bool RegisterConstVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eDEVICEVARIABLETYPE eDevVarType);
		bool VariableOutputEnable(const _TCHAR* cszVarName, bool bOutputEnable);

		VARINDEXMAPCONSTITR VariablesBegin();
		VARINDEXMAPCONSTITR VariablesEnd();

		CONSTVARINDEXMAPCONSTITR ConstVariablesBegin();
		CONSTVARINDEXMAPCONSTITR ConstVariablesEnd();

		ptrdiff_t GetVariableIndex(const _TCHAR* cszVarName)	  const;
		ptrdiff_t GetConstVariableIndex(const _TCHAR* cszVarName) const;
		CDevice* GetDeviceByIndex(ptrdiff_t nIndex);
		CDevice* GetDevice(CDeviceId* pDeviceId);
		CDevice* GetDevice(ptrdiff_t nId);
		bool AddDevice(CDevice* pDevice);
		bool RemoveDevice(ptrdiff_t nId);
		bool RemoveDeviceByIndex(ptrdiff_t nIndex); 
		size_t Count();
		inline DEVICEVECTORITR begin() { return m_DevVec.begin(); }
		inline DEVICEVECTORITR end() { return m_DevVec.end(); }
		void Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage, ptrdiff_t nDBIndex = -1);

		LINKSVEC m_Links;
		bool IsKindOfType(eDFW2DEVICETYPE eType);
		bool CreateLink(CDeviceContainer* pLinkContainer);
		ptrdiff_t GetLinkIndex(CDeviceContainer* pLinkContainer);
		ptrdiff_t GetLinkIndex(eDFW2DEVICETYPE eDeviceType); 
		bool IncrementLinkCounter(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		bool AllocateLinks(ptrdiff_t nLinkIndex);
		bool AddLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex, CDevice* pDevice);
		CMultiLink* GetCheckLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		bool RestoreLinks(ptrdiff_t nLinkIndex);
		bool EstimateBlock(CDynaModel *pDynaModel);
		bool BuildBlock(CDynaModel* pDynaModel);
		bool BuildRightHand(CDynaModel* pDynaModel);
		bool BuildDerivatives(CDynaModel* pDynaModel);
		bool NewtonUpdateBlock(CDynaModel* pDynaModel);
		bool LeaveDiscontinuityMode(CDynaModel *pDynaModel);
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel *pDynaModel);
		void UnprocessDiscontinuity();
		double CheckZeroCrossing(CDynaModel *pDynaModel);
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		CDynaModel* GetModel() { return m_pDynaModel;  }
		bool PushVarSearchStack(CDevice*pDevice);
		bool PopVarSearchStack(CDevice* &pDevice);
		void ResetStack();
		virtual ptrdiff_t GetPossibleSingleLinksCount();
		CDeviceContainer* DetectLinks(CDeviceContainer* pExtContainer, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom);
		size_t GetResultVariablesCount();
		bool HasAlias(const _TCHAR *cszAlias);
	};

	typedef vector<CDeviceContainer*> DEVICECONTAINERS;
	typedef DEVICECONTAINERS::iterator DEVICECONTAINERITR;
};

