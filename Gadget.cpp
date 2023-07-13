#include <fmt/color.h>
#include <fmt/std.h>

#include <assert.h>

#include <functional>
#include <ranges>

#include "Modules/battle_models.hpp"
#include "Modules/descr_mount.hpp"
#include "Modules/export_descr_sounds_prebattle.hpp"
#include "Modules/export_descr_sounds_soldier_voice.hpp"
#include "Modules/export_descr_sounds_stratmap_voice.hpp"
#include "Modules/export_descr_sounds_units_battle_events.hpp"
#include "Modules/export_descr_sounds_units_voice.hpp"
#include "Modules/export_descr_unit.hpp"

#include "StringBin/StringBin.hpp"

#include "String.hpp"

namespace fs = std::filesystem;

using namespace std;
using namespace std::experimental;
using namespace std::literals;

namespace Gadget
{
	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	template <typename T>
	using Set = std::set<T, CaseIgnoredLess>;

	inline constexpr auto Error = fmt::fg(fmt::color::red);
	inline constexpr auto Info = fmt::fg(fmt::color::gray);
	inline constexpr auto Progress = fmt::fg(fmt::color::light_slate_gray);
	inline constexpr auto Warning = fmt::fg(fmt::color::golden_rod);
}


__forceinline void CopyFiles(std::ranges::input_range auto&& files, fs::path const& SourceFolder, fs::path const& DestFolder) noexcept
{
	for (auto&& file : files)
	{
		auto const from = SourceFolder / file;
		auto const to = DestFolder / file;

		if (fs::exists(to))
		{
			fmt::print(Gadget::Info, "[Message] Skipping existing file: {}\n", to);
			continue;
		}

		if (fs::exists(from))
		{
			std::error_code ec{};
			fs::create_directories(to.parent_path());
			fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);

			if (ec)
				fmt::print(Gadget::Error, "[Error] std::error_code '{}': {}\n", ec.value(), ec.message());
		}
		else
			fmt::print(Gadget::Error, "[Error] FILE no found: {}\n", from.u8string());
	}
}

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
			UTIL_Split({ p, (size_t)len + 1 }, "\n\r"sv)
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

					auto Verses = UTIL_Split(rgszLines[j], ", \t\f\v") | std::ranges::to<vector>();
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

vector<Units::unit_t_ver_2 const*> GetRoaster(Units::CEDU const& edu, string_view szFaction) noexcept
{
	using namespace Units;

	// #TODO reading from descr_sm_faction?
	static const Gadget::Dictionary<string_view, string_view> CultureOf{
		{"antioch"sv,		"northern_european"sv},
		{"byzantium"sv,		"greek"sv},
		{"denmark"sv,		"northern_european"sv},
		{"egypt"sv,			"middle_eastern"sv},
		{"england"sv,		"northern_european"sv},
		{"france"sv,		"northern_european"sv},
		{"hre"sv,			"northern_european"sv},
		{"hungary"sv,		"eastern_european"sv},
		{"jerusalem"sv,		"northern_european"sv},
		{"milan"sv,			"southern_european"sv},
		{"mongols"sv,		"middle_eastern"sv},
		{"moors"sv,			"middle_eastern"sv},
		{"norway"sv,		"northern_european"sv},
		{"papal_states"sv,	"southern_european"sv},
		{"poland"sv,		"eastern_european"sv},
		{"portugal"sv,		"southern_european"sv},
		{"russia"sv,		"eastern_european"sv},
		{"scotland"sv,		"northern_european"sv},
		{"sicily"sv,		"southern_european"sv},
		{"slave"sv,			"southern_european"sv},
		{"spain"sv,			"southern_european"sv},
		{"timurids"sv,		"middle_eastern"sv},
		{"turks"sv,			"middle_eastern"sv},
		{"venice"sv,		"southern_european"sv},
	};

	return edu.m_Info
		| std::views::values
		| std::views::filter([&](unit_t_ver_2 const& unit) noexcept { return unit.m_ownership && (std::ranges::contains(*unit.m_ownership, szFaction) || std::ranges::contains(*unit.m_ownership, CultureOf.at(szFaction))); })
//		| std::views::filter([&](CUnit const& unit) noexcept { return !std::ranges::contains(unit.at("attributes"), "mercenary_unit"); })
		| std::views::transform([](unit_t_ver_2 const& unit) noexcept -> unit_t_ver_2 const* { return &unit; })
		| std::ranges::to<vector>();
}

