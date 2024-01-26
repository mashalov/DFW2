#pragma once
#include "DeviceId.h"
#include "SerializerValidation.h"
#include <array>

namespace DFW2
{
	class CDeviceContainer;
	class CDevice;
	class CDynaModel;
	class PrimitiveVariableExternal;
	class CDynaPrimitive;
	class CDynaPrimitiveState;


	using STATEPRIMITIVESLIST = std::list<CDynaPrimitiveState*>;
	using PRIMITIVESVEC = std::vector<CDynaPrimitive*>;

	template<typename T>
	class LinkWalker
	{
	friend class CLinkPtrCount;
	protected:
		T** pointer = nullptr;
	public:
		inline T* operator->() noexcept { return *pointer; }
		inline bool empty() const noexcept { return pointer == nullptr; }
		inline operator T* () noexcept { return *pointer; };
	};

	// класс для хранения связей устройства
	// с помощью него можно обходить связанные с данным устройства
	class CLinkPtrCount
	{
	friend class CDeviceContainer;
	friend class CMultiLink;
	protected:
		CDevice **pPointerBegin_ = nullptr;		// вектор указателей на связанные устройства
		CDevice	**pPointerEnd_ = nullptr;		// указатель на конец связанных устройств
	public:

		// последовательное получение очередного связанного устройства _включенного_в_матрицу
		template<typename T>
		bool InMatrix(LinkWalker<T>& p) const noexcept
		{
			while (In(p))
			{
				if (p->InMatrix())
					return true;
			}
			return false;
		}

		// функция обхода связей устройства (типа обход ветвей узла, генераторов узла и т.п.)
		// на входе указатель на указатель устройства, с которым связь. Каждый следующий
		// вызов In() возвращает очередную связь и true, или false - если связи закончились
		// начало последовательности требует чтобы на вход был передан указатель на null
		template<typename T>
		bool In(LinkWalker<T>& p) const noexcept
		{
			if (!p.pointer)
			{
				// если передан указатель на null
				if (Count())
				{
					// если связи есть - возвращаем первую
					p.pointer = reinterpret_cast<T**>(pPointerBegin_);
					return true;
				}
				else
				{
					// если связей нет - завершаем обход
					p.pointer = nullptr;
					return false;
				}
			}

			// если передан указатель не на null, это
			// означает, что мы уже начали обходить связи
			// переходм к следующей
			p.pointer++;

			// проверяем, не достили ли конца списка связей
			if (reinterpret_cast<CDevice**>(p.pointer) < end())
				return true;

			// если достигли - завершаем обход
			p.pointer = nullptr;
			return false;
		}

		inline size_t Count() const noexcept { return pPointerEnd_ - pPointerBegin_; }
		inline CDevice** begin() const noexcept { return pPointerBegin_; }
		inline CDevice** end() const noexcept { return pPointerEnd_; }
	};

	// элемент для хранения/передачи списка связанных устройств одного типа
	struct SingleLinksRange
	{
	protected:
		CDevice **ppLinkStart_ = nullptr;		// начало и конец списка устройств
		CDevice **ppLinkEnd_   = nullptr;
	public:

		SingleLinksRange() {}
		// конструктор с заданием вектора начала и конца
		SingleLinksRange(CDevice **ppStart, CDevice **ppEnd) : ppLinkStart_(ppStart), ppLinkEnd_(ppEnd)
		{
			_ASSERTE(ppLinkStart_ <= ppLinkEnd_);
		}

		inline ptrdiff_t Count() const noexcept { return ppLinkEnd_ - ppLinkStart_; }
		inline CDevice** begin() const noexcept { return ppLinkStart_; }
		inline CDevice** end() const noexcept { return ppLinkStart_ + Count(); }

	};

	// класс связей устройства
	// CSingleLink представляет собой вектор указателей на устройства.
	// Каждый элемент вектора соответствует связи один к одному с другим устройством.
	// Тип связанного устройства задается индексом.

