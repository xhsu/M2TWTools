#include <assert.h>

#include <ranges>

#include "export_descr_sounds_units_voice.hpp"

#include <fmt/color.h>

using namespace Voice::Units;

string Voice::Units::CEvent::Serialize() const noexcept
{
	string ret{};

	switch (m_Type)
	{
	case CEvent::Engine:
		ret += fmt::format("\t\t\t\tengine {}\n", fmt::join(m_Troops, ", "));
		break;
	case CEvent::Unit:
		ret += fmt::format("\t\t\t\tunit {}\n", fmt::join(m_Troops, ", "));
		break;

	case CEvent::None:
	default:
		break;
	}

	ret += "\t\t\t\tevent\n";
	ret += fmt::format("\t\t\t\t\tfolder {}\n", m_Folder);

	for (auto&& file : m_Files)
		ret += fmt::format("\t\t\t\t\t\t{}\n", file);

	ret += "\t\t\t\tend\n";

	return ret;
}

string Voice::Units::CVocal::Serialize() const noexcept
{
	auto ret = fmt::format("\t\t\tvocal {} {}\n", m_Name, fmt::join(m_Arguments, " "));

	for (auto&& Event : m_Events)
		ret += Event.Serialize();

	return ret;
}

string Voice::Units::CClass::Serialize() const noexcept
{
	auto ret = fmt::format("\t\tclass {}\n", m_Name);

	for (auto&& Vocal : m_Vocals | std::views::values)
		ret += Vocal.Serialize();

	return ret;
}

string Voice::Units::CAccent::Serialize() const noexcept
{
	auto ret = fmt::format("\taccent {}\n", m_Name);

	for (auto&& Class : m_Classes | std::views::values)
		ret += Class.Serialize();

	return ret;
}

Voice::Units::CUnitVoices::CUnitVoices(fs::path const& Path) noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"rb"); f != nullptr)
	{
		fseek(f, 0, SEEK_END);
		m_length = ftell(f);

		m_p = (char*)calloc(sizeof(char), m_length + 1);
		fseek(f, 0, SEEK_SET);
		fread(m_p, sizeof(char), m_length, f);

		fclose(f);

		Deserialize();
	}
}

Voice::Units::CUnitVoices::~CUnitVoices() noexcept
{
	if (m_p)
	{
		free(m_p);

		m_p = nullptr;
		m_length = 0;
	}
}

void Voice::Units::CUnitVoices::Deserialize() noexcept
{
	CAccent* m_ThisAccent{};
	CClass* m_ThisClass{};
	CVocal* m_ThisVocal{};
	CEvent* m_ThisEvent{};

	for (auto&& Line : UTIL_Split({ m_p, m_length }, "\n\r"))
	{
		if (Line.starts_with("BANK:"))
			continue;

		if (Line.starts_with("accent"))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisAccent = &m_Accents.insert_or_assign(Verses[1], CAccent{ .m_Name{Verses[1]} }).first->second;
			m_ThisClass = nullptr;
			m_ThisVocal = nullptr;
			assert(m_ThisEvent == nullptr);
		}
		else if (Line.starts_with("class"))
		{
			auto const Verses = string_view{
				Line
				| std::views::drop(sizeof("class"))
				| std::views::drop_while([](char c) noexcept { return std::isspace(c); })
			};

			assert(!Verses.contains(' '));

			m_ThisClass = &m_ThisAccent->m_Classes.insert_or_assign(Verses, CClass{ .m_Name{ Verses } }).first->second;
			m_ThisVocal = nullptr;
			assert(m_ThisEvent == nullptr);
		}
		else if (Line.starts_with("vocal"))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() <= 3);

			m_ThisVocal = &m_ThisClass->m_Vocals.insert_or_assign(Verses[1], CVocal{ .m_Name{Verses[1]}, .m_Arguments{Verses.begin() + 2, Verses.end()} }).first->second;
		}
		else if (Line.starts_with("engine"))
		{
			assert(m_ThisEvent == nullptr);
			m_ThisEvent = &m_ThisVocal->m_Events.emplace_back(
				CEvent{
					.m_Type{ CEvent::Engine },
					.m_Troops{ UTIL_Split(Line) | std::views::drop(1) | std::ranges::to<vector>() },
				}
			);
		}
		else if (Line.starts_with("unit"))
		{
			assert(m_ThisEvent == nullptr);
			m_ThisEvent = &m_ThisVocal->m_Events.emplace_back(
				CEvent{
					.m_Type{ CEvent::Unit },
					.m_Troops{ string_view{Line | std::views::drop(sizeof("unit")) | std::views::drop_while([](char c) noexcept { return std::isspace(c); })} },
				}
			);

			while (std::isspace(m_ThisEvent->m_Troops.back().back()))
				m_ThisEvent->m_Troops.back() = m_ThisEvent->m_Troops.back().substr(0, m_ThisEvent->m_Troops.back().length() - 1);
		}
		else if (Line.starts_with("event"))
		{
			assert(m_ThisEvent == nullptr || m_ThisEvent->m_Type != CEvent::None);

			if (m_ThisEvent == nullptr)
				m_ThisEvent = &m_ThisVocal->m_Events.emplace_back();
		}
		else if (Line.starts_with("folder"))
		{
			m_ThisEvent->m_Folder = string_view{
				Line
				| std::views::drop(sizeof("folder"))
				| std::views::drop_while([](char c) noexcept { return std::isspace(c); })
			};
		}
		else if (Line.starts_with("end"))
		{
			assert(m_ThisEvent != nullptr);
			m_ThisEvent = nullptr;
		}
		else if (m_ThisEvent != nullptr)
		{
			m_ThisEvent->m_Files.emplace_back(Line);
		}
		else
			fmt::print(fg(fmt::color::red), "[ERROR] Unknown script command '{}'\n", Line);
	}
}

