#pragma once
using namespace std;
#include "Device.h"
#include <limits.h>

namespace DFW2
{
	class CDynaModel;
	struct RightVector;

	class PrimitiveVariableBase
	{
	protected:
		ptrdiff_t m_nIndex;
	public:
		virtual double& Value() = 0;
		inline bool Indexed() { return m_nIndex != nIndexUnassigned; }
		inline void UnIndex() { m_nIndex = nIndexUnassigned; }
		inline ptrdiff_t Index() { return m_nIndex; }
		void Index(ptrdiff_t nIndex) { m_nIndex = nIndex; }
		virtual void IndexAndValue(ptrdiff_t nIndex, double* pValue) {}
		static const ptrdiff_t nIndexUnassigned;
	};

	class PrimitiveVariable : public PrimitiveVariableBase
	{
	protected:
		double&   m_dValue;
	public:
		PrimitiveVariable(ptrdiff_t nIndex, double& pdValue) : m_dValue(pdValue)  { m_nIndex = nIndex; }
		virtual double& Value()   { return m_dValue; }
		virtual void IndexAndValue(ptrdiff_t nIndex, double* pValue) {}
	};

	class PrimitiveVariableExternal: public PrimitiveVariableBase
	{
	protected:
		double *m_pValue;
	public:
		virtual double& Value()   { return *m_pValue; }
		virtual ptrdiff_t Index() { return m_nIndex;  }
		void IndexAndValue(ptrdiff_t nIndex, double* pValue)
		{
			m_nIndex = nIndex;
			m_pValue = pValue;
		}
		void IndexAndValue(ExternalVariable& ExtVar)
		{
			m_nIndex = ExtVar.nIndex;
			m_pValue = ExtVar.pValue;
		}

		void Value(double* pValue)
		{
			m_pValue = pValue;
			UnIndex(); 
		}
	};

	class CDynaPrimitive
	{
	protected:
		PrimitiveVariableBase *m_Input;
		double *m_Output;
		CDevice *m_pDevice;
		ptrdiff_t A(ptrdiff_t nOffset);
		ptrdiff_t m_OutputEquationIndex;

