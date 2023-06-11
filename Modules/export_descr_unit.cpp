#include "export_descr_unit.hpp"

#include <assert.h>

#include <charconv>
#include <experimental/generator>
#include <ranges>
#include <string_view>

#include <fmt/color.h>
#include <fmt/ranges.h>

using namespace Units;

using std::experimental::generator;
using std::string_view;

static inline generator<string_view> Split(string_view sz, string_view delimiters = ", \n\f\v\t\r") noexcept
{
	for (auto lastPos = sz.find_first_not_of(delimiters, 0), pos = sz.find_first_of(delimiters, lastPos);
		sz.npos != pos || sz.npos != lastPos;
		lastPos = sz.find_first_not_of(delimiters, pos), pos = sz.find_first_of(delimiters, lastPos)
		)
	{
		co_yield sz.substr(lastPos, pos - lastPos);
	}

	co_return;
}

static inline void ReplaceAll(string* str, string_view from, string_view to) noexcept
{
	size_t start_pos = 0;
	while ((start_pos = str->find(from, start_pos)) != str->npos)
	{
		str->replace(start_pos, from.length(), to);
		start_pos += to.length();	// In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

template <typename T>
static inline auto StrToNum(string_view sz) noexcept
{
	if constexpr (std::is_enum_v<T>)
	{
		if (std::underlying_type_t<T> ret{}; std::from_chars(sz.data(), sz.data() + sz.size(), ret).ec == std::errc{})
			return static_cast<T>(ret);
	}
	else
	{
		if (T ret{}; std::from_chars(sz.data(), sz.data() + sz.size(), ret).ec == std::errc{})
			return ret;
	}

	return T{};
}

static consteval size_t strlen_c(const char* str) noexcept
{
	return *str ? 1 + strlen_c(str + 1) : 0;
}

string Units::Serialize(CUnit const& unit) noexcept
{
	static constexpr array SerializeOrder
	{
		"type",
		"dictionary",
		"category",
		"class",
		"voice_type",
		"accent",
		"banner faction",
		"banner holy",

		"soldier",
		"officer",

		"ship",
		"engine",
		"animal",
		"mount_effect",
		"mount",

		"attributes",
		"formation",
		"stat_health",
		"stat_pri_armour",
		"stat_sec_armour",

		"stat_pri_attr",
		"stat_pri_ex",
		"stat_pri",
		"stat_sec_attr",
		"stat_sec_ex",
		"stat_sec",
		"stat_ter_attr",
		"stat_ter_ex",
		"stat_ter",

		"stat_heat",
		"stat_ground",
		"stat_mental",
		"stat_charge_dist",
		"stat_fire_delay",

		"stat_food",

		"stat_cost",
		"stat_stl",
		"armour_ug_levels",
		"armour_ug_models",
		"ownership",
		"era 0",
		"era 1",
		"era 2",
		"recruit_priority_offset",
		"move_speed_mod",
	};

	auto const iLongest = std::ranges::max(
		unit
		| std::views::keys
		| std::views::transform([](auto&& s) noexcept { return s.length(); })
	);

	string ret{};

	for (auto&& key : SerializeOrder)
	{
		if (unit.contains(key))
			ret += fmt::format("{0:<{2}}{1}\n", key, fmt::join(unit.at(key), ", "), iLongest + 1);
	}

	//for (auto&& [key, vals] : unit)
	//{
	//	ret += fmt::format("{0:<{2}}{1}\n", key, fmt::join(vals, ", "), iLongest + 1);
	//}

	return ret;
}

CFile Units::Deserialize(const char* file) noexcept
{
	CFile ret{};

	if (auto f = fopen(file, "rb"); f)
	{
		fseek(f, 0, SEEK_END);
		auto const len = ftell(f);

		auto p = (char*)calloc(sizeof(char), len + 1);
		fseek(f, 0, SEEK_SET);
		fread(p, sizeof(char), len, f);

		fclose(f);
		f = nullptr;

		string_view Script{ p, (size_t)len + 1 };

		for (CUnit* pUnit{}; auto Line : Split(Script, "\n\r"))	// intentional copy.
		{
			if (Line[0] == ';' || Line.length() <= 1)
				continue;

			if (auto const semicol = Line.find_first_of(';'); semicol != Line.npos)
				Line = Line.substr(0, semicol);

			auto rgsz = 
				Split(Line)
				| std::views::transform([](auto&& arg) noexcept -> string { return (string)arg; })
				| std::ranges::to<vector>();

			if (rgsz.size() <= 1)
			{
				fmt::print(fg(fmt::color::red), "ERROR: unknown script command \"{}\"\n", Line);
				continue;
			}

			if (rgsz[0] == "type")
			{
				pUnit = &ret.emplace_back();
				(*pUnit)["type"].emplace_back(fmt::format("{}", fmt::join(rgsz | std::views::drop(1), " ")));

				assert(rgsz.size() >= 2);
			}
			else if (rgsz[0] == "banner")
			{
				(*pUnit)["banner "s + rgsz[1]].emplace_back(std::move(rgsz[2]));

				assert(rgsz.size() == 3);
			}
			else if (rgsz[0] == "officer")
			{
				(*pUnit)["officer"].emplace_back(std::move(rgsz[1]));
				assert(rgsz.size() == 2);
			}
			else if (rgsz[0] == "era")
			{
				(*pUnit)["era "s + rgsz[1]].append_range(rgsz | std::views::drop(2));

				assert(rgsz.size() >= 3);
			}
			else if (pUnit)
			{
				auto &vals = (*pUnit)[rgsz[0]];
				rgsz.erase(rgsz.begin());
				vals = std::move(rgsz);
			}
		}

		free(p);
		p = nullptr;
	}

	return ret;
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
	ReplaceAll(&ret, "true,", "musket_shot_set,");

	return ret;
}

string Units::Serialize(CStatMental const& stat_mental, int16_t iIndent) noexcept
{
	auto ret = fmt::format("{0:<{2}}{1}\n", "stat_mental"sv, fmt::join(stat_mental, ", "), iIndent);
	ReplaceAll(&ret, "true", "lock_morale");

	return ret;
}

CSoldier Units::Deserializer::Soldier(string_view sz) noexcept
{
	auto const rgsz = Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		(string)rgsz[0],
		StrToNum<int32_t>(rgsz[1]),
		StrToNum<int32_t>(rgsz[2]),
		StrToNum<float>(rgsz[3]),
	};
}

vector<CMountEffect> Units::Deserializer::MountEffect(string_view sz) noexcept
{
	vector<CMountEffect> ret{};

	for (auto&& r : Split(sz) | std::views::drop(1) | std::views::chunk(2))
	{
		auto&& it = r.begin();
		auto&& mount = *it; ++it;
		auto&& effect = StrToNum<int32_t>(*it); ++it;	// because C++ does not gurentee the initialize evaluation order.

		assert(it == r.end());

		ret.emplace_back(
			CMountEffect{ (string)mount, effect }
		);
	}

	return ret;
}

CFormation Units::Deserializer::Formation(string_view sz) noexcept
{
	auto const rgsz = Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		array{StrToNum<float>(rgsz[0]), StrToNum<float>(rgsz[1])},
		array{StrToNum<float>(rgsz[2]), StrToNum<float>(rgsz[3])},
		StrToNum<int32_t>(rgsz[4]),
		rgsz | std::views::drop(5) | std::views::transform([](auto&& s) noexcept { return (string)s; }) | std::ranges::to<vector>(),
	};
}

