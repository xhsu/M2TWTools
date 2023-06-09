#pragma once

#include <array>
#include <map>
#include <tuple>
#include <vector>

#include <fmt/core.h>

using namespace std::literals;

namespace Units
{
	using std::array;
	using std::map;
	using std::string;
	using std::string_view;
	using std::tuple;
	using std::vector;

	struct LitStrV final
	{
		template <size_t N>
		inline constexpr LitStrV(char(&sz)[N]) noexcept : m_psz{ sz }, m_length{ N } {}

		inline constexpr operator string_view() const noexcept { return string_view(m_psz, m_length); }

		char const* m_psz{};
		size_t m_length{};
	};

	using CUnit = map<string, vector<string>>;
	using CFile = vector<CUnit>;

	string Serialize(CUnit const& unit) noexcept;
	CFile Deserialize(const char* file = "export_descr_unit.txt") noexcept;
}

