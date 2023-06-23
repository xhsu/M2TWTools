#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

//#include "Modules/export_descr_sounds_units_voice.hpp"
//#include "Modules/battle_models.hpp"
//#include "Modules/export_descr_unit.hpp"

#include "String.hpp"

#include <assert.h>

#include <ranges>
#include <map>
#include <vector>
#include <set>

namespace fs = std::filesystem;

using namespace std;
using namespace std::literals;

extern void CopyUnitEssentialFiles(fs::path const& SourceData, fs::path const& DestData, string_view const* itUnitNames, size_t len) noexcept;
extern void ListFactionUnitPriority(fs::path const& edu, string_view szFaction) noexcept;
extern void Method(fs::path const& SourceData, fs::path const& DestData) noexcept;

struct Ancillary_t final
{
	using Effect_t = pair<string_view, int32_t>;

	string_view m_Name{};
	string_view m_Type{};
	int32_t m_Transferable{};
	string_view m_Image{};
	vector<string_view> m_ExcludedAncillaries{};
	vector<string_view> m_ExcludeCultures{};
	string_view m_Description{};
	string_view m_EffectsDescription{};
	vector<Effect_t> m_Effects{};
	bool m_Unique{};

	string Serialize() const noexcept
	{
		string ret = fmt::format("Ancillary {}\n", m_Name);

		if (!m_Type.empty())
			ret += fmt::format("\tType {}\n", m_Type);

		ret += fmt::format("Transferable {}", m_Transferable);

		if (!m_Image.empty())
			ret += fmt::format("\tImage {}\n", m_Image);
		if (m_Unique)
			ret += "\tUnique\n";
		if (!m_ExcludedAncillaries.empty())
			ret += fmt::format("\tExcludedAncillaries {}\n", fmt::join(m_ExcludedAncillaries, ", "));
		if (!m_ExcludeCultures.empty())
			ret += fmt::format("\tExcludeCultures {}\n", fmt::join(m_ExcludeCultures, ", "));
		if (!m_Description.empty())
			ret += fmt::format("\tDescription {}\n", m_Description);
		if (!m_EffectsDescription.empty())
			ret += fmt::format("\tEffectsDescription {}\n", m_EffectsDescription);

		for (auto&& [szWhat, iLevel] : m_Effects)
			ret += fmt::format("\tEffect {} {}\n", szWhat, iLevel);
	}
};

struct Trigger_t final
{
	string_view m_Name{};
	string_view m_WhenToTest{};
};

struct export_descr_ancillaries final : public CBaseParser
{
	map<string_view, Ancillary_t, CaseIgnoredLess> m_Ancillaries{};

	explicit export_descr_ancillaries(fs::path const& Path) noexcept : CBaseParser{ Path } { Deserialize(); }

	void Deserialize() noexcept
	{
		Ancillary_t* pThisOne{};

		for (auto Line = Parse("\r\n"); !Eof(); Line = Parse("\r\n"))
		{
			if (Line.starts_with(';'))
				continue;

			if (Line.starts_with("Trigger") || Line.starts_with("WhenToTest") || Line.starts_with("Condition") || Line.starts_with("and") || Line.starts_with("AcquireAncillary"))
				continue;

			if (auto const pos = Line.find_first_of(';'); pos != Line.npos)
				Line = Line.substr(0, pos);

			if (Line.empty())
				continue;

			auto const Verses = UTIL_Split(Line, " ,\t") | std::ranges::to<vector>();

			if (Verses.empty())
				continue;

			else if (Verses.front() == "Ancillary")
				pThisOne = &m_Ancillaries.try_emplace(Verses[1], Ancillary_t{ .m_Name{Verses[1]} }).first->second;
			else if (Verses.front() == "Type")
				pThisOne->m_Type = Verses[1];
			else if (Verses.front() == "Transferable")
				pThisOne->m_Transferable = std::stoi(string{ Verses[1] });
			else if (Verses.front() == "Image")
				pThisOne->m_Image = Verses[1];
			else if (Verses.front() == "Unique")
				pThisOne->m_Unique = true;
			else if (Verses.front() == "ExcludedAncillaries")
				pThisOne->m_ExcludedAncillaries = vector<string_view>{ Verses.begin() + 1, Verses.end() };
			else if (Verses.front() == "ExcludeCultures")
				pThisOne->m_ExcludeCultures = vector<string_view>{ Verses.begin() + 1, Verses.end() };
			else if (Verses.front() == "Description")
				pThisOne->m_Description = Verses[1];
			else if (Verses.front() == "EffectsDescription")
				pThisOne->m_EffectsDescription = Verses[1];
			else if (Verses.front() == "Effect")
				pThisOne->m_Effects.emplace_back(pair{ Verses[1], std::stoi(string{ Verses[2] }) });
			else
				fmt::print(fg(fmt::color::red), "[Error] Unknown script command '{}'\n", Line);
		}
	}
};

int main(int, char* []) noexcept
{
	export_descr_ancillaries f{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\export_descr_ancillaries.txt)" };
	export_descr_ancillaries f2{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\export_descr_ancillaries.txt)" };

	auto const s1 = f.m_Ancillaries | std::views::keys | std::ranges::to<set>();
	auto const s2 = f2.m_Ancillaries | std::views::keys | std::ranges::to<set>();

	vector<string_view> onlyInVan{};
	std::ranges::set_difference(s1, s2, back_inserter(onlyInVan));

	vector<string_view> onlyInCrus{};
	std::ranges::set_difference(s2, s1, back_inserter(onlyInCrus));


	return EXIT_SUCCESS;
}
