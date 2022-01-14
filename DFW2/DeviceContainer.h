#pragma once
#include "Device.h"
#include "DeviceContainerProperties.h"

namespace DFW2
{
	class CDynaModel;
	// для поиска устройств по идентификаторам используем сет
	using DEVSEARCHSET = std::set<CDeviceId*, CDeviceIdComparator> ;
	// для хранения уникальных указателей
	using DEVICEPTRSET = std::set<CDevice*> ;

	class CDeviceContainer;

	using DevicePtr = CDevice * [];
	using DevicesPtrs = std::unique_ptr<DevicePtr>;

	// контейнер для связей устройства
	class CMultiLink
	{
	public:
		DevicesPtrs  m_ppPointers;											// вектор указателей на связанные устройства
		CDeviceContainer *m_pContainer = nullptr;							// внешний контейнер, с устройствами которого строится связь
		std::vector<CLinkPtrCount> m_LinkInfo;								// вектор ссылок с количеством связей
		size_t   m_nCount = 0;												// количество возможных связей 
		CMultiLink(CDeviceContainer* pContainer, size_t nCount) : m_pContainer(pContainer)
		{
			// память выделим под известное количество связей в AllocateLinks()
			m_LinkInfo.resize(nCount);
		}
		// конструктор копирования нет. Для создания CMultiLink в контейнере нужно использовать emplace
		void Join(CMultiLink& pLink);

		inline CLinkPtrCount* GetLink(ptrdiff_t nDeviceInContainerIndex)
		{
			if (nDeviceInContainerIndex >= static_cast<ptrdiff_t>(m_LinkInfo.size()))
				throw dfw2error("CLinkPtrCount::GetLink - Device index out of range");
			return &m_LinkInfo[nDeviceInContainerIndex];
		}
	};

	// вектор "векторов" связей с устройствами различных типов
	using LINKSVEC = std::vector<CMultiLink>;

	// контейнер устройств
	// Идея контейнера в том, чтобы исключить из экземпляров устройств общую для них информацию:
	// тип, опции линковки, опции расчета, названия переменных и т.д.
	// можно считать, что это некий static в описании устройств, который мы не хотим наследовать,
	// а хотим формировать индивидуально для каждого из типов устройств
	class CDeviceContainer
	{
	protected:

		// тип управления данными в контейнере
		enum class ContainerMemoryManagementType
		{
			Unspecified,		// не инициализирован
			BySolid,			// вектор устройств в CDeviceProperties
			ByPieces			// указатели на отдельные устройства
		}
		m_MemoryManagement = ContainerMemoryManagementType::Unspecified;

		void SetMemoryManagement(ContainerMemoryManagementType ManagementType);

		eDEVICEFUNCTIONSTATUS m_eDeviceFunctionStatus = eDEVICEFUNCTIONSTATUS::DFS_NOTREADY;

		DEVICEVECTOR m_DevVec;												// вектор указателей на экземпляры хранимых в контейнере устройств
		DEVICEVECTOR m_DevInMatrix;											// вектор указателей на экземпляры устройств, у которых есть уравнения
		DEVSEARCHSET m_DevSet;												// сет для поиска устройств по идентификаторам
		bool SetUpSearch();													// подготовка к поиску устройства в сете по идентификаторам
		DevicesPtrs m_ppSingleLinks;										// вектор указателей на устройства с одиночными ссылками
		void CleanUp();														// очистка контейнера
		CDynaModel *m_pDynaModel;											// через указатель на модель контейнеры и устройства обмениваются общими данными
		void PrepareSingleLinks();
		CDevice *m_pClosestZeroCrossingDevice = nullptr;

		// привязать созданные устройства к контейнеру
		void SettleDevicesFromSolid()
		{
			ptrdiff_t nIndex(0);
			for (auto&& it : m_DevVec)
				SettleDevice(it, nIndex++);
		}
	public:

		// описание статических атрибутов контейнера: тип и связи с другими устройствами
		CDeviceContainerProperties m_ContainerProps;
		// количество уравнений одного устройства в данном контейнере
		// функция виртуальная, так как для ряда устройств количество переменных не постоянное
		// количество таких устройств в контейнере всегда одно. В свойствах контейнера 
		// должен стоять флаг bVolatile
		virtual ptrdiff_t EquationsCount() const;							

		// тип устройства хранится в атрибутах контейнера. Устройство вне контейнера не знает своего типа
		inline eDFW2DEVICETYPE GetType() const { return m_ContainerProps.GetType(); }
		// текстовое описание типа устройства
		const char* GetTypeName() const { return m_ContainerProps.GetVerbalClassName(); }

