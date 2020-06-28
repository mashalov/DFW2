#pragma once
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

	using PRIMITIVEPARAMETERDEFAULT = std::pair<std::reference_wrapper<double>, double>;
	using PRIMITIVEPARAMETERSDEFAULT = std::vector<PRIMITIVEPARAMETERDEFAULT>;
	using DOUBLEREFVEC = std::vector<std::reference_wrapper<double>>;

	struct InputVariable
	{
		double*& pValue;
		double* pInternal;
		ptrdiff_t& Index;
		InputVariable(VariableIndex& Variable) : pInternal(&Variable.Value), pValue(pInternal), Index(Variable.Index) {}
		InputVariable(VariableIndexExternal& Variable) : pInternal(nullptr), pValue(Variable.m_pValue), Index(Variable.Index) {}
		InputVariable(const InputVariable& Input) : pValue(Input.pValue), pInternal(Input.pInternal), Index(Input.Index) {}
		constexpr operator double& () { return *pValue; }
		constexpr operator const double& () const { return *pValue; }
		constexpr double& operator= (double value) { *pValue = value;  return *pValue; }
	};

	using ExtraOutputList = std::initializer_list<std::reference_wrapper<VariableIndex>>;
	using InputList = std::initializer_list<PrimitiveVariableBase*>;
	using InputList2 = std::initializer_list<InputVariable>;

	class CDynaPrimitive
	{
	protected:
		PrimitiveVariableBase *m_Input = nullptr;
		InputVariable m_InputV;
		VariableIndex& m_Output;
		CDevice& m_Device;
		//ptrdiff_t A(ptrdiff_t nOffset);
		bool ChangeState(CDynaModel *pDynaModel, double Diff, double TolCheck, double Constraint, ptrdiff_t ValueIndex, double &rH);
		bool UnserializeParameters(PRIMITIVEPARAMETERSDEFAULT ParametersList, const DOUBLEVECTOR& Parameters);
		bool UnserializeParameters(DOUBLEREFVEC ParametersList, const DOUBLEVECTOR& Parameters);
	public:
		constexpr operator double& () { return m_Output.Value; }
		constexpr operator const double& () const { return m_Output.Value; }
		constexpr double& operator= (double value) { m_Output.Value = value;  return m_Output.Value; }
		constexpr operator VariableIndex& () { return m_Output; }
		constexpr operator const VariableIndex& () const { return m_Output; }

		static void InitializeInputs(std::initializer_list<PrimitiveVariableBase**> InputVariables, std::initializer_list<PrimitiveVariableBase*> Input)
		{
			auto source = Input.begin();
			for (auto& inp : InputVariables)
			{
				if (source == Input.end())
					throw dfw2error(_T("CDynaPrimitive::InitializeInputs - inputs count mismatch"));
				*inp = *source;
				source++;
			}
		}

		CDynaPrimitive(CDevice& Device, 
					   VariableIndex& OutputVariable,
					   InputList Input,
					   ExtraOutputList ExtraOutputVariables = {}) : m_Device(Device),
																    m_Output(OutputVariable),
																	m_InputV(OutputVariable)
		{
			m_Output = 0.0;
			InitializeInputs({&m_Input}, Input);
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
		virtual PrimitiveVariableBase& Input(ptrdiff_t nIndex) { return *m_Input; }
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
