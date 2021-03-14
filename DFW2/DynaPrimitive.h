#pragma once
#include "Device.h"
#include <limits.h>
#include <type_traits>

namespace DFW2
{
	class CDynaModel;
	struct RightVector;

	using PRIMITIVEPARAMETERDEFAULT = std::pair<std::reference_wrapper<double>, double>;
	using PRIMITIVEPARAMETERSDEFAULT = std::vector<PRIMITIVEPARAMETERDEFAULT>;
	using DOUBLEREFVEC = std::vector<std::reference_wrapper<double>>;

	// Входая переменная примитива 
	// Унифицирует типы переменных - внутренней и внешней
	// Если входная переменная связана с внешней - используем ссылку на указатель внешней переменной, поэтому
	// InputVariable всегда тождественна внешней переменной
	// Если входная переменная связана с внутренней, у нас есть только указатель на значение и ссылку мы использовать не можем
	// Чтобы использовать единый интерфейс доступа значение внутренней переменной берется на указатель pInternal, а ссылка инициализуется
	// этим указателем.

	struct InputVariable
	{
	private:
		double*& pValue;		// ссылка на указатель на значение
		double* pInternal;		// промежуточный указатель для внутренней переменной
	public:
		ptrdiff_t& Index;		// ссылка на индекс переменной
		// конструктор для внутренней переменной берет указатель на значение внутренней переменной
		// а ссылку инициализирует на внутренний указатель
		InputVariable(VariableIndex& Variable) : pInternal(&Variable.Value), pValue(pInternal), Index(Variable.Index) {}
		// конструктор для внешней переменной просто берет указатель из внешней переменной и не использует
		InputVariable(VariableIndexExternal& Variable) : pInternal(nullptr), pValue(Variable.pValue), Index(Variable.Index) {}

		// конструктор копирования 
		// если аргумент был связан с внутренней переменной
		// ссылка на указатель инициализируется ссылкой на указатель _своей_ внутренней переменной
		// иначе берется ссылка из копии
		// внутренний указатель просто копируется
		InputVariable(const InputVariable& Input) : 
			pValue(Input.pInternal ? pInternal : Input.pValue),		
			pInternal(Input.pInternal),
			Index(Input.Index)
			{}

		// оператор присваивания для InputVariable блокирован
		InputVariable& operator =(const InputVariable& other) = delete;

		// значение всегда возвращается с указателя pValue
		// для внешней переменной этот совпадает с указателем на значение переменной
		// для внутренней указатель указывает на промежуточный внутренний указатель, 
		// который иницализирован указателем на значение внутренней переменной
		constexpr operator double& () { return *pValue; }
		constexpr operator const double& () const { return *pValue; }
		// значение double можно присвоить
		constexpr double& operator= (double value) { *pValue = value;  return *pValue; }
	};

	using OutputList = std::initializer_list<std::reference_wrapper<VariableIndex>>;
	using InputList = std::initializer_list<InputVariable>;
	using InputVariableVec = std::vector<InputVariable>;

	// адаптер диапазона для конструкторов примитивов
	// принимает contiguous контейнер с range, запоминает итераторы диапазона
	// дает доступ по индексу
	// используется для конструктора с векторами входных и выходных переменных
	// и для адаптации initializer_list из соответствующего конструктора примитива

	template<typename T>
	struct IORangeT
	{
		typename T::const_iterator begin, end;

		IORangeT(const T& rng) : begin(rng.begin()), end(rng.end()) {}

		typename T::value_type operator[] (const int index) const
		{
			if (end - begin > index)
				return *(begin + index);
			else
				throw dfw2error(_T("IORange Iterator out of range"));
		}
	};

	// специализация для входных и выходных переменных примитива
	using IRange = IORangeT<InputVariableVec>;
	using ORange = IORangeT<VariableIndexRefVec>;
		

