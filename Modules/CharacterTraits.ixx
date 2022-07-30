module;

#include <string>
#include <vector>
#include <variant>
#include <deque>
#include <unordered_set>

#include <cassert>

#include <fmt/color.h>

export module CharacterTraits;

using std::string;
using std::vector;
using std::variant;
using std::deque;
using std::unordered_set;

static const unordered_set<string> g_rgszPositiveEffects =
{
	"Ambush",
	"ArtilleryCommand",
	"Attack",
	"Authority",
	"BribeResistance",
	"Bribery",
	"CavalryCommand",
	"Charm",
	"Chivalry",
	// Combat_V_...
	"Command",
	"Construction",
	"Defence",
	"Eligibility",
	"Farming",
	"Fertility",
	"Finance",
	"GunpowderCommand",
	"HeresyImmunity",
	"HitPoints",
	"InfantryCommand",
	"Influence",
	"Law",
	"Level",
	"LineOfSight",
	"LocalPopularity",
	"Looting",
	"Loyalty",
	"Magic",
	"Mining",
	"MovementPoints",
	"NavalCommand",
	"NightBattle",
	"PersonalSecurity",
	"Piety",
	"Purity",
	"PublicSecurity",
	"Sabotage",
	"SiegeAttack",
	"SiegeDefence",
	"SiegeEngineering",
	"Subterfuge",
	"TaxCollection",
	"Trading",
	"TrainingAgents",
	"TroopMorale",
	"Violence",
};

static const unordered_set<string> g_rgszNegativeEffects =
{
	"Squalor",
	"Unorthodoxy",
	"Unrest",
};

export struct Effect_t final
{
	//Effect_t(const string& szType, short iModifier) noexcept : m_Type(szType), m_Modification(iModifier) {}
	//Effect_t(const Effect_t& rhs) noexcept = default;
	//Effect_t(Effect_t&& rhs) noexcept = default;
	//Effect_t& operator=(const Effect_t& rhs) noexcept = default;
	//Effect_t& operator=(Effect_t&& rhs) noexcept = default;
	//~Effect_t(void) noexcept = default;

	string m_Type{};
	short m_Modification;

	bool IsPositive(void) const noexcept
	{
		int iSign = 0;

		if (g_rgszPositiveEffects.contains(m_Type) || m_Type.starts_with("Combat_V_"))
		{
			iSign = 1;
		}
		else if (g_rgszNegativeEffects.contains(m_Type))
		{
			iSign = -1;
		}
		else
		{
			fmt::print(fg(fmt::color::red), "Unknown effect \"{}\" detected, with modifier {}.\n", m_Type, m_Modification);
			return false;
		}

		return (m_Modification * iSign) >= 0;
	}

	decltype(auto) GetFormatStyle(void) const noexcept { return fmt::fg(IsPositive() ? fmt::color::lime_green : fmt::color::pale_violet_red); }
};

export struct Level_t
{
	enum : std::uint8_t
	{
		NEGATIVE,
		LEAN_NEGATIVE,
		NEUTER,
		LEAN_POSITIVE,
		POSITIVE,
	};

	string m_Name{};
	string m_Description{};
	string m_EffectsDescription{};
	string m_GainMessage{};
	string m_LoseMessage{};
	string m_Epithet{};
	short m_Threshold = 0;
	short m_Level = 0;
	vector<Effect_t> m_Effects{};

	decltype(auto) Evaluate(void) const noexcept
	{
		bool bPositivePresent = false, bNegativePresent = false;
		short iIntegral = 0;

		for (auto&& Effect : m_Effects)
		{
			bool const bThisPositive = Effect.IsPositive();
			short const iSign = bThisPositive ? 1 : -1;

			bPositivePresent = bPositivePresent || bThisPositive;
			bNegativePresent = bNegativePresent || !bThisPositive;

			iIntegral += iSign * std::abs(Effect.m_Modification);
		}

		if (bPositivePresent && bNegativePresent)
		{
			if (iIntegral > 0)
				return LEAN_POSITIVE;
			else if (iIntegral < 0)
				return LEAN_NEGATIVE;
			else
				return NEUTER;
		}
		else if (bPositivePresent)
			return POSITIVE;
		else if (bNegativePresent)
			return NEGATIVE;
		else
			return NEUTER;
	}

