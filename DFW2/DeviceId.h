#pragma once
#include "Header.h"
#include "DeviceTypes.h"

namespace DFW2
{
	//! Класс для идентификации устройств. Является базовым для всех устройств
	class CDeviceId
	{
	protected:
		ptrdiff_t Id_ = -1;					// идентификатор (для многоидентификаторных, типа ветвей - только индекс)
		std::string Name_;					// имя устройства без подробностей. Типа имя узла
		ptrdiff_t DBIndex_ = -1;			// индекс устройства во внешней БД
		std::string VerbalName_;			// имя устройства с подробностями - с типом и т.д.
		virtual void UpdateVerbalName();		// функция обновления подробного имени устройства, при установке типа и т.д.
	public:
		CDeviceId();
		explicit CDeviceId(ptrdiff_t Id);
		virtual ~CDeviceId() {}
		ptrdiff_t GetId() const;				//! получить идентификатор
		void SetId(ptrdiff_t Id);				//! задать идентификатор
		const char* GetName() const;			//! получить имя без подробностей
		void SetName(std::string_view Name);	//! задать имя без подробностей
		const char* GetVerbalName() const;		//! получить имя с подробностями
		ptrdiff_t GetDBIndex() const;			//! получить индекс в БД
		void SetDBIndex(ptrdiff_t nIndex);		//! задать индекс в БД

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