set<Units::unit_t_ver_2 const*> CrossRef(vector<Units::unit_t_ver_2 const*> const& lhs, vector<Units::unit_t_ver_2 const*> const& rhs) noexcept
{
	using namespace Units;

	set<unit_t_ver_2 const*> ret{};

	for (auto&& pUnit : lhs)
	{
		if (std::ranges::find_if(rhs, [&](unit_t_ver_2 const* elem) noexcept { return elem->m_type == pUnit->m_type; }) == rhs.end())
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

set<Units::unit_t_ver_2 const*> CrossRef(Units::CEDU const& lhs, Units::CEDU const& rhs) noexcept
{
	using namespace Units;

	//set<CUnit const*> ret{};

	//for (auto&& unit : lhs)
	//{
	//	auto& szName = unit.at("type")[0];

	//	if (std::ranges::find_if(rhs, [&](CUnit const& elem) noexcept { return elem.at("type")[0] == szName; }) == rhs.end())
	//		ret.emplace(&unit);
	//}

	return lhs.m_Info
		| std::views::values
		| std::views::filter([&](unit_t_ver_2 const& elem) noexcept -> bool { return !std::ranges::contains(rhs.m_Info | std::views::values, elem.m_type, [](unit_t_ver_2 const& r_el) noexcept { return r_el.m_type; }); })
		| std::views::transform([](unit_t_ver_2 const& elem) noexcept { return &elem; })
		| std::ranges::to<set>();
}

void ListFactionUnitPriority(fs::path const& edu, string_view szFaction) noexcept
{
	auto const EDU = Units::CEDU{ edu };
	auto const Roaster = GetRoaster(EDU, szFaction);

	auto const iWidth = std::ranges::max(
		Roaster
		| std::views::transform([](auto&& unit) noexcept -> size_t { return unit->m_type->length(); })
	);

	for (auto&& unit : Roaster)
	{
		if (unit->m_recruit_priority_offset)
			fmt::println("unit:{0:*<{1}}{2}", unit->m_type, iWidth, unit->m_recruit_priority_offset);
	}
}

vector<string_view> ToDictionaryNames(vector<Units::unit_t_ver_2 const*> const& Roaster) noexcept
{
	return Roaster
		| std::views::filter([](Units::unit_t_ver_2 const* unit) noexcept { return unit->m_dictionary.has_value(); })
		| std::views::transform([](Units::unit_t_ver_2 const* unit) noexcept { return *unit->m_dictionary; })
		| std::ranges::to<vector>();
}

set<string_view> ExtractFactions(set<Units::unit_t_ver_2 const*> const& Roaster) noexcept
{
	set<string_view> ret{};

	for (auto&& pUnit : Roaster)
		ret.insert_range(*pUnit->m_ownership);

	return ret;
}

set<string> AssembleUnitUIFiles(fs::path const& DataPath, string_view szDic) noexcept
{
	set<string> ret{};

	for (auto&& Entry : fs::recursive_directory_iterator(DataPath / "UI"))
	{
		if (fs::path Path{ Entry }; std::ranges::contains_subrange(
			Path.filename().u8string(),
			szDic,
			{},
			ToLower,
			ToLower
		))
		{
			ret.emplace(fs::relative(Path, DataPath).u8string());
		}
	}

	return ret;
}

void CopyBattleModel(fs::path const& SourceData, fs::path const& DestData, string_view szFaction) noexcept
{
	using namespace BattleModels;

	fs::path const SourceUnitModelsFolder = SourceData / L"unit_models";
	fs::path const DestinationUnitModelsFolder = DestData / L"unit_models";

	CFile const SourceModelDB(SourceUnitModelsFolder / L"battle_models.modeldb");
	CFile DestinationModelDB(DestinationUnitModelsFolder / L"battle_models.modeldb");

	auto ModelsToCopy = SourceModelDB.m_rgBattleModels
		| std::views::values
		| std::views::filter([](auto&& Model) noexcept { return !Model.m_szName.starts_with("dummy_"); })
		| std::views::filter([&](auto&& Model) noexcept { return std::ranges::contains(Model.m_UnitTex | std::views::keys, szFaction); })
		| std::views::transform([](auto&& obj) noexcept { return std::addressof(obj); })
		| std::ranges::to<vector>();

	// representing all files that norway's unit will need.
	set<fs::path> files{};

	// Patching battle_models file.
	for (auto&& Model : DestinationModelDB.m_rgBattleModels | std::views::values)
	{
		auto const it = std::ranges::find(ModelsToCopy, Model.m_szName, [](auto&& p) noexcept { return p->m_szName; });

		// model name is not in list.
		if (it == ModelsToCopy.end())
			continue;

		auto const Source = *it;

		if (Source->m_UnitTex.contains(szFaction))
		{
			Model.m_UnitTex.try_emplace(szFaction, Source->m_UnitTex.at(szFaction));
			files.insert_range(Source->m_UnitTex.at(szFaction).ListOfFiles(SourceData));
			fmt::print(Gadget::Progress, "Patched unit tex ");
		}

		if (Source->m_AttachmentTex.contains(szFaction))
		{
			Model.m_AttachmentTex.try_emplace(szFaction, Source->m_AttachmentTex.at(szFaction));
			files.insert_range(Source->m_AttachmentTex.at(szFaction).ListOfFiles(SourceData));
			fmt::print(Gadget::Progress, "and attachment tex ");
		}

		fmt::print(Gadget::Progress, "into <{}>.\n", Source->m_szName);
		ModelsToCopy.erase(it);
	}

	if (!ModelsToCopy.empty())
	{
		fmt::print(Gadget::Warning, "NorwayLib1: {} left.\n", ModelsToCopy.size());

		for (auto&& pModel : ModelsToCopy)
			fmt::print("Model: <{}> for [{}]\n", pModel->m_szName, fmt::join(pModel->m_UnitTex | std::views::keys, ", "));

		fmt::print(Gadget::Progress, "Inserting entries above this line in.\n");

		for (auto&& pModel : ModelsToCopy)
		{
			DestinationModelDB.m_rgBattleModels.try_emplace(pModel->m_szName, *pModel);
			files.insert_range(pModel->ListOfFiles(SourceData));
		}
	}

	DestinationModelDB.Save(DestinationUnitModelsFolder / L"battle_models.modeldb");

	for (auto&& file : files)
	{
		fs::path const to = DestData / fs::relative(file, SourceData);

		if (fs::exists(to))
		{
			fmt::print(Gadget::Info, "[Message] Skipping existing file: {}\n", to);
			continue;
		}

		if (fs::exists(file))
		{
			std::error_code ec{};
			fs::create_directories(to.parent_path());
			fs::copy(file, to, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);

			if (ec)
				fmt::print(Gadget::Error, "[Error] std::error_code '{}': {}\n", ec.value(), ec.message());
		}
		else
			fmt::print(Gadget::Error, "[Error] FILE no found: {}\n", file.u8string());
	}
}

inline void CopyBattleModel(fs::path const& SourceData, fs::path const& DestData, RangeOfStr auto&& rgszModels) noexcept
{
	using namespace BattleModels;

	fs::path const SourceUnitModelsFolder = SourceData / L"unit_models";
	fs::path const DestinationUnitModelsFolder = DestData / L"unit_models";

	CFile const SourceModelDB(SourceUnitModelsFolder / L"battle_models.modeldb");
	CFile DestinationModelDB(DestinationUnitModelsFolder / L"battle_models.modeldb");

	set<fs::path> files{};

	auto fnContains = [&](auto&& Model) noexcept -> bool
	{
		for (auto&& szModel : rgszModels)
		{
			if (strieql(Model.m_szName, szModel))
				return true;
		}

		return false;
	};

	auto fnNotExistInDest = [&](CBattleModel const& Model) noexcept -> bool
	{
		return !DestinationModelDB.m_rgBattleModels.contains(Model.m_szName);
	};

	for (auto&& szModel : rgszModels)
	{
		if (!SourceModelDB.m_rgBattleModels.contains(szModel))
		{
			fmt::print(Gadget::Error, "[Error] Model def '{}' no found from source file!\n", szModel);
			continue;
		}
		else if (DestinationModelDB.m_rgBattleModels.contains(szModel))
		{
			fmt::print(Gadget::Warning, "[Warning] Model def '{}' already exists in dest file!\n", szModel);
			continue;
		}

		auto&& Model = SourceModelDB.m_rgBattleModels.at(szModel);

		fmt::print(Gadget::Progress, "[Message] Model def '{}' inserted.\n", szModel);

		DestinationModelDB.m_rgBattleModels.try_emplace(szModel, Model);
		files.insert_range(Model.ListOfFiles(SourceData));
	}

	for (auto&& file : files)
	{
		fs::path const to = DestData / fs::relative(file, SourceData);

		if (fs::exists(to))
		{
			fmt::print(Gadget::Info, "[Message] Skipping existing file: {}\n", to);
			continue;
		}

		if (fs::exists(file))
		{
			std::error_code ec{};
			fs::create_directories(to.parent_path());
			fs::copy(file, to, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);

			if (ec)
				fmt::print(Gadget::Error, "[Error] std::error_code '{}': {}\n", ec.value(), ec.message());
		}
		else
			fmt::print(Gadget::Error, "[Error] FILE no found: {}\n", file.u8string());
	}

	DestinationModelDB.Save(DestinationUnitModelsFolder / L"battle_models.modeldb");
}

inline void CopyUnitVoices(fs::path const& SourceData, fs::path const& DestData, RangeOfStr auto&& rgszUnits) noexcept
{
	using namespace Voice::Units;

	fs::path const SourceFile{ SourceData / L"export_descr_sounds_units_voice.txt" };
	fs::path const DestFile{ DestData / L"export_descr_sounds_units_voice.txt" };

	CUnitVoices SourceUnitVoice{ SourceFile };
	CUnitVoices DestUnitVoice{ DestFile };

	for (auto&& szUnit : rgszUnits)
	{
		for (auto&& [pAcc, pClass, pVoc, pEv] : SourceUnitVoice.EveryUnitOf(szUnit))
		{
			fmt::println("[Info] Event: {}/{}/{}/unit {}", pAcc->m_Name, pClass->m_Name, pVoc->m_Name, szUnit);

			if (auto p = DestUnitVoice.At(pAcc->m_Name, pClass->m_Name, pVoc->m_Name); p != nullptr)
			{
				//auto const bExisted = std::ranges::fold_left(
				//	p->m_Events
				//	| std::views::filter([](CEvent const& ev) noexcept -> bool { return ev.m_Type == ev.Unit; })
				//	| std::views::transform([](CEvent const& ev) noexcept -> vector<string_view> const& { return ev.m_Troops; }),
				//	false,
				//	[&](bool prev, vector<string_view> const& rgsz) noexcept -> bool { return prev || std::ranges::contains(rgsz, szUnit); }
				//);

				if (std::ranges::contains(
					p->m_Events
					| std::views::filter([](CEvent const& ev) noexcept -> bool { return ev.m_Type == ev.Unit; })
					| std::views::transform([](CEvent const& ev) noexcept -> vector<string_view> const& { return ev.m_Troops; })
					| std::views::join,
					szUnit)
					)
				{
					fmt::print(Gadget::Warning, "\t[Warning] Unit voice already defined here!\n");
					continue;
				}

				p->m_Events.emplace_back(*pEv);
				fmt::print(Gadget::Info, "\t[Info] Copied.\n");
			}
			else
				fmt::print(Gadget::Error, "\t[Info] Structure no found!\n");
		}
	}

	DestUnitVoice.Save(DestFile);
}

inline Gadget::Set<string_view> FindModelEntriesOfUnits(Units::CEDU const& DescrUnits, RangeOfStr auto&& rgszUnits) noexcept
{
	Gadget::Set<string_view> ret{};

	for (auto&& szUnit : rgszUnits)
	{
		if (!DescrUnits.m_Info.contains(szUnit))
		{
			fmt::print(Gadget::Error, "[Error] Unit '{}' cannot be found in 'export_descr_unit.txt'.\n", szUnit);
			continue;
		}

		auto const pUnit = &DescrUnits.m_Info.at(szUnit);

		ret.emplace(std::get<0>(*pUnit->m_soldier));	// arg 0 of soldier script command: default model for this unit.
		ret.insert_range(*pUnit->m_armour_ug_models);	// all models for its upgraded state.
	}

	return ret;
}

inline generator<string_view> FindMountsModelsOfUnits(Mount::CFile const& MountFile, Units::CEDU const& DescrUnits, RangeOfStr auto&& rgszUnits) noexcept
{
	for (auto&& szUnit : rgszUnits)
	{
		if (!DescrUnits.m_Info.contains(szUnit))
		{
			fmt::print(Gadget::Error, "[Error] Unit '{}' cannot be found in 'export_descr_unit.txt'.\n", szUnit);
			continue;
		}

		auto const pUnit = &DescrUnits.m_Info.at(szUnit);

		if (!pUnit->m_mount)
			continue;

		if (auto&& mdl = MountFile.ModelOf(*pUnit->m_mount); !mdl.empty())
		{
			co_yield mdl;
		}
		else
			fmt::print(Gadget::Error, "[Error] Mount '{}' cannot be found in 'descr_mount.txt'.\n", *pUnit->m_mount);
	}
}

inline void CopyUnitUIFiles(fs::path const& SourceData, fs::path const& DestData, RangeOfStr auto&& rgszUnitDictionaryEntries) noexcept
{
	set<fs::path> SourceFiles{};

	for (auto&& Entry : fs::recursive_directory_iterator(SourceData / "UI"))
	{
		for (auto&& szDic : rgszUnitDictionaryEntries)
		{
			if (fs::path Path{ Entry }; std::ranges::contains_subrange(
				Path.filename().u8string(),
				szDic,
				{},
				ToLower,
				ToLower
			))
			{
				Path = fs::relative(Path, SourceData);
				SourceFiles.emplace(std::move(Path));
			}
		}
	}

	CopyFiles(SourceFiles, SourceData, DestData);
}

inline void CopyUnitStringsBin(fs::path const& SourceData, fs::path const& DestData, RangeOfStr auto&& rgszUnitDictionaryEntries) noexcept
{
	auto const from = SourceData / L"text" / L"export_units.txt.strings.bin";
	auto const to = DestData / L"text" / L"export_units.txt.strings.bin";
	auto const to_txt = DestData / L"text" / L"export_units.txt";

	Gadget::Dictionary<string, string> rgszDest{};
	if (auto f = _wfopen(to.c_str(), L"rb"); f)
	{
		for (auto&& [pszKey, iKeyLen, pszValue, iValueLen] : StringsBin::Deserialize(f))
		{
			rgszDest.try_emplace(
				ToUTF8({ reinterpret_cast<wchar_t const*>(pszKey), iKeyLen }),
				ToUTF8({ reinterpret_cast<wchar_t const*>(pszValue), iValueLen })
			);
		}

		fclose(f);
	}

	// Extract entries from source.
	if (auto f = _wfopen(from.c_str(), L"rb"); f)
	{
		map<string, string, CaseIgnoredLess> rgszTranslations{};
		for (auto&& [pszKey, iKeyLen, pszValue, iValueLen] : StringsBin::Deserialize(f))
		{
			rgszTranslations.try_emplace(
				ToUTF8({ reinterpret_cast<wchar_t const*>(pszKey), iKeyLen }),
				ToUTF8({ reinterpret_cast<wchar_t const*>(pszValue), iValueLen })
			);
		}

		for (auto&& szUnitDic : rgszUnitDictionaryEntries)
		{
			array<string, 3> const rgszEntries =
			{
				(string)szUnitDic,
				fmt::format("{}_descr", szUnitDic),
				fmt::format("{}_descr_short", szUnitDic),
			};

			for (auto&& szEntry : rgszEntries)
			{
				if (rgszDest.contains(szEntry))
					fmt::print(Gadget::Warning, "[Warning] Entry '{}' already exist in dest file.\n", szEntry);
				else if (rgszTranslations.contains(szEntry))
					rgszDest.try_emplace(szEntry, rgszTranslations[szEntry]);
				else
					fmt::print(Gadget::Error, "[Error] Entry: '{}' no found in {}\n", szEntry, from);
			};
		}

		fclose(f);
	}

	StringsBin::Serialize(to_txt.c_str(), rgszDest);
}

void CopyUnitEssentialFiles(fs::path const& SourceData, fs::path const& DestData, string_view const* itUnitNames, size_t len) noexcept
{
	span rgszUnits{ itUnitNames, len };

	auto const EDUPath = SourceData / L"export_descr_unit.txt";
	auto const EDUFile = Units::CEDU{ EDUPath };
	auto const MountFile = Mount::CFile{ SourceData / L"descr_mount.txt" };

	auto const rgszDictionaryEntries =
		EDUFile.m_Info
		| std::views::values
		| std::views::filter([&](Units::unit_t_ver_2 const& unit) noexcept { return std::ranges::contains(rgszUnits, *unit.m_type); })
		| std::views::transform([](Units::unit_t_ver_2 const& unit) noexcept -> string_view { return *unit.m_dictionary; })
		| std::ranges::to<vector>();

	if (rgszUnits.size() != rgszDictionaryEntries.size())
		fmt::print(Gadget::Warning, "[Warning] Some entries from your input cannot be found in EDU!\n");

	Gadget::Set<string_view> Models{};
	Models.insert_range(FindModelEntriesOfUnits(EDUFile, rgszUnits));
	Models.insert_range(FindMountsModelsOfUnits(MountFile, EDUFile, rgszUnits));

	CopyBattleModel(SourceData, DestData, Models);
	CopyUnitStringsBin(SourceData, DestData, rgszDictionaryEntries);
	CopyUnitUIFiles(SourceData, DestData, rgszDictionaryEntries);
	CopyUnitVoices(SourceData, DestData, rgszUnits);
}

void SimplifyBuildingLocale(fs::path const& ExportBuilding) noexcept
{
	static constexpr array rgszCultures =
	{
		// Empty, as the original default name.
		""sv,

		// Not alphabetic, but succession hierarchy
		"_southern_european"sv,
		"_northern_european"sv,
		"_eastern_european"sv,

		"_middle_eastern"sv,
		"_greek"sv,

		"_mesoamerican"sv,
	};

	static constexpr array rgszBuildingChainNames =
	{
		"core_building"sv,
		"core_castle_building"sv,
		"tower"sv,
		"castle_tower"sv,
		"equestrian"sv,
		"barracks"sv,
		"castle_barracks"sv,
		"professional_military"sv,
		"missiles"sv,
		"siege"sv,
		"castle_siege"sv,
		"cannon"sv,
		"castle_cannon"sv,
		"urban_equestrian"sv,
		"smith"sv,
		"castle_smith"sv,
		"port"sv,
		"castle_port"sv,
		"sea_trade"sv,
		"admiralty"sv,
		"market"sv,
		"hinterland_roads"sv,
		"hinterland_castle_roads"sv,
		"hinterland_farms"sv,
		"hinterland_mines"sv,
		"hinterland_castle_mines"sv,
		"health"sv,
		"hospital"sv,
		"academic"sv,
		"temple_catholic"sv,
		"temple_catholic_castle"sv,
		"temple_orthodox"sv,
		"temple_orthodox_castle"sv,
		"temple_muslim"sv,
		"temple_muslim_castle"sv,
		"taverns"sv,
		"city_hall"sv,
		"art"sv,
		"bullring"sv,
		"bank"sv,
		"paper"sv,
		"icon_art"sv,
		"music"sv,
		"tourney"sv,
		"castle_academic"sv,
		"caravan"sv,
		"guild_assassins_guild"sv,
		"guild_assassins_muslim_guild"sv,
		"guild_masons_guild"sv,
		"guild_theologians_guild"sv,
		"guild_merchants_guild"sv,
		"guild_alchemists_guild"sv,
		"guild_thiefs_guild"sv,
		"guild_explorers_guild"sv,
		"guild_swordsmiths_guild"sv,
		"templars_chapter_house"sv,
		"st_johns_chapter_house"sv,
		"guild_teutonic_knights_chapter_house"sv,
		"guild_knights_of_santiago_chapter_house"sv,
		"guild_woodsmens_guild"sv,
		"guild_horse_breeders_guild"sv,
		"convert_to_castle"sv,
		"convert_to_city"sv,
		"temple_pagan"sv,
	};

	Gadget::Dictionary<string, string> rgszTranslations{}, rgszOutput{};

	if (auto f = _wfopen(ExportBuilding.c_str(), L"rb"); f)
	{
		static constexpr auto fn = [](tuple<char16_t const*, size_t, char16_t const*, size_t> tpl) noexcept
		{
			auto&& [pKey, iKey, pVal, iVal] = tpl;

			return pair{
				ToUTF8({reinterpret_cast<wchar_t const*>(pKey), iKey}),
				ToUTF8({reinterpret_cast<wchar_t const*>(pVal), iVal}),
			};
		};

		for (auto&& [szKey, szValue] :
			StringsBin::Deserialize(f)
			| std::views::transform(fn)
			)
		{
			auto&& [it, bEmplaced] = rgszTranslations.try_emplace(szKey, szValue);

			if (!bEmplaced)
				fmt::print(Gadget::Warning, "[Warning] Duplicated key '{}'. Kept value: '{}', discard value: '{}'\n", szKey, it->second, szValue);
		}

		fclose(f);
	}

	auto UnclassifiedTranslations{ rgszTranslations };
	auto const rgszKeys =
		rgszTranslations
		| std::views::keys
		| std::views::transform([](auto&& obj) noexcept -> string_view { return string_view{ obj }; })
		| std::ranges::to<set>();

	// Step 1: add all names of building tree into output.
	auto iErased = 0;
	for (auto&& key :
		rgszBuildingChainNames
		| std::views::transform([](auto&& sz) noexcept { return fmt::format("{}_name", sz); })
		)
	{
		if (!rgszTranslations.contains(key))
		{
			fmt::print(Gadget::Warning, "[Warning] Building tree '{}' no found in current translation.\n", key);
			continue;
		}

		rgszOutput.try_emplace(key, rgszTranslations.at(key));
		UnclassifiedTranslations.erase(key);
		++iErased;
	}

	fmt::print("[Message] {} building tree names eliminated.\n", iErased);

	// Step 2: Generate output data.
	for (auto&& sz : rgszKeys)
	{
		auto const szBuildingRootName = string{ sz };
		auto const szBuildingDefDesc = szBuildingRootName + "_desc";
		auto const szBuildingDefShort = szBuildingDefDesc + "_short";

		auto const Entries =
			rgszKeys
			| std::views::filter(std::bind_back<bool(*)(string_view, string_view)>(&::StartsWith_I, sz))
			| std::ranges::to<set>();

		if (Entries.size() <= 3)
			continue;

		fmt::print("[Message] Word '{}' has leading {} entries.\n", sz, Entries.size());

		static constexpr auto fnIsPlaceholder = [](const string& sz) noexcept -> bool
		{
			return (sz.contains("NOT") || sz.contains("WARNING") || sz.contains("placeholder"));
		};

		// Step 2-1: gather frequency of the descriptions.
		auto fnAdd = [&](string const& szKey, string_view const& szCulture, auto&& rgszSet) noexcept -> bool
		{
			// no translation, no handling.
			if (!rgszTranslations.contains(szKey))
				return false;

			auto const& szTranslation = rgszTranslations.at(szKey);	
			UnclassifiedTranslations.erase(szKey);

			// same text, placeholder then.
			if (szKey == szTranslation)
				return false;

			// placeholder...
			if (fnIsPlaceholder(szTranslation))
				return false;

			rgszSet[szTranslation].emplace(szCulture);
			return true;
		};

		Gadget::Dictionary<
			string_view,	// desc/translation text
			Gadget::Set<string_view>	// culture using that text.
		> rgszBuildingNames{}, rgszBuildingDesc{}, rgszBuildingShort{};

		for (auto&& szCul : rgszCultures)
		{
			auto const szName = fmt::format("{}{}", sz, szCul);
			auto const szDesc = szName + "_desc";
			auto const szShort = szDesc + "_short";

			fnAdd(szName, szCul, rgszBuildingNames);
			fnAdd(szDesc, szCul, rgszBuildingDesc);
			fnAdd(szShort, szCul, rgszBuildingShort);
		}

		fmt::print(Gadget::Info,
			"\tPossible translations: name: {}\n"
			"\t                       desc: {}\n"
			"\t                       shor: {}\n",
			rgszBuildingNames | std::views::keys,
			rgszBuildingDesc | std::views::keys,
			rgszBuildingShort | std::views::keys
		);

		if (rgszBuildingNames.size() > 1)
			fmt::print(Gadget::Warning, "\t[Warning] {} possible translation in name of {}.\n", rgszBuildingNames.size(), sz);
		if (rgszBuildingDesc.size() > 1)
			fmt::print(Gadget::Warning, "\t[Warning] {} possible translation in desc of {}.\n", rgszBuildingDesc.size(), sz);
		if (rgszBuildingShort.size() > 1)
			fmt::print(Gadget::Warning, "\t[Warning] {} possible translation in short desc of {}.\n", rgszBuildingShort.size(), sz);

		// Step 2-2: Decides who becomes default.
		auto fnFindDefault = [&](decltype(rgszBuildingNames) const& rgszSet, string_view hint, string const& def_entry_key, string_view entry_key_ext) noexcept
		{
			switch (rgszSet.size())
			{
			case 0:
				fmt::print(Gadget::Error, "\t[Error] Empty {} set!\n", hint);
				break;

			case 1:
				rgszOutput.try_emplace(def_entry_key, rgszSet.begin()->first);
				fmt::print(Gadget::Progress, "\t[Message] Only one candidate for {} set.\n", hint);
				break;

			default:
				auto const itDefault = std::ranges::max_element(
					rgszSet,
					{},
					[](auto&& elem) noexcept { return elem.second.size(); }	// the more culture using this text the better.
				);

				rgszOutput.try_emplace(def_entry_key, itDefault->first);
				fmt::print(Gadget::Progress, "\t[Message] Candidate used by {} cultures won in {} set.\n", itDefault->second.size(), hint);

				for (auto it = rgszSet.begin(); it != rgszSet.end(); ++it)
				{
					if (it == itDefault)
						continue;

					for (auto&& cul : it->second)
					{
						rgszOutput.try_emplace(
							fmt::format("{}{}{}", sz, cul, entry_key_ext),
							it->first
						);

						fmt::print(Gadget::Progress, "\t[Message] Inserting entry '{}{}{}' as side.\n", sz, cul, entry_key_ext);
					}
				}
				break;
			}
		};

		fnFindDefault(rgszBuildingNames, "name", szBuildingRootName, "");
		fnFindDefault(rgszBuildingDesc, "desc", szBuildingDefDesc, "_desc");
		fnFindDefault(rgszBuildingShort, "short", szBuildingDefShort, "_desc_short");
	}

	// Step 3: Output
	auto const OutputTextFile = ExportBuilding.parent_path() / L"export_buildings.txt";
	StringsBin::Serialize(OutputTextFile, rgszOutput);

	auto const DiscardFile = ExportBuilding.parent_path() / L"export_buildings_discard.txt";
	StringsBin::Serialize(DiscardFile, UnclassifiedTranslations);
}

void PortSoundFiles(fs::path const& SourceData, fs::path const& DestData, string_view Faction) noexcept
{
	Voice::Prebattle::CFile Source_Prebattle{SourceData / L"export_descr_sounds_prebattle.txt"};
	Voice::Soldier::CFile Source_Soldier{SourceData / L"export_descr_sounds_soldier_voice.txt"};
	Voice::Strat::CFile Source_Strat{SourceData / L"export_descr_sounds_stratmap_voice.txt"};
	Voice::Battle::CFile Source_Battle{SourceData / L"export_descr_sounds_units_battle_events.txt"};

	Voice::Prebattle::CFile Dest_Prebattle{DestData / L"export_descr_sounds_prebattle.txt"};
	Voice::Soldier::CFile Dest_Soldier{DestData / L"export_descr_sounds_soldier_voice.txt"};
	Voice::Strat::CFile Dest_Strat{DestData / L"export_descr_sounds_stratmap_voice.txt"};
	Voice::Battle::CFile Dest_Battle{DestData / L"export_descr_sounds_units_battle_events.txt"};

	auto fnInsertion = [&](auto&& Source, auto&& Dest, string_view FileName) noexcept -> bool
	{
		if (!Source.m_Accents.contains(Faction))
		{
			fmt::print(Gadget::Error, "[Error] Accent '{}' does not existed in the source file '{}'\n", Faction, FileName);
			return false;
		}

		if (auto&& [it, bNewInsertion] = Dest.m_Accents.try_emplace(Faction, Source.m_Accents.at(Faction));
			!bNewInsertion)
		{
			fmt::print(Gadget::Warning, "[Warning] Accent '{}' already existed in the dest file '{}'\n", Faction, FileName);
			return false;
		}

		return Dest.Save(DestData / FileName);
	};

	fnInsertion(Source_Prebattle, Dest_Prebattle, "export_descr_sounds_prebattle.txt");
	fnInsertion(Source_Soldier, Dest_Soldier, "export_descr_sounds_soldier_voice.txt");
	fnInsertion(Source_Strat, Dest_Strat, "export_descr_sounds_stratmap_voice.txt");
	fnInsertion(Source_Battle, Dest_Battle, "export_descr_sounds_units_battle_events.txt");
}

void EliminateFactionEntryOfBattleModel(fs::path const& Data, string_view Faction) noexcept
{
	using namespace BattleModels;

	fs::path const FilePath = Data / L"unit_models" / L"battle_models.modeldb";
	CFile f{ FilePath };

	for (auto it = f.m_rgBattleModels.begin(); it != f.m_rgBattleModels.end(); /* does nothing */)
	{
		auto&& [szEntry, Model] = *it;

		if (Model.m_UnitTex.contains(Faction))
		{
			fmt::print(Gadget::Progress, "[Info] '{}' entry found in unit texture group of '{}'.\n", Faction, Model.m_szName);
			Model.m_UnitTex.erase(Faction);
		}
		if (Model.m_AttachmentTex.contains(Faction))
		{
			fmt::print(Gadget::Progress, "[Info] '{}' entry found in attachment texture group of '{}'.\n", Faction, Model.m_szName);
			Model.m_AttachmentTex.erase(Faction);
		}

		if (Model.m_UnitTex.empty() && Model.m_AttachmentTex.empty())
		{
			fmt::print(Gadget::Progress, "[Info] '{}' becomes empty after removing '{}' entry.\n", Model.m_szName, Faction);
			it = f.m_rgBattleModels.erase(it);
		}
		else
			++it;
	}

	f.Save(FilePath);
}

void CopyBattleModelFromFactionToAnother(fs::path const& Data, string_view SourceFaction, string_view DestFaction, string_view const* itModels, size_t len) noexcept
{
	using namespace BattleModels;

	span const rgszModels{ itModels, len };

	fs::path const FilePath = Data / L"unit_models" / L"battle_models.modeldb";
	CFile f{ FilePath };

	for (auto&& ModelName : rgszModels)
	{
		if (!f.m_rgBattleModels.contains(ModelName))
		{
			fmt::print(Gadget::Error, "[Error] Unit model '{}' no found.\n", ModelName);
			return;
		}

		if (f.m_rgBattleModels.at(ModelName).m_UnitTex.contains(SourceFaction))
		{
			f.m_rgBattleModels.at(ModelName).m_UnitTex.try_emplace(
				DestFaction,
				f.m_rgBattleModels.at(ModelName).m_UnitTex.at(SourceFaction)
			);
		}
		else
			fmt::print(Gadget::Warning, "[Warning] Unit texture of '{}' no found.\n", SourceFaction);

		if (f.m_rgBattleModels.at(ModelName).m_AttachmentTex.contains(SourceFaction))
		{
			f.m_rgBattleModels.at(ModelName).m_AttachmentTex.try_emplace(
				DestFaction,
				f.m_rgBattleModels.at(ModelName).m_AttachmentTex.at(SourceFaction)
			);
		}
		else
			fmt::print(Gadget::Warning, "[Warning] Attachment texture of '{}' no found.\n", SourceFaction);
	}

	f.Save(FilePath);
}

void Method() noexcept
{
	array rgszUnits{ "Chivalric Knights"sv, "E Chivalric Knights"sv, "Dismounted Chivalric Knights"sv, "Dismounted E Chivalric Knights"sv };

	auto const SourceData = fs::path{ LR"(C:\Program Files (x86)\Steam\steamapps\common\Medieval II Total War\mods\MyMod\data)" };
	auto const EDUPath = SourceData / L"export_descr_unit.txt";
	auto const EDUFile = Units::CEDU{ EDUPath };
	auto const MountFile = Mount::CFile{ SourceData / L"descr_mount.txt" };

	Gadget::Set<string_view> Models{};
	Models.insert_range(FindModelEntriesOfUnits(EDUFile, rgszUnits));
	Models.insert_range(FindMountsModelsOfUnits(MountFile, EDUFile, rgszUnits));

	fmt::println("{}", Models);
}
