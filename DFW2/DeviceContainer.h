#pragma once
#include "Device.h"
#include "DeviceContainerProperties.h"

namespace DFW2
{
	class CDynaModel;

	typedef vector<CDevice*> DEVICEVECTOR;
	typedef DEVICEVECTOR::iterator DEVICEVECTORITR;

	// для поиска устройств по идентификаторам используем сет
	typedef set<CDeviceId*, CDeviceIdComparator> DEVSEARCHSET;
	typedef DEVSEARCHSET::iterator DEVSEARCHSETITR;
	typedef DEVSEARCHSET::const_iterator DEVSEARCHSETCONSTITR;
	// для хранения уникальных указателей
	typedef set<CDevice*> DEVICEPTRSET;
	typedef DEVICEPTRSET::iterator DEVICEPTRSETITR;

	class CDeviceContainer;


	// контейнер для связей устройства
	class CMultiLink
	{
	public:
		CDevice  **m_ppPointers;											// вектор указателей на связанные устройства
		CDeviceContainer *m_pContainer;										// внешний контейнер, с устройствами которого строится связь
		CLinkPtrCount *m_pLinkInfo;											// вектор ссылок с количеством связей
		size_t	 m_nSize;													// количество связей 
		size_t   m_nCount;													// количество возможных связей (размерность m_ppPointers)
		CMultiLink(CDeviceContainer* pContainer, size_t nCount);
		bool Join(CMultiLink *pLink);
		virtual ~CMultiLink()
		{
			delete m_ppPointers;
			delete m_pLinkInfo;
		}
	};

	// вектор "векторов" связей с устройствами различных типов
	typedef vector<CMultiLink*> LINKSVEC;
	typedef LINKSVEC::iterator LINKSVECITR;

	// контейнер устройств
	// Идея контейнера в том, чтобы исключить из экземпляров устройств общую для них информацию:
	// тип, опции линковки, опции расчета, названия переменных и т.д.
	// можно считать, что это некий static в описании устройств, который мы не хотим наследовать,
	// а хотим формировать индивидуально для каждого из типов устройств
	class CDeviceContainer
	{
	protected:
		eDEVICEFUNCTIONSTATUS m_eDeviceFunctionStatus;
		DEVICEVECTOR m_DevVec;												// вектор указателей на экземпляры хранимых в контейнере устройств
		DEVSEARCHSET m_DevSet;												// сет для поиска устройств по идентификаторам
		bool SetUpSearch();													// подготовка к поиску устройства в сете по идентификаторам
		CDevice *m_pControlledData;											// вектор указателей созданных устройств для быстрого заполнения контейнера
		CDevice **m_ppSingleLinks;
		void CleanUp();														// очистка контейнера
		CDynaModel *m_pDynaModel;											// через указатель на модель контейнеры и устройства обмениваются общими данными
		void PrepareSingleLinks();

	public:

		CDeviceContainerProperties m_ContainerProps;						// описание статических атрибутов контейнера: тип и связи с другими устройствами
		ptrdiff_t EquationsCount();											// количество уравнений одного устройства в данном контейнере

		// тип устройства хранится в атрибутах контейнера. Устройство вне контейнера не знает своего типа
		inline eDFW2DEVICETYPE GetType() { return m_ContainerProps.eDeviceType; }
		// текстовое описание типа устройства
		const _TCHAR* GetTypeName() { return m_ContainerProps.m_strClassName.c_str(); }

		CDeviceContainer(CDynaModel *pDynaModel);
		virtual ~CDeviceContainer();

		CDevice **m_ppDevicesAux;
		size_t   m_nVisitedCount;

		// передает контейнеру под управление линейный массив указателей с созданными в нем экземплярами
		// устройств
		template<typename T> bool AddDevices(T* pDevice, size_t nCount)
		{
			// очистка предыдущего содержимого контейнера
			CleanUp();
			// фиксация внешнего указателя из аргумента
			// устройства из этого массива будут по указателям перенесены в контейнер
			// при очистке контейнера данный массив будет обработан delete []
			m_pControlledData = pDevice;
			T* p = pDevice;
			bool bRes = true;
			m_DevVec.reserve(m_DevVec.size() + nCount);
			// добавление устройств в контейнер
			for (size_t i = 0; i < nCount; i++)
				bRes = AddDevice(p++) && bRes;

			return bRes;
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
		bool RegisterVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eVARUNITS eVarUnits);
		bool RegisterConstVariable(const _TCHAR* cszVarName, ptrdiff_t nVarIndex, eDEVICEVARIABLETYPE eDevVarType);
		// управление выводом переменной в результаты
		bool VariableOutputEnable(const _TCHAR* cszVarName, bool bOutputEnable);

