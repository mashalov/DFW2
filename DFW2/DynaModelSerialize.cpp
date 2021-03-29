﻿#include "stdafx.h"
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
	for (auto&& container : m_DeviceContainers)
		jsonSerializer.AddDeviceTypeDescription(container->GetType(), container->m_ContainerProps.GetSystemClassName());

	// обходим контейнеры снова
	for (auto&& container : m_DeviceContainers)
	{
		if (!container->Count()) continue;
		jsonSerializer.SerializeClass(container->GetDeviceByIndex(0)->GetSerializer());
	}
	// завершаем сериализацию
	jsonSerializer.Commit();
}

void CDynaModel::DeSerialize(const std::filesystem::path path)
{
	try
	{
		Log(CDFW2Messages::DFW2LOG_INFO, fmt::format(CDFW2Messages::m_cszLoadingJson, path.generic_string()));

		// создаем json-сериализатор
		CSerializerJson jsonSerializer;
		jsonSerializer.CreateNewSerialization(path);
		std::ifstream js;
		js.exceptions(js.exceptions() | std::ios::failbit);
		js.open(path);

		// делаем первый проход - считаем сколько чего мы можем взять из json
		auto saxCounter = std::make_unique<JsonSaxCounter>();
		nlohmann::json::sax_parse(js, saxCounter.get());
		//saxCounter->Dump();

		// после первого прохода в saxCounter количество объекто для каждого найденного
		// в json контейнера
		// создаем сериалиазатор для второго прохода
		// и добавляем в него сериализаторы для каждого из контейнеров,
		// в которых первый проход нашел данные
		auto saxSerializer = std::make_unique<JsonSaxSerializer>();

		for (const auto& [objkey, objsize] : saxCounter->GetObjectSizeMap())
		{
			// если описатель контейнера есть, а данных нет, то второй проход 
			// по нему не делаем
			if (objsize == 0) continue;

			// находим контейнер в модели
			if (auto pContainer(GetContainerByAlias(objkey)); pContainer)
			{
				Log(CDFW2Messages::DFW2LOG_DEBUG, fmt::format(CDFW2Messages::m_cszFoundContainerData, objkey, objsize));
				// создаем заданное в json количество объектов
				pContainer->CreateDevices(objsize);
				// достаем из первого созданного устройства контейнера сериализатор
				auto&& serializer = pContainer->GetDeviceByIndex(0)->GetSerializer();
				// и отдаем сериализатор контейнера сериализатору json
				saxSerializer->AddSerializer(objkey, serializer);
			}
		}

		// перематываем файл в начало для второго прохода
		js.clear(); js.seekg(0);
		// делаем второй проход с реальным чтением данных
		nlohmann::json::sax_parse(js, saxSerializer.get());
	}
	catch (nlohmann::json::exception& e)
	{
		throw dfw2error(fmt::format(CDFW2Messages::m_cszJsonParserError, e.what()));
	}
	catch (std::ofstream::failure&)
	{
		throw dfw2errorGLE(fmt::format(CDFW2Messages::m_cszStdFileStreamError, path.generic_string()));
	}
}