CWeaponStat Units::Deserializer::WeaponStat(string_view sz) noexcept
{
	auto const rgsz = Split(sz) | std::views::drop(1) | std::ranges::to<vector>();
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
		StrToNum<uint16_t>(rgsz[0]),
		StrToNum<uint16_t>(rgsz[1]),
		(string)rgsz[2],
		StrToNum<float>(rgsz[3]),
		StrToNum<uint16_t>(rgsz[4]),
		(string)rgsz[5],
		(string)rgsz[6],
		(string)rgsz[7],
		(string)rgsz[8],
		len == 12,
		StrToNum<float>(rgsz[len == 12 ? 10 : 9]),
		StrToNum<int16_t>(rgsz[len == 12 ? 11 : 10]),
	};
}

CPrimaryArmour Units::Deserializer::PrimaryArmour(string_view sz) noexcept
{
	auto const rgsz = Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		StrToNum<uint16_t>(rgsz[0]),
		StrToNum<uint16_t>(rgsz[1]),
		StrToNum<uint16_t>(rgsz[2]),
		(string)rgsz[3],
	};
}

CSecondaryArmour Units::Deserializer::SecondaryArmour(string_view sz) noexcept
{
	auto const rgsz = Split(sz) | std::views::drop(1) | std::ranges::to<vector>();

	return tuple{
		StrToNum<uint16_t>(rgsz[0]),
		StrToNum<uint16_t>(rgsz[1]),
		(string)rgsz[2],
	};
}

