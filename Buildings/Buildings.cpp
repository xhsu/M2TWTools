
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

#include "String.hpp"

namespace fs = std::filesystem;

using namespace std::literals;

using std::array;
using std::experimental::generator;
using std::set;
using std::string;
using std::string_view;
using std::variant;
using std::vector;

extern void CopyBattleModel(const char* pszModelDbFrom, const char* pszModelDbTo, string_view szFaction) noexcept;

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
