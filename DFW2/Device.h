#pragma once
#include "DeviceId.h"
#include "DeviceContainerProperties.h"
#include "DLLStructs.h"

using namespace std;

namespace DFW2
{
	class CDeviceContainer;
	class CDevice;
	class CDynaModel;
	class PrimitiveVariableExternal;
		
	class CLinkPtrCount
	{
	public:
		CDevice  **m_pPointer;
		size_t	 m_nCount;
		CLinkPtrCount() : m_nCount(0) {}
		bool In(CDevice ** & p);
	};


	struct SingleLinksRange
	{
		CDevice **m_ppLinkStart;
		CDevice **m_ppLinkEnd;
		SingleLinksRange() {}
		SingleLinksRange(CDevice **ppStart, CDevice **ppEnd) : m_ppLinkStart(ppStart), m_ppLinkEnd(ppEnd)
		{
			_ASSERTE(m_ppLinkStart <= m_ppLinkEnd);
		}
	};

	class CSingleLink
	{
	protected:
		SingleLinksRange m_LinkRange;
	public:
		void SetRange(SingleLinksRange& LinkRange)
		{
			m_LinkRange = LinkRange;
		}
		const SingleLinksRange& GetLinksRange() const
		{
			return m_LinkRange;
		}

		bool SetLink(ptrdiff_t nIndex, CDevice *pDevice) const
		{
			if (nIndex >= 0 && nIndex < m_LinkRange.m_ppLinkEnd - m_LinkRange.m_ppLinkStart)
			{
				*(m_LinkRange.m_ppLinkStart + nIndex) = pDevice;
				return true;
			}
			return false;
		}

		CDevice* GetLink(ptrdiff_t nIndex) const
		{
#ifdef _DEBUG
			if (nIndex >= 0 && nIndex < m_LinkRange.m_ppLinkEnd - m_LinkRange.m_ppLinkStart)
			{
#endif
				return *(m_LinkRange.m_ppLinkStart + nIndex);
#ifdef _DEBUG
			}
			else
			{
				_ASSERTE(!_T("SingleLinkRange::SetLink() Wrong link index"));
				return NULL;
			}
#endif
		}
	};

	enum DEVICE_EQUATION_TYPE
	{
		DET_ALGEBRAIC		= 0,
		DET_DIFFERENTIAL	= 1
	};

	enum ACTIVE_POWER_DAMPING_TYPE
	{
		APDT_NODE,
		APDT_ISLAND
	};

	

	struct RightVector
	{
		double *pValue;
		CDevice *pDevice;
		double Nordsiek[3];
		double Error;
		double Atol;
		double Rtol;
		DEVICE_EQUATION_TYPE EquationType;
		double SavedNordsiek[3];
		double SavedError;
		double Tminus2Value;
		DEVICE_EQUATION_TYPE PhysicalEquationType;
		PrimitiveBlockType PrimitiveBlock;
		ptrdiff_t nErrorHits;
		double GetWeightedError(double dError, double dAbsValue)
		{
			_ASSERTE(Atol > 0.0);
			return dError / (fabs(dAbsValue) * Rtol + Atol);
		}

		double GetWeightedError(double dAbsValue)
		{
			_ASSERTE(Atol > 0.0);
			return Error / (fabs(dAbsValue) * Rtol + Atol);
		}
	};

	enum eDEVICEFUNCTIONSTATUS
	{
		DFS_OK,
		DFS_NOTREADY,
		DFS_DONTNEED,
		DFS_FAILED
	};

	enum eDEVICESTATE
	{
		DS_OFF,
		DS_ON,
		DS_READY,
		DS_DETERMINE,
		DS_ABSENT
	};

	enum eDEVICESTATECAUSE
	{
		DSC_EXTERNAL,
		DSC_INTERNAL
	};



	class CDevice : public CDeviceId
	{
	protected:
		CDeviceContainer *m_pContainer;
		CSingleLink m_DeviceLinks;
		eDEVICEFUNCTIONSTATUS m_eInitStatus;
		bool CheckAddVisited(CDevice *pDevice);
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);
		ptrdiff_t m_nMatrixRow;
		eDEVICESTATE m_State;
		eDEVICESTATECAUSE m_StateCause;
		bool InitExternalVariable(PrimitiveVariableExternal& ExtVar, CDevice* pFromDevice, const _TCHAR* cszName, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);
		bool InitConstantVariable(double& ConstVar, CDevice* pFromDevice, const _TCHAR* cszName, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);

