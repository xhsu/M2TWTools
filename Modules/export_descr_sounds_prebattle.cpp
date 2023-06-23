#include "export_descr_sounds_prebattle.hpp"

#include <fmt/color.h>
#include <fmt/std.h>

#include <assert.h>

#include <ranges>

using namespace Voice::Prebattle;

string Voice::Prebattle::CEvent::Serialize() const noexcept
{
	string ret{};

	switch (Type())
	{
	case VnV:
		ret += fmt::format("\t\t\t{}\n", m_rgszTypeName[VnV]);
		ret += fmt::format("\t\t\t{}\n", fmt::join(m_Arguments | std::views::drop(1), " "));
		break;

	case Unknown:
	case None:
		break;

	default:
		ret += fmt::format("\t\t\t{}\n", fmt::join(m_Arguments, " "));
		break;
	}

	ret += "\t\t\t\tevent\n";
	ret += fmt::format("\t\t\t\t\tfolder {}\n", m_Folder);

	for (auto&& file : m_Files)
		ret += fmt::format("\t\t\t\t\t\t{}\n", file);

	ret += "\t\t\t\tend\n";

	return ret;
}

string Voice::Prebattle::CElement::Serialize() const noexcept
{
	auto ret = std::format("\t\telement {}\n", m_Name);

	for (auto&& Event : m_Events)
		ret += Event.Serialize();

	return ret;
}

string Voice::Prebattle::CAccent::Serialize() const noexcept
{
	auto ret = std::format("\taccent {}\n", m_Name);

	for (auto&& Elem : m_Elements | std::views::values)
		ret += Elem.Serialize();

	return ret;
}

void Voice::Prebattle::CFile::Deserialize() noexcept
{
	CAccent* m_ThisAccent{};
	CElement* m_ThisElement{};
	CEvent* m_ThisEvent{};

	for (auto Line = Parse("\r\n"); !Eof(); Line = Parse("\r\n"))
	{
		if (StartsWith_I(Line, "BANK:"))
			continue;

		else if (Line.starts_with("accent"))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisAccent = &m_Accents.insert_or_assign(Verses[1], CAccent{ .m_Name{Verses[1]} }).first->second;
			m_ThisElement = nullptr;
			assert(m_ThisEvent == nullptr);
		}
		else if (Line.starts_with("element"))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisElement = &m_ThisAccent->m_Elements.insert_or_assign(Verses[1], CElement{ .m_Name{Verses[1]} }).first->second;
			assert(m_ThisEvent == nullptr);
		}
		else if (Line.starts_with("VnV"))
		{
			assert(Line == "VnV");
			assert(m_ThisEvent == nullptr);

			m_ThisEvent = &m_ThisElement->m_Events.emplace_back(CEvent{ .m_Arguments{"VnV"} });
			m_ThisEvent->m_Arguments.append_range(UTIL_Split(Parse("\r\n")) | std::ranges::to<vector>());

			assert(m_ThisEvent->m_Arguments.size() == 3);
		}
		else if (Line.starts_with("relationship") || Line.starts_with("situation") || Line.starts_with("condition"))
		{
			assert(m_ThisEvent == nullptr);

			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			m_ThisEvent = &m_ThisElement->m_Events.emplace_back(CEvent{ .m_Arguments{std::move(Verses)} });

#ifdef _DEBUG
			auto const bAssertion =
				(m_ThisEvent->m_Arguments[0] == "relationship" && m_ThisEvent->m_Arguments.size() == 3)
				|| (m_ThisEvent->m_Arguments[0] == "situation" && m_ThisEvent->m_Arguments.size() == 2)
				|| (m_ThisEvent->m_Arguments[0] == "condition" && m_ThisEvent->m_Arguments.size() == 2);

			assert(bAssertion);
#endif // _DEBUG
		}
		else if (Line.starts_with("event"))
		{
			if (m_ThisEvent == nullptr)
			{
				fmt::print(fg(fmt::color::light_slate_gray), "[Message] Event without condition under '{}/{}'\n", m_ThisAccent->m_Name, m_ThisElement->m_Name);
				m_ThisEvent = &m_ThisElement->m_Events.emplace_back();
			}

			assert(Line == "event");

			auto Folder = Parse("\r\n");
			assert(Folder.starts_with("folder "));

			m_ThisEvent->m_Folder = Folder | std::views::drop("folder "sv.length());

			for (auto file = Parse("\r\n"); !Eof() && file != "end"; file = Parse("\r\n"))
				m_ThisEvent->m_Files.emplace_back(file);

			m_ThisEvent = nullptr;
		}
		else
		{
			fmt::print(fg(fmt::color::red), "[Error] Unknown script command: '{}'\n", Line);
		}
	}
}

string Voice::Prebattle::CFile::Serialize() const noexcept
{
	string ret{ "BANK: prebattle_speech\n" };

	for (auto&& Accent : m_Accents | std::views::values)
		ret += Accent.Serialize();

	return ret;
}

bool Voice::Prebattle::CFile::Save(fs::path const& Path) const noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"wt"); f)
	{
		fmt::print(f, "{}\n", Serialize());
		fclose(f);

		return true;
	}

	return false;
}
