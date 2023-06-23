#include "export_descr_sounds_soldier_voice.hpp"

#include <fmt/color.h>
#include <fmt/std.h>

#include <assert.h>

#include <ranges>

using namespace std::literals;
using namespace Voice::Soldier;


string Voice::Soldier::CFolder::Serialize() const noexcept
{
	auto ret = fmt::format("\t\t\t\t\tfolder {}\n", m_Folder);

	for (auto&& file : m_Files)
		ret += fmt::format("\t\t\t\t\t\t{}\n", file);

	return ret;
}

string Voice::Soldier::CEvent::Serialize() const noexcept
{
	auto ret = fmt::format("\t\t\t\tevent {}\n", fmt::join(m_Arguments, " "));

	for (auto&& folder : m_Folders)
		ret += folder.Serialize();

	ret += "\t\t\t\tend\n";
	return ret;
}

string Voice::Soldier::CVocal::Serialize() const noexcept
{
	auto ret = std::format("\t\t\tvocal {}\n", m_Name);

	for (auto&& Event : m_Events)
		ret += Event.Serialize();

	return ret;
}

string Voice::Soldier::CClass::Serialize() const noexcept
{
	auto ret = std::format("\t\tclass {}\n", m_Name);

	for (auto&& Vocal : m_Vocals | std::views::values)
		ret += Vocal.Serialize();

	return ret;
}

string Voice::Soldier::CAccent::Serialize() const noexcept
{
	auto ret = std::format("\taccent {}\n", m_Name);

	for (auto&& Class : m_Classes | std::views::values)
		ret += Class.Serialize();

	return ret;
}

void Voice::Soldier::CFile::Deserialize() noexcept
{
	CAccent* m_ThisAccent{};
	CClass* m_ThisClass{};
	CVocal* m_ThisVocal{};
	CEvent* m_ThisEvent{};

	for (auto Line = Parse("\r\n"); !Eof(); Line = Parse("\r\n"))
	{
		if (StartsWith_I(Line, "BANK:"))
			continue;

		else if (Line.starts_with("accent "))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisAccent = &m_Accents.insert_or_assign(Verses[1], CAccent{ .m_Name{Verses[1]} }).first->second;
			m_ThisClass = nullptr;
			m_ThisVocal = nullptr;
			assert(m_ThisEvent == nullptr);
		}
		else if (Line.starts_with("class "))
		{
			string_view const Verses = Line | std::views::drop("class "sv.length());

			assert(!Verses.contains(' '));

			m_ThisClass = &m_ThisAccent->m_Classes.insert_or_assign(Verses, CClass{ .m_Name{ Verses } }).first->second;
			m_ThisVocal = nullptr;
			assert(m_ThisEvent == nullptr);
		}
		else if (Line.starts_with("vocal "))
		{
			string_view const Verses = Line | std::views::drop("vocal "sv.length());
			assert(!Verses.contains(' '));

			m_ThisVocal = &m_ThisClass->m_Vocals.insert_or_assign(Verses, CVocal{ .m_Name{Verses} }).first->second;
			assert(m_ThisEvent == nullptr);
		}
		else if (Line.starts_with("event "))
		{
			assert(m_ThisEvent == nullptr);

			auto const Verses = UTIL_Split(Line) | std::views::drop(1) | std::ranges::to<vector>();
			assert(Verses.size() % 2 == 0 || Verses.back() == "looped");

			m_ThisEvent = &m_ThisVocal->m_Events.emplace_back(CEvent{ .m_Arguments{std::move(Verses)} });

			CFolder* m_ThisFolder{};
			for (Line = Parse("\r\n"); !Eof() && Line != "end"; Line = Parse("\r\n"))
			{
				if (Line.starts_with("folder "))
					m_ThisFolder = &m_ThisEvent->m_Folders.emplace_back(CFolder{ .m_Folder{Line | std::views::drop("folder "sv.length())} });
				else
					m_ThisFolder->m_Files.emplace_back(Line);
			}

			m_ThisEvent = nullptr;
		}
		else
		{
			fmt::print(fg(fmt::color::red), "[Error] Unknown script command: '{}'\n", Line);
		}
	}

	assert(m_ThisEvent == nullptr);
}

string Voice::Soldier::CFile::Serialize() const noexcept
{
	string ret{ "BANK: unit_voice\n" };

	for (auto&& Accent : m_Accents | std::views::values)
		ret += Accent.Serialize();

	return ret;
}

bool Voice::Soldier::CFile::Save(fs::path const& Path) const noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"wt"); f)
	{
		fmt::print(f, "{}\n", Serialize());
		fclose(f);

		return true;
	}

	return false;
}