		inline static double ZeroDivGuard(double Nom, double Denom)
		{
			return (fabs(Denom) < DFW2_EPSILON) ? Nom : Nom / Denom;
		}

		const CSingleLink& GetSingleLinks() { return m_DeviceLinks; }
		virtual void UpdateVerbalName();

		typedef eDEVICEFUNCTIONSTATUS(CheckMasterDeviceFunction)(CDevice*, LinkDirectionFrom&);

		static CheckMasterDeviceFunction CheckMasterDeviceInit;
		static CheckMasterDeviceFunction CheckMasterDeviceDiscontinuity;

		eDEVICEFUNCTIONSTATUS MastersReady(CheckMasterDeviceFunction* pFnCheckMasterDevice);
	public:
		CDevice();

		eDFW2DEVICETYPE GetType() const;
		bool IsKindOfType(eDFW2DEVICETYPE eType);

		void Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage);

		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		double* GetVariablePtr(const _TCHAR* cszVarName);

		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		double* GetConstVariablePtr(const _TCHAR* cszVarName);

		virtual ExternalVariable GetExternalVariable(const _TCHAR* cszVarName);

		const double* GetVariableConstPtr(ptrdiff_t nVarIndex) const;
		const double* GetVariableConstPtr(const _TCHAR* cszVarName) const;

		const double* GetConstVariableConstPtr(ptrdiff_t nVarIndex) const;
		const double* GetConstVariableConstPtr(const _TCHAR* cszVarName) const;

		double GetValue(ptrdiff_t nVarIndex) const;
		double SetValue(ptrdiff_t nVarIndex, double Value);
		double GetValue(const _TCHAR* cszVarName) const;
		double SetValue(const _TCHAR* cszVarName, double Value);

		bool SetSingleLink(ptrdiff_t nIndex, CDevice *pDevice);
		CDevice* GetSingleLink(ptrdiff_t nIndex);
		CDevice *GetSingleLink(eDFW2DEVICETYPE eDevType);
		void SetContainer(CDeviceContainer* pContainer);
		virtual ~CDevice();
		virtual bool LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom);
		bool IncrementLinkCounter(ptrdiff_t nLinkIndex);
		ptrdiff_t m_nInContainerIndex;
		CLinkPtrCount* GetLink(ptrdiff_t nLinkIndex);
		void ResetVisited();
		void SetSingleLinkStart(CDevice **ppLinkStart);
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		virtual bool NewtonUpdateEquation(CDynaModel *pDynaModel);
		eDEVICEFUNCTIONSTATUS CheckInit(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS Initialized() { return m_eInitStatus; }
		virtual bool InitExternalVariables(CDynaModel *pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		eDEVICEFUNCTIONSTATUS DiscontinuityProcessed() { return m_eInitStatus; }
		void UnprocessDiscontinuity() { m_eInitStatus = DFS_NOTREADY;  }
		inline ptrdiff_t A(ptrdiff_t nOffset) { return m_nMatrixRow + nOffset; }
		virtual void InitNordsiek(CDynaModel* pDynaModel);
		virtual void Predict() {};
		virtual void EstimateEquations(CDynaModel *pDynaModel);
		virtual bool LeaveDiscontinuityMode(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS CheckProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICESTATE GetState() const { return m_State; }
		bool IsStateOn() const { return GetState() == DS_ON;  }
		bool IsPresent() const {  return GetState() != DS_ABSENT; }
		eDEVICESTATECAUSE GetStateCause() { return m_StateCause; }
		virtual eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause);
		const _TCHAR* VariableNameByPtr(double *pVariable);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel) { return 1.0; }


		static eDEVICEFUNCTIONSTATUS DeviceFunctionResult(eDEVICEFUNCTIONSTATUS Status1, eDEVICEFUNCTIONSTATUS Status2);
		inline static double Round(double Value, double Precision)
		{
			return round(Value / Precision) * Precision;
		}
		inline static bool IsFunctionStatusOK(eDEVICEFUNCTIONSTATUS Status)
		{
			return Status == DFS_OK || Status == DFS_DONTNEED;
		}

#ifdef _DEBUG
		static _TCHAR UnknownVarIndex[80];
#endif

	};

#define MAP_VARIABLE(VarName, VarId)  case VarId: p = &VarName; break; 
}