		CDeviceContainer(CDynaModel *pDynaModel);
		virtual ~CDeviceContainer();

		DevicesPtrs m_ppDevicesAux;
		size_t   m_nVisitedCount = 0;

		template<class T>
		T* CreateDevices(size_t nCount)
		{
			CleanUp();
			// ставим тип управления памятью
			SetMemoryManagement(ContainerMemoryManagementType::BySolid);
			// создаем фабрику устройств заданного типа
			auto factory = std::make_unique<CDeviceFactory<T>>();
			// создаем вектор устройств
			T* ptr = factory->CreateRet(nCount, m_DevVec);
			// оставляем фабрику устройств в свойствах устройства (??? может оставлять старую ?)
			m_ContainerProps.DeviceFactory = std::move(factory);
			// указатели на отдельные устройства теперь в m_DevVec, а
			// вектор устройств остался в фабрике под ее контролем
			// привязываем устройства к контейнеру
			SettleDevicesFromSolid();
			return ptr;
		}

		void CreateDevices(size_t nCount) 
		{
			CleanUp();
			// ставим тип управления памятью
			SetMemoryManagement(ContainerMemoryManagementType::BySolid);
			// проверяем есть ли встроенная фабрика устройств
			if (!m_ContainerProps.DeviceFactory)
				throw dfw2error(fmt::format("CDynaNodeContainer::CreateDevice - DeviceFactory not defined for \"{}\"", GetSystemClassName()));
			// создаем устройства с помощью фабрики
			m_ContainerProps.DeviceFactory->Create(nCount, m_DevVec);
			// и привязываем их к контейнеру
			SettleDevicesFromSolid();
		}

		// извлечение устройства из контейнера по идентификатору
		template<typename T> bool GetDevice(ptrdiff_t nId, T* &pDevice)
		{
			pDevice = static_cast<T*>(GetDevice(nId));
			return pDevice ? true : false;
		}
		// извлечение устройства по базовому классу идентификации
		template<typename T> bool GetDevice(CDeviceId* pDeviceId, T* &pDevice)
		{
			pDevice = static_cast<T*>(GetDevice(pDeviceId));
			return pDevice ? true : false;
		}
		// извлечение устройства по индексу
		template<typename T> bool GetDeviceByIndex(ptrdiff_t nIndex, T* &pDevice)
		{
			pDevice = static_cast<T*>(GetDeviceByIndex(nIndex));
			return pDevice ? true : false;
		}

		// добавление переменных состояния и констант устройств к атрибутам контейнера
		// константы и переменные состояния обрабатываются по разному, т.к. для констант нет уравнений
		// и они в процессе расчета не изменяются
		bool RegisterVariable(std::string_view VarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits);
		bool RegisterConstVariable(std::string_view VarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits, eDEVICEVARIABLETYPE eDevVarType);
		// управление выводом переменной в результаты
		bool VariableOutputEnable(std::string_view VarName, bool bOutputEnable);

		// диапазон карты переменных состояния
		inline VARINDEXMAP::const_iterator VariablesBegin() { return m_ContainerProps.m_VarMap.begin(); }
		inline VARINDEXMAP::const_iterator VariablesEnd() { return m_ContainerProps.m_VarMap.end(); }

		// диапазон карты констант
		inline CONSTVARINDEXMAP::const_iterator ConstVariablesBegin() { return m_ContainerProps.m_ConstVarMap.begin(); }
		inline CONSTVARINDEXMAP::const_iterator ConstVariablesEnd() { return m_ContainerProps.m_ConstVarMap.end(); }

		
		ptrdiff_t GetVariableIndex(std::string_view VarName)	  const;	// получить индекс переменной состояния по имени
		ptrdiff_t GetConstVariableIndex(std::string_view VarName) const;	// получить индекс константы состояния по имени
		CDevice* GetDeviceByIndex(ptrdiff_t nIndex);						// получить устройство по индексу
		CDevice* GetDevice(CDeviceId* pDeviceId);							// получить устройство по базовому идентификатору
		CDevice* GetDevice(ptrdiff_t nId);									// получить устройство по идентификатору
		void AddDevice(CDevice* pDevice);									// добавить устройство в контейнер
		void SettleDevice(CDevice *pDevice, ptrdiff_t nIndex);				// привязать устройство в контейнере
		void RemoveDevice(ptrdiff_t nId);									// удалить устройство по идентификатору или индексу
		void RemoveDeviceByIndex(ptrdiff_t nIndex); 
		size_t Count() const;												// получить количество устройств в контейнере
		size_t CountNonPermanentOff() const;								// получить количество устройств в конейнере без признака PermanentOff
		inline DEVICEVECTOR::iterator begin() { return m_DevVec.begin(); }			// диапазон вектора устройств
		inline DEVICEVECTOR::iterator end() { return m_DevVec.end(); }

