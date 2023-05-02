#include "stdafx.h"
#include "DynaModel.h"
#include "klu.h"
#include "cs.h"
#include "SerializerJson.h"

using namespace DFW2;

// сериализация в json
void CDynaModel::Serialize(const std::filesystem::path path)
{
	// создаем json-сериализатор
	CSerializerJson jsonSerializer;
	jsonSerializer.CreateNewSerialization(path);

	jsonSerializer.SerializeClass(m_Parameters.GetSerializer());
	jsonSerializer.SerializeClass(sc.GetSerializer());

	// обходим контейнеры устройств и регистрируем перечисление типов устройств
	for (auto&& container : DeviceContainers_)
		jsonSerializer.AddDeviceTypeDescription(container->GetType(), container->GetSystemClassName());

	// обходим контейнеры снова
	for (auto&& container : DeviceContainers_)
		jsonSerializer.SerializeClass(container->GetSerializer());

	// сериализуем автоматику
	jsonSerializer.SerializeClass(Automatic().GetSerializer());

	// завершаем сериализацию
	jsonSerializer.Commit();
}


// читает дополнительную конфигурацию в виде json из рабочего каталога
// и заменяет параметры непосредственно перед расчетом

void CDynaModel::DeserializeParameters(const std::filesystem::path path)
{
	try
	{
		// если файл отсутствует - параметры не трогаем
		if (!std::filesystem::exists(path))
			return;

		// файл найден - сообщаем что принялись его читать
		Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLoadingParameters,
			CDynaModel::cszJson,
			stringutils::utf8_encode(path.c_str())));

		std::ifstream js;
		js.exceptions(js.exceptions() | std::ios::failbit);
		js.open(path);

		// получаем сериализатор параметров
		// и сохраняем указатель на него чтобы определить
		// какие параметры изменились
		auto serializer{ m_Parameters.GetSerializer() };
		const auto pSerializer{ serializer.get() };
		auto saxSerializer{ std::make_unique<JsonSaxParametersSerializer>() };
		const std::string Class{ serializer->GetClassName() };
		saxSerializer->AddSerializer(Class, std::move(serializer));
		nlohmann::json::sax_parse(js, saxSerializer.get());

		// получаем карту параметров, которые не изменились
		const auto UnsetMap{ pSerializer->GetUnsetValues() };
		
		// проходим по всем параметрам
		// если параметра нет в карте не изменившихся - выводим его значение
		for (const auto& var : *pSerializer)
			if (UnsetMap.find(var.first) == UnsetMap.end())
				Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(
					CDFW2Messages::m_cszExternalParameterAccepted, 
					var.first, 
					var.second->String()));
	}

	catch (nlohmann::json::exception& e)
	{
		throw dfw2error(fmt::format(CDFW2Messages::m_cszJsonParserError, e.what()));
	}
	catch (std::ofstream::failure&)
	{
		throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszStdFileStreamError,
			stringutils::utf8_encode(path.c_str())));
	}
}

void CDynaModel::DeSerialize(const std::filesystem::path path)
{
	try
	{
		Log(DFW2MessageStatus::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLoadingModelFormat, 
			CDynaModel::cszJson,
			stringutils::utf8_encode(path.c_str())));

		// создаем json-сериализатор
		CSerializerJson jsonSerializer;
		jsonSerializer.CreateNewSerialization(path);
		std::ifstream js;
		js.exceptions(js.exceptions() | std::ios::failbit);
		js.open(path);

		// делаем первый проход - считаем сколько чего мы можем взять из json
		auto acceptorCounter = std::make_unique<JsonSaxElementCounter>();
		nlohmann::json::sax_parse(js, acceptorCounter.get());
		// после первого прохода в saxCounter количество объектов для каждого найденного
		// в json контейнера
		// создаем сериализатор для второго прохода
		// и добавляем в него сериализаторы для каждого из контейнеров,
		// в которых первый проход нашел данные


		auto saxSerializer = std::make_unique<JsonSaxMainSerializer>();
		const auto& acceptorObjects(acceptorCounter->Objects());

		for (const auto& [objkey, objsize] : acceptorObjects)
		{
			// если описатель контейнера есть, а данных нет, то второй проход 
			// по нему не делаем
			if (objsize == 0) continue;

			// находим контейнер в модели
			if (auto pContainer(GetContainerByAlias(objkey)); pContainer)
			{
				Log(DFW2MessageStatus::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszFoundContainerData, objkey, pContainer->ContainerProps().GetVerbalClassName(), objsize));
				// создаем заданное в json количество объектов
				pContainer->CreateDevices(objsize);
				// и отдаем сериализатор контейнера сериализатору json
				saxSerializer->AddSerializer(objkey, pContainer->GetSerializer());
			}
		}

		// ищем акцепторы для параметров и управления шагом, если такие акцепторы есть,
		// добавляем сериализаторы
		std::array<SerializerPtr, 3> parameters{ m_Parameters.GetSerializer() , sc.GetSerializer(), Automatic().GetSerializer() };
		for (auto&& param : parameters)
		{
			if (const auto name(param->GetClassName()); acceptorObjects.find(name) != acceptorObjects.end())
				saxSerializer->AddSerializer(name, std::move(param));
		}

		// перематываем файл в начало для второго прохода
		js.clear(); js.seekg(0);
		// делаем второй проход с реальным чтением данных
		nlohmann::json::sax_parse(js, saxSerializer.get());

		SetModelName(stringutils::utf8_encode(path.filename().c_str()));
	}
	catch (nlohmann::json::exception& e)
	{
		throw dfw2error(fmt::format(CDFW2Messages::m_cszJsonParserError, e.what()));
	}
	catch (std::ofstream::failure&)
	{
		throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszStdFileStreamError, 
			stringutils::utf8_encode(path.c_str())));
	}
}
