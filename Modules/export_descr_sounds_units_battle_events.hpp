#pragma once

#include "export_descr_sounds.hpp"

namespace Voice::Battle
{
	namespace fs = std::filesystem;

	using std::map;
	using std::string;
	using std::string_view;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	struct CNotification final
	{
		vector<string_view> m_Arguments{};
		vector<CSimpleEvent> m_Events{};

		string Serialize() const noexcept;
	};

	struct CAccent final
	{
		string_view m_Name{};
		vector<CNotification> m_Notifications{};

		string Serialize() const noexcept;

		__forceinline decltype(auto) operator[] (size_t idx) const noexcept { return m_Notifications[idx]; }
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

