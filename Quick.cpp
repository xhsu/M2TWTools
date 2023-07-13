#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

//#include "Modules/export_descr_sounds_units_voice.hpp"
//#include "Modules/battle_models.hpp"
//#include "Modules/export_descr_unit.hpp"
//#include "Modules/descr_mount.hpp"
#include "Modules/export_descr_unit.hpp"
#include "String.hpp"

//#include "String.hpp"

#include <assert.h>

#include <array>
#include <ranges>
#include <map>
#include <vector>
#include <set>
#include <random>

#include <fstream>

namespace fs = std::filesystem;

using namespace std;
using namespace std::literals;

extern void CopyUnitEssentialFiles(fs::path const& SourceData, fs::path const& DestData, string_view const* itUnitNames, size_t len) noexcept;
extern void ListFactionUnitPriority(fs::path const& edu, string_view szFaction) noexcept;
extern void CopyBattleModelFromFactionToAnother(fs::path const& Data, string_view SourceFaction, string_view DestFaction, string_view const* itModels, size_t len) noexcept;

inline constexpr array units = {
	"Richard the Lionheart"sv,
	"Philip King of France"sv,
};

struct CFactionNames final
{
	string_view m_Faction{};
	set<string_view> m_MaleNames{};
	set<string_view> m_SurNames{};
	set<string_view> m_FemaleNames{};

	string Serialize() const noexcept
	{
		string ret{ fmt::format("faction: {}\n", m_Faction) };

		static constexpr auto fn = [](set<string_view> const& names) /*static*/ noexcept -> string
		{
			string ret{};
			for (auto&& s : names)
				ret += fmt::format("\t\t{}\n", s);

			return ret;
		};

		ret += '\n';

		ret += "\tcharacters\n";
		ret += fn(m_MaleNames);
		ret += fmt::format("\t;{} add more names\n", m_Faction);

		ret += '\n';

		ret += "\tsurnames\n";
		ret += fn(m_SurNames);
		ret += fmt::format("\t;{} add more surnames\n", m_Faction);

		ret += '\n';

		ret += "\twomen\n";
		ret += fn(m_FemaleNames);
		ret += fmt::format("\t;{} add more female names\n", m_Faction);

		ret += '\n';

		return ret;
	}
};

struct DescrNames : public IBaseFile
{
	map<string_view, CFactionNames, CaseIgnoredLess> m_Factions{};

	explicit DescrNames(fs::path const& Path) noexcept : IBaseFile{ Path } { Deserialize(); }

	void Deserialize() noexcept override
	{
		CFactionNames* m_ThisFaction{};
		set<string_view>* m_ThisNameSet{};

		for (auto Line = Parse("\r\n"); !Eof(); Line = Parse("\r\n"))
		{
			if (Line.starts_with("faction:"))
			{
				auto Verses = UTIL_Split(Line, " \t:") | std::views::drop(1) | std::ranges::to<vector>();
				assert(Verses.size() == 1);

				m_ThisFaction = &m_Factions.try_emplace(Verses[0], CFactionNames{ .m_Faction{Verses[0]} }).first->second;
			}
			else if (Line == "characters")
			{
				m_ThisNameSet = &m_ThisFaction->m_MaleNames;
			}
			else if (Line == "surnames")
			{
				m_ThisNameSet = &m_ThisFaction->m_SurNames;
			}
			else if (Line == "women")
			{
				m_ThisNameSet = &m_ThisFaction->m_FemaleNames;
			}
			else
			{
				m_ThisNameSet->emplace(std::move(Line));
			}
		}
	}

	string Serialize() const noexcept override
	{
		string ret{};

		for (auto&& [_, Faction] : m_Factions)
			ret += Faction.Serialize();

		return ret;
	}

	decltype(auto) operator[] (string_view sv) noexcept { return m_Factions.at(sv); }
	decltype(auto) operator[] (string_view sv) const noexcept { return m_Factions.at(sv); }

	// Draw a name
	enum ENameType { Male, Female };

	pair<string_view, string_view> Draw(string_view Faction, ENameType Sex = Male) const noexcept
	{
		static std::random_device dev;
		static std::mt19937 mt(dev());

		using Random_t = std::uniform_int_distribution<std::mt19937::result_type>;

		if (!m_Factions.contains(Faction))
			return pair{ "_JOHN"sv, "_DOE"sv };

		// Given Name

		auto&& GivenNamePool{
			Sex == Male ? m_Factions.at(Faction).m_MaleNames : m_Factions.at(Faction).m_FemaleNames
		};

		assert(GivenNamePool.size() > 0);

		auto pGivenName = GivenNamePool.begin();
		Random_t Random(0, GivenNamePool.size() - 1);
		std::advance(pGivenName, Random(mt));

		// Surname

		auto&& SurnamePool{
			m_Factions.at(Faction).m_SurNames
		};

		assert(SurnamePool.size() > 0);

		auto pSurname = SurnamePool.begin();
		Random_t Random2(0, SurnamePool.size() - 1);
		std::advance(pSurname, Random2(mt));

		return pair{ *pGivenName, *pSurname };
	}
};

