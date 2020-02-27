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
		inline bool Indexed() { return m_nIndex != CDevice::nIndexUnassigned; }
		inline void UnIndex() { m_nIndex = CDevice::nIndexUnassigned; }
		inline ptrdiff_t Index() 
		{ 
			if (!Indexed())
				throw dfw2error(_T("PrimitiveVariableBase::Index - access to unindexed variable"));
			return m_nIndex; 
		}
		void Index(ptrdiff_t nIndex) { m_nIndex = nIndex; }
		virtual void IndexAndValue(ptrdiff_t nIndex, double* pValue) {}
	};

	class PrimitiveVariable : public PrimitiveVariableBase
	{
	protected:
		double&   m_dValue;
	public:
		PrimitiveVariable(ptrdiff_t nIndex, double& pdValue) : m_dValue(pdValue)  { m_nIndex = nIndex; }
		double& Value() override { return m_dValue; }
		void IndexAndValue(ptrdiff_t nIndex, double* pValue) override {}
	};

	class PrimitiveVariableExternal: public PrimitiveVariableBase
	{
	protected:
		double *m_pValue;
	public:
		double& Value()  override { return *m_pValue; }
		virtual ptrdiff_t Index() { return m_nIndex;  }
		void IndexAndValue(ptrdiff_t nIndex, double* pValue) override
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
		bool ChangeState(CDynaModel *pDynaModel, double Diff, double TolCheck, double Constraint, ptrdiff_t ValueIndex, double &rH);
	public:
		CDynaPrimitive(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : m_pDevice(pDevice), 
																												  m_Input(Input), 
																												  m_Output(pOutput),
																												  m_OutputEquationIndex(nOutputIndex)
		{
			*m_Output = 0.0;
			pDevice->RegisterPrimitive(this);
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

	class CDynaPrimitiveState : public CDynaPrimitive 
	{
	public:
		CDynaPrimitiveState(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input);
		virtual void StoreState() = 0;
		virtual void RestoreState() = 0;
	};

	// �������� � ����������� � ������������ ������������� 
	class CDynaPrimitiveLimited : public CDynaPrimitiveState
	{
	public:

		// ��������� ��������� ���������
		enum eLIMITEDSTATES
		{
			LS_MIN,		// �� ��������
			LS_MID,		// ��� �����������
			LS_MAX		// �� ���������
		};

	private:
		eLIMITEDSTATES eCurrentState;		// ������� ��������� ���������
		eLIMITEDSTATES eSavedState;			// ����������� ��������� ���������

	protected:
		void SetCurrentState(CDynaModel *pDynaModel, eLIMITEDSTATES CurrentState);

		// ��������� �������� ����������� � ����������� �����������
		double m_dMin, m_dMax, m_dMinH, m_dMaxH;
		// ����������� ������� ��������� ��������� �������� ������� � ����������� �� �������� ��������� ���������
		virtual double OnStateMax(CDynaModel *pDynaModel) { return 1.0; }
		virtual double OnStateMin(CDynaModel *pDynaModel) { return 1.0; }
		virtual double OnStateMid(CDynaModel *pDynaModel) { return 1.0; }
	public:
		inline eLIMITEDSTATES GetCurrentState() { return eCurrentState; }
		void SetMinMax(CDynaModel *pDynaModel, double dMin, double dMax);
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		CDynaPrimitiveLimited(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveState(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CDynaPrimitiveLimited() {}
		bool Init(CDynaModel *pDynaModel) override;
		void StoreState() override { eSavedState = eCurrentState; }
		void RestoreState() override { eCurrentState = eSavedState; }
	};

	class CDynaPrimitiveBinary : public CDynaPrimitiveState
	{
	protected:
		enum eRELAYSTATES
		{
			RS_ON,
			RS_OFF,
		};
		eRELAYSTATES eCurrentState;		// ������� ��������� ����
		eRELAYSTATES eSavedState;		// ����������� ��������� ����
		inline eRELAYSTATES GetCurrentState() { return eCurrentState; }
		virtual void RequestZCDiscontinuity(CDynaModel* pDynaModel);
	public:
		CDynaPrimitiveBinary(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveState(pDevice, pOutput, nOutputIndex, Input) {}
		void InvertState(CDynaModel *pDynaModel);
		virtual void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState);
		bool BuildEquations(CDynaModel *pDynaModel) override;
		bool BuildRightHand(CDynaModel *pDynaModel)  override;
		bool BuildDerivatives(CDynaModel *pDynaModel)  override { return true; }
		void StoreState() override { eSavedState = eCurrentState; }
		void RestoreState() override { eCurrentState = eSavedState; }
	};

	class CDynaPrimitiveBinaryOutput : public CDynaPrimitiveBinary
	{
	protected:
		virtual double OnStateOn(CDynaModel *pDynaModel)  { return 1.0; }
		virtual double OnStateOff(CDynaModel *pDynaModel) { return 1.0; }
	public:
		CDynaPrimitiveBinaryOutput(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveBinary(pDevice, pOutput, nOutputIndex, Input) {}
		static double FindZeroCrossingOfDifference(CDynaModel *pDynaModel, RightVector* pRightVector1, RightVector* pRightVector2);
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
	};
}
