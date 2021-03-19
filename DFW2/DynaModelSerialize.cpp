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

	// создаем базовый сериализатор для параметров расчета
	SerializerPtr SerializerParameteres = std::make_unique<CSerializerBase>();
	m_Parameters.UpdateSerializer(SerializerParameteres);
	jsonSerializer.SerializeClass(SerializerParameteres);

	// создаем базовый сериализатор для глобальных переменных расчета
	// и сериализуем их аналогично параметрам расчета
	SerializerPtr SerializerStepControl = std::make_unique<CSerializerBase>();
	sc.UpdateSerializer(SerializerStepControl);
	jsonSerializer.SerializeClass(SerializerStepControl);

	// обходим контейнеры устройств и регистрируем перечисление типов устройств
	for (auto&& container : m_DeviceContainers)
		jsonSerializer.AddDeviceTypeDescription(container->GetType(), container->m_ContainerProps.GetSystemClassName());

	// обходим контейнеры снова
	for (auto&& container : m_DeviceContainers)
	{
		auto itb = container->begin();
		const auto ite = container->end();

		// если контейнер пустой - пропускаем
		if (itb == ite) continue;

		auto&& serializer = static_cast<CDevice*>(*container->begin())->GetSerializer();
		jsonSerializer.SerializeClass(serializer);
	}
	// завершаем сериализацию
	jsonSerializer.Commit();
}

void CDynaModel::DeSerialize(const std::filesystem::path path)
{
	// создаем json-сериализатор
	CSerializerJson jsonSerializer;
	jsonSerializer.CreateNewSerialization(path);

	std::ifstream js(path);
	if (js.is_open())
	{
		auto sax = std::make_unique<CJsonSax>();
		nlohmann::json::sax_parse(js, sax.get());

		js.clear();
		js.seekg(0);

		nlohmann::json json;
		js >> json;

		const auto& objects = json.at("powerSystemModel").at("data").at("objects").at("node");

		for (auto& [key, value] : objects.items()) {
			for (auto& [k, v] : value.items()) 
			{
				//std::string va(v.dump());
				//std::string ky(key);
			}
		}
	}


	


}