struct CCharacter final
{
	pair<string_view, string_view> m_Name{"_JOHN", "_DOE"};
	int32_t m_Age{};
	optional<int32_t> m_Dead{ std::nullopt };
	enum ESex { Male, Female } m_Sex{ Male };

	CCharacter(
		std::add_rvalue_reference_t<decltype(m_Name)> Name = { "_JOHN", "_DOE" },
		std::add_rvalue_reference_t<decltype(m_Sex)> Sex = Male,
		std::add_rvalue_reference_t<decltype(m_Age)> Age = 16,
		std::add_rvalue_reference_t<decltype(m_Dead)> Dead = std::nullopt
	) noexcept :
		m_Name{ std::forward<decltype(m_Name)>(Name) },
		m_Sex{ std::forward<decltype(m_Sex)>(Sex) },
		m_Age{ std::forward<decltype(m_Age)>(Age) },
		m_Dead{ std::forward<decltype(m_Dead)>(Dead) }
	{
		Everybody.emplace(this);
	}

	/*virtual*/ ~CCharacter() noexcept { Everybody.erase(this); }

	CCharacter(CCharacter&&) noexcept = delete;
	CCharacter(CCharacter const&) noexcept = delete;

	CCharacter& operator= (CCharacter&&) noexcept = delete;
	CCharacter& operator= (CCharacter const&) noexcept = delete;

	inline constexpr auto ExistingFor() const noexcept { return m_Age + m_Dead.value_or(0); }

	inline enum EAdoptRes { GoodAdoption, TooManyKids, BadAgeDiff, Twin, } Adopt(CCharacter* p) noexcept
	{
		if (m_Issues.size() >= 4)
		{
			return TooManyKids;
		}
		if (this->ExistingFor() - p->ExistingFor() < 16)
		{
			return BadAgeDiff;
		}
		if (m_Issues.contains(p))
		{
			return Twin;
		}

		m_Issues.emplace(p);

		if (m_Spouse)
			m_Spouse->m_Issues.emplace(p);

		return GoodAdoption;
	}

	inline enum EMarryRes { GoodMarrage, Pedophile, TooYoungToParentKids, Homo, } Merry(CCharacter* p) noexcept
	{
		if (p->m_Sex == m_Sex)
		{
			return Homo;
		}
		if (m_Age < 16 || p->m_Age < 16)
		{
			return Pedophile;
		}
		if (!m_Issues.empty() && p->m_Age - (*m_Issues.begin())->m_Age < 16)
		{
			// compare to the oldest kid would be sufficient.
			return TooYoungToParentKids;
		}

		p->m_Spouse = this;
		m_Spouse = p;

		// Sync kids
		p->m_Issues = m_Issues;

		return GoodMarrage;
	}

	inline int32_t Width() const noexcept
	{
		int32_t w{};

		for (auto&& pKid : this->m_Issues)
			w += pKid->Width();

		if (m_Spouse)
			w += 1;

		return w;
	}

	inline constexpr bool operator== (CCharacter const& rhs) const noexcept { return m_Name.first == rhs.m_Name.first && m_Name.second == rhs.m_Name.second; }
	inline constexpr std::strong_ordering operator<=> (CCharacter const& rhs) const noexcept { return ExistingFor() <=> rhs.ExistingFor(); }

	CCharacter* m_Spouse{ nullptr };
	set<CCharacter*, decltype([](CCharacter* lhs, CCharacter* rhs) noexcept { return *lhs < *rhs; })> m_Issues{};
	vector<string_view> m_Args{};

	static inline set<CCharacter*> Everybody{};
};


int main(int, char* []) noexcept
{
	//DescrNames f(R"(C:\Program Files (x86)\Steam\steamapps\common\Medieval II Total War\mods\MyMod\data\descr_names.txt)");
	auto edu = Units::CEDU{ R"(C:\Program Files (x86)\Steam\steamapps\common\Medieval II Total War\mods\MyMod\data\export_descr_unit.txt)" };
	fmt::print("{}\n", edu.Serialize());



	return EXIT_SUCCESS;
}
