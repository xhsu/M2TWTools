#include "export_descr_sounds_units_battle_events.hpp"

#include <fmt/ranges.h>

#include <assert.h>

#include <functional>
#include <ranges>

using namespace Voice::Battle;

string Voice::Battle::CNotification::Serialize() const noexcept
{
	return fmt::format(
		"\t\tnotification {}\n{}",
		fmt::join(m_Arguments, " "),
		fmt::join(m_Events | std::views::transform(std::bind_back(&CSimpleEvent::Serialize, 3)), "")
	);
}

string Voice::Battle::CAccent::Serialize() const noexcept
{
	return fmt::format(
		"\taccent {}\n{}",
		m_Name,
		fmt::join(m_Notifications | std::views::transform(&CNotification::Serialize), "")
	);
}

void Voice::Battle::CFile::Deserialize() noexcept
{
	CAccent* m_ThisAccent{};
	CNotification* m_ThisNote{};

	for (auto Line = Parse("\r\n"); !Eof(); Line = Parse("\r\n"))
	{
		if (StartsWith_I(Line, "BANK:"))
			continue;

		else if (Line.starts_with("accent "))
		{
			auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();
			assert(Verses.size() == 2);

			m_ThisAccent = &m_Accents.insert_or_assign(Verses[1], CAccent{ .m_Name{Verses[1]} }).first->second;
			m_ThisNote = nullptr;
		}
		else if (Line.starts_with("notification "))
		{
			auto const Verses = UTIL_Split(Line) | std::views::drop(1) | std::ranges::to<vector>();
			assert(Verses.size() == 2);	// dropping first 'notification' word.

			m_ThisNote = &m_ThisAccent->m_Notifications.emplace_back(CNotification{ .m_Arguments{std::move(Verses)} });
		}
		else if (Line.starts_with("event"))
		{
			m_ThisNote->m_Events.emplace_back(CSimpleEvent::Deserialize(this, Line));
		}
	}

	assert(Eof());
}

string Voice::Battle::CFile::Serialize() const noexcept
{
	return fmt::format(
		"BANK: battle_events\n{}",
		fmt::join(m_Accents | std::views::values | std::views::transform(&CAccent::Serialize), "")
	);
}

bool Voice::Battle::CFile::Save(fs::path const& Path) const noexcept
{
	if (auto f = _wfopen(Path.c_str(), L"wt"); f)
	{
		fmt::print(f, "{}\n", Serialize());
		fclose(f);

		return true;
	}

	return false;
}