	// Общий для всех устройств контейнера вектор указателей на указатели хранится в CDeviceConatainer,
	// Внутри CDevice есть экземпляр CSingleLink,
	// которому при инициализации дается SingleLinksRange из этого вектора
	// внутри диапазона SingleLinkRange можно размещать связи с устройствами
	// принцип связи : индексу в SingleLinkRange соответствует тип: например для возбудителя
	// 0 - АРВ, 1 - РФ
	// размерность вектора формируется с помощью PossibleLinksCount - при линковке определяется
	// количество контейнеров, которые можно потенциально связать с данным контейнером

	
	class CSingleLink
	{
	protected:
		SingleLinksRange LinkRange_;		// список связей
	public:
		void SetRange(SingleLinksRange&& LinkRange)
		{
			LinkRange_ = LinkRange;
		}
		const SingleLinksRange& GetLinksRange() const noexcept
		{
			return LinkRange_;
		}

		// задать связь с индексом
		bool SetLink(ptrdiff_t Index, CDevice *pDevice) const
		{
			// проверка на корректность индекса в отладке и в релизе
			// потому что вызовов в процессе расчета нет
			if (Index >= 0 && Index < LinkRange_.Count())
			{
				*(LinkRange_.begin() + Index) = pDevice;
				return true;
			}
			return false;
		}

		// получить связь с индексом
		CDevice* GetLink(ptrdiff_t Index) const
		{
#ifdef _DEBUG
			if (Index >= 0 && Index < LinkRange_.Count())
			{
#endif
				// проверка на корректность индекса только в отладке для производительности
				return *(LinkRange_.begin() + Index);
#ifdef _DEBUG
			}
			else
			{
				_ASSERTE(!"SingleLinkRange::SetLink() Wrong link index");
				return nullptr;
			}
#endif
		}
	};

	// типы уравнений
	enum DEVICE_EQUATION_TYPE
	{
		DET_ALGEBRAIC		= 0,
		DET_DIFFERENTIAL	= 1
	};

	// способ учета демпфирования в генератора
	enum class ACTIVE_POWER_DAMPING_TYPE
	{
		APDT_NODE,			// по скольжению узла
		APDT_ISLAND			// по скольжению средневзвешенного узла синхронной зоны
	};

	// способ подавления рингинга метода Адамса 2-го порядка
	enum class ADAMS_RINGING_SUPPRESSION_MODE
	{
		ARSM_NONE,			// подавление не используется
		ARSM_GLOBAL,		// подавление для всех дифференциальных уравнений по кратности шага
		ARSM_INDIVIDUAL,	// подавление только для тех дифференциальных переменных, по которым обнаружен рингинг
		ARSM_DAMPALPHA		// подавление путем коррекции коэффициентов Адамса
	};

	// метод расчета параметров моделей генераторов Парк
	enum class PARK_PARAMETERS_DETERMINATION_METHOD
	{
		Kundur,
		NiiptTo,
		NiiptToTd,
		Canay
	};
	// Представление генерации в узле без генераторов
	// с помощью СХН
	enum class GeneratorLessLRC
	{
		Sconst,	// постоянная мощность
		Iconst	// постоянный ток
	};

	enum class DiscontinuityLevel
	{
		None,
		Light,
		Hard
	};

	struct RightVectorTotal
	{
		double *pValue;													// значение переменной состояния
		CDevice *pDevice;												// устройство, в котором эта переменная
		double Atol;													// абсолютная и относительная погрешности
		double Rtol;													// задаются индивидуально для каждой переменной
		ptrdiff_t ErrorHits;											// количество ограничений шага или завалов итераций Ньютона по этой переменной
		DEVICE_EQUATION_TYPE EquationType;								// тип уравнения переменной для выбора коэффициентов метода (BDF - алгебраический, ADAM - дифференциальный)
		DEVICE_EQUATION_TYPE PhysicalEquationType;						// физический тип уравнения - дифференциальный или алгебраический
	};

	// правая часть уравнения
	struct RightVector : RightVectorTotal
	{
		alignas (32) double Nordsiek[4];								// вектор Nordsieck по переменной до 2 порядка (размерность 4 - под SIMD)
		alignas (32) double SavedNordsiek[4];							// сохраненные перед шагом Nordsieck и ошибка 
		double b;														// ошибка на итерации Ньютона
		double Error;													// ошибка корректора
		double SavedError;												// предыдущего шага
		double Tminus2Value;											// значение на пред-предыдыущем шаге для реинита Nordsieck
		PrimitiveBlockType PrimitiveBlock;								// тип блока примитива если есть
		ptrdiff_t RingsCount;
		ptrdiff_t RingsSuppress;

		// расчет взвешенной ошибки по значению снаружи
		// но с допустимыми погрешностями для этой переменной
		inline double GetWeightedError(const double dError, const double dAbsValue) const noexcept
		{
			_ASSERTE(Atol > 0.0);
#ifdef USE_FMA_FULL
			return dError / std::fma(std::abs(dAbsValue), Rtol, Atol);
#else
			return dError / (std::abs(dAbsValue) * Rtol + Atol);
#endif
		}

		// расчет взвешенной ошибки по значению данной переменной
		inline double GetWeightedError(const double dAbsValue) const noexcept
		{
			_ASSERTE(Atol > 0.0);
#ifdef USE_FMA_FULL
			return Error / std::fma(std::abs(dAbsValue), Rtol, Atol);
#else
			return Error / (std::abs(dAbsValue) * Rtol + Atol);
#endif
		}
	};

	//! класс устройства, наследует все что связано с идентификацией
	class CDevice : public CDeviceId
	{
	protected:
		CDeviceContainer* pContainer_ = nullptr;										// контейнер устройства
		CSingleLink DeviceLinks_;														// связи устройств один к одному
		eDEVICEFUNCTIONSTATUS eInitStatus_ = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;		// статус инициализации устройства (заполняется в Init)
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);						// инициализация устройства
		ptrdiff_t MatrixRow_ = 0;														// строка в матрице с которой начинаются уравнения устройства
		eDEVICESTATE State_ = eDEVICESTATE::DS_ON;										// состояние устройства
		double StateVar_ = 1.0;															// фиктивная переменная для возврата состояния устройства в виде double: 1 - вкл, 0 - выкл
		eDEVICESTATECAUSE StateCause_ = eDEVICESTATECAUSE::DSC_INTERNAL;				// причина изменения состояния устройства
		STATEPRIMITIVESLIST StatePrimitives_;
		PRIMITIVESVEC Primitives_;
		bool InitExternalVariable(VariableIndexExternal& ExtVar, CDevice* pFromDevice, std::string_view Name, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);
		bool InitExternalVariable(VariableIndexExternalOptional& OptExtVar, CDevice* pFromDevice, std::string_view Name, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);
		bool InitConstantVariable(double& ConstVar, CDevice* pFromDevice, std::string_view Name, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);
		const CSingleLink& GetSingleLinks() { return DeviceLinks_; }

		// формирование подробного имени устройства. По умолчанию учитывается описание типа устройства
		void UpdateVerbalName() override;
		typedef eDEVICEFUNCTIONSTATUS(CheckMasterDeviceFunction)(CDevice*, LinkDirectionFrom const*);
		static CheckMasterDeviceFunction CheckMasterDeviceInit;
		static CheckMasterDeviceFunction CheckMasterDeviceDiscontinuity;
		eDEVICEFUNCTIONSTATUS MastersReady(CheckMasterDeviceFunction* pFnCheckMasterDevice);
		void DumpIntegrationStep(ptrdiff_t nId, ptrdiff_t nStepNumber);
		ptrdiff_t ZeroCrossings_ = 0;								// количество zero-crossings, короторе вызвало устройство
		ptrdiff_t DiscontinuityRequests_ = 0;						// количество запросов на обработку разрыва
		ptrdiff_t InContainerIndex_ = -1;
	public:

