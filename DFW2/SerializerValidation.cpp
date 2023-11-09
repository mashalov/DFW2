#include "stdafx.h"
#include "SerializerValidation.h"
#include "DynaModel.h"

using namespace DFW2;


const char* CSerializerValidator::ClassName() const
{
	if (auto device(m_Serializer->GetDevice()); device)
		return device->GetVerbalName();
	else
		return m_Serializer->GetClassName();
}

void CSerializerValidator::Log(DFW2MessageStatus Status, std::string_view Message)
{
	if (auto device(m_Serializer->GetDevice()); device)
		device->Log(Status, Message);
	else if (m_pDynaModel)
		m_pDynaModel->Log(Status, Message);
	else
		throw dfw2error("CSerializerValidator::Log - no logging backend available");
}


eDEVICEFUNCTIONSTATUS CSerializerValidator::Validate()
{
	auto Status(eDEVICEFUNCTIONSTATUS::DFS_OK);

	CDFW2Messages msg;
	const auto units(msg.VarNameMap());

	std::string message;

	do
	{
		for (const auto& ruleVariable : *m_Rules)
		{
			auto value{ m_Serializer->at(ruleVariable.first) };
			if (!value || value->bState) continue;
			for (const auto& rule : ruleVariable.second)
			{
				const double originalValue(value->Double());
				// последовательно вызываем 2 валидатора
				// 1 - от устройства (упрощенный)
				auto res{ rule->Validate(value, m_Serializer->GetDevice(), message) };
				// 2 - если первый отработал - вызываем второй
				// который позволяет получить доступ к сериализатору
		
				if (res == ValidationResult::Ok)
					res = rule->Validate(value, m_Serializer, message);
				if (res == ValidationResult::Ok)
					continue;

				// если проверка успешна - не формируем описание переменной и продолжаем
				std::string verbalValue(fmt::format("\"{}\" : {} = {} {}", ClassName(), ruleVariable.first, originalValue, units.VerbalUnits(value->Units)));

				switch (res)
				{
				case ValidationResult::Error:
					Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format("{} {}", verbalValue, message));
					Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
					break;
				case ValidationResult::Warning:
					Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("{} {}", verbalValue, message));
					break;
				case ValidationResult::Replaced:
					Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("{} {}. {}{}",
						verbalValue,
						message,
						CDFW2Messages::m_cszValidationChangedTo,
						rule->ReplaceValue()));
					break;
				}
			}

		}
	} while (m_Serializer->NextItem());

	return Status;
}