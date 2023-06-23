#pragma once

#include "export_descr_sounds.hpp"

#include <map>	// #UPDATE_AT_CPP23 flat_map

namespace Voice::Strat
{
	namespace fs = std::filesystem;

	using std::map;
	using std::string;
	using std::string_view;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	struct CVocal final
	{
		string_view m_Name{};
		vector<CSimpleEvent> m_Events{};

		string Serialize() const noexcept;

		__forceinline decltype(auto) operator[] (size_t idx) const noexcept { return m_Events[idx]; }
	};

	struct CType final
	{
		string_view m_Name{};
		Dictionary<string_view, CVocal> m_Vocals{};

		string Serialize() const noexcept;

		template <typename T> __forceinline decltype(auto) operator[] (T&& arg) const noexcept { return m_Vocals.at(std::forward<T>(arg)); }
	};

	struct CAccent final
	{
		string_view m_Name{};
		Dictionary<string_view, CType> m_Types{};

		string Serialize() const noexcept;

		template <typename T> __forceinline decltype(auto) operator[] (T&& arg) const noexcept { return m_Types.at(std::forward<T>(arg)); }
	};

	struct CFile final : public CBaseParser
	{
		Dictionary<string_view, CAccent> m_Accents{};

		explicit CFile(fs::path const& Path) noexcept : CBaseParser{ Path } { Deserialize(); }

		void Deserialize() noexcept;
		string Serialize() const noexcept;
		bool Save(fs::path const& Path) const noexcept;

		template <typename T> __forceinline decltype(auto) operator[] (T&& arg) const noexcept { return m_Accents.at(std::forward<T>(arg)); }
	};
};

