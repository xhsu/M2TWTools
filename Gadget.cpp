#include <fmt/color.h>
#include <fmt/std.h>

#include <experimental/generator>
#include <ranges>

#include "Modules/battle_models.hpp"
#include "Modules/export_descr_unit.hpp"

namespace fs = std::filesystem;

using namespace std;
using namespace std::experimental;
using namespace std::literals;

extern generator<string_view> Split(string_view sz, string_view delimiters = ", \n\f\v\t\r"sv) noexcept;

void ReadAllCommand(const char* pszPath = R"(C:\Users\xujia\Downloads\EBII_noninstaller\mods\ebii\data\export_descr_buildings.txt)") noexcept
{
	if (auto f = fopen(pszPath, "rb"))
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

inline auto GetRoaster(Units::CFile const& edu, const char* Faction) noexcept
{
	using namespace Units;

	return edu
		| std::views::filter([&](CUnit const& unit) noexcept { return std::ranges::contains(unit.at("ownership"), Faction); })
		| std::views::filter([&](CUnit const& unit) noexcept { return !std::ranges::contains(unit.at("attributes"), "mercenary_unit"); })
		| std::views::transform([](CUnit const& unit) noexcept -> CUnit const* { return &unit; })
		| std::ranges::to<vector>();
}

inline auto CrossRef(vector<Units::CUnit const*> const& lhs, vector<Units::CUnit const*> const& rhs) noexcept
{
	using namespace Units;

	set<CUnit const*> ret{};

	for (auto&& pUnit : lhs)
	{
		auto& szName = pUnit->at("type")[0];

		if (std::ranges::find_if(rhs, [&](CUnit const* elem) noexcept { return elem->at("type")[0] == szName; }) == rhs.end())
			ret.emplace(pUnit);
	}

	//for (auto&& pUnit : rhs)
	//{
	//	auto& szName = pUnit->at("type")[0];

	//	if (std::ranges::find_if(lhs, [&](CUnit const* elem) noexcept { return elem->at("type")[0] == szName; }) == lhs.end())
	//		ret.emplace(pUnit);
	//}

	return ret;
}

inline auto CrossRef(Units::CFile const& lhs, Units::CFile const& rhs) noexcept
{
	using namespace Units;

	//set<CUnit const*> ret{};

	//for (auto&& unit : lhs)
	//{
	//	auto& szName = unit.at("type")[0];

	//	if (std::ranges::find_if(rhs, [&](CUnit const& elem) noexcept { return elem.at("type")[0] == szName; }) == rhs.end())
	//		ret.emplace(&unit);
	//}

	return lhs
		| std::views::filter([&](CUnit const& elem) noexcept -> bool { return !std::ranges::contains(rhs, elem.at("type")[0], [](CUnit const& r_el) noexcept { return r_el.at("type")[0]; }); })
		| std::views::transform([](CUnit const& elem) noexcept { return &elem; })
		| std::ranges::to<set>();
}

inline auto ExtractFactions(set<Units::CUnit const*> const& Roaster) noexcept
{
	set<string> ret{};

	for (auto&& pUnit : Roaster)
		ret.insert_range(pUnit->at("ownership"));

	return ret;
}

set<string> AssembleUnitUIFiles(fs::path const& DataPath, string_view szDic) noexcept
{
	set<string> ret{};
	auto const fnToLower = [](char c) noexcept -> char { return std::tolower(c); };

	for (auto&& Entry : fs::recursive_directory_iterator(DataPath / "UI"))
	{
		if (fs::path Path{ Entry }; std::ranges::contains_subrange(
			Path.filename().u8string(),
			szDic,
			{},
			fnToLower,
			fnToLower
		))
		{
			ret.emplace(fs::relative(Path, DataPath).u8string());
		}
	}

	return ret;
}

void CopyBattleModel(const char* pszModelDbFrom, const char* pszModelDbTo, string_view szFaction) noexcept
{
	using namespace BattleModels;

	CFile DestinationModelDB(pszModelDbTo);
	CFile SourceModelDB(pszModelDbFrom);

	fs::path const SourceUnitModelsFolder = fs::path{ pszModelDbFrom }.parent_path();
	fs::path const SourceDataFolder = SourceUnitModelsFolder.parent_path();

	fs::path const DestinationUnitModelsFolder = fs::path{ pszModelDbTo }.parent_path();
	fs::path const DestinationDataFolder = DestinationUnitModelsFolder.parent_path();

	auto ModelsToCopy = SourceModelDB.m_rgBattleModels
		| std::views::filter([](auto&& Model) noexcept { return !Model.m_szName.starts_with("dummy_"); })
		| std::views::filter([&](auto&& Model) noexcept { return std::ranges::contains(Model.m_UnitTex | std::views::keys, szFaction); })
		| std::views::transform([](auto&& obj) noexcept { return std::addressof(obj); })
		| std::ranges::to<vector>();

	// representing all files that norway's unit will need.
	set<fs::path> files{};

	// Patching battle_models file.
	for (auto&& Model : DestinationModelDB.m_rgBattleModels)
	{
		auto const it = std::ranges::find(ModelsToCopy, Model.m_szName, [](CBattleModel* p) noexcept { return p->m_szName; });

		// model name is not in list.
		if (it == ModelsToCopy.end())
			continue;

		auto const Source = *it;

		if (auto it2 = std::ranges::find(Source->m_UnitTex, szFaction, [](auto&& val) noexcept { return val.first; }); it2 != Source->m_UnitTex.end())
		{
			Model.m_UnitTex.emplace_back(*it2);
			files.insert_range(it2->second.ListOfFiles(SourceDataFolder));
			fmt::print("Patched unit tex ");
		}

		if (auto it2 = std::ranges::find(Source->m_AttachmentTex, szFaction, [](auto&& val) noexcept { return val.first; }); it2 != Source->m_AttachmentTex.end())
		{
			Model.m_AttachmentTex.emplace_back(*it2);
			files.insert_range(it2->second.ListOfFiles(SourceDataFolder));
			fmt::print("and attachment tex ");
		}

		fmt::print("into <{}>.\n", Source->m_szName);
		ModelsToCopy.erase(it);
	}

	if (!ModelsToCopy.empty())
	{
		fmt::print(fg(fmt::color::light_golden_rod_yellow), "NorwayLib1: {} left.\n", ModelsToCopy.size());

		for (auto&& pModel : ModelsToCopy)
			fmt::print("Model: <{}> for [{}]\n", pModel->m_szName, fmt::join(pModel->m_UnitTex | std::views::keys, ", "));

		fmt::print(fg(fmt::color::lawn_green), "Inserting entries above this line in.\n");

		for (auto&& pModel : ModelsToCopy)
		{
			DestinationModelDB.m_rgBattleModels.emplace_back(*pModel);
			files.insert_range(pModel->ListOfFiles(SourceDataFolder));
		}
	}

	DestinationModelDB.Save(pszModelDbTo);

	for (auto&& file : files)
	{
		fs::path const to = DestinationDataFolder / fs::relative(file, SourceDataFolder);

		if (fs::exists(to))
			fmt::print(fg(fmt::color::gray), "[Message] Skipping existing file: {}\n", to);

		if (fs::exists(file))
		{
			std::error_code ec{};
			fs::create_directories(to.parent_path());
			fs::copy(file, to, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);

			if (ec)
				fmt::print(fg(fmt::color::red), "[Error] std::error_code '{}': {}\n", ec.value(), ec.message());
		}
		else
			fmt::print(fg(fmt::color::red), "[Error] FILE no found: {}\n", file.u8string());
	}
}
