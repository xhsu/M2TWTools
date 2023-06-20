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

inline void CopyFiles(std::ranges::input_range auto&& files, fs::path const& SourceFolder, fs::path const& DestFolder) noexcept
{
	for (auto&& file : files)
	{
		auto const from = SourceFolder / file;
		auto const to = DestFolder / file;

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

inline constexpr array TurksUnits =
{
	"Hasham"sv,
	"iqta'dar"sv,
	"Dismounted Hasham"sv,
	"Kurdish Auxiliaries"sv,
	"Turkish Crossbowmen"sv,
	"Ahdath"sv,
};

int main(int, char* []) noexcept
{
	//CopyUnitEssentialFiles(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data)",
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data)",
	//	TurksUnits.data(),
	//	TurksUnits.size()
	//);

	//ListFactionUnitPriority(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\export_descr_unit.txt)",
	//	"turks"
	//);
}
