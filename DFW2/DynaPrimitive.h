#pragma once
#include "Device.h"
#include <limits.h>

namespace DFW2
{
	class CDynaModel;
	struct RightVector;

	using PRIMITIVEPARAMETERDEFAULT = std::pair<std::reference_wrapper<double>, double>;
	using PRIMITIVEPARAMETERSDEFAULT = std::vector<PRIMITIVEPARAMETERDEFAULT>;
	using DOUBLEREFVEC = std::vector<std::reference_wrapper<double>>;

	struct InputVariable
	{
	private:
		double*& pValue;
		double* pInternal;
	public:
		ptrdiff_t& Index;
		InputVariable(VariableIndex& Variable) : pInternal(&Variable.Value), pValue(pInternal), Index(Variable.Index) {}
		InputVariable(VariableIndexExternal& Variable) : pInternal(nullptr), pValue(Variable.pValue), Index(Variable.Index) {}
		InputVariable(const InputVariable& Input) : 
			pValue(Input.pInternal ? pInternal : Input.pValue),
			pInternal(Input.pInternal),
			Index(Input.Index)
			{}

		InputVariable& operator =(const InputVariable& other) = delete;

		constexpr operator double& () { return *pValue; }
		constexpr operator const double& () const { return *pValue; }
		constexpr double& operator= (double value) { *pValue = value;  return *pValue; }
	};

	using ExtraOutputList = std::initializer_list<std::reference_wrapper<VariableIndex>>;
	using InputList = std::initializer_list<InputVariable>;

	class CDynaPrimitive
	{
	protected:
		InputVariable  m_Input;
		VariableIndex& m_Output;
		CDevice& m_Device;
		bool ChangeState(CDynaModel *pDynaModel, double Diff, double TolCheck, double Constraint, ptrdiff_t ValueIndex, double &rH);
		bool UnserializeParameters(PRIMITIVEPARAMETERSDEFAULT ParametersList, const DOUBLEVECTOR& Parameters);
		bool UnserializeParameters(DOUBLEREFVEC ParametersList, const DOUBLEVECTOR& Parameters);
	public:
		constexpr operator double& () { return m_Output.Value; }
		constexpr operator const double& () const { return m_Output.Value; }
		constexpr double& operator= (double value) { m_Output.Value = value;  return m_Output.Value; }
		constexpr operator VariableIndex& () { return m_Output; }
		constexpr operator const VariableIndex& () const { return m_Output; }

		CDynaPrimitive(CDevice& Device, 
					   VariableIndex& OutputVariable,
					   InputList Input,
					   ExtraOutputList ExtraOutputVariables = {}) : m_Device(Device),
																    m_Output(OutputVariable),
																	m_Input(*Input.begin())
		{
			m_Output = 0.0;
			m_Device.RegisterPrimitive(this);
		}

		virtual ~CDynaPrimitive() {}

		virtual bool BuildEquations(CDynaModel *pDynaModel) = 0;
		virtual bool BuildRightHand(CDynaModel *pDynaModel) = 0;
		virtual bool BuildDerivatives(CDynaModel *pDynaModel) = 0;

		virtual bool Init(CDynaModel *pDynaModel);
		virtual const _TCHAR* GetVerbalName() { return _T(""); }
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel) { return eDEVICEFUNCTIONSTATUS::DFS_OK; }
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);
		//virtual PrimitiveVariableBase& Input(ptrdiff_t nIndex) { return *m_Input; }
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) { return true; }
		static double GetZCStepRatio(CDynaModel *pDynaModel, double a, double b, double c);
		static double FindZeroCrossingToConst(CDynaModel *pDynaModel, RightVector* pRightVector, double dConst);
		//inline const double* Output() const { return m_Output; }
	};

	class CDynaPrimitiveState : public CDynaPrimitive 
	{
	public:
		CDynaPrimitiveState(CDevice& Device, 
							VariableIndex& OutputVariable, 
							InputList Input, 
							ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitive(Device, OutputVariable, Input, ExtraOutputVariables)
		{
			m_Device.RegisterStatePrimitive(this);
		}
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
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
		CDynaPrimitiveLimited(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveState(Device, OutputVariable, Input, ExtraOutputVariables) {}
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
		eRELAYSTATES eCurrentState;		// текущее состояние реле
		eRELAYSTATES eSavedState;		// сохраненное состояние реле
		virtual inline eRELAYSTATES GetCurrentState() { return eCurrentState; }
		virtual void RequestZCDiscontinuity(CDynaModel* pDynaModel);
	public:
		CDynaPrimitiveBinary(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveState(Device, OutputVariable, Input, ExtraOutputVariables) {}
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
		virtual double OnStateOn(CDynaModel *pDynaModel) { return 1.0; }
		virtual double OnStateOff(CDynaModel *pDynaModel) { return 1.0; }
	public:
		CDynaPrimitiveBinaryOutput(CDevice& Device, VariableIndex& OutputVariable, InputList Input, ExtraOutputList ExtraOutputVariables) :
			CDynaPrimitiveBinary(Device, OutputVariable, Input, ExtraOutputVariables) {}
		static double FindZeroCrossingOfDifference(CDynaModel *pDynaModel, RightVector* pRightVector1, RightVector* pRightVector2);
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
	};
}
