#include "export_descr_unit.hpp"

#include <assert.h>

#include <experimental/generator>
#include <ranges>
#include <string_view>

#include <fmt/color.h>

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