		eDFW2DEVICETYPE GetType() const noexcept;					// получить тип устройства
		bool IsKindOfType(eDFW2DEVICETYPE eType) const;				// проверить, входит ли устройство в цепочку наследования от заданного типа устройства

		void Log(DFW2MessageStatus Status, std::string_view Message) const;
		void DebugLog(std::string_view Message) const;

		// функция маппинга указателя на переменную к индексу переменной
		// Должна быть перекрыта во всех устройствах, которые наследованы от CDevice
		// внутри этой функции также делается "наследование" переменных
		virtual double* GetVariablePtr(ptrdiff_t VarIndex);
		double* GetVariablePtr(std::string_view VarName);
		virtual VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec);
		VariableIndex& GetVariable(ptrdiff_t VarIndex);
		// Объединяет заданный список переменных данного устройства, список переменных примитивов и дочерние переменные
		VariableIndexRefVec& JoinVariables(const VariableIndexRefVec& ThisVars, VariableIndexRefVec& ChildVec);
		// функция маппинга указателя на переменную к индексу переменной
		// Аналогична по смыслу virtual double* GetVariablePtr()
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		double* GetConstVariablePtr(std::string_view VarName);
		virtual VariableIndexExternal GetExternalVariable(std::string_view VarName);

		// константные указатели на переменную. Врапперы virtual double* GetVariablePtr()
		const double* GetVariableConstPtr(ptrdiff_t nVarIndex) const;
		const double* GetVariableConstPtr(std::string_view VarName) const;

		// константные указатели на переменную константы. Врапперы virtual double* GetConstVariablePtr()
		const double* GetConstVariableConstPtr(ptrdiff_t nVarIndex) const;
		const double* GetConstVariableConstPtr(std::string_view VarName) const;

		double GetValue(ptrdiff_t nVarIndex) const;
		double SetValue(ptrdiff_t nVarIndex, double Value);
		double GetValue(std::string_view VarName) const;
		double SetValue(std::string_view VarName, double Value);

		bool SetSingleLink(ptrdiff_t Index, CDevice* pDevice);
		CDevice* GetSingleLink(ptrdiff_t Index);
		CDevice* GetSingleLink(eDFW2DEVICETYPE eDevType);
		void SetContainer(CDeviceContainer* pContainer, ptrdiff_t Index);
		inline ptrdiff_t InContainerIndex() const noexcept { return InContainerIndex_; }
		CDeviceContainer* GetContainer() noexcept { return pContainer_; }
		const CDeviceContainer* GetContainer() const { return pContainer_; }
		virtual ~CDevice() = default;
		virtual bool LinkToContainer(CDeviceContainer* pContainer, CDeviceContainer* pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom);
		void IncrementLinkCounter(ptrdiff_t nLinkIndex);
		// получить связи устроства из слоя nLinkIndex
		const  CLinkPtrCount* const GetLink(ptrdiff_t nLinkIndex);
		void ResetVisited();
		ptrdiff_t CheckAddVisited(CDevice* pDevice);
		void SetSingleLinkStart(CDevice** ppLinkStart);

		CDynaModel* GetModel() noexcept;
		const CDynaModel* GetModel() const noexcept;
		void ProcessTopologyRequest();
		
		// построение блока уравнения в Якоби
		virtual void BuildEquations(CDynaModel* pDynaModel);
		// построение правой части уравнений
		virtual void BuildRightHand(CDynaModel* pDynaModel);
		// расчет значений производных дифференциальных уравнений
		virtual void BuildDerivatives(CDynaModel* pDynaModel);
		// функция обновления данных после итерации Ньютона (если надо)



		using fnDerivative = void(CDynaModel::*)(const VariableIndexBase& variable, double Value);

		// функция расчета производных модели. Должна быть перекрыта в наследованном классе
		virtual void CalculateDerivatives(CDynaModel* pDynaModel, fnDerivative fn) {};
		// функция ввода производных в правую часть уравнений (BuildRightHand). Вызывает CalculateDerivatives
		void SetFunctionsDiff(CDynaModel* pDynaModel);
		// функция ввода производных в вектор производных (BuildDerivatives). Вызывает CalculateDerivatives
		void SetDerivatives(CDynaModel* pDynaModel);

		virtual void NewtonUpdateEquation(CDynaModel* pDynaModel);
		// предварительная однократная инициалиация устройства
		virtual eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS CheckInit(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS Initialized() const noexcept { return eInitStatus_; }
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS DiscontinuityProcessed() const noexcept { return eInitStatus_; }
		// ставит статус обработки в "неготово", для того чтобы различать устройства с обработанными разрывами
		void UnprocessDiscontinuity() noexcept { eInitStatus_ = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY; }

		// функция ремапа номера уравнения устройства в номер уравнения в Якоби
		inline ptrdiff_t A(ptrdiff_t nOffset) const
		{
			if (!AssignedToMatrix())
				throw dfw2error("CDevice::A - access to device not in matrix");
			return MatrixRow_ + nOffset;
		}

		// возвращает true если для устройства есть уравнения в системе
		inline bool AssignedToMatrix() const
		{
			return MatrixRow_ != IndexUnassigned;
		}
		virtual void InitNordsiek(CDynaModel* pDynaModel);
		virtual void Predict(const CDynaModel& DynaModel) {};
		virtual bool InMatrix() const noexcept;
		void EstimateEquations(CDynaModel* pDynaModel);
		virtual bool LeaveDiscontinuityMode(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS CheckProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		//! Вызывается после успешного выполнения шага для расчета зависимых переменных
		virtual void FinishStep(const CDynaModel& DynaModel);
		virtual eDEVICESTATE GetState() const { return State_; }
		bool IsStateOn() const { return GetState() == eDEVICESTATE::DS_ON; }
		bool IsPermanentOff() const { return GetState() == eDEVICESTATE::DS_OFF && GetStateCause() == eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT; }
		eDEVICESTATECAUSE GetStateCause() const { return StateCause_; }
		virtual eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice* pCauseDevice = nullptr);
		eDEVICEFUNCTIONSTATUS ChangeState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause);

		const char* VariableNameByPtr(double* pVariable) const;
		//! Выполняет zerocrossing
		virtual double CheckZeroCrossing(CDynaModel* pDynaModel);
		//! Сallback для девайса, который позволяет ему определить нужно ли делать zerocrossing для заданного примитива.
		/*! 
		Используется, например, в форсировке, чтобы не контролировать выход из форсировки пока не вышли на форсировку 
		*/
		virtual bool DetectZeroCrossingFine(const CDynaPrimitive* primitive) { return true; }
		//! Сохраняет состояния устройства и его примитивов
		virtual void StoreStates();
		//! Восстанавливает сохраненные состояния устройства и его примитивов
		virtual void RestoreStates();

		//! Возвращает сериализатор для данного типа устройств
		SerializerPtr GetSerializer();
		//! Возвращает валидатор для данного типа устройств
		SerializerValidatorRulesPtr GetValidator();
		//! Обновляет сериализатор для данного типа устройств
		virtual void UpdateSerializer(CSerializerBase* Serializer);
		//! Обновляет валидатор для данного типа устройств
		virtual void UpdateValidator(CSerializerValidatorRules* Validator);
		// shortcut добавляет в сериализатор свойство состояния
		void AddStateProperty(CSerializerBase* Serializer);

		static eDEVICEFUNCTIONSTATUS DeviceFunctionResult(eDEVICEFUNCTIONSTATUS Status1, eDEVICEFUNCTIONSTATUS Status2);
		static eDEVICEFUNCTIONSTATUS DeviceFunctionResult(eDEVICEFUNCTIONSTATUS Status1, bool Status2);
		static eDEVICEFUNCTIONSTATUS DeviceFunctionResult(bool Status1);

		// простая статическая округлялка до заданного Precision количества знаков
		inline static double Round(double Value, double Precision)
		{
			return round(Value / Precision) * Precision;
		}

		// обобщение до bool статуса выполнения функции устройства
		inline static bool IsFunctionStatusOK(eDEVICEFUNCTIONSTATUS Status)
		{
			// если выполнилось или не было нужно - все OK
			return Status == eDEVICEFUNCTIONSTATUS::DFS_OK || Status == eDEVICEFUNCTIONSTATUS::DFS_DONTNEED;
		}

		// функция "безопасного" деления
		inline static double ZeroDivGuard(double Nom, double Denom)
		{
			// если делитель - ноль, деление не выполняем
			return Consts::Equal(Denom,0.0) ? Nom : Nom / Denom;
		}

		template<typename T>
		inline static void FromComplex(T& Re, T& Im, const cplx& source)
		{
			Re = source.real();		Im = source.imag();
		}

		template<typename T>
		inline static void ToComplex(const T& Re, const T& Im, cplx& dest)
		{
			dest.real(Re);			
			dest.imag(Im);
		}

		static void FromComplex(double*& pB, const cplx& source)
		{
			*pB = source.real();	pB++;
			*pB = source.imag();	pB++;
		}

		void RegisterStatePrimitive(CDynaPrimitiveState *pPrimitive);
		void RegisterPrimitive(CDynaPrimitive *pPrimitive);

		template<typename T>
		static void CheckIndex(const T& Container, ptrdiff_t Index, const char* cszErrorMsg = nullptr)
		{
			if (Index < 0 || Index >= static_cast<ptrdiff_t>(Container.size()))
				throw dfw2error(fmt::format("{} - index check failed: index {} container size {}",
					cszErrorMsg ? cszErrorMsg : "CDevice::CheckIndex", Index, Container.size()));
		}

		bool CheckLimits(double& Min, double& Max);
		bool HasStatePrimitives() const { return !StatePrimitives_.empty(); }

