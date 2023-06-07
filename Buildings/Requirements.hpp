#pragma once

#include <array>
#include <string_view>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace Requirements
{
	using namespace std::literals;

	using std::array;
	using std::string;
	using std::string_view;
	using std::tuple;
	using std::variant;
	using std::vector;

	inline constexpr array Keywords
	{
		"building_present"sv,				// 0
		"building_present_min_level"sv,
		"event_counter"sv,
		"factions"sv,
		"hidden_resource"sv,				// 4
		"region_religion"sv,
		"resource"sv,
	};

	using _Impl_IndexType = tuple<
		array<string, 1>,	// 0
		array<string, 2>,
		array<string, 2>,
		vector<string>,
		array<string, 1>,	// 4
		array<string, 2>,
		array<string, 1>
	>;

	template <size_t iIndex>
	using CParameter = std::tuple_element_t<iIndex, _Impl_IndexType>;

	struct CBuildingPresent final
	{
		inline static constexpr auto Function{ "building_present"sv };
		inline static constexpr auto Parameters{ 1 };

		bool m_Inverted{};
		array<string, Parameters> m_Arguments{};	// line
	};

	struct CBuildingMinLevel final
	{
		inline static constexpr auto Function{ "building_present_min_level"sv };
		inline static constexpr auto Parameters{ 2 };

		bool m_Inverted{};
		array<string, Parameters> m_Arguments{};	// line, level
	};

	struct CEventCounter final
	{
		inline static constexpr auto Function{ "event_counter"sv };
		inline static constexpr auto Parameters{ 2 };

		bool m_Inverted{};
		array<string, Parameters> m_Arguments{};	// what, number
	};

	struct CFactions final
	{
		inline static constexpr auto Function{ "factions"sv };
		inline static constexpr auto Parameters{ -1 };

		bool m_Inverted{};
		vector<string> m_Arguments{};	// faction names
	};

	struct CHiddenResources final
	{
		inline static constexpr auto Function{ "hidden_resource"sv };
		inline static constexpr auto Parameters{ 1 };

		bool m_Inverted{};
		array<string, Parameters> m_Arguments{};	// what
	};

	struct CRegionReligion final
	{
		inline static constexpr auto Function{ "region_religion"sv };
		inline static constexpr auto Parameters{ 2 };

		bool m_Inverted{};
		array<string, Parameters> m_Arguments{};	// religion, percentage
	};

	struct CResources final
	{
		inline static constexpr auto Function{ "resource"sv };
		inline static constexpr auto Parameters{ 1 };

		bool m_Inverted{};
		array<string, Parameters> m_Arguments{};	// what
	};

	using CCell = variant<CBuildingPresent, CBuildingMinLevel, CEventCounter, CFactions, CHiddenResources, CRegionReligion, CResources>;
	using CObject = vector<vector<CCell>>;

	CObject Compile(string_view sz) noexcept;
	string Decompile(CObject const& Tree) noexcept;

	void UnitTest(void) noexcept;
};

