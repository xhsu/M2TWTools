#include "export_descr_unit.hpp"

#include <assert.h>

#include <charconv>
#include <ranges>


using namespace Units;

using std::experimental::generator;
using std::string_view;

static consteval size_t strlen_c(const char* str) noexcept
{
	return *str ? 1 + strlen_c(str + 1) : 0;
}

string Units::Serialize(vector<CMountEffect> const& mount_effect, int16_t iIndent) noexcept
{
	return fmt::format("{0:<{1}}{2}\n", "mount_effect"sv, iIndent,
		fmt::join(mount_effect | std::views::transform([](CMountEffect const& arg) noexcept -> string { return fmt::format("{}", fmt::join(arg, " ")); }), ", ")
	);
}

string Units::Serialize(CFormation const& formation, int16_t iIndent) noexcept
{
	return fmt::format("{0:<{1}}{2}, {3}, {4}, {5}\n",
		"formation"sv,
		iIndent,
		fmt::join(std::get<0>(formation), ", "),
		fmt::join(std::get<1>(formation), ", "),
		std::get<2>(formation),
		fmt::join(std::get<3>(formation), ", ")
	);
}

string Units::Serialize(string_view key, CWeaponStat const& wpnstat, int16_t iIndent) noexcept
{
	auto ret = fmt::format("{0:<{2}}{1}\n", key, fmt::join(wpnstat, ", "), iIndent);
	UTIL_Replace(&ret, "true,", "musket_shot_set,");

	return ret;
}

string Units::Serialize(CStatMental const& stat_mental, int16_t iIndent) noexcept
{
	auto ret = fmt::format("{0:<{2}}{1}\n", "stat_mental"sv, fmt::join(stat_mental, ", "), iIndent);
	UTIL_Replace(&ret, "true", "lock_morale");
	UTIL_Replace(&ret, ", false", "");

	return ret;
}

CSoldier Units::Deserializer::Soldier(string_view sz) noexcept
{
	auto const rgsz = UTIL_Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		rgsz[0],
		UTIL_StrToNum<int32_t>(rgsz[1]),
		UTIL_StrToNum<int32_t>(rgsz[2]),
		UTIL_StrToNum<float>(rgsz[3]),
	};
}

vector<CMountEffect> Units::Deserializer::MountEffect(string_view sz) noexcept
{
	vector<CMountEffect> ret{};

	for (auto&& r : UTIL_Split(sz) | std::views::drop(1) | std::views::chunk(2))
	{
		auto it = r.begin();
		auto mount = *it; ++it;
		auto effect = UTIL_StrToNum<int32_t>(*it); ++it;	// because C++ does not gurentee the initialize evaluation order.

		assert(it == r.end());

		ret.emplace_back(
			CMountEffect{ mount, effect }
		);
	}

	return ret;
}

CFormation Units::Deserializer::Formation(string_view sz) noexcept
{
	auto const rgsz = UTIL_Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		array{UTIL_StrToNum<float>(rgsz[0]), UTIL_StrToNum<float>(rgsz[1])},
		array{UTIL_StrToNum<float>(rgsz[2]), UTIL_StrToNum<float>(rgsz[3])},
		UTIL_StrToNum<int32_t>(rgsz[4]),
		rgsz | std::views::drop(5) | std::ranges::to<vector>(),
	};
}

CWeaponStat Units::Deserializer::WeaponStat(string_view sz) noexcept
{
	auto const rgsz = UTIL_Split(sz) | std::views::drop(1) | std::ranges::to<vector>();
	auto const len = rgsz.size();

	[[unlikely]]
	if (len != 11 && len != 12)
	{
		fmt::print(fg(fmt::color::red), "[Error] Script command stat_[pri/sec/ter] received {} arguments.\n", len);
		fmt::print(fg(fmt::color::light_gray), "[Message] Script command stat_[pri/sec/ter] expected to have 11 arguments for regular troops, or 12 arguments for gunpowder units.\n");
		fmt::print(fg(fmt::color::light_gray), "[Message] Error occurs during parsing: '{}'\n", sz);

		if (len < 11)
			return {};
	}

	return tuple
	{
		UTIL_StrToNum<uint16_t>(rgsz[0]),
		UTIL_StrToNum<uint16_t>(rgsz[1]),
		rgsz[2],
		UTIL_StrToNum<float>(rgsz[3]),
		UTIL_StrToNum<uint16_t>(rgsz[4]),
		rgsz[5],
		rgsz[6],
		rgsz[7],
		rgsz[8],
		len == 12,
		UTIL_StrToNum<float>(rgsz[len == 12 ? 10 : 9]),
		UTIL_StrToNum<int16_t>(rgsz[len == 12 ? 11 : 10]),
	};
}

