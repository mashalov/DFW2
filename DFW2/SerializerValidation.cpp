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

	for (const auto& ruleVariable : *m_Rules)
	{
		auto value = m_Serializer->at(ruleVariable.first);
		if (!value || value->bState) continue;
		for (const auto& rule : ruleVariable.second)
		{
			const auto res(rule->Validate(value));
			// если проверка успешна - не формируем описание переменной и продолжаем
			if (res == ValidationResult::Ok) continue;

			std::string verbalValue(fmt::format("\"{}\" : {} = {} {}", ClassName(), ruleVariable.first, value->String(), units.VerbalUnits(value->Units)));

			switch(res)
			{
			case ValidationResult::Error:
				Log(DFW2MessageStatus::DFW2LOG_ERROR, fmt::format("{} {}", verbalValue, rule->Message()));
				Status = eDEVICEFUNCTIONSTATUS::DFS_FAILED;
				break;
			case ValidationResult::Warning:
				Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("{} {}", verbalValue, rule->Message()));
				break;
			case ValidationResult::Replaced:
				Log(DFW2MessageStatus::DFW2LOG_WARNING, fmt::format("{} {}. {} {}",
					verbalValue,
					rule->Message(),
					CDFW2Messages::m_cszValidationChangedTo,
					rule->ReplaceValue()));
				break;
			}
		}
		
	}

	return Status;
}