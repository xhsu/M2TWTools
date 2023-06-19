#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

//#include "Modules/export_descr_sounds_units_voice.hpp"
//#include "Modules/battle_models.hpp"
#include "Modules/export_descr_unit.hpp"
#include "String.hpp"

#include <assert.h>

#include <array>
#include <span>
#include <set>
#include <map>

namespace fs = std::filesystem;

using namespace std;
using namespace std::literals;

extern void CopyUnitVoices(const char* pszFrom, const char* pszTo, string_view const* it, size_t len) noexcept;
extern void CopyBattleModel(const char* pszModelDbFrom, const char* pszModelDbTo, string_view const* pArrayOfUnitNames, size_t len) noexcept;
extern set<string> AssembleUnitUIFiles(fs::path const& DataPath, string_view szDic) noexcept;
extern set<string> FindModelEntriesOfUnits(const char* EDU, string_view const* it, size_t len) noexcept;
extern vector<Units::CUnit const*> GetRoaster(Units::CFile const& edu, const char* Faction) noexcept;
extern set<Units::CUnit const*> CrossRef(vector<Units::CUnit const*> const& lhs, vector<Units::CUnit const*> const& rhs) noexcept;
extern void CopyUnitStringsBin(fs::path const& SourceData, fs::path const& DestData, string_view const* itUnitDictionaryEntry, size_t len) noexcept;
extern void CopyUnitEssentialFiles(fs::path const& SourceData, fs::path const& DestData, string_view const* itUnitNames, size_t len) noexcept;

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

template <typename T>
static inline auto UTIL_StrToNum(string_view sz) noexcept
{
	if constexpr (std::is_enum_v<T>)
	{
		if (std::underlying_type_t<T> ret{}; std::from_chars(sz.data(), sz.data() + sz.size(), ret).ec == std::errc{})
			return static_cast<T>(ret);
	}
	else
	{
		if (T ret{}; std::from_chars(sz.data(), sz.data() + sz.size(), ret).ec == std::errc{})
			return ret;
	}

	return T{};
}

struct CTest final : public CBaseParser
{
	template <size_t N>
	CTest(char (&rgc)[N]) noexcept : CBaseParser()
	{
		m_cur = m_p = rgc;
		m_length = N;
	}

	~CTest() noexcept override
	{
		m_cur = m_p = nullptr;
		m_length = 0;
	}

	template <typename T>
	auto ParseArgument(string_view delimiters = ", ") noexcept
	{
		if constexpr (std::convertible_to<T, string_view>)
			return Parse(delimiters);
		else if constexpr (std::is_arithmetic_v<T>)
			return UTIL_StrToNum<T>(Parse(delimiters));
	}

	template <typename T>
	T ParseTuple() noexcept
	{
		static_assert(
			requires (T t) {
				{ tuple_size_v<T> } -> convertible_to<size_t>;
				std::get<0>(t) = ParseArgument<tuple_element_t<0, T>>();
			},
			"Must be a tuple to use ParseTuple()!"
		);

		auto const fnImpl = [&]<size_t... I>(index_sequence<I...>) noexcept -> T
		{
			T ret{};
			((std::get<I>(ret) = ParseArgument<tuple_element_t<I, T>>()), ...);

			return ret;

			//return tuple{ ParseArgument<tuple_element_t<I, T>>()... };
			// Fuck MSVC evaluation sequence.
		};

		return fnImpl(make_index_sequence<tuple_size_v<T>>{});
	}
};

using stuff_t = tuple<int, string_view, float, short>;
char words[]{ "keyword            1, text, 0.999, -3\t" };


inline constexpr array TurksUnits =
{
	"Hasham"sv,
	"iqta'dar"sv,
	"Dismounted Hasham"sv,
	"Kurdish Auxiliaries"sv,
	"Turkish Crossbowmen"sv,
	"Ahdath"sv,
};

inline constexpr array TurksUnits_Dict =
{
	"Hasham"sv,
	"iqta'dar"sv,
	"Dismounted_Hasham"sv,
	"Kurdish_Auxiliaries"sv,
	"Turkish_Crossbowmen"sv,
	"Ahdath"sv,
};

int main(int, char* []) noexcept
{
	//auto const ModelEntries = FindModelEntriesOfUnits(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data\export_descr_unit.txt)",
	//	TurksUnits.data(),
	//	TurksUnits.size()
	//);

	//CopyUnitStringsBin(
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data)",
	//	R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data)",
	//	TurksUnits_Dict.data(),
	//	TurksUnits_Dict.size()
	//);

	CopyUnitEssentialFiles(
		R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\crusades_data)",
		R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\mods\bare_geomod\data)",
		TurksUnits.data(),
		TurksUnits.size()
	);
}