CStatMental Units::Deserializer::StatMental(string_view sz) noexcept
{
	auto const rgsz = Split(sz) | std::views::drop_while([](auto&& s) noexcept { return s == "stat_mental"; }) | std::ranges::to<vector>();

	return tuple{
		StrToNum<int16_t>(rgsz[0]),
		(string)rgsz[1],
		(string)rgsz[2],
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
								m_##x = Split(sz) | std::views::transform([](auto&& s) noexcept { return (string)s; }) | std::ranges::to<vector>();	\
							}

#define PARSE_NUMBER(x)	else if (sz.starts_with(#x))		\
						{											\
							sz = sz.substr(strlen_c(#x));			\
							sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
																	\
							m_##x = StrToNum<decltype(m_##x)>(sz);	\
						}

#define PARSE_ARR_NUM(x)	else if (sz.starts_with(#x))	\
							{										\
								sz = sz.substr(strlen_c(#x));		\
								sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
																	\
								for (auto&& [iVal, szVal] : std::views::zip(m_##x, Split(sz)))	\
								{																\
									static_assert(std::is_lvalue_reference_v<decltype(iVal)>);	\
									iVal = StrToNum<std::remove_cvref_t<decltype(iVal)>>(szVal);\
								}																\
							}

#define PARSE_VARY_NUM(x)	else if (sz.starts_with(#x))	\
							{										\
								sz = sz.substr(strlen_c(#x));		\
								sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));	\
																	\
								m_##x =								\
									Split(sz)						\
									| std::views::transform([](string_view const& s) noexcept { return StrToNum<std::remove_cvref_t<std::ranges::range_value_t<decltype(m_##x)>>>(s); })	\
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
		sz = sz.substr(strlen_c("banner"));
		sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));

		if (sz.starts_with("faction"))
		{
			sz = sz.substr(strlen_c("faction"));
			sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));

			m_banner_faction = sz;
		}
		else if (sz.starts_with("holy"))
		{
			sz = sz.substr(strlen_c("holy"));
			sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));

			m_banner_holy = sz;
		}
		else
		{
			fmt::print(fg(fmt::color::red), "[Error] Script command 'banner {}' cannot be parsed.\n", sz);
		}
	}
	else if (sz.starts_with("soldier"))
		m_soldier = Deserializer::Soldier(sz);
	else if (sz.starts_with("officer"))	// "officer" attribute is not separated by comma, but new line.
	{
		sz = sz.substr(strlen_c("officer"));
		sz = sz.substr(sz.find_first_not_of(" \t\n\r\f\v"));

		m_officer.emplace_back(sz);
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
		auto const rgsz = Split(sz) | std::ranges::to<vector>();
		auto const idx = StrToNum<int16_t>(rgsz[1]);

		m_era[idx] = rgsz | std::views::drop(2) | std::views::transform([](auto&& s) noexcept { return (string)s; }) | std::ranges::to<vector>();
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

#define SERIALIZE(x)		fmt::format("{0:<{2}}{1}\n", #x, m_##x, iIndent)
#define SERIALIZE_OBJ(x)	Units::Serialize(m_##x, iIndent)
#define SERIALIZE_RNG(x)	fmt::format("{0:<{2}}{1}\n", #x, fmt::join(m_##x, ", "), iIndent)
#define SERIALIZE_KEY(x)	Units::Serialize(#x, m_##x, iIndent)

	ret += SERIALIZE(type);
	ret += SERIALIZE(dictionary);

	ret += SERIALIZE(category);
	ret += SERIALIZE(class);
	ret += SERIALIZE(voice_type);
	ret += SERIALIZE(accent);
	ret += SERIALIZE(banner_faction);
	ret += SERIALIZE(banner_holy);
	ret += SERIALIZE_OBJ(soldier);

	for (auto&& sz : m_officer)
		ret += fmt::format("{0:<{2}}{1}\n", "officer", sz, iIndent);

	ret += SERIALIZE(ship);
	ret += SERIALIZE(engine);
	ret += SERIALIZE(animal);
	ret += SERIALIZE(mount);
	ret += SERIALIZE_OBJ(mount_effect);
	ret += SERIALIZE_RNG(attributes);
	ret += SERIALIZE_OBJ(formation);
	ret += SERIALIZE_RNG(stat_health);

	ret += SERIALIZE_KEY(stat_pri);
	ret += SERIALIZE_RNG(stat_pri_ex);
	ret += SERIALIZE_RNG(stat_pri_attr);
	ret += SERIALIZE_KEY(stat_sec);
	ret += SERIALIZE_RNG(stat_sec_ex);
	ret += SERIALIZE_RNG(stat_sec_attr);
	ret += SERIALIZE_KEY(stat_ter);
	ret += SERIALIZE_RNG(stat_ter_ex);
	ret += SERIALIZE_RNG(stat_ter_attr);
	ret += SERIALIZE_RNG(stat_pri_armour);
	ret += SERIALIZE_OBJ(stat_sec_armour);
	ret += SERIALIZE(stat_heat);
	ret += SERIALIZE_RNG(stat_ground);
	ret += SERIALIZE_OBJ(stat_mental);
	ret += SERIALIZE(stat_charge_dist);
	ret += SERIALIZE(stat_fire_delay);	// Type unknown #UNDONE
	ret += SERIALIZE_RNG(stat_cost);
	ret += SERIALIZE(stat_stl);	// Number of soldiers needed for unit to count as alive
	ret += SERIALIZE_RNG(armour_ug_levels);
	ret += SERIALIZE_RNG(armour_ug_models);
	ret += SERIALIZE_RNG(ownership);
//	array<vector<string>, 3> m_era{};
	ret += SERIALIZE(recruit_priority_offset);
	ret += SERIALIZE(move_speed_mod);	// Although if you really want to have this value displayed, it should be 1.0f

#undef SERIALIZE
#undef SERIALIZE_OBJ
#undef SERIALIZE_RNG
#undef SERIALIZE_KEY

	return ret;
}

auto ConstructEDU(string_view sz = R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\export_descr_unit.txt)") noexcept
{
	auto f = fopen(sz.data(), "rb");
	fseek(f, 0, SEEK_END);
	auto const len = ftell(f);

	auto const p = (char*)calloc(sizeof(char), len + 1);
	fseek(f, 0, SEEK_SET);
	fread(p, sizeof(char), len, f);

	fclose(f);

	vector<Units::unit_t_ver_2> ret{};

	for (Units::unit_t_ver_2* pUnit{}; auto Line : Split(string_view{ p, (size_t)len }, "\r\n"))
	{
		if (auto pos = Line.find_first_of(';'); pos != Line.npos)
			Line = Line.substr(0, pos);

		if (Line.empty() || Line[0] == ';')
			continue;

		if (Line.starts_with("type"))
			pUnit = &ret.emplace_back();

		pUnit->ParseLine(Line);
	}

	free(p);

	return ret;
}
