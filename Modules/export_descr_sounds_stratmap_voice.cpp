#include "export_descr_sounds_stratmap_voice.hpp"

#include <fmt/color.h>

#include <assert.h>

#include <functional>
#include <ranges>

using namespace Voice::Strat;
using namespace std::literals;

string Voice::Strat::CVocal::Serialize() const noexcept
{
	return fmt::format(
		"\t\t\tvocal {}\n{}",
		m_Name,
		fmt::join(m_Events | std::views::transform(std::bind_back(&CSimpleEvent::Serialize, 4)), "")
	);
}

string Voice::Strat::CType::Serialize() const noexcept
{
	return fmt::format(
		"\t\ttype {}\n{}",
		m_Name,
		fmt::join(m_Vocals | std::views::values | std::views::transform(&CVocal::Serialize), "")
	);
}

string Voice::Strat::CAccent::Serialize() const noexcept
{
	return fmt::format(
		"\taccent {}\n{}",
		m_Name,
		fmt::join(m_Types | std::views::values | std::views::transform(&CType::Serialize), "")
	);
}

void Voice::Strat::CFile::Deserialize() noexcept
{
	CAccent* m_ThisAccent{};
	CType* m_ThisType{};
	CVocal* m_ThisVocal{};

	for (auto Line = Parse("\r\n"); !Eof(); Line = Parse("\r\n"))
	{
		if (StartsWith_I(Line, "BANK:"))
			continue;

		else if (Line.starts_with("accent "))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisAccent = &m_Accents.insert_or_assign(Verses[1], CAccent{ .m_Name{Verses[1]} }).first->second;
			m_ThisType = nullptr;
			m_ThisVocal = nullptr;
		}
		else if (Line.starts_with("type "))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisType = &m_ThisAccent->m_Types.insert_or_assign(Verses[1], CType{ .m_Name{Verses[1]} }).first->second;
			m_ThisVocal = nullptr;
		}
		else if (Line.starts_with("vocal "))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisVocal = &m_ThisType->m_Vocals.insert_or_assign(Verses[1], CVocal{ .m_Name{Verses[1]} }).first->second;
		}
		else if (Line.starts_with("event"))
		{
			m_ThisVocal->m_Events.emplace_back(CSimpleEvent::Deserialize(this, Line));
		}
	}

	assert(Eof());
}

string Voice::Strat::CFile::Serialize() const noexcept
{
	return fmt::format(
		"BANK: character_vocal\n{}",
		fmt::join(m_Accents | std::views::values | std::views::transform(&CAccent::Serialize), "")
	);
}

bool Voice::Strat::CFile::Save(fs::path const& Path) const noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"wt"); f)
	{
		fmt::print(f, "{}\n", Serialize());
		fclose(f);

		return true;
	}

	return false;
}
