#pragma once

#include "String.hpp"

#include <map>	// #UPDATE_AT_CPP23 flat_map

namespace Voice::Soldier
{
	namespace fs = std::filesystem;

	using std::map;
	using std::string;
	using std::string_view;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	struct CFolder final
	{
		string_view m_Folder{};
		vector<string_view> m_Files{};

		string Serialize() const noexcept;
	};

	struct CEvent final
	{
		vector<string_view> m_Arguments{};
		vector<CFolder> m_Folders{};

		string Serialize() const noexcept;
	};

	struct CVocal final
	{
		string_view m_Name{};
		vector<CEvent> m_Events{};

		string Serialize() const noexcept;

		__forceinline decltype(auto) operator[] (size_t idx) const noexcept { return m_Events[idx]; }
	};

	struct CClass final
	{
		string_view m_Name{};
		Dictionary<string_view, CVocal> m_Vocals{};

		string Serialize() const noexcept;

		template <typename T> __forceinline decltype(auto) operator[] (T&& arg) const noexcept { return m_Vocals.at(std::forward<T>(arg)); }
	};

	struct CAccent final
	{
		string_view m_Name{};
		Dictionary<string_view, CClass> m_Classes{};

		string Serialize() const noexcept;

		template <typename T> __forceinline decltype(auto) operator[] (T&& arg) const noexcept { return m_Classes.at(std::forward<T>(arg)); }
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