		// диапазон карты переменных состояния
		VARINDEXMAPCONSTITR VariablesBegin();
		VARINDEXMAPCONSTITR VariablesEnd();

		// диапазон карты констант
		CONSTVARINDEXMAPCONSTITR ConstVariablesBegin();
		CONSTVARINDEXMAPCONSTITR ConstVariablesEnd();

		
		ptrdiff_t GetVariableIndex(const _TCHAR* cszVarName)	  const;	// получить индекс переменной состояния по имени
		ptrdiff_t GetConstVariableIndex(const _TCHAR* cszVarName) const;	// получить индекс константы состояния по имени
		CDevice* GetDeviceByIndex(ptrdiff_t nIndex);						// получить устройство по индексу
		CDevice* GetDevice(CDeviceId* pDeviceId);							// получить устройство по базовому идентификатору
		CDevice* GetDevice(ptrdiff_t nId);									// получить устройство по идентификатору
		bool AddDevice(CDevice* pDevice);									// добавить устройство в контейнер
		bool RemoveDevice(ptrdiff_t nId);									// удалить устройство по идентификатору или индексу
		bool RemoveDeviceByIndex(ptrdiff_t nIndex); 
		size_t Count();														// получить количество устройств в контейнере
		inline DEVICEVECTORITR begin() { return m_DevVec.begin(); }			// диапазон вектора устройств
		inline DEVICEVECTORITR end() { return m_DevVec.end(); }
		void Log(CDFW2Messages::DFW2MessageStatus Status, const _TCHAR* cszMessage, ptrdiff_t nDBIndex = -1);

		LINKSVEC m_Links;													// вектор возможных связей. Элемент вектора - связь с определенным типом устройств
		bool IsKindOfType(eDFW2DEVICETYPE eType);
		bool CreateLink(CDeviceContainer* pLinkContainer);
		ptrdiff_t GetLinkIndex(CDeviceContainer* pLinkContainer);			// получить индекс ссылок по внешнему контейнеру
		ptrdiff_t GetLinkIndex(eDFW2DEVICETYPE eDeviceType);				// получить индекс ссылок по типу внешнего устройства
		bool IncrementLinkCounter(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		bool AllocateLinks(ptrdiff_t nLinkIndex);
		bool AddLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex, CDevice* pDevice);
		CMultiLink* GetCheckLink(ptrdiff_t nLinkIndex, ptrdiff_t nDeviceIndex);
		bool RestoreLinks(ptrdiff_t nLinkIndex);
		bool EstimateBlock(CDynaModel *pDynaModel);							// подсчитать количество уравнений устройств и привязать устройства к строкам Якоби
		bool BuildBlock(CDynaModel* pDynaModel);							// построить блок уравнений устройств в Якоби
		bool BuildRightHand(CDynaModel* pDynaModel);						// рассчитать правую часть уравнений устройств
		bool BuildDerivatives(CDynaModel* pDynaModel);						// рассчитать производные дифуров устройств
		bool NewtonUpdateBlock(CDynaModel* pDynaModel);						// обновить данные в устройствах после итерации Ньютона (для тех устройств, которым нужно)
		bool LeaveDiscontinuityMode(CDynaModel *pDynaModel);				// выйти из режима обработки разрыва
		eDEVICEFUNCTIONSTATUS ProcessDiscontinuity(CDynaModel *pDynaModel);	// обработать разрыв
		void UnprocessDiscontinuity();
		double CheckZeroCrossing(CDynaModel *pDynaModel);					// проверить zerocrossing и вернуть долю текущего шага до zerocrossing
		eDEVICEFUNCTIONSTATUS Init(CDynaModel* pDynaModel);					// инициализировать устройства
		CDynaModel* GetModel() { return m_pDynaModel;  }
		bool PushVarSearchStack(CDevice*pDevice);
		bool PopVarSearchStack(CDevice* &pDevice);
		void ResetStack();
		virtual ptrdiff_t GetPossibleSingleLinksCount();
		CDeviceContainer* DetectLinks(CDeviceContainer* pExtContainer, LinkDirectionTo& LinkTo, LinkDirectionFrom& LinkFrom);
		size_t GetResultVariablesCount();									// получить количество переменных, которое нужно выводить в результаты
		bool HasAlias(const _TCHAR *cszAlias);								// соответствует ли тип устройства заданному псевдониму
		ptrdiff_t GetSingleLinkIndex(eDFW2DEVICETYPE eDevType);				// получить индекс ссылки один-к-одному по типу устройства
	};

	typedef vector<CDeviceContainer*> DEVICECONTAINERS;
	typedef DEVICECONTAINERS::iterator DEVICECONTAINERITR;
};