CPrimaryArmour Units::Deserializer::PrimaryArmour(string_view sz) noexcept
{
	auto const rgsz = UTIL_Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		UTIL_StrToNum<uint16_t>(rgsz[0]),
		UTIL_StrToNum<uint16_t>(rgsz[1]),
		UTIL_StrToNum<uint16_t>(rgsz[2]),
		rgsz[3],
	};
}

CSecondaryArmour Units::Deserializer::SecondaryArmour(string_view sz) noexcept
{
	auto const rgsz = UTIL_Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		UTIL_StrToNum<uint16_t>(rgsz[0]),
		UTIL_StrToNum<uint16_t>(rgsz[1]),
		rgsz[2],
	};
}

CStatMental Units::Deserializer::StatMental(string_view sz) noexcept
{
	auto const rgsz = UTIL_Split(sz) | std::views::drop_while([](auto&& s) noexcept { return s == "stat_mental"; }) | std::ranges::to<vector>();

	return tuple{
		UTIL_StrToNum<int16_t>(rgsz[0]),
		rgsz[1],
		rgsz[2],
		rgsz.size() > 3,
	};
}

void Units::unit_t_ver_2::ParseLine(string_view sz) noexcept
{
	if (false)
		return;

#define PARSE_STRING(x)	else if (sz.starts_with(#x))	\
						{										\
							sz = sz.substr(strlen_c(#x));		\
							sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
							m_##x = sz;							\
						}

#define PARSE_VARY_STR(x)	else if (sz.starts_with(#x))	\
							{										\
								sz = sz.substr(strlen_c(#x));		\
								sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
																	\
								m_##x = UTIL_Split(sz) | std::ranges::to<vector>();	\
							}

#define PARSE_NUMBER(x)	else if (sz.starts_with(#x))		\
						{											\
							sz = sz.substr(strlen_c(#x));			\
							sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
																	\
							m_##x = UTIL_StrToNum<decltype(m_##x)::value_type>(sz);	\
						}

#define PARSE_ARR_NUM(x)	else if (sz.starts_with(#x))	\
							{										\
								sz = sz.substr(strlen_c(#x));		\
								sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
																	\
								m_##x.emplace();					\
																	\
								for (auto&& [iVal, szVal] : std::views::zip(*m_##x, UTIL_Split(sz)))	\
								{																\
									static_assert(std::is_lvalue_reference_v<decltype(iVal)>);	\
									iVal = UTIL_StrToNum<std::remove_cvref_t<decltype(iVal)>>(szVal);\
								}																\
							}

#define PARSE_VARY_NUM(x)	else if (sz.starts_with(#x))	\
							{										\
								sz = sz.substr(strlen_c(#x));		\
								sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
																	\
								m_##x =								\
									UTIL_Split(sz)						\
									| std::views::transform([](string_view const& s) noexcept { return UTIL_StrToNum<std::remove_cvref_t<std::ranges::range_value_t<decltype(m_##x)::value_type>>>(s); })	\
									| std::ranges::to<vector>();	\
							}

	PARSE_STRING(type)
	PARSE_STRING(dictionary)
	PARSE_STRING(category)
	PARSE_STRING(class)
	PARSE_STRING(voice_type)
	PARSE_STRING(accent)

	else if (sz.starts_with("banner"))
	{
		auto const Verses = UTIL_Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

		assert(Verses.size() == 2);

		if (!m_banner)
			m_banner.emplace();

		m_banner->emplace_back(array{ Verses[0], Verses[1] });
	}
	else if (sz.starts_with("soldier"))
		m_soldier = Deserializer::Soldier(sz);
	else if (sz.starts_with("officer"))	// "officer" attribute is not separated by comma, but new line.
	{
		sz = sz.substr(strlen_c("officer"));
		sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));

		if (!m_officer)
			m_officer.emplace();

		m_officer->emplace_back(sz);
	}

	PARSE_STRING(ship)
	PARSE_STRING(engine)
	PARSE_STRING(animal)

	else if (sz.starts_with("mount_effect"))
		m_mount_effect = Deserializer::MountEffect(sz);

	PARSE_STRING(mount)
	PARSE_VARY_STR(attributes)

	else if (sz.starts_with("formation"))
		m_formation = Deserializer::Formation(sz);

	PARSE_ARR_NUM(stat_health)

	else if (sz.starts_with("stat_pri_armour"))
		m_stat_pri_armour = Deserializer::PrimaryArmour(sz);
	else if (sz.starts_with("stat_sec_armour"))
		m_stat_sec_armour = Deserializer::SecondaryArmour(sz);

	PARSE_VARY_STR(stat_pri_attr)
	PARSE_ARR_NUM(stat_pri_ex)
	else if (sz.starts_with("stat_pri"))
		m_stat_pri = Deserializer::WeaponStat(sz);

	PARSE_VARY_STR(stat_sec_attr)
	PARSE_ARR_NUM(stat_sec_ex)
	else if (sz.starts_with("stat_sec"))
		m_stat_sec = Deserializer::WeaponStat(sz);

	PARSE_VARY_STR(stat_ter_attr)
	PARSE_ARR_NUM(stat_ter_ex)
	else if (sz.starts_with("stat_ter"))
		m_stat_ter = Deserializer::WeaponStat(sz);

	PARSE_NUMBER(stat_heat)
	PARSE_ARR_NUM(stat_ground)
	else if (sz.starts_with("stat_mental"))
		m_stat_mental = Deserializer::StatMental(sz);
	PARSE_NUMBER(stat_charge_dist)
	PARSE_NUMBER(stat_fire_delay)

	else if (sz.starts_with("stat_food"))
		return;	// Obsolete

	PARSE_ARR_NUM(stat_cost)
	PARSE_NUMBER(stat_stl)
	PARSE_VARY_NUM(armour_ug_levels)
	PARSE_VARY_STR(armour_ug_models)
	PARSE_VARY_STR(ownership)

	else if (sz.starts_with("era"))
	{
		auto const rgsz = UTIL_Split(sz) | std::ranges::to<vector>();
		auto const idx = UTIL_StrToNum<int16_t>(rgsz[1]);

		if (!m_era)
			m_era.emplace();

		m_era->at(idx) = rgsz | std::views::drop(2) | std::ranges::to<vector>();
	}

	PARSE_NUMBER(recruit_priority_offset)
	PARSE_NUMBER(move_speed_mod)

#undef PARSE_STRING
#undef PARSE_VARY_STR
#undef PARSE_NUMBER
#undef PARSE_ARR_NUM
#undef PARSE_VARY_NUM

	else
	{
		fmt::print(fg(fmt::color::red), "[Error] Script command '{}' cannot be parsed.\n", sz);
	}
}

string Units::unit_t_ver_2::Serialize() const noexcept
{
	static constexpr auto iIndent = 25;

	string ret{};

#define SERIALIZE(x)		if (m_##x) ret += fmt::format("{0:<{2}}{1}\n", #x, *m_##x, iIndent)
#define SERIALIZE_OBJ(x)	if (m_##x) ret += Units::Serialize(*m_##x, iIndent)
#define SERIALIZE_RNG(x)	if (m_##x) ret += fmt::format("{0:<{2}}{1}\n", #x, fmt::join(*m_##x, ", "), iIndent)
#define SERIALIZE_KEY(x)	if (m_##x) ret += Units::Serialize(#x, *m_##x, iIndent)

	SERIALIZE(type);
	SERIALIZE(dictionary);

	SERIALIZE(category);
	SERIALIZE(class);
	SERIALIZE(voice_type);
	SERIALIZE(accent);

	if (m_banner)
		for (auto&& args : *m_banner)
			ret += fmt::format("{0:<{2}}{1}\n", fmt::format("banner {}", args[0]), args[1], iIndent);

	SERIALIZE_OBJ(soldier);

	if (m_officer)
		for (auto&& sz : *m_officer)
			ret += fmt::format("{0:<{2}}{1}\n", "officer", sz, iIndent);

	SERIALIZE(ship);
	SERIALIZE(engine);
	SERIALIZE(animal);
	SERIALIZE(mount);
	SERIALIZE_OBJ(mount_effect);
	SERIALIZE_RNG(attributes);
	SERIALIZE_OBJ(formation);
	SERIALIZE_RNG(stat_health);

	SERIALIZE_KEY(stat_pri);
	SERIALIZE_RNG(stat_pri_ex);
	SERIALIZE_RNG(stat_pri_attr);
	SERIALIZE_KEY(stat_sec);
	SERIALIZE_RNG(stat_sec_ex);
	SERIALIZE_RNG(stat_sec_attr);
	SERIALIZE_KEY(stat_ter);
	SERIALIZE_RNG(stat_ter_ex);
	SERIALIZE_RNG(stat_ter_attr);
	SERIALIZE_RNG(stat_pri_armour);
	SERIALIZE_OBJ(stat_sec_armour);
	SERIALIZE(stat_heat);
	SERIALIZE_RNG(stat_ground);
	SERIALIZE_OBJ(stat_mental);
	SERIALIZE(stat_charge_dist);
	SERIALIZE(stat_fire_delay);	// Type unknown #UNDONE
	SERIALIZE_RNG(stat_cost);
	SERIALIZE(stat_stl);	// Number of soldiers needed for unit to count as alive
	SERIALIZE_RNG(armour_ug_levels);
	SERIALIZE_RNG(armour_ug_models);
	SERIALIZE_RNG(ownership);

	if (m_era)
	{
		for (auto&& [i, era] : std::views::enumerate(*m_era))
		{
			if (era.empty())
				continue;

			ret += fmt::format(
				"{0:<{2}}{1}\n",
				fmt::format("era {}", i),
				fmt::join(era, ", "),
				iIndent
			);
		}
	}

	SERIALIZE(recruit_priority_offset);
	SERIALIZE(move_speed_mod);	// Although if you really want to have this value displayed, it should be 1.0f

#undef SERIALIZE
#undef SERIALIZE_OBJ
#undef SERIALIZE_RNG
#undef SERIALIZE_KEY

	return ret;
}

void Units::CEDU::Deserialize() noexcept
{
	Units::unit_t_ver_2* pUnit{};

	for (auto Line = Peek("\r\n"); !Eof() && Line.length() > 0; Line = Peek("\r\n"))
	{
		if (Line.starts_with("type"))
		{
#ifdef _DEBUG
			assert(Peek() == "type");
#endif
			Line = Peek("\r\n");	// save the entire line.

			Parse();	// drop 'type'

			auto const szName = Parse("\r\n");
			pUnit = &m_Info.try_emplace(szName, Units::unit_t_ver_2{.m_type{szName} }).first->second;
		}
		else
			pUnit->ParseLine(Parse("\r\n"));
	}
}

string Units::CEDU::Serialize() const noexcept
{
	string ret{
	R"(; -- Data entries are as follows
;
; Type          The internal name of the unit. Note this not necessarily the same as the on screen name
; dictionary    The tag used to look up the on screen name
;
; -- Category and class define the rough type of the unit. They're used for setting some default attributes and for
;       determining where units go in formation amongst other things such as tags to support AI army formation
; category      infantry, cavalry, siege, handler, ship or non_combatant
; class         light, heavy, missile or spearmen
;
; voice_type    Used to determine the type of voice used by the unit
;
; soldier       Name of the soldier model to use (from descr_models_battle.txt)
;               followed by the number of ordinary soldiers in the unit
;               followed by the number of extras (pigs dogs, elephants, chariots artillery pieces etc attached to the unit)
;               followed by the collision mass of the men. 1.0 is normal. [Only applies to infantry]
; officer       Name of officer model. There may be up to 0-3 officer lines per unit
; ship          Type of ship used if applicable
; engine        Type of siege engine used by unit
; animal        The type of (non ridden) animals used by the unit
; mount         Type of animal or vehicle ridden on
;
; mount_effect  Factors to add when in combat against enemy units that have the specified mounts
;               Up to three factors may be specified, which may be classes of mount, or specific types
;
; attributes    A miscellanious list of attributes and abilities the unit may have. Including
;               sea_faring = can board ships; can_swim = can swim across rivers
;               hide_forest, hide_improved_forest, hide_anywhere   = defines where the unit can hide
;               can_sap = Can dig tunnels under walls
;               frighten_foot, frighten_mounted = Cause fear to certain nearby unit types
;               can_run_amok = Unit may go out of control when riders lose control of animals
;               general_unit = The unit can be used for a named character's bodyguard
;               cantabrian_circle = The unit has this special ability
;               no_custom = The unit may not be selected in custom battles
;               command = The unit carries a legionary eagle, and gives bonuses to nearby units
;               mercenary_unit = The unit is s mercenary unit available to all factions
;               is_peasant = unknown
;               druid = Can do a special morale raising chant
;               power_charge = unkown
;               free_upkeep_unit = Unit can be supported free in a city
;
; formation     soldier spacing (in metres) side to side, then front to back for close formation
;               followed by the same measurements in loose formation.
;               followed by the default number of ranks for the unit
;               followed by the formations possible for the unit. One or two of
;               square, horde, schiltrom, shield_wall, phalanx, testudo, or wedge
;
; stat_health   Hit points of man, followed by hit points of mount or attached animal (if applicable)
;               Ridden horses and camels do not have separate hit points
;
; -- Details of unit's primary weapon. If the unit has a missile weapon it must be the primary
; stat_pri      From left to right
;               attack factor
;               attack bonus factor if charging
;               missile type fired (no if not a missile weapon type)
;               range of missile
;               amount of missile ammunition per man
;               Weapon type = melee, thrown, missile, or siege_missile
;               Tech type = simple, other, blade, archery or siege
;               Damage type = piercing, blunt, slashing or fire. (I don't think this is used anymore)
;               Sound type when weapon hits = none, knife, mace, axe, sword, or spear
;               Optional. Name of effect to play when weapon fires
;               Min delay between attacks (in 1/10th of a second)
;               Skeleton compensation factor in melee. Should be 1
; stat_pri_ex   Optional. attack bonus vs mounted, defence bonus vs mounted, armour penetration
; stat_pri_attr
;               primary weapon attributes any or all of
;                   ap = armour piercing. Only counts half of target's armour
;                   bp = body piercing. Missile can pass through men and hit those behind
;                   spear = Used for long spears. Gives bonuses fighting cavalry, and penalties against infantry
;                   long_pike = Use very long pikes. Phalanx capable units only
;                   short_pike = Use shorter than normal spears.
;                   prec = Missile weapon is only thrown/ fired just before charging into combat
;                   thrown = The missile type if thrown rather than fired
;                   launching = attack may throw target men into the air
;                   area = attack affects an area, not just one man
;                   spear & light_spear = The unit when braced has various protecting mechanisms versus cavalry charges from the front
;                   spear_bonus_x = attack bonus against cavalry. x = 2, 4, 6, 8, 10 or 12
;
; -- Details of secondary weapons. If the unit rides on, or has attached animals or vehicles
;       then the secondary weapon details refer to their attacks. If the unit has missile weapons
;       the secondary weapon will be the one used for melee
;       If the unit has a primary melee weapon, it may have a secondary side arm
; stat_sec      As per stat_pri     (a value of no means no secondary weapon)
; stat_sec_ex   as per stat_pri_ex
; stat_sec_attr As per stat_pri_attr
; stat_ter      As per stat_pri If there is no ternary weapon, then there are no ternary data entries
; stat_ter_ex   as per stat_pri_ex
; stat_ter_attr As per stat_pri_attr
;
; stat_pri_armour   Details of the man's defences
;                   armour factor
;                   defensive skill factor (not used when shot at)
;                   shield factor (only used for attacks from the front of left)
;                   sound type when hit = flesh, leather, or metal
; stat_armour_ex    Details of the man's defences
;                   armour factor. 4 values. First for base value, then 3 upgrade levels
;                   defensive skill factor (not used when shot at)
;                   shield factors. First for melee, second for missile fire
;                   sound type when hit = flesh, leather, or metal
;
; stat_sec_armour   Details of animal's or vehicle's defenses (note riden horses do not have a separate defence)
;                   As per stat_pri_armour, except that the shield entry is ommited
;
; stat_heat         Extra fatigue suffered by the unit in hot climates
;
; stat_ground       Combat modifiers on different ground types. From left to right
;                   scrub, sand, forest, snow
; stat_mental       The base morale level, followed by discipline and training
;                   discipline may be normal, low, disciplined or impetuous. Impetuous units may charge without orders
;                   training determines how tidy the unit's formation is. Discipline the response to morale SHOCKS
;                   optional lock_morale stops unit from ever routing
;
; stat_charge_distance Distance from the enemy that the unit will begin charging
; stat_fire_delay   Extra delay over that imposed by animation, hetween volleys
;
; stat_food         No longer used
; stat_cost         Number of turns to build,
;                   Cost of unit to construct
;                   Cost of upkeep
;                   Cost of upgrading weapons
;                   Cost of upgrading armour
;                   Cost for custom battles
; stat_stl          Number of soldiers needed for unit to count as alive
; armour_ug_levels  Smith level needed for each armour upgrade
; armour_ug_models  Body for each upgrade level
; stat_ownership    List of factions and cultures that may have this unit
; era 0,            Optional List of factions that use this in multiplayer era 0
; era 1,            Optional List of factions that use this in multiplayer era 1
; era 2,            Optional List of factions that use this in multiplayer era 2
; info_pic_dir      Optional. Dir to find the info pic in (instead of faction dir)
; card_pic_dir      Optional. Dir to find the unit card in (instead of faction dir)
; unit_info         Info for unit info panel. Melee attack, missile attack, defence


)"
	};

	for (auto&& Unit : m_Info | std::views::values)
	{
		ret += Unit.Serialize();
		ret += "\n\n";
	}

	return ret;
}
