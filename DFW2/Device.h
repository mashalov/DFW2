#pragma once
#include "DeviceId.h"
#include "Serializer.h"

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

	// класс для хранения связей устройства
	// с помощью него можно обходить связанные с данным устройства
	class CLinkPtrCount
	{
	public:
		CDevice  **m_pPointer = nullptr;		// вектор указателей на связанные устройства
		size_t	 m_nCount = 0;					// количество связанных устройств
		bool In(CDevice ** & p) const;			// последовательное получение очередного связанного устройства из вектора
		bool InMatrix(CDevice**& p);			// последовательное получение очередного связанного устройства _включенного_в_матрицу
	};

	// элемент для хранения/передачи списка связанных устройств одного типа
	struct SingleLinksRange
	{
		CDevice **m_ppLinkStart = nullptr;		// начало и конец списка устройств
		CDevice **m_ppLinkEnd   = nullptr;
		SingleLinksRange() {}
		// конструктор с заданием вектора начала и конца
		SingleLinksRange(CDevice **ppStart, CDevice **ppEnd) : m_ppLinkStart(ppStart), m_ppLinkEnd(ppEnd)
		{
			_ASSERTE(m_ppLinkStart <= m_ppLinkEnd);
		}
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
		SingleLinksRange m_LinkRange;		// список связей
	public:
		void SetRange(SingleLinksRange&& LinkRange)
		{
			m_LinkRange = LinkRange;
		}
		const SingleLinksRange& GetLinksRange() const
		{
			return m_LinkRange;
		}

		// задать связь с индексом
		bool SetLink(ptrdiff_t nIndex, CDevice *pDevice) const
		{
			// проверка на корректность индекса в отладке и в релизе
			// потому что вызовов в процессе расчета нет
			if (nIndex >= 0 && nIndex < m_LinkRange.m_ppLinkEnd - m_LinkRange.m_ppLinkStart)
			{
				*(m_LinkRange.m_ppLinkStart + nIndex) = pDevice;
				return true;
			}
			return false;
		}

		// получить связь с индексом
		CDevice* GetLink(ptrdiff_t nIndex) const
		{
#ifdef _DEBUG
			if (nIndex >= 0 && nIndex < m_LinkRange.m_ppLinkEnd - m_LinkRange.m_ppLinkStart)
			{
#endif
				// проверка на корректность индекса только в отладке для производительности
				return *(m_LinkRange.m_ppLinkStart + nIndex);
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
	enum ACTIVE_POWER_DAMPING_TYPE
	{
		APDT_NODE,			// по скольжению узла
		APDT_ISLAND			// по скольжению средневзвешенного узла синхронной зоны
	};

	// способ подавления рингинга метода Адамса 2-го порядка
	enum ADAMS_RINGING_SUPPRESSION_MODE
	{
		ARSM_NONE,			// подавление не используется
		ARSM_GLOBAL,		// подавление для всех дифференциальных уравнений по кратности шага
		ARSM_INDIVIDUAL,	// подавление только для тех дифференциальных переменных, по которым обнаружен рингинг
		ARSM_DAMPALPHA		// подавление путем коррекции коэффициентов Адамса
	};

	struct RightVectorTotal
	{
		double *pValue;													// значение переменной состояния
		CDevice *pDevice;												// устройство, в котором эта переменная
		double Atol;													// абсолютная и относительная погрешности
		double Rtol;													// задаются индивидуально для каждой переменной
		ptrdiff_t nErrorHits;											// количество ограничений шага или завалов итераций Ньютона по этой переменной
		DEVICE_EQUATION_TYPE EquationType;								// тип уравнения переменной для выбора коэффициентов метода (BDF - алгебраический, ADAM - дифференциальный)
		DEVICE_EQUATION_TYPE PhysicalEquationType;						// физический тип уравнения - дифференциальный или алгебраический
	};

	// правая часть уравнения
	struct RightVector : RightVectorTotal
	{
		double Nordsiek[3];												// вектор Nordsieck по переменной до 2 порядка
		double b;														// ошибка на итерации Ньютона
		double Error;													// ошибка корректора
		double SavedNordsiek[3];										// сохраненные перед шагом Nordsieck и ошибка 
		double SavedError;												// предыдущего шага
		double Tminus2Value;											// значение на пред-предыдыущем шаге для реинита Nordsieck
		PrimitiveBlockType PrimitiveBlock;								// тип блока примитива если есть
		ptrdiff_t nRingsCount;
		ptrdiff_t nRingsSuppress;

		// расчет взвешенной ошибки по значению снаружи
		// но с допустимыми погрешностями для этой переменной
		double GetWeightedError(double dError, double dAbsValue)
		{
			_ASSERTE(Atol > 0.0);
			return dError / (fabs(dAbsValue) * Rtol + Atol);
		}

		// расчет взвешенной ошибки по значению данной переменной
		double GetWeightedError(double dAbsValue)
		{
			_ASSERTE(Atol > 0.0);
			return Error / (fabs(dAbsValue) * Rtol + Atol);
		}
	};

	// класс устройства, наследует все что связано с идентификацией
	class CDevice : public CDeviceId
	{
	protected:
		CDeviceContainer *m_pContainer;										// контейнер устройства
		CSingleLink m_DeviceLinks;											// связи устройств один к одному
		eDEVICEFUNCTIONSTATUS m_eInitStatus;								// статус инициализации устройства (заполняется в Init)
		virtual eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);			// инициализация устройства
		ptrdiff_t m_nMatrixRow;												// строка в матрице с которой начинаются уравнения устройства
		eDEVICESTATE m_State;												// состояние устройства
		eDEVICESTATECAUSE m_StateCause;										// причина изменения состояния устройства
		STATEPRIMITIVESLIST m_StatePrimitives;
		PRIMITIVESVEC m_Primitives;
		bool InitExternalVariable(VariableIndexExternal& ExtVar, CDevice* pFromDevice, std::string_view Name, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);
		bool InitConstantVariable(double& ConstVar, CDevice* pFromDevice, std::string_view Name, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);
		const CSingleLink& GetSingleLinks() { return m_DeviceLinks; }

		// формирование подробного имени устройства. По умолчанию учитывается описание типа устройства
		void UpdateVerbalName() override;
		typedef eDEVICEFUNCTIONSTATUS(CheckMasterDeviceFunction)(CDevice*, LinkDirectionFrom const *);
		static CheckMasterDeviceFunction CheckMasterDeviceInit;
		static CheckMasterDeviceFunction CheckMasterDeviceDiscontinuity;
		eDEVICEFUNCTIONSTATUS MastersReady(CheckMasterDeviceFunction* pFnCheckMasterDevice);

		void DumpIntegrationStep(ptrdiff_t nId, ptrdiff_t nStepNumber);
	public:
		CDevice();

		eDFW2DEVICETYPE GetType() const;							// получить тип устройства
		bool IsKindOfType(eDFW2DEVICETYPE eType);					// проверить, входит ли устройство в цепочку наследования от заданного типа устройства

		void Log(CDFW2Messages::DFW2MessageStatus Status, std::string_view Message);

		// функция маппинга указателя на переменную к индексу переменной
		// Должна быть перекрыта во всех устройствах, которые наследованы от CDevice
		// внутри этой функции также делается "наследование" переменных
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);
		double* GetVariablePtr(std::string_view VarName);
		virtual VariableIndexRefVec& GetVariables(VariableIndexRefVec& ChildVec);
		VariableIndex& GetVariable(ptrdiff_t nVarIndex);
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

		bool SetSingleLink(ptrdiff_t nIndex, CDevice *pDevice);
		CDevice* GetSingleLink(ptrdiff_t nIndex);
		CDevice *GetSingleLink(eDFW2DEVICETYPE eDevType);
		void SetContainer(CDeviceContainer* pContainer);
		CDeviceContainer* GetContainer();
		virtual ~CDevice();
		virtual bool LinkToContainer(CDeviceContainer *pContainer, CDeviceContainer *pContLead, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom);
		void IncrementLinkCounter(ptrdiff_t nLinkIndex);
		ptrdiff_t m_nInContainerIndex;
		// получить связи устроства из слоя nLinkIndex
		CLinkPtrCount* GetLink(ptrdiff_t nLinkIndex);
		void ResetVisited();
		ptrdiff_t CheckAddVisited(CDevice *pDevice);
		void SetSingleLinkStart(CDevice **ppLinkStart);

		CDynaModel* GetModel();

		// построение блока уравнения в Якоби
		virtual bool BuildEquations(CDynaModel *pDynaModel);
		// построение правой части уравнений
		virtual bool BuildRightHand(CDynaModel *pDynaModel);
		// расчет значений производных дифференциальных уравнений
		virtual bool BuildDerivatives(CDynaModel *pDynaModel);
		// функция обновления данных после итерации Ньютона (если надо)
		virtual void NewtonUpdateEquation(CDynaModel *pDynaModel);
		eDEVICEFUNCTIONSTATUS CheckInit(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS Initialized() { return m_eInitStatus; }
		virtual eDEVICEFUNCTIONSTATUS UpdateExternalVariables(CDynaModel *pDynaModel);
		eDEVICEFUNCTIONSTATUS DiscontinuityProcessed() { return m_eInitStatus; }
		// ставит статус обработки в "неготово", для того чтобы различать устройства с обработанными разрывами
		void UnprocessDiscontinuity() { m_eInitStatus = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;  }

		// функция ремапа номера уравнения устройства в номер уравнения в Якоби
		inline ptrdiff_t A(ptrdiff_t nOffset) 
		{ 
			if (!AssignedToMatrix())
				throw dfw2error("CDevice::A - access to device not in matrix");
			return m_nMatrixRow + nOffset; 
		}

		// возвращает true если для устройства есть уравнения в системе
		inline bool AssignedToMatrix()
		{
			return m_nMatrixRow != nIndexUnassigned;
		}
		virtual void InitNordsiek(CDynaModel* pDynaModel);
		virtual void Predict() {};
		virtual bool InMatrix();
		void EstimateEquations(CDynaModel *pDynaModel);
		virtual bool LeaveDiscontinuityMode(CDynaModel* pDynaModel);
		eDEVICEFUNCTIONSTATUS CheckProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel* pDynaModel);
		virtual eDEVICESTATE GetState() const { return m_State; }
		bool IsStateOn() const { return GetState() == eDEVICESTATE::DS_ON;  }
		bool IsPermanentOff() const { return GetState() == eDEVICESTATE::DS_OFF && GetStateCause() == eDEVICESTATECAUSE::DSC_INTERNAL_PERMANENT; }
		eDEVICESTATECAUSE GetStateCause() const { return m_StateCause; }
		virtual eDEVICEFUNCTIONSTATUS SetState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause, CDevice *pCauseDevice = nullptr);
		eDEVICEFUNCTIONSTATUS ChangeState(eDEVICESTATE eState, eDEVICESTATECAUSE eStateCause);

		const char* VariableNameByPtr(double *pVariable);
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);

		virtual void StoreStates();
		virtual void RestoreStates();

		// возвращает сериализатор для данного типа устройств
		DeviceSerializerPtr GetSerializer();
		// обновляет сериализатор для данного типа устройств
		virtual void UpdateSerializer(DeviceSerializerPtr& Serializer);
		// shortcut добавляет в сериализатор свойство состояния
		void AddStateProperty(DeviceSerializerPtr& Serializer);

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
			return (fabs(Denom) < DFW2_EPSILON) ? Nom : Nom / Denom;
		}

		void RegisterStatePrimitive(CDynaPrimitiveState *pPrimitive);
		void RegisterPrimitive(CDynaPrimitive *pPrimitive);

		template<typename T>
		static void CheckIndex(const T& Container, ptrdiff_t nIndex, const char* cszErrorMsg = nullptr)
		{
			if (nIndex < 0 || nIndex >= static_cast<ptrdiff_t>(Container.size()))
				throw dfw2error(fmt::format("{} - index check failed: index {} container size {}",
					cszErrorMsg ? cszErrorMsg : "CDevice::CheckIndex", nIndex, Container.size()));
		}


#ifdef _DEBUG
		static char UnknownVarIndex[80];
#endif
		static const ptrdiff_t nIndexUnassigned = (std::numeric_limits<ptrdiff_t>::max)();
	};

using DEVICEVECTOR = std::vector<CDevice*>;
using DEVICEVECTORITR = DEVICEVECTOR::iterator;

// макрос для упрощения связи имени и идентификатора переменной, используется в switch CDevice::GetVariablePtr
#define MAP_VARIABLE(VarName, VarId)  case VarId: p = &VarName; break; 
#define MAP_VARIABLEINDEX(VarName, VarId)  case VarId: p = &VarName; break; 
}
