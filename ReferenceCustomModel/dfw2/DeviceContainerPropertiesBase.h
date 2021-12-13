#pragma once
#include "DataStructs.h"

namespace DFW2
{
	// атрибуты контейнера устройств
	// Атрибуты контейнера можно "наследовать" в рантайме - брать некий атрибут и изменять его для другого типа устройств,
	// а далее по нему специфицировать контейнер устройств
	class CDeviceContainerPropertiesBase
	{
	public:
		// добавить тип устройства, или тип, от которого устройство наследовано
		void SetType(eDFW2DEVICETYPE eDevType)
		{
			m_TypeInfoSet.insert(eDeviceType = eDevType);
		}
		void SetClassName(std::string_view VerbalName, std::string_view SystemName)
		{
			m_strClassName = VerbalName;
			m_strClassSysName = SystemName;
		}

		// добавить возможность связи от устройства
		// указываем тип устройства, режим связи и тип зависимости. Режим связи и тип зависимости указываются для внешнего устройства
		// вызов этой функции для контейнера означает, что контейнер должен принимать связи от заданного типа устройств, считая заднный тип устройства ведущим или подчиненным
		// данная функция кроме всего предыдущего принимает еще имя константы во внешнем устройстве, в которой задан номер из этого контейнера
		// связи контейнеров двух типов a и b должны быть взаимными - a.From b, b.To a, в зависимости от того, в каком контейнере есть константа идентификатора для связи
		// eDFW2DEVICEDEPENDENCY должны быть комплиментарны - Master - Slave.
		void AddLinkTo(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency, std::string_view strIdField)
		{
			if (m_LinksTo.find(eDevType) == m_LinksTo.end())
			{
				m_LinksTo.insert(std::make_pair(eDevType, LinkDirectionTo(eLinkMode, Dependency, m_LinksTo.size() + m_LinksFrom.size(), strIdField)));
				if (eLinkMode == DLM_SINGLE)
					nPossibleLinksCount++;
			}
		}

		// добавить возможность связи от устройства
		// указываем тип устройства, режим связи и тип зависимости. Режим связи и тип зависимости указываются для внешнего устройства
		void AddLinkFrom(eDFW2DEVICETYPE eDevType, eDFW2DEVICELINKMODE eLinkMode, eDFW2DEVICEDEPENDENCY Dependency)
		{
			if (m_LinksFrom.find(eDeviceType) == m_LinksFrom.end())
			{
				// если связи с данным типом устройства еще нет
				// добавляем ее
				m_LinksFrom.insert(std::make_pair(eDevType, LinkDirectionFrom(eLinkMode, Dependency, m_LinksTo.size() + m_LinksFrom.size())));
				if (eLinkMode == DLM_SINGLE)
					nPossibleLinksCount++;
			}
		}

		bool bNewtonUpdate = false;											// нужен ли контейнеру вызов NewtonUpdate. По умолчанию устройство не требует особой обработки итерации Ньютона 
		bool bCheckZeroCrossing = false;									// нужен ли контейнеру вызов ZeroCrossing
		bool bPredict = false;												// нужен ли контейнеру вызов предиктора
		bool bVolatile = false;												// устройства в контейнере могут создаваться и удаляться динамически во время расчета
		bool bFinishStep = false;											// нужен ли контейнеру расчет независимых переменных после завершения шага
		ptrdiff_t nPossibleLinksCount = 0;									// возможное для устройства в контейнере количество ссылок на другие устройства
		ptrdiff_t nEquationsCount = 0;										// количество уравнений устройства в контейнере
		eDFW2DEVICETYPE	eDeviceType = DEVTYPE_UNKNOWN;
		VARINDEXMAP m_VarMap;												// карта индексов перменных состояния
		CONSTVARINDEXMAP m_ConstVarMap;										// карта индексов констант
		EXTVARINDEXMAP m_ExtVarMap;											// карта индексов внешних переменных
		TYPEINFOSET m_TypeInfoSet;											// набор типов устройства
		LINKSFROMMAP m_LinksFrom;
		LINKSTOMAP  m_LinksTo;
		std::list<std::string> m_lstAliases;								// возможные псевдонимы типа устройства (типа "Node","node")
	protected:
		std::string m_strClassName;											// имя типа устройства
		std::string m_strClassSysName;										// системное имя имя типа устройства для сериализации
	};
}
