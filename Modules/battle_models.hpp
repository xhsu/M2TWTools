/*
* Module:	Dec 06 2021
* Fix:		Jun 08 2023
*/

#pragma once

#include <array>
#include <set>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "String.hpp"

namespace BattleModels
{
	using std::array;
	using std::pair;
	using std::set;
	using std::string_view;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::unordered_map<K, V, CaseIgnoredString, CaseIgnoredString>;

	struct CMesh final
	{
		string_view m_szPath{};
		size_t m_iDistance{};
	};

	struct CTexture final
	{
		string_view m_szTex{};
		string_view m_szNorm{};
		string_view m_szSprite{};

		set<std::filesystem::path> ListOfFiles(std::filesystem::path const& DataPath) const noexcept;
	};

	struct CMount final
	{
		string_view m_szType{};
		string_view m_szWpn1{};
		string_view m_szWpn2{};
	};

	struct CStance final
	{
		CMount m_Mount{};
		vector<string_view> m_rgszPrim{};
		vector<string_view> m_rgszSec{};
	};

	struct CBattleModel final
	{
		string_view m_szName{};
		vector<vector<CMesh>> m_rgrgMeshGroup{};
		Dictionary<string_view, CTexture> m_UnitTex{};
		Dictionary<string_view, CTexture> m_AttachmentTex{};	// #UPDATE_AT_CPP23 flat_map
		vector<CStance> m_rgStances{};
		int32_t m_iMysteryNum{};
		array<float, 3> m_MysteryVector1{};
		array<float, 3> m_MysteryVector2{};

		set<string_view> ListOfFiles() const noexcept;
		set<std::filesystem::path> ListOfFiles(std::filesystem::path const& DataPath) const noexcept;
	};

	struct CFile final : public CBaseParser
	{
	private:
		template <bool bSpecial = false> void ParseBlock(void) noexcept;	// NOTE this can pass link phase is it NEVER got called outside of its impl cpp.
		template <typename T = int> T ParseNumber(void) noexcept;
		string_view ParseString(void) noexcept;

	public:
		bool m_bSpecialFirst{};
		size_t m_iTotalModels{};
		//vector<CBattleModel> m_rgBattleModels{};
		Dictionary<string_view, CBattleModel> m_rgBattleModels{};

		explicit CFile(const char* pszFilePath = "battle_models.modeldb") noexcept : CBaseParser{ pszFilePath } { Initialize(); }

		CFile(CFile const&) noexcept = delete;
		CFile(CFile&&) noexcept = delete;

		CFile& operator= (CFile const&) noexcept = delete;
		CFile& operator= (CFile&&) noexcept = delete;

		void Initialize(void) noexcept;
		bool Sanity(void) const noexcept;	// #UNDONE
		void Save(const char* pszFilePath = "battle_models.modeldb") const noexcept;
		size_t Count(void) const noexcept;
	};
}
