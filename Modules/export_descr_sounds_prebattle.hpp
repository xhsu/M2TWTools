#pragma once

#include "String.hpp"

#include <array>
#include <map>	// #UPDATE_AT_CPP23 flat_map

using namespace std::literals;

namespace Voice::Prebattle
{
	namespace fs = std::filesystem;

	using std::array;
	using std::string;
	using std::string_view;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	struct CEvent final
	{
		vector<string_view> m_Arguments{};
		string_view m_Folder{};
		vector<string_view> m_Files{};

		static inline constexpr array m_rgszTypeName{ "VnV"sv, "relationship"sv, "situation"sv, "condition"sv };
		enum EType : unsigned { VnV, Relation, Situation, Condition, None, Unknown };

		__forceinline EType Type() const noexcept
		{
			if (m_Arguments.empty())
				return None;

			for (unsigned i = 0; i < m_rgszTypeName.size(); ++i)
				if (strieql(m_rgszTypeName[i], m_Arguments.front()))
					return static_cast<EType>(i);

			return Unknown;
		}

		string Serialize() const noexcept;
	};

	struct CElement final
	{
		string_view m_Name{};
		vector<CEvent> m_Events{};

		string Serialize() const noexcept;

		__forceinline decltype(auto) operator[] (size_t idx) const noexcept { return m_Events[idx]; }
	};

	struct CAccent final
	{
		string_view m_Name{};
		Dictionary<string_view, CElement> m_Elements{};

		string Serialize() const noexcept;

		template <typename T> __forceinline decltype(auto) operator[] (T&& arg) const noexcept { return m_Elements.at(std::forward<T>(arg)); }
	};

	struct CFile final : public CBaseParser
	{
		explicit CFile(fs::path const& Path) noexcept : CBaseParser{ Path } { Deserialize(); }

		void Deserialize() noexcept;
		string Serialize() const noexcept;
		bool Save(fs::path const& Path) const noexcept;

		template <typename T> __forceinline decltype(auto) operator[] (T&& arg) const noexcept { return m_Accents.at(std::forward<T>(arg)); }

		Dictionary<string_view, CAccent> m_Accents{};
	};
}
