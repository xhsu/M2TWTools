#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

//#include "Modules/export_descr_sounds_units_voice.hpp"
//#include "Modules/battle_models.hpp"
#include "Modules/export_descr_unit.hpp"
#include "String.hpp"

#include <assert.h>

#include <array>
#include <set>
#include <map>

namespace fs = std::filesystem;

using namespace std;
using namespace std::literals;

extern void CopyUnitEssentialFiles(fs::path const& SourceData, fs::path const& DestData, string_view const* itUnitNames, size_t len) noexcept;
extern void ListFactionUnitPriority(fs::path const& edu, string_view szFaction) noexcept;
extern void Method(fs::path const& SourceData, fs::path const& DestData) noexcept;

inline constexpr array EgyptUnits =
{
	"Abid al Shira"sv,
	"Al Haqa Infantry"sv,
	"Dismounted Ghulams"sv,
	"Ghulams"sv,
	"Khassaki"sv,
	"Khasseki"sv,
	"Mamluk Handgunners"sv,
	"Sibyan al Khass"sv,
	"al Ashair"sv,
};

int main(int, char* []) noexcept
{
	//CopyUnitEssentialFiles(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data)",
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data)",
	//	EgyptUnits.data(),
	//	EgyptUnits.size()
	//);

	//ListFactionUnitPriority(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\export_descr_unit.txt)",
	//	"turks"
	//);

	Method(
		R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data)",
		R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data)"
	);
}
