#include "descr_mount.hpp"

#include <fmt/color.h>

#include <assert.h>

#include <ranges>
#include <set>

using namespace Mount;
using namespace std;

inline constexpr auto KEYWORD_WIDTH = 20;

#pragma region File Introduction
inline constexpr char INTRODUCTION[] =
R"(;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	This file contains the mount linkages; it gets parsed on application
;	startup but is not otherwise referred to. The data format is thus:
;
;	Fields should be entered in the order shown.
;
;	;						indicates a comment ;-)
;	/ /						indicates a section
;	[]						indicates optional
;
;	/generic/
;
;	type					indicates a new mount type, must be followed by id name string
;	class					mount class (current possibilities - horse, camel, elephant)
;	model*					model id from descr_model_battle
;	radius					mount radius
;	[ x_radius ]			mount x axis radius for elliptical mounts (radius is y radius in this case)
;   [ y_offset ]			mount y offset for elliptical mounts
;	height					mount height
;	mass					mount mass
;	banner_height			height of banners above mount
;	bouyancy_offset			bouyancy offset of mount above root node
;	water_trail_effect		display effect for moving through water
;
;	/horse, camel or elephant specific/
;
;	root_node_height		height of the horse, camel or elephants root node above the ground
;
;	/horse and camel specific/
;
;	rider_offset			(x, y, z) for the rider relative to horse or camel root node
;
;	/elephant specific/
;
;	attack_delay			delay between mount attacks (tusks and scythes) in seconds
;
;	/elephant specific/
;
;	dead_radius				radius of dead obstacle
;	tusk_z					distance along the z axis of tusks from centre
;	tusk_radius				radius of tusk attack
;	riders					number of riders
;	rider_offset			(x, y, z) for each rider relative to elephant root node
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


)";
#pragma endregion File Introduction

template <typename T>
static inline auto UTIL_StrToNum(string_view sz) noexcept
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

#pragma region Elephent exclusive parser
//#UPDATE_AT_CPP23 static operator()

#define PARSER_OF(x) \
struct elephent_only_##x final\
{\
	auto operator() (auto&&) const noexcept {}\
	auto operator() (CElephant& Mnt) const noexcept { Mnt.m_##x = m_arg; }\
	float m_arg{};\
}

PARSER_OF(attack_delay);
PARSER_OF(dead_radius);
PARSER_OF(tusk_z);
PARSER_OF(tusk_radius);

#undef PARSER_OF
#pragma endregion Elephent exclusive parser

struct parser_rider_offset final
{
	auto operator() (CElephant& Mnt) const noexcept
	{
		Mnt.m_rgvecRiderOffsets.emplace_back(
			array{
				UTIL_StrToNum<float>(Verses[1]),
				UTIL_StrToNum<float>(Verses[2]),
				UTIL_StrToNum<float>(Verses[3]),
			}
		);
	}

	auto operator() (CHorse& Mnt) const noexcept
	{
		Mnt.m_rider_offset = array{
			UTIL_StrToNum<float>(Verses[1]),
			UTIL_StrToNum<float>(Verses[2]),
			UTIL_StrToNum<float>(Verses[3]),
		};
	}

