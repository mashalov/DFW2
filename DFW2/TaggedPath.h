#pragma once
#include <string>
#include <list>
#include <stack>
#include <stdexcept>
#include <functional>
#include <optional>
#include <fstream>
#include <charconv>
#include <filesystem>
#include "fmt/chrono.h"
#include "dfw2exception.h"
#include "Messages.h"

class TaggedString
{
	// функция обработки тега - принимает тег, возвращает пустой optional
	// если тег не обработан, или строку вместо тега
	using TagFunctionType = std::function<std::optional<std::string>(const std::string&)>;
public:
	TaggedString(const std::string_view& Path) : Path_(Path)
	{
		Parse();
	}
	// добавляет функцию обработки тега
	void AddFunction(const TagFunctionType& function)
	{
		TagFunctions_.push_back(function);
	}

	// формирует строку из разобранных тегов
	std::string Process()
	{
		std::stack<std::reference_wrapper<TagPositionType>> Stack;
		// обрабатываем вложенные теги с помощью стека
		// корневой тег от начала до конца исходной строки
		Root_.openpos_ = 0;
		Root_.closepos_ = Path_.length();
		Stack.push(Root_);

		// очищаем флаги от предыдущей обработки
		Stack.push(Root_);
		while (!Stack.empty())
		{
			auto& tag{ Stack.top().get() };
			tag.substitution_.clear();
			if (tag.ProcessedChildren_)
			{
				tag.ProcessedChildren_ = 0;
				for (auto&& ChildTag : tag.children_)
					Stack.push(ChildTag);
			}
			else
				Stack.pop();
		}

		Stack.push(Root_);

		while (!Stack.empty())
		{
			auto& tag{ Stack.top().get() };
			// если все вложенные теги были обработаны 
			if (tag.children_.size() == tag.ProcessedChildren_)
			{
				// формируем строку замещения тега
				size_t start{ tag.openpos_ };
				tag.substitution_.clear();
				// проходим по вложенным тегам
				for (const auto& childTag : tag.children_)
				{
					// добавляем исходную строку до вложенного тега
					tag.substitution_.append(Path_.substr(start, childTag.openpos_ - start));
					// добавляем замену вложенного тега
					tag.substitution_.append(childTag.substitution_);
					start = childTag.closepos_ + 1;
				}

				if (tag.closepos_ == std::string::npos)
						throw dfw2error(DFW2::CDFW2Messages::m_cszWrongTaggedPath, tag.openpos_);


				// добавляем остаток исходной строки после последенго вложенного тега
				tag.substitution_.append(Path_.substr(start, tag.closepos_ - start + 1));


				// обрабатываем тег набором функций
				for (const auto& fn : TagFunctions_)
				{
					// формируем строку для функции без угловых скобок и проверяем 
					// что она делает с такой строкой
					const auto fnOut{ (fn)(tag.substitution_.substr(1, tag.substitution_.length() - 2)) };
					if (fnOut.has_value())
					{
						// если функция что-то сделала и вернула строку - готово, больше
						// функций из списка не вызываем
						tag.substitution_ = fnOut.value();
						break;
					}
				}
				// тег обработан - вынимаем его из стека
				Stack.pop();
			}
			else
			{
				// есть необработанные вложенные теги
				for (auto&& ChildTag : tag.children_)
					if (ChildTag.substitution_.empty())
					{
						// если для вложенного тега не было строки замещения
						// помещаем его в стек и инкрементим счетчик ушедших
						// на обработку вложенных тегов
						Stack.push(ChildTag);
						tag.ProcessedChildren_++;
					}
			}
		}

		return Root_.substitution_;
	}
protected:
	// символ открывания тега 
	std::string::value_type OpenTag() const
	{
		return '<';
	}
	// символ закрывания тега
	std::string::value_type CloseTag() const
	{
		return '>';
	}
	// входной путь
	std::string Path_;

	// структура разметки тегов
	struct TagPositionType
	{
		size_t openpos_ = std::string::npos;		// позиция открытия тега
		size_t closepos_ = std::string::npos;		// позиция закрытия тега
		using TagList = std::list<TagPositionType>;	// спискок вложенных тегов
		std::string substitution_;					// строка тега после обработки включая вложенные теги
		TagList children_;
		size_t ProcessedChildren_ = 0;				// количество обработанных вложенных тегов для процессинга
	};

