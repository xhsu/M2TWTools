#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

//#include "Modules/export_descr_sounds_units_voice.hpp"
//#include "Modules/battle_models.hpp"
//#include "Modules/export_descr_unit.hpp"
#include "Modules/descr_mount.hpp"

//#include "String.hpp"

#include <assert.h>

#include <array>
#include <ranges>
#include <map>
#include <vector>
#include <set>

namespace fs = std::filesystem;

using namespace std;
using namespace std::literals;

extern void CopyUnitEssentialFiles(fs::path const& SourceData, fs::path const& DestData, string_view const* itUnitNames, size_t len) noexcept;
extern void ListFactionUnitPriority(fs::path const& edu, string_view szFaction) noexcept;
extern void CopyBattleModelFromFactionToAnother(fs::path const& Data, string_view SourceFaction, string_view DestFaction, string_view const* itModels, size_t len) noexcept;

inline constexpr array units = {
	"Richard the Lionheart"sv,
	"Philip King of France"sv,
};


int main(int, char* []) noexcept
{
	CopyBattleModelFromFactionToAnother(
		R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data)",
		"england",
		"jerusalem",
		units.data(),
		units.size()
	);

	return EXIT_SUCCESS;
}
