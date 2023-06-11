
#include <stdint.h>
#include <stdio.h>

#include <fmt/color.h>
#include <fmt/ranges.h>

#include <array>
#include <experimental/generator>
#include <filesystem>
#include <functional>
#include <ranges>
#include <set>
#include <string_view>
#include <variant>
#include <vector>

#include "Modules/battle_models.hpp"
#include "Modules/export_descr_unit.hpp"

namespace fs = std::filesystem;

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

int32_t main(int32_t argc, char* argv[]) noexcept
{
	//auto const mymod = Units::Deserialize(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\export_descr_unit.txt)");
	//auto const amc = Units::Deserialize(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\americas_data\export_descr_unit.txt)");
	//auto const bri = Units::Deserialize(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\british_isles_data\export_descr_unit.txt)");
	//auto const crus = Units::Deserialize(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\export_descr_unit.txt)");
	//auto const tut = Units::Deserialize(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\teutonic_data\export_descr_unit.txt)");

//-------------------------------------------------------

	//auto const spain_mymod = GetRoaster(mymod, "denmark");
	//auto const spain_amc = GetRoaster(bri, "norway");
	//auto const spain_cross_1 = CrossRef(spain_amc, spain_mymod);
	//auto const spain_cross_2 = CrossRef(spain_mymod, spain_amc);

	//for (auto&& p : spain_cross_1)
	//{
	//	fmt::print("unique to bri: {} - {}\n", p->at("type")[0], p->at("ownership"));
	//}

	//fmt::print("\n");

	//for (auto&& p : spain_cross_2)
	//{
	//	fmt::print("unique to van: {} - {}\n", p->at("type")[0], p->at("ownership"));
	//}

//---------------------------------------------------

	//auto const cr = CrossRef(crus, mymod);
	//auto const facs = ExtractFactions(cr);

	//for (auto&& fac : facs)
	//{
	//	if (fac == "slave" || fac == "aztecs" || fac == "saxons" || fac == "normans")
	//		continue;

	//	fmt::print(fg(fmt::color::cyan), "{}: \n", fac);
	//	for (auto&& pUnit : cr)
	//	{
	//		if (std::ranges::contains(pUnit->at("ownership"), fac) && (!std::ranges::contains(pUnit->at("attributes"), "mercenary_unit") || pUnit->at("ownership").size() > 1))
	//			fmt::print("{}\n", pUnit->at("type")[0]);
	//	}
	//	fmt::print("\n");
	//}

//---------------------------------------------------

	//BattleModels::CFile mymod_bmd(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\unit_models\battle_models.modeldb)");
	//BattleModels::CFile crus_bmd(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\unit_models\battle_models.modeldb)");

	//auto const pGreekFirethrower = std::ranges::find(crus_bmd.m_rgBattleModels, "greek_firethrower", [](BattleModels::CBattleModel const& Model) noexcept { return Model.m_szName; });
	//auto const pGreekFirethrowerUg1 = std::ranges::find(crus_bmd.m_rgBattleModels, "greek_firethrower_ug1", [](BattleModels::CBattleModel const& Model) noexcept { return Model.m_szName; });

	//mymod_bmd.m_rgBattleModels.emplace_back(*pGreekFirethrower);
	//mymod_bmd.m_rgBattleModels.emplace_back(*pGreekFirethrowerUg1);

	//mymod_bmd.Save(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\unit_models\Out.modeldb)");

	//auto files = pGreekFirethrower->ListOfFiles();
	//files.insert_range(pGreekFirethrowerUg1->ListOfFiles());

	//for (auto&& file : files)
	//{
	//	fs::path const from = fs::path{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\)" } / file;
	//	fs::path const to = fs::path{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\)" } / file;

	//	if (!fs::exists(from))
	//		fmt::print(fg(fmt::color::red), "FILE no found: {}\n", from.u8string());
	//	else
	//	{
	//		std::error_code ec{};
	//		fs::create_directories(to.parent_path());
	//		fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::update_existing, ec);

	//		if (ec)
	//			fmt::print(fg(fmt::color::red), "std::error_code '{}': {}\n", ec.value(), ec.message());
	//	}
	//}

//----------------------------------------------------------------------------

	using namespace BattleModels;

	CFile mymod_bmd(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\unit_models\battle_models.modeldb)");
	CFile brit_bmd(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\british_isles_data\unit_models\battle_models.modeldb)");

	vector<CBattleModel*> NorwayLib1{}/*, NorwayLib2{}*/;

	for (auto&& Model : brit_bmd.m_rgBattleModels)
	{
		if (Model.m_szName.starts_with("dummy_"))
			continue;

		if (std::ranges::contains(Model.m_UnitTex | std::views::keys, "norway"))
			NorwayLib1.push_back(&Model);
		//if (std::ranges::contains(Model.m_AttachmentTex | std::views::keys, "norway"))
		//	NorwayLib2.push_back(&Model);
	}

	// representing all files that norway's unit will need.
	set<fs::path> files{};

	// Patching battle_models file.
	for (auto&& Model : mymod_bmd.m_rgBattleModels)
	{
		auto const it = std::ranges::find(NorwayLib1, Model.m_szName, [](CBattleModel* p) noexcept { return p->m_szName; });

		// model name is not in list.
		if (it == NorwayLib1.end())
			continue;

		auto const Source = *it;

		if (auto it2 = std::ranges::find(Source->m_UnitTex, "norway", [](auto&& val) noexcept { return val.first; }); it2 != Source->m_UnitTex.end())
		{
			Model.m_UnitTex.emplace_back(*it2);
			files.insert_range(it2->second.ListOfFiles(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\british_isles_data\)"));
			fmt::print("Patched unit tex ");
		}

		if (auto it2 = std::ranges::find(Source->m_AttachmentTex, "norway", [](auto&& val) noexcept { return val.first; }); it2 != Source->m_AttachmentTex.end())
		{
			Model.m_AttachmentTex.emplace_back(*it2);
			files.insert_range(it2->second.ListOfFiles(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\british_isles_data\)"));
			fmt::print("and attachment tex ");
		}

		fmt::print("into <{}>.\n", Source->m_szName);
		NorwayLib1.erase(it);
	}

	if (!NorwayLib1.empty())
	{
		fmt::print(fg(fmt::color::light_golden_rod_yellow), "NorwayLib1: {} left.\n", NorwayLib1.size());

		for (auto&& pModel : NorwayLib1)
			fmt::print("Model: <{}> for [{}]\n", pModel->m_szName, fmt::join(pModel->m_UnitTex | std::views::keys, ", "));

		fmt::print(fg(fmt::color::lawn_green), "Inserting entries above this line in.\n");

		for (auto&& pModel : NorwayLib1)
		{
			mymod_bmd.m_rgBattleModels.emplace_back(*pModel);
			files.insert_range(pModel->ListOfFiles(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\british_isles_data\)"));
		}
	}

	mymod_bmd.Save(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\unit_models\Out.modeldb)");

	for (auto&& file : files)
	{
		fs::path const to = fs::path{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\)" } / fs::relative(file, R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\british_isles_data\)");

		if (fs::exists(file))
		{
			std::error_code ec{};
			fs::create_directories(to.parent_path());
			fs::copy(file, to, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);

			if (ec)
				fmt::print(fg(fmt::color::red), "std::error_code '{}': {}\n", ec.value(), ec.message());
		}
		else
			fmt::print(fg(fmt::color::red), "FILE no found: {}\n", file.u8string());
	}


//----------------------------------------------------------------------------

	//auto const files = AssembleUnitUIFiles(R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\)", "Greek_Firethrower");

	//for (auto&& file : files)
	//{
	//	fs::path const from = fs::path{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\)" } / file;
	//	fs::path const to = fs::path{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\)" } / file;

	//	if (fs::exists(from))
	//	{
	//		std::error_code ec{};
	//		fs::create_directories(to.parent_path());
	//		fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::update_existing, ec);

	//		if (ec)
	//			fmt::print(fg(fmt::color::red), "std::error_code '{}': {}\n", ec.value(), ec.message());
	//	}
	//	else
	//		fmt::print(fg(fmt::color::red), "FILE no found: {}\n", from.u8string());
	//}

	return 0;

	/*
	amc = 3

	New World Cuirassers
	New World Bodyguard
	Spanish Dragoons

	bri = 26

	Horseboys
	Hobiguir
	Ridire
	Mounted Calivermen
	Lords Retinue
	Ulster Swordsmen
	Muire
	Cliathairi
	Ceitherne
	Ostmen
	Saighdeoir
	Deisi Javelinmen
	Calivermen

	Sami Axemen
	Gotland Footmen
	Svenner

	Teulu
	Teulu Skirmishers
	Mathrafal Horsemen
	Rhyfelwyr
	Meirionnydd Spearmen
	Morgannwg Spearmen
	Welsh Militiamen
	Gwent Raiders
	Magnelwyr
	Helwyr
	Saethwyr
	Welsh Skirmishers

	crus = 54 or 59

	Syrian Militia
	Sodeer Archers
	Knights of Antioch
	Knights of Edessa
	Marshall of the Hospitallers
	Seljuk Auxiliary
	Edessan Squires
	Edessan Guard
	Dismounted Knights of Antioch
	Canons of the Holy Sepulcher
	Hospitaller Sergeant
	Antioch Militia
	Frankish Swordsmen
	Armenians of Celicia
	Hospitaller Crossbowmen
	Hospitaller Gunner
	--Philip King of France
	Mangonel

	Archontopoulai
	Greek Firethrower
	Alamanoi
	Pronoia Infantry
	Byzantine Gunners
	--Emperor Manuel
	Greek Mangonel

	--Sultan_Saladin
	Ghulams
	Khassaki
	Sibyan al Khass
	Dismounted Ghulams
	Al Haqa Infantry
	al Ashair
	Khasseki
	Abid al Shira
	Mamluk Handgunners
	ME Mangonel

	--Richard the Lionheart
	Knights of Jerusalem
	Templar Confrere Knights
	Knights of Tripoli
	Constable of Jerusalem
	Marshall of the Templars
	Tripolitan Squires
	Dismounted Knights Of Jerusalem
	Templar Sergeant
	Syrian Militia
	Frankish Axemen
	Sodeer Archers
	Maronites of Lebanon
	Templar Crossbowmen
	Templar Gunners
	Mangonel

	Hasham
	iqta'dar
	Dismounted Hasham
	Kurdish Auxiliaries
	Turkish Crossbowmen
	Ahdath
	--Nur adDin
	ME Mangonel

	tut = 31

	Followers of Perkunas
	Sudovian Tribesmen
	Estonian Rebels
	Samogitian Axemen
	Giltines Chosen
	Dismounted Tartar Lancers
	Dismounted Szlachta
	Dzukijan Horsemen
	Baltic Archers
	Lettish Crossbowmen
	Lithuanian Arquebusiers
	Tartar Lancers
	Dievas Guard
	Szlachta
	L Chivalric Knights
	L Dismounted Chivalric Knights
	L Cavalry Militia

	Order Militia
	Burgher Pikemen
	Sword Brethren
	Order Spearmen
	Clergymen
	Dismounted Halbbruder
	Dismounted Ritterbruder
	Prussian Archers
	Livonian Auxiliaries
	Christ Knights
	Knechten
	Halbbruder
	Ritterbruder
	TO Bodyguard
	TO Late Bodyguard
	Mangonel

	*/
}