	decltype(auto) GetFormatStyle(void) const noexcept
	{
		switch (Evaluate())
		{
		case NEGATIVE:
			return fmt::fg(fmt::color::red);
		case LEAN_NEGATIVE:
			return fmt::fg(fmt::color::orange_red);
		case NEUTER:
			return fmt::fg(fmt::color::light_golden_rod_yellow);
		case LEAN_POSITIVE:
			return fmt::fg(fmt::color::pale_green);
		case POSITIVE:
			return fmt::fg(fmt::color::lime);
		default: [[unlikely]]
			std::unreachable();
			return fmt::fg(fmt::color::black);
		}
	}
};

export struct Trait_t
{
	string m_Name{};
	vector<string> m_Character{};
	vector<string> m_ExcludeCultures{};
	short m_NoGoingBackLevel = -1;
	bool m_Hidden : 1 { false };
	vector<variant<string, const Trait_t*>> m_AntiTraits{};
	deque<Level_t> m_Levels{};	// std::vector would move and thus invalidate our pointer.

	inline void Print(void) const noexcept
	{
		static constexpr auto SPACES_COUNT = 17;
		static constexpr auto SPACES_COUNT2 = 20;

		fmt::print("{}\n", m_Name);

		if (!m_Character.empty())
		{
			string ListStr{};

			for (const auto& s : m_Character)
				ListStr += s + ", ";

			ListStr.pop_back();
			ListStr.pop_back();

			fmt::print("\t{0: <{2}}{1}\n", "Characters", ListStr, SPACES_COUNT);
		}

		if (!m_ExcludeCultures.empty())
		{
			string ListStr{};

			for (const auto& s : m_ExcludeCultures)
				ListStr += s + ", ";

			ListStr.pop_back();
			ListStr.pop_back();

			fmt::print("\t{0: <{2}}{1}\n", "ExcludeCultures", ListStr, SPACES_COUNT);
		}

		if (m_NoGoingBackLevel >= 0)
		{
			fmt::print("\t{0: <{2}}{1}\n", "NoGoingBackLevel", m_NoGoingBackLevel, SPACES_COUNT);
		}

		if (m_Hidden)
		{
			fmt::print("\tHidden\n");
		}

		if (!m_AntiTraits.empty())
		{
			string ListStr{};

			for (const auto& AntiTrait : m_AntiTraits)
			{
				try
				{
					ListStr += std::get<string>(AntiTrait) + ", ";
				}
				catch (...)
				{
					try
					{
						ListStr += std::get<const Trait_t*>(AntiTrait)->m_Name + ", ";
					}
					catch (...)
					{
						ListStr = "ERROR";
						fmt::print(fg(fmt::color::dark_red), "ERROR: YOU SHOULD NEVER SEE THIS!");
					}
				}
			}

			ListStr.pop_back();
			ListStr.pop_back();

			fmt::print("\t{0: <{2}}{1}\n", "AntiTraits", ListStr, SPACES_COUNT);
		}

		for (const auto& Level : m_Levels)
		{
			fmt::print(Level.GetFormatStyle(), "\n\tLevel {}\n", Level.m_Name);

			if (!Level.m_Description.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "Description", Level.m_Description, SPACES_COUNT2);
			}

			if (!Level.m_EffectsDescription.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "EffectsDescription", Level.m_EffectsDescription, SPACES_COUNT2);
			}

			if (!Level.m_GainMessage.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "GainMessage", Level.m_GainMessage, SPACES_COUNT2);
			}

			if (!Level.m_LoseMessage.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "LoseMessage", Level.m_LoseMessage, SPACES_COUNT2);
			}

			if (!Level.m_Epithet.empty())
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "Epithet", Level.m_Epithet, SPACES_COUNT2);
			}

			if (Level.m_Threshold)
			{
				fmt::print("\t\t{0: <{2}}{1}\n", "Threshold", Level.m_Threshold, SPACES_COUNT2);
			}

			for (const auto& Effect : Level.m_Effects)
			{
				fmt::print(Effect.GetFormatStyle(), "\t\t{0: <{1}}{2} {3}\n", "Effect", SPACES_COUNT2, Effect.m_Type, Effect.m_Modification);
			}
		}

		fmt::print("\n;------------------------------------------\n");
	}
};