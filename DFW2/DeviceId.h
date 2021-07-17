#pragma once
#include "Header.h"
#include "DeviceTypes.h"

namespace DFW2
{
	// класс для идентификации устройств. Является базовым для всех устройств
	class CDeviceId
	{
	protected:
		ptrdiff_t m_Id = -1;					// идентификатор (для многоидентификаторных, типа ветвей - только индекс)
		std::string m_strName;					// имя устройства без подробностей. Типа имя узла
		ptrdiff_t m_DBIndex = -1;				// индекс устройства во внешней БД
		std::string m_strVerbalName;			// имя устройства с подробностями - с типом и т.д.
		virtual void UpdateVerbalName();		// функция обновления подробного имени устройства, при установке типа и т.д.
	public:
		CDeviceId();
		CDeviceId(ptrdiff_t nId);
		virtual ~CDeviceId() {}
		ptrdiff_t GetId() const;				// получить идентификатор
		void SetId(ptrdiff_t nId);				// задать идентификатор
		const char* GetName() const;			// получить имя без подробностей
		void SetName(std::string_view Name);	// задать имя без подробностей
		const char* GetVerbalName() const;		// получить имя с подробностями
		ptrdiff_t GetDBIndex() const;			// получить индекс в БД
		void SetDBIndex(ptrdiff_t nIndex);		// задать индекс в БД

		static constexpr const char* m_cszid = "id";
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