	public:
		CDynaPrimitive(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : m_pDevice(pDevice), 
																												  m_Input(Input), 
																												  m_Output(pOutput),
																												  m_OutputEquationIndex(nOutputIndex)
		{
			*m_Output = 0.0;
		}
		virtual ~CDynaPrimitive() {}

		virtual bool BuildEquations(CDynaModel *pDynaModel) = 0;
		virtual bool BuildRightHand(CDynaModel *pDynaModel) = 0;
		virtual bool BuildDerivatives(CDynaModel *pDynaModel) = 0;

		virtual bool Init(CDynaModel *pDynaModel);
		virtual const _TCHAR* GetVerbalName() { return _T(""); }
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) { return eDEVICEFUNCTIONSTATUS::DFS_OK; }
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		virtual PrimitiveVariableBase& Input(ptrdiff_t nIndex) { return *m_Input; }
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, double *pParameters, size_t nParametersCount) { return true; }
		static double GetZCStepRatio(CDynaModel *pDynaModel, double a, double b, double c);
		static double FindZeroCrossingToConst(CDynaModel *pDynaModel, RightVector* pRightVector, double dConst);
		inline const double* Output() const { return m_Output; }
	};

	class CDynaPrimitiveLimited : public CDynaPrimitive
	{
	public:

		enum eLIMITEDSTATES
		{
			LS_MIN,
			LS_MID,
			LS_MAX
		};

	private:
		eLIMITEDSTATES eCurrentState;

	protected:
		void SetCurrentState(CDynaModel *pDynaModel, eLIMITEDSTATES CurrentState);

		double m_dMin, m_dMax, m_dMinH, m_dMaxH;
		virtual double OnStateMax(CDynaModel *pDynaModel) { return 1.0; }
		virtual double OnStateMin(CDynaModel *pDynaModel) { return 1.0; }
		virtual double OnStateMid(CDynaModel *pDynaModel) { return 1.0; }
	public:
		inline eLIMITEDSTATES GetCurrentState() { return eCurrentState; }
		void SetMinMax(CDynaModel *pDynaModel, double dMin, double dMax);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		CDynaPrimitiveLimited(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitive(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CDynaPrimitiveLimited() {}
		virtual bool Init(CDynaModel *pDynaModel);
	};

	class CDynaPrimitiveBinary : public CDynaPrimitive
	{
	protected:
		enum eRELAYSTATES
		{
			RS_ON,
			RS_OFF,
		};
		eRELAYSTATES eCurrentState;
		inline eRELAYSTATES GetCurrentState() { return eCurrentState; }
		virtual void RequestZCDiscontinuity(CDynaModel* pDynaModel);
	public:
		CDynaPrimitiveBinary(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitive(pDevice, pOutput, nOutputIndex, Input) {}
		virtual void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState);
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel) { return true; }
	};

	class CDynaPrimitiveBinaryOutput : public CDynaPrimitiveBinary
	{
	protected:
		virtual double OnStateOn(CDynaModel *pDynaModel)  { return 1.0; }
		virtual double OnStateOff(CDynaModel *pDynaModel) { return 1.0; }
	public:
		CDynaPrimitiveBinaryOutput(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input) {}
		static double FindZeroCrossingOfDifference(CDynaModel *pDynaModel, RightVector* pRightVector1, RightVector* pRightVector2);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
	};

	

	template<size_t nSize>
	class CPrimitives
	{
	protected:
		size_t m_nSize;
		size_t m_nCounter;
	public:
		CPrimitives() : m_nSize(nSize), m_nCounter(0) {}
		CDynaPrimitive *m_pPrimitives[nSize];
		void Add(CDynaPrimitive *pPrimitive)
		{
			if (m_nCounter < m_nSize)
				m_pPrimitives[m_nCounter++] = pPrimitive;
			else
				_ASSERTE(!_T("Wrong CPrimitives initial size"));
		}

		bool BuildDerivatives(CDynaModel *pDynaModel)
		{
			bool bRes = true;
			CDynaPrimitive **pEnd = nSize + m_pPrimitives;
			for (CDynaPrimitive **pBegin = m_pPrimitives; pBegin < pEnd; pBegin++)
			{
				bRes = bRes && (*pBegin)->BuildDerivatives(pDynaModel);
			}
			return bRes;
		}


		bool BuildEquations(CDynaModel *pDynaModel)
		{
			bool bRes = true;
			CDynaPrimitive **pEnd = nSize + m_pPrimitives;
			for (CDynaPrimitive **pBegin = m_pPrimitives; pBegin < pEnd; pBegin++)
			{
				bRes = bRes && (*pBegin)->BuildEquations(pDynaModel);
			}
			_ASSERTE(bRes);
			return bRes;
		}

		bool BuildRightHand(CDynaModel *pDynaModel)
		{
			bool bRes = true;
			CDynaPrimitive **pEnd = nSize + m_pPrimitives;
			for (CDynaPrimitive **pBegin = m_pPrimitives; pBegin < pEnd; pBegin++)
			{
				bRes = bRes && (*pBegin)->BuildRightHand(pDynaModel);
			}
			_ASSERTE(bRes);
			return bRes;
		}

		double CheckZeroCrossing(CDynaModel *pDynaModel)
		{
			bool bRes = true;
			CDynaPrimitive **pEnd = nSize + m_pPrimitives;

			double rH = 1.0;

			for (CDynaPrimitive **pBegin = m_pPrimitives; pBegin < pEnd; pBegin++)
			{
				double rHcurrent = (*pBegin)->CheckZeroCrossing(pDynaModel);
				if (rHcurrent < rH)
					rH = rHcurrent;
			}
			return rH;
		}


		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel *pDynaModel)
		{
			eDEVICEFUNCTIONSTATUS Status = DFS_OK;
			CDynaPrimitive **pEnd = nSize + m_pPrimitives;
			for (CDynaPrimitive **pBegin = m_pPrimitives; pBegin < pEnd; pBegin++)
			{
				switch ((*pBegin)->ProcessDiscontinuity(pDynaModel))
				{
				case DFS_FAILED:
					Status = DFS_FAILED;
					break;
				case DFS_NOTREADY:
					Status = DFS_NOTREADY;
					break;
				}
			}

			_ASSERTE(Status != DFS_FAILED);

			return Status;
		}
	};
}