		inline const DEVICEVECTOR::const_iterator begin() const { return m_DevVec.begin(); }			// const-диапазон вектора устройств
		inline const DEVICEVECTOR::const_iterator end() const   { return m_DevVec.end(); }

		void Log(DFW2MessageStatus Status, const std::string_view Message, ptrdiff_t nDBIndex = -1);

		LINKSVEC m_Links;													// вектор возможных связей. Элемент вектора - связь с определенным типом устройств
		bool IsKindOfType(eDFW2DEVICETYPE eType);
		bool CreateLink(CDeviceContainer* pLinkContainer);
		ptrdiff_t GetLinkIndex(CDeviceContainer* pLinkContainer);			// получить индекс ссылок по внешнему контейнеру
		ptrdiff_t GetLinkIndex(eDFW2DEVICETYPE eDeviceType);				// получить индекс ссылок по типу внешнего устройства
		void IncrementLinkCounter(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		void IncrementLinkCounter(CMultiLink& pLink, ptrdiff_t nDeviceIndex);
		void AllocateLinks(ptrdiff_t nLinkIndex);
		void AllocateLinks(CMultiLink& pLink);
		void AddLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex, CDevice* pDevice);
		void AddLink(CMultiLink& pLink, ptrdiff_t nDeviceIndex, CDevice* pDevice);
		void RestoreLinks(ptrdiff_t nLinkIndex);
		void RestoreLinks(CMultiLink& pLink);
		CMultiLink& GetCheckLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		CMultiLink& GetCheckLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex, LINKSVEC& LinksVec);
		bool CheckLink(ptrdiff_t nLinkIndex);
		bool CheckLink(ptrdiff_t nLinkIndex, LINKSVEC& LinksVec);
		void InitNordsieck(CDynaModel *pDynaModel);
		void Predict();
  		void EstimateBlock(CDynaModel *pDynaModel);							// подсчитать количество уравнений устройств и привязать устройства к строкам Якоби
		void BuildBlock(CDynaModel* pDynaModel);							// построить блок уравнений устройств в Якоби
		void BuildRightHand(CDynaModel* pDynaModel);						// рассчитать правую часть уравнений устройств
		void BuildDerivatives(CDynaModel* pDynaModel);						// рассчитать производные дифуров устройств
		void NewtonUpdateBlock(CDynaModel* pDynaModel);						// обновить данные в устройствах после итерации Ньютона (для тех устройств, которым нужно)
		bool LeaveDiscontinuityMode(CDynaModel *pDynaModel);				// выйти из режима обработки разрыва
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel *pDynaModel);	// обработать разрыв
		void UnprocessDiscontinuity();
		double CheckZeroCrossing(CDynaModel *pDynaModel);					// проверить zerocrossing и вернуть долю текущего шага до zerocrossing
		CDevice *GetZeroCrossingDevice();
		eDEVICEFUNCTIONSTATUS PreInit(CDynaModel* pDynaModel);				// предварительно инициализировать и проверить параметры устройств
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);					// инициализировать устройства
		void FinishStep();													// завершить шаг и рассчитать зависимые переменные
		CDynaModel* GetModel();
		void PushVarSearchStack(CDevice*pDevice);
		bool PopVarSearchStack(CDevice* &pDevice);
		void ResetStack();
		virtual ptrdiff_t GetPossibleSingleLinksCount();
		CDeviceContainer* DetectLinks(CDeviceContainer* pExtContainer, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom);
		size_t GetResultVariablesCount();									// получить количество переменных, которое нужно выводить в результаты
		bool HasAlias(std::string_view Alias) const;						// соответствует ли тип устройства заданному псевдониму
		const char* GetSystemClassName() const;
		ptrdiff_t GetSingleLinkIndex(eDFW2DEVICETYPE eDevType);				// получить индекс ссылки один-к-одному по типу устройства
		SerializerPtr GetSerializer();
	};

	using DEVICECONTAINERS = std::vector<CDeviceContainer*>;
};

