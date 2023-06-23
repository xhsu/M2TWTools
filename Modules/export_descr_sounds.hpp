#pragma once

#include "String.hpp"

#include <map>	// #UPDATE_AT_CPP23 flat_map

namespace Voice
{
	namespace fs = std::filesystem;

	using std::map;
	using std::string;
	using std::string_view;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	struct CSimpleFolder final
	{
		string_view m_Folder{};
		vector<string_view> m_Files{};

		string Serialize(int iIndent) const noexcept;
	};

	struct CSimpleEvent final
	{
		vector<string_view> m_Arguments{};
		vector<CSimpleFolder> m_Folders{};

		string Serialize(int iIndent) const noexcept;
		static CSimpleEvent Deserialize(CBaseParser* File, string_view szThisLine) noexcept;
	};
};