string Voice::Units::CUnitVoices::Serialize() const noexcept
{
	string ret{ "BANK: unit_voice\n" };
	ret.reserve(m_length);

	for (auto&& [szName, Accent] : m_Accents)
		ret += Accent.Serialize();

	return ret;
}

bool Voice::Units::CUnitVoices::Save(fs::path const& Path) const noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"wt"); f != nullptr)
	{
		fmt::print(f, "{}\n", Serialize());
		fclose(f);

		return true;
	}

	return false;
}

CAccent* Voice::Units::CUnitVoices::At(string_view szAccent) noexcept
{
	try
	{
		return &m_Accents.at(szAccent);
	}
	catch (...)
	{
		return nullptr;
	}
}

CClass* Voice::Units::CUnitVoices::At(string_view szAccent, string_view szClass) noexcept
{
	try
	{
		return &m_Accents.at(szAccent).m_Classes.at(szClass);
	}
	catch (...)
	{
		return nullptr;
	}
}

CVocal* Voice::Units::CUnitVoices::At(string_view szAccent, string_view szClass, string_view szVocal) noexcept
{
	try
	{
		return &m_Accents.at(szAccent).m_Classes.at(szClass).m_Vocals.at(szVocal);
	}
	catch (...)
	{
		return nullptr;
	}
}

generator<tuple<CAccent*, CClass*, CVocal*>> Voice::Units::CUnitVoices::EveryVocalOf(string_view szVocal) noexcept
{
	for (auto&& Accent : m_Accents | std::views::values)
	{
		for (auto&& Class : Accent.m_Classes | std::views::values)
		{
			for (auto&& Vocal : Class.m_Vocals | std::views::values)
			{
				if (Vocal.m_Name == szVocal)
					co_yield tuple{ &Accent, &Class, &Vocal };
			}
		}
	}

	co_return;
}

generator<tuple<CAccent*, CClass*, CVocal*, CEvent*>> Voice::Units::CUnitVoices::EveryUnitOf(string_view szUnit) noexcept
{
	for (auto&& Accent : m_Accents | std::views::values)
	{
		for (auto&& Class : Accent.m_Classes | std::views::values)
		{
			for (auto&& Vocal : Class.m_Vocals | std::views::values)
			{
				for (auto&& Event : Vocal.m_Events)
				{
					if (Event.m_Type == CEvent::Unit && Event.m_Troops[0] == szUnit)
						co_yield tuple{ &Accent, &Class, &Vocal, &Event };
				}
			}
		}
	}
}
