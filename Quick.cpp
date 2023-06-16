#include <fmt/color.h>
#include <fmt/std.h>

//#include "Modules/export_descr_sounds_units_voice.hpp"
//#include "Modules/battle_models.hpp"

#include <assert.h>

#include <array>
#include <span>
#include <set>

namespace fs = std::filesystem;

using namespace std;
using namespace std::literals;

extern void CopyUnitVoices(const char* pszFrom, const char* pszTo, string_view const* it, size_t len) noexcept;
extern void CopyBattleModel(const char* pszModelDbFrom, const char* pszModelDbTo, string_view const* pArrayOfUnitNames, size_t len) noexcept;
extern set<string> AssembleUnitUIFiles(fs::path const& DataPath, string_view szDic) noexcept;

int main(int, char* []) noexcept
{
	//using namespace UnitsVoice;

	//static constexpr array rgszUnits{
	//	"Archontopoulai"sv,
	//		"Byzantine Gunners"sv,
	//		"Alamanoi"sv,
	//		"Pronoia Infantry"sv,
	//};

	//CopyUnitVoices(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\export_descr_sounds_units_voice.txt)",
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\export_descr_sounds_units_voice.txt)",
	//	rgszUnits.data(),
	//	rgszUnits.size()
	//);

	//static constexpr array rgszModelEntries{
	//	"Archontopoulai"sv, "Archontopoulai_ug1"sv,
	//		"Byzantine_gunners"sv,
	//		"Alamanoi"sv, "Alamanoi_ug1"sv,
	//		"Pronoia_Infantry"sv, "Pronoia_Infantry_ug1"sv,
	//};

	//CopyBattleModel(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\unit_models\battle_models.modeldb)",
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\unit_models\battle_models.modeldb)",
	//	rgszModelEntries.data(),
	//	rgszModelEntries.size()
	//);

	static constexpr array rgszUnitDicts{
		"Archontopoulai"sv,
			"Byzantine_Gunners"sv,
			"Alamanoi"sv,
			"Pronoia_Infantry"sv,
	};


	fs::path const CrusData{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\)" };
	fs::path const MyData{ R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data\)" };

	set<string> files{};
	for (auto&& unit : rgszUnitDicts)
		files.insert_range(AssembleUnitUIFiles(CrusData, unit));

	for (auto&& file : files)
	{
		auto const from = CrusData / file;
		auto const to = MyData / file;

		if (fs::exists(to))
		{
			fmt::print(fg(fmt::color::gray), "[Message] Skipping existing file: {}\n", to);
			continue;
		}

		if (fs::exists(from))
		{
			std::error_code ec{};
			fs::create_directories(to.parent_path());
			fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);

			if (ec)
				fmt::print(fg(fmt::color::red), "[Error] std::error_code '{}': {}\n", ec.value(), ec.message());
		}
		else
			fmt::print(fg(fmt::color::red), "[Error] FILE no found: {}\n", from.u8string());
	}
}