	// базовый класс примитива
	class CDynaPrimitive
	{
	protected:
		InputVariable  m_Input;			// одна входная переменная
		VariableIndex& m_Output;		// одна выходная переменная, которая должна быть снаружи
		CDevice& m_Device;				// ссылка на устройство, к которому принадлежит примитив
		bool ChangeState(CDynaModel *pDynaModel, double Diff, double TolCheck, double Constraint, ptrdiff_t ValueIndex, double &rH);
		bool UnserializeParameters(PRIMITIVEPARAMETERSDEFAULT ParametersList, const DOUBLEVECTOR& Parameters);
		bool UnserializeParameters(DOUBLEREFVEC ParametersList, const DOUBLEVECTOR& Parameters);
	public:
		// возвращает значение выхода
		constexpr operator double& () { return m_Output.Value; }
		constexpr operator const double& () const { return m_Output.Value; }
		constexpr operator double* () { return &m_Output.Value; }
		// присваивает значение связанной выходной переменной
		constexpr double& operator= (double value) { m_Output.Value = value;  return m_Output.Value; }
		// возвращает ссылки на выходную переменную
		constexpr operator VariableIndex& () { return m_Output; }
		constexpr operator const VariableIndex& () const { return m_Output; }

		// примитив может иметь несколько входов и выходов (количество выходов соответствует порядку примитива)
		// поэтому конструктор принимает список входных переменных, одну обязательную выходную переменную
		// и список дополнительных выходных переменных

		CDynaPrimitive(CDevice& Device,
			const OutputList& Output,
			const InputList& Input) : CDynaPrimitive(Device, ORange(Output), IRange(Input)) {}

		CDynaPrimitive(CDevice& Device, const ORange& Output, const IRange& Input) :
			m_Device(Device),
			m_Output(Output[0]),
			m_Input(Input[0])
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
		virtual bool UnserializeParameters(CDynaModel *pDynaModel, const DOUBLEVECTOR& Parameters) { return true; }
		static double GetZCStepRatio(CDynaModel *pDynaModel, double a, double b, double c);
		static double FindZeroCrossingToConst(CDynaModel *pDynaModel, RightVector* pRightVector, double dConst);
	};

	// примитив с состоянием, изменение которого может вызывать разрыв
	class CDynaPrimitiveState : public CDynaPrimitive 
	{
	public:

		CDynaPrimitiveState(CDevice& Device, const ORange& Output, const IRange& Input) : CDynaPrimitive(Device, Output, Input)
		{
			m_Device.RegisterStatePrimitive(this);
		}
		CDynaPrimitiveState(CDevice& Device, const OutputList& Output, const InputList& Input) :
			CDynaPrimitiveState(Device, ORange(Output), IRange(Input)) { }

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

		CDynaPrimitiveLimited(CDevice& Device, const ORange& Output, const IRange& Input) :
			CDynaPrimitiveState(Device, Output, Input) {}
		CDynaPrimitiveLimited(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CDynaPrimitiveLimited(Device, ORange(Output), IRange(Input)) { }

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

		CDynaPrimitiveBinary(CDevice& Device, const ORange& Output, const IRange& Input) : CDynaPrimitiveState(Device, Output, Input) {}
		CDynaPrimitiveBinary(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CDynaPrimitiveBinary(Device, ORange(Output), IRange(Input)) { }

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
		CDynaPrimitiveBinaryOutput(CDevice& Device, const ORange& Output, const IRange& Input) : CDynaPrimitiveBinary(Device, Output, Input) {}
		CDynaPrimitiveBinaryOutput(CDevice& Device, const OutputList& Output, const InputList& Input) : 
			CDynaPrimitiveBinaryOutput(Device, ORange(Output), IRange(Input)) { }

		static double FindZeroCrossingOfDifference(CDynaModel *pDynaModel, RightVector* pRightVector1, RightVector* pRightVector2);
		double CheckZeroCrossing(CDynaModel *pDynaModel) override;
	};
}