	vector<string_view> const &Verses;
};

void Mount::CFile::Deserialize() noexcept
{
#define PARSE_STR(x) \
else if (Verses[0] == #x)\
{\
	std::visit(\
		[&](auto&& Mnt) noexcept { Mnt.m_##x = Verses[1]; },\
		*m_ThisMount\
	);\
\
	assert(Verses.size() == 2);\
}

#define PARSE_FLT(x) \
else if (Verses[0] == #x)\
{\
	std::visit(\
		[&](auto&& Mnt) noexcept { Mnt.m_##x = UTIL_StrToNum<float>(Verses[1]); },\
		*m_ThisMount\
	);\
\
	assert(Verses.size() == 2);\
}

#define PARSE_ELEF_FLT(x) \
else if (Verses[0] == #x)\
{\
	std::visit(\
		elephent_only_##x{ UTIL_StrToNum<float>(Verses[1]) },\
		*m_ThisMount\
	);\
\
	assert(Verses.size() == 2);\
}

	decltype(m_Mounts)::value_type::second_type* m_ThisMount{};
	string_view SavedTypeName{};

	for (auto Line = Parse("\r\n"); !Eof(); Line = Parse("\r\n"))
	{
		auto const Verses = UTIL_Split(Line) | std::ranges::to<vector>();

		if (Verses[0] == "type")
		{
			assert(Verses.size() > 1);

			auto const pos1 = Line.find_first_not_of(", \r\n\f\v\t", sizeof("type") - 1);
			SavedTypeName = Line.substr(pos1);
		}
		else if (Verses[0] == "class")
		{
			if (Verses[1] == "camel")
			{
				m_ThisMount = &m_Mounts.try_emplace(
					SavedTypeName,
					CCamel{ .m_type{SavedTypeName}, .m_class{Verses[1]}, }
				).first->second;
			}
			else if (Verses[1] == "horse")
			{
				m_ThisMount = &m_Mounts.try_emplace(
					SavedTypeName,
					CHorse{ .m_type{SavedTypeName}, .m_class{Verses[1]}, }
				).first->second;
			}
			else if (Verses[1] == "elephant")
			{
				m_ThisMount = &m_Mounts.try_emplace(
					SavedTypeName,
					CElephant{ .m_type{SavedTypeName}, .m_class{Verses[1]}, }
				).first->second;
			}
			else
			{
				fmt::print(fg(fmt::color::red), "[Error] Unknow class '{}' of mount '{}'\n", Verses[1], SavedTypeName);
			}

			assert(Verses.size() == 2);
		}

		PARSE_STR(model)
		PARSE_FLT(radius)
		PARSE_FLT(x_radius)
		PARSE_FLT(y_offset)
		PARSE_FLT(height)
		PARSE_FLT(mass)
		PARSE_FLT(banner_height)
		PARSE_FLT(bouyancy_offset)
		PARSE_STR(water_trail_effect)

		PARSE_FLT(root_node_height)

		PARSE_ELEF_FLT(attack_delay)
		PARSE_ELEF_FLT(dead_radius)
		PARSE_ELEF_FLT(tusk_z)
		PARSE_ELEF_FLT(tusk_radius)

		else if (Verses[0] == "riders")
		{
			assert(Verses.size() == 2);
		}
		else if (Verses[0] == "rider_offset")
		{
			std::visit(
				parser_rider_offset{ Verses },
				*m_ThisMount
			);

			assert(Verses.size() == 4);
		}
	}

#undef PARSE_STR
#undef PARSE_FLT
#undef PARSE_ELEF_FLT
}

string Mount::CFile::Serialize() const noexcept
{
	string ret{ INTRODUCTION };

	ret +=
		R"(
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; camels
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

)";

	for (auto&& Mount : m_Mounts
		| std::views::values
		| std::views::filter([](CMount const& mnt) noexcept -> bool { return std::visit([](auto&& m) noexcept { return m.m_class; }, mnt) == "camel"; })
		| std::ranges::to<set>()
		)
	{
		ret += std::visit(
			[](auto&& mnt) noexcept { return mnt.Serialize(); },
			Mount
		);

		ret += '\n';
	}

	ret +=
		R"(
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; horses
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

)";

	for (auto&& Mount : m_Mounts
		| std::views::values
		| std::views::filter([](CMount const& mnt) noexcept -> bool { return std::visit([](auto&& m) noexcept { return m.m_class; }, mnt) == "horse"; })
		| std::ranges::to<set>()
		)
	{
		ret += std::visit(
			[](auto&& mnt) noexcept { return mnt.Serialize(); },
			Mount
		);

		ret += '\n';
	}

	ret +=
		R"(
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; elephants
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

)";

	for (auto&& Mount : m_Mounts
		| std::views::values
		| std::views::filter([](CMount const& mnt) noexcept -> bool { return std::visit([](auto&& m) noexcept { return m.m_class; }, mnt) == "elephant"; })
		| std::ranges::to<set>()
		)
	{
		ret += std::visit(
			[](auto&& mnt) noexcept { return mnt.Serialize(); },
			Mount
		);

		ret += '\n';
	}

	return ret;
}

string_view Mount::CFile::ModelOf(string_view szMountType) const noexcept
{
	if (!m_Mounts.contains(szMountType))
		return "";

	return std::visit(
		[](auto&& mnt) noexcept -> string_view { return mnt.m_model; },
		m_Mounts.at(szMountType)
	);
}

#define SERILIZE(x) ret += fmt::format("{0:<{2}}{1}\n", #x, m_##x, KEYWORD_WIDTH)
#define SERILIZE_OPT(x) if (m_##x) ret += fmt::format("{0:<{2}}{1}\n", #x, *m_##x, KEYWORD_WIDTH)
#define SERILIZE_ARR(x) ret += fmt::format("{0:<{2}}{1}\n", #x, fmt::join(m_##x, ", "), KEYWORD_WIDTH)

string Mount::CHorse::Serialize() const noexcept
{
	string ret{};

	SERILIZE(type);
	SERILIZE(class);
	SERILIZE(model);
	SERILIZE(radius);
	SERILIZE_OPT(x_radius);
	SERILIZE_OPT(y_offset);
	SERILIZE(height);
	SERILIZE(mass);
	SERILIZE(banner_height);
	SERILIZE(bouyancy_offset);
	SERILIZE(water_trail_effect);

	SERILIZE(root_node_height);
	SERILIZE_ARR(rider_offset);

	return ret;
}

string Mount::CElephant::Serialize() const noexcept
{
	string ret{};

	SERILIZE(type);
	SERILIZE(class);
	SERILIZE(model);
	SERILIZE(radius);
	SERILIZE_OPT(x_radius);
	SERILIZE_OPT(y_offset);
	SERILIZE(height);
	SERILIZE(mass);
	SERILIZE(banner_height);
	SERILIZE(bouyancy_offset);
	SERILIZE(water_trail_effect);

	SERILIZE(root_node_height);
	SERILIZE(attack_delay);
	SERILIZE(dead_radius);
	SERILIZE(tusk_z);
	SERILIZE(tusk_radius);

	ret += fmt::format("{0:<{2}}{1}\n", "riders", m_rgvecRiderOffsets.size(), KEYWORD_WIDTH);

	for (auto&& m_rider_offset : m_rgvecRiderOffsets)
		SERILIZE_ARR(rider_offset);

	return ret;
}

#undef SERILIZE
#undef SERILIZE_OPT
#undef SERILIZE_ARR