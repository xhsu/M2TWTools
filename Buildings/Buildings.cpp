
#include <stdint.h>
#include <stdio.h>

#include <fmt/color.h>

#include <array>
#include <experimental/generator>
#include <functional>
#include <ranges>
#include <set>
#include <string_view>
#include <variant>
#include <vector>

#include "Modules/battle_models.hpp"

using namespace std::literals;

using std::array;
using std::experimental::generator;
using std::set;
using std::string;
using std::string_view;
using std::variant;
using std::vector;

generator<string_view> Split(string_view sz, string_view delimiters = ", \n\f\v\t\r"sv) noexcept
{
	for (auto lastPos = sz.find_first_not_of(delimiters, 0), pos = sz.find_first_of(delimiters, lastPos);
		sz.npos != pos || sz.npos != lastPos;
		lastPos = sz.find_first_not_of(delimiters, pos), pos = sz.find_first_of(delimiters, lastPos)
		)
	{
		co_yield string_view{
			sz.substr(lastPos, pos - lastPos) | std::views::drop_while([](const char c) noexcept { return std::isspace(c); })
		};
	}

	co_return;
}

void ReadAllCommand() noexcept
{
	if (auto f = fopen(R"(C:\Users\xujia\Downloads\EBII_noninstaller\mods\ebii\data\export_descr_buildings.txt)", "rb"))
	{
		fseek(f, 0, SEEK_END);
		auto const len = ftell(f);

		auto const p = (char*)calloc(sizeof(char), len + 1);
		fseek(f, 0, SEEK_SET);
		fread(p, sizeof(char), len, f);

		fclose(f);

		auto const rgszLines =
			Split({ p, (size_t)len + 1 }, "\n\r"sv)
			| std::views::filter([](string_view const& sz) noexcept { return sz.size() > 0 && sz[0] != ';'; })
			| std::ranges::to<vector>();

		set<string> rgszBonuses{};

		for (size_t i = 0; i < rgszLines.size(); ++i)
		{
			if (rgszLines[i][0] == ';')
				continue;

			if (rgszLines[i] == "capability" && rgszLines[i + 1] == "{")
			{
				for (auto j = i + 2; rgszLines[j] != "}" && j < rgszLines.size(); ++j)
				{
					if (rgszLines[j].starts_with("recruit_pool"))
						continue;

					auto Verses = Split(rgszLines[j], ", \t\f\v") | std::ranges::to<vector>();
					if (auto const it = std::ranges::find(Verses, "requires"); it != Verses.end())
						Verses.erase(it, Verses.end());
					if (auto const it = std::ranges::find_if(Verses, [](auto&& sz) noexcept { return sz.starts_with(';'); }); it != Verses.end())
						Verses.erase(it, Verses.end());

					bool bShouldDiscardBack{ true };

					try
					{
						[[maybe_unused]]
						auto const bonus = std::stoi((string)Verses.back());
					}
					catch (...)
					{
						bShouldDiscardBack = false;
					}

					if (bShouldDiscardBack)
						Verses.pop_back();

					rgszBonuses.emplace(
						fmt::format("{}", fmt::join(Verses, " "))
					);
				}
			}
		}

		free(p);
		for (auto&& sz : rgszBonuses)
			fmt::println("{}", sz);
	}
}


int32_t main(int32_t argc, char* argv[]) noexcept
{
	using namespace BattleModels;

	CFile f{ R"(C:\Program Files (x86)\Steam\steamapps\common\Medieval II Total War\mods\MyMod\data\unit_models\battle_models.modeldb)" };

	for (auto&& Model : f.m_rgBattleModels)
	{
		std::erase_if(Model.m_UnitTex, [](auto&& cell) noexcept { return cell.first == "saxons"; });
		std::erase_if(Model.m_AttachmentTex, [](auto&& cell) noexcept { return cell.first == "saxons"; });

		if (Model.m_szName.starts_with("mount_"sv))
			continue;

		if (Model.m_UnitTex.empty())
			fmt::print("{}.m_UnitTex being empty!\n", Model.m_szName);
		if (Model.m_AttachmentTex.empty())
			fmt::print("{}.m_AttachmentTex being empty!\n", Model.m_szName);
	}

	std::erase_if(
		f.m_rgBattleModels,
		[](auto&& Model) noexcept
		{
			return Model.m_UnitTex.empty() && Model.m_AttachmentTex.empty();
		}
	);

	f.Save(R"(C:\Program Files (x86)\Steam\steamapps\common\Medieval II Total War\mods\MyMod\data\unit_models\Out.modeldb)");
}
