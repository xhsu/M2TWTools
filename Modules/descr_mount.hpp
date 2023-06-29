#pragma once

#include "String.hpp"

#include <array>
#include <map>
#include <optional>
#include <variant>

namespace Mount
{
	namespace fs = std::filesystem;

	using std::array;
	using std::optional;
	using std::string;
	using std::string_view;
	using std::variant;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	struct CHorse final
	{
		string_view m_type{};
		string_view m_class{};
		string_view m_model{};
		float m_radius{};
		optional<float> m_x_radius{};
		optional<float> m_y_offset{};
		float m_height{};
		float m_mass{};
		float m_banner_height{};
		float m_bouyancy_offset{};
		string_view m_water_trail_effect{};

		float m_root_node_height{};
		array<float, 3> m_rider_offset{};

		string Serialize() const noexcept;
	};

	using CCamel = CHorse;

	struct CElephant final
	{
		string_view m_type{};
		string_view m_class{};
		string_view m_model{};
		float m_radius{};
		optional<float> m_x_radius{};
		optional<float> m_y_offset{};
		float m_height{};
		float m_mass{};
		float m_banner_height{};
		float m_bouyancy_offset{};
		string_view m_water_trail_effect{};

		float m_root_node_height{};
		float m_attack_delay{};
		float m_dead_radius{};
		float m_tusk_z{};
		float m_tusk_radius{};
		vector<array<float, 3>> m_rgvecRiderOffsets{};

		string Serialize() const noexcept;
	};

	struct CFile : public IBaseFile
	{
		Dictionary<
			string_view,
			variant<CHorse, CElephant>
		> m_Mounts{};

		explicit CFile(fs::path const& Path) noexcept : IBaseFile{ Path } { Deserialize(); }

		void Deserialize() noexcept override;
		string Serialize() const noexcept override;

		string_view ModelOf(string_view szMountType) const noexcept;
	};

	using CMount = decltype(CFile::m_Mounts)::value_type::second_type;

	inline auto operator<=> (CMount const& lhs, CMount const& rhs) noexcept
	{
		static constexpr auto fnClass = [](auto&& arg) constexpr noexcept -> decltype(auto) { return arg.m_class; };
		static constexpr auto fnType = [](auto&& arg) constexpr noexcept -> decltype(auto) { return arg.m_type; };

		if (auto const cmp = std::visit(fnClass, lhs) <=> std::visit(fnClass, rhs);
			cmp == std::strong_ordering::greater || cmp == std::strong_ordering::less)
		{
			return cmp;
		}

		return std::visit(fnType, lhs) <=> std::visit(fnType, rhs);
	}
}