#ifdef _DEBUG
		static char UnknownVarIndex[80];
#endif
		static const ptrdiff_t IndexUnassigned = (std::numeric_limits<ptrdiff_t>::max)();

		// Инкремент zero-crossing
		void IncrementZeroCrossings() { ZeroCrossings_++; }
		// Ввернуть накопленное значение zero-crossing
		ptrdiff_t GetZeroCrossings() const { return ZeroCrossings_; }

		// Инкремент запросов обработки разрывов
		void IncrementDiscontinuityRequests() { DiscontinuityRequests_++; }
		// Ввернуть накопленное значение zero-crossing
		ptrdiff_t GetDiscontinuityRequests() const { return DiscontinuityRequests_; }

		static constexpr const char* cszName_ = "Name";
		static constexpr const char* cszname_ = "name";
		static constexpr const char* cszSta_ = "sta";
		static constexpr const char* cszState_ = "State";
		static constexpr const char* cszstate_ = "state";
		static constexpr const char* cszStates_[4] = { "Off", "On", "Ready", "Determine", };
		static constexpr const std::array<const std::string_view, 3> StateAliases_ = { cszSta_, cszState_, cszstate_ };
	};

using DEVICEVECTOR = std::vector<CDevice*>;

// макрос для упрощения связи имени и идентификатора переменной, используется в switch CDevice::GetVariablePtr
#define MAP_VARIABLE(VarName, VarId)  case VarId: p = &VarName; break; 
#define MAP_VARIABLEINDEX(VarName, VarId)  case VarId: p = &VarName; break; 
}
