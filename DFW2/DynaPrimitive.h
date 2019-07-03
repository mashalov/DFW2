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

	// примитив с минимальным и максимальным ограничениями 
	class CDynaPrimitiveLimited : public CDynaPrimitiveState
	{
	public:

		// возможные состояния примитива
		enum eLIMITEDSTATES
		{
			LS_MIN,		// на минимуме
			LS_MID,		// вне ограничения
			LS_MAX		// на максимуме
		};

	private:
		eLIMITEDSTATES eCurrentState;		// текущее состояние примитива
		eLIMITEDSTATES eSavedState;			// сохраненное состояние примитива

	protected:
		void SetCurrentState(CDynaModel *pDynaModel, eLIMITEDSTATES CurrentState);

		// численные значения ограничений и гистерезиса ограничений
		double m_dMin, m_dMax, m_dMinH, m_dMaxH;
		// виртуальные функции обработки изменения входного сигнала в зависимости от текущего состояния примитива
		virtual double OnStateMax(CDynaModel *pDynaModel) { return 1.0; }
		virtual double OnStateMin(CDynaModel *pDynaModel) { return 1.0; }
		virtual double OnStateMid(CDynaModel *pDynaModel) { return 1.0; }
	public:
		inline eLIMITEDSTATES GetCurrentState() { return eCurrentState; }
		void SetMinMax(CDynaModel *pDynaModel, double dMin, double dMax);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		CDynaPrimitiveLimited(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveState(pDevice, pOutput, nOutputIndex, Input) {}
		virtual ~CDynaPrimitiveLimited() {}
		virtual bool Init(CDynaModel *pDynaModel);
		virtual void StoreState() override { eSavedState = eCurrentState; }
		virtual void RestoreState() override { eCurrentState = eSavedState; }
	};

	class CDynaPrimitiveBinary : public CDynaPrimitiveState
	{
	protected:
		enum eRELAYSTATES
		{
			RS_ON,
			RS_OFF,
		};
		eRELAYSTATES eCurrentState;		// текущее состояние реле
		eRELAYSTATES eSavedState;		// сохраненное состояние реле
		inline eRELAYSTATES GetCurrentState() { return eCurrentState; }
		virtual void RequestZCDiscontinuity(CDynaModel* pDynaModel);
	public:
		CDynaPrimitiveBinary(CDevice *pDevice, double* pOutput, ptrdiff_t nOutputIndex, PrimitiveVariableBase* Input) : CDynaPrimitiveState(pDevice, pOutput, nOutputIndex, Input) {}
		virtual void SetCurrentState(CDynaModel *pDynaModel, eRELAYSTATES CurrentState);
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		virtual bool BuildDerivatives(CDynaModel *pDynaModel) { return true; }
		virtual void StoreState() override { eSavedState = eCurrentState; }
		virtual void RestoreState() override { eCurrentState = eSavedState; }
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
}