	TagPositionType Root_;							// корневой тег (весь Path_)
	std::list<TagFunctionType> TagFunctions_;		// список функций обработки тегов

	// разбор строки с тегами
	void Parse()
	{
		// стек вложенных тегов
		Root_ = {};
		std::stack<std::reference_wrapper<TagPositionType::TagList>> Stack;
		// инициализируем стек корневым элементом
		Stack.push(Root_.children_);

		// проходим по строке
		for (size_t pos{ 0 }; pos < Path_.size(); pos++)
		{
			const auto& symbol{ Path_[pos] };

			// работаем с со списком тегов наверху стека
			auto& Tags{ Stack.top().get() };

			if (symbol == OpenTag())					// если нашли открывание
			{
				if (Tags.empty())			
					Tags.push_back({ pos });			// если список тегов пустой - создаем новый тег
				else
				{
					auto& Tag{ Tags.back() };			// если теги уже есть, проверяем закрыт ли последний
					if (Tag.closepos_ != std::string::npos)
						Tags.push_back({ pos });		// если закрыт - создаем новый
					else
					{									
						// если открыт - мы открываем вложенный тег
						Tag.children_.push_back({ pos });	
						// уходим на новый уровень стека
						Stack.push(Tag.children_);
					}
				}
			}
			else if (symbol == CloseTag())				// если нашли закрывание
			{
				if (Tags.empty())						// если тегов не было открыто - ошибка
					throw dfw2error(DFW2::CDFW2Messages::m_cszWrongTaggedPath, pos);
				else
				{
					auto& Tag{ Tags.back() };			// если теги есть, закрываем последний, если он не был закрыт
					if (Tag.closepos_ == std::string::npos)
						Tag.closepos_ = pos;
					else
					{
						Stack.pop();					// но если последний тег был закрыт - нам нужно закрыть тег
						if(Stack.empty())				// нижнего уровня стека
							throw dfw2error(DFW2::CDFW2Messages::m_cszWrongTaggedPath, pos);
						auto& Tags{ Stack.top().get() };
						if (Tags.empty())
							throw dfw2error(DFW2::CDFW2Messages::m_cszWrongTaggedPath, pos);
						Tags.back().closepos_ = pos;
					}
				}
			}
		}
	}
};

class TaggedPath : public TaggedString
{
public:
	TaggedPath(const std::string_view& Path) : TaggedString(Path)
	{
		AddFunction([this](const std::string& Tag) -> std::optional<std::string>
			{
				constexpr std::string_view szCount{ "count" };
				size_t padding{ 5 };
				if (Tag.substr(0, szCount.length()) == szCount)
				{
					std::from_chars(&Tag[szCount.length()], &Tag[Tag.length()], padding);
					return fmt::format("{:0{}d}", Count_++, padding);
				}
				else
					return {};
			});

		AddFunction([](const std::string& Tag) -> std::optional<std::string>
			{
				if (Tag == "date")
				return fmt::format("{:%Y-%m-%d}", fmt::localtime(std::time(nullptr)));
				else
					return {};
			});

		AddFunction([](const std::string& Tag) -> std::optional<std::string>
			{
				if (Tag == "time")
				return fmt::format("{:%H-%M-%S}", fmt::localtime(std::time(nullptr)));
				else
					return {};
			});
	}

	std::ofstream Create()
	{
		std::filesystem::path path{ stringutils::utf8_decode(Process()) };
		while (std::filesystem::exists(path) && Count_ > 0)
			path = std::filesystem::weakly_canonical(stringutils::utf8_decode(Process()));

		std::filesystem::path folder{ path };
		std::filesystem::create_directories(folder.remove_filename());
		std::ofstream dummy(path.string());
		if (dummy.is_open())
			return dummy;
		else
			throw dfw2error(DFW2::CDFW2Messages::m_cszStdFileStreamError, stringutils::utf8_encode(path.c_str()));
	}

	const std::string& Path() const
	{
		return Root_.substitution_;
	}
protected:
	size_t Count_ = 0;
};