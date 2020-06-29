#pragma once
#include "header.h"
#include "DeviceTypes.h"

namespace DFW2
{
	// класс для идентификации устройств. Является базовым для всех устройств
	class CDeviceId
	{
	protected:
		ptrdiff_t m_Id;							// идентификатор (для многоидентификаторных, типа ветвей - только индекс)
		std::wstring m_strName;					// имя устройства без подробностей. Типа имя узла
		ptrdiff_t m_DBIndex;					// индекс устройства во внешней БД
		std::wstring m_strVerbalName;			// имя устройства с подробностями - с типом и т.д.
		virtual void UpdateVerbalName();		// функция обновления подробного имени устройства, при установке типа и т.д.
	public:
		CDeviceId();
		CDeviceId(ptrdiff_t nId);
		virtual ~CDeviceId() {}
		ptrdiff_t GetId() const;				// получить идентификатор
		void SetId(ptrdiff_t nId);				// задать идентификатор
		const _TCHAR* GetName() const;			// получить имя без подробностей
		void SetName(std::wstring_view Name);	// задать имя без подробностей
		const _TCHAR* GetVerbalName() const;	// получить имя с подробностями
		ptrdiff_t GetDBIndex();					// получить индекс в БД
		void SetDBIndex(ptrdiff_t nIndex);		// задать индекс в БД
	};

	// оператор для сортировки и поиска устройств. Упорядочивает по GetId
	struct CDeviceIdComparator
	{
		bool operator()(const CDeviceId* lhs, const CDeviceId* rhs) const
		{
			return lhs->GetId() < rhs->GetId();
		}
	};
}
