﻿#pragma once
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
	class CDynaPrimitive;
	class CDynaPrimitiveState;

	typedef list<CDynaPrimitiveState*> STATEPRIMITIVESLIST;
	typedef STATEPRIMITIVESLIST::iterator STATEPRIMITIVEITERATOR;
	typedef vector<CDynaPrimitive*> PRIMITIVESVEC;
	typedef PRIMITIVESVEC::iterator PRIMITIVEITERATOR;

	// класс для хранения связей устройства
	// с помощью него можно обходить связанные с данным устройства
	class CLinkPtrCount
	{
	public:
		CDevice  **m_pPointer;		// вектор указателей на связанные устройства
		size_t	 m_nCount;			// количество связанных устройств
		CLinkPtrCount() : m_nCount(0) {}
		bool In(CDevice ** & p);	// последовательное получение очередного связанного устройства из вектора
	};

	// элемент для хранения/передачи списка связанных устройств одного типа
	struct SingleLinksRange
	{
		CDevice **m_ppLinkStart;		// начало и конец списка устройств
		CDevice **m_ppLinkEnd;
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
		void SetRange(SingleLinksRange& LinkRange)
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
				_ASSERTE(!_T("SingleLinkRange::SetLink() Wrong link index"));
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

	// правая часть уравнения
	struct RightVector
	{
		double *pValue;													// значение переменной состояния
		CDevice *pDevice;												// устройство, в котором эта переменная
		double Nordsiek[3];												// вектор Nordsieck по переменной до 2 порядка
		double b;														// ошибка на итерации Ньютона
		double Error;													// ошибка корректора
		double Atol;													// абсолютная и относительная погрешности
		double Rtol;													// задаются индивидуально для каждой переменной
		DEVICE_EQUATION_TYPE EquationType;								// тип уравнения переменной для выбора коэффициентов метода (BDF - алгебраический, ADAM - дифференциальный)
		double SavedNordsiek[3];										// сохраненные перед шагом Nordsieck и ошибка 
		double SavedError;												// предыдущего шага
		double Tminus2Value;											// значение на пред-предыдыущем шаге для реинита Nordsieck
		DEVICE_EQUATION_TYPE PhysicalEquationType;						// физический тип уравнения - дифференциальный или алгебраический
		PrimitiveBlockType PrimitiveBlock;								// тип блока примитива если есть
		ptrdiff_t nErrorHits;											// количество ограничений шага или завалов итераций Ньютона по этой переменной
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

	// статусы выполнения функций устройства
	enum eDEVICEFUNCTIONSTATUS
	{
		DFS_OK,							// OK
		DFS_NOTREADY,					// Надо повторить (есть какая-то очередность обработки устройств или итерационный процесс)
		DFS_DONTNEED,					// Функция для данного устройства не нужна
		DFS_FAILED						// Ошибка
	};

	// статусы состояния устройства
	enum eDEVICESTATE
	{
		DS_OFF,							// полностью отключено
		DS_ON,							// включено
		DS_READY,						// готово (не используется ?)
		DS_DETERMINE,					// должно быть определено (мастер-устройством, например)
		DS_ABSENT						// должно быть удалено из модели
	};

	// причина изменения состояния устройства
	enum eDEVICESTATECAUSE
	{
		DSC_EXTERNAL,					// состояние изменено снаружи устройство
		DSC_INTERNAL					// состояние изменено действием самого устройства
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
		bool InitExternalVariable(PrimitiveVariableExternal& ExtVar, CDevice* pFromDevice, const _TCHAR* cszName, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);
		bool InitConstantVariable(double& ConstVar, CDevice* pFromDevice, const _TCHAR* cszName, eDFW2DEVICETYPE eLimitDeviceType = DEVTYPE_UNKNOWN);

		const CSingleLink& GetSingleLinks() { return m_DeviceLinks; }

		// формирование подробного имени устройства. По умолчанию учитывается описание типа устройства
		virtual void UpdateVerbalName();
		typedef eDEVICEFUNCTIONSTATUS(CheckMasterDeviceFunction)(CDevice*, LinkDirectionFrom&);
		static CheckMasterDeviceFunction CheckMasterDeviceInit;
		static CheckMasterDeviceFunction CheckMasterDeviceDiscontinuity;
		eDEVICEFUNCTIONSTATUS MastersReady(CheckMasterDeviceFunction* pFnCheckMasterDevice);

		void DumpIntegrationStep(ptrdiff_t nId, ptrdiff_t nStepNumber);
	public:
		CDevice();

		eDFW2DEVICETYPE GetType() const;							// получить тип устройства
		bool IsKindOfType(eDFW2DEVICETYPE eType);					// проверить, входит ли устройство в цепочку наследования от заданного типа устройства

		void Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage);

		// функция маппинга указателя на переменную к индексу переменной
		// Должна быть перекрыта во всех устройствах, которые наследованы от CDevice
		// внутри этой функции также делается "наследование" переменных
		virtual double* GetVariablePtr(ptrdiff_t nVarIndex);

		double* GetVariablePtr(const _TCHAR* cszVarName);

		// функция маппинга указателя на переменную к индексу переменной
		// Аналогична по смыслу virtual double* GetVariablePtr()
		virtual double* GetConstVariablePtr(ptrdiff_t nVarIndex);
		double* GetConstVariablePtr(const _TCHAR* cszVarName);

		virtual ExternalVariable GetExternalVariable(const _TCHAR* cszVarName);

		// константные указатели на переменную. Врапперы virtual double* GetVariablePtr()
		const double* GetVariableConstPtr(ptrdiff_t nVarIndex) const;
		const double* GetVariableConstPtr(const _TCHAR* cszVarName) const;

		// константные указатели на переменную константы. Врапперы virtual double* GetConstVariablePtr()
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
		void UnprocessDiscontinuity() { m_eInitStatus = DFS_NOTREADY;  }

		// функция ремапа номера уравнения устройства в номер уравнения в Якоби
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
		virtual double CheckZeroCrossing(CDynaModel *pDynaModel);

		virtual void StoreStates();
		virtual void RestoreStates();


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
			return Status == DFS_OK || Status == DFS_DONTNEED;
		}

		// функция "безопасного" деления
		inline static double ZeroDivGuard(double Nom, double Denom)
		{
			// если делитель - ноль, деление не выполняем
			return (fabs(Denom) < DFW2_EPSILON) ? Nom : Nom / Denom;
		}

		void RegisterStatePrimitive(CDynaPrimitiveState *pPrimitive);
		void RegisterPrimitive(CDynaPrimitive *pPrimitive);


#ifdef _DEBUG
		static _TCHAR UnknownVarIndex[80];
#endif

	};

// макрос для упрощения связи имени и идентификатора переменной, используется в switch CDevice::GetVariablePtr
#define MAP_VARIABLE(VarName, VarId)  case VarId: p = &VarName; break; 
}
