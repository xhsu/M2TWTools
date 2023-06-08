/*
* Module:	Dec 06 2021
* Fix:		Jun 08 2023
*/

#pragma once

#include <array>
#include <string>
#include <vector>

namespace BattleModels
{
	using std::array;
	using std::string;
	using std::vector;
	using std::pair;

	struct CMesh final
	{
		string m_szPath{};
		size_t m_iDistance{};
	};

	struct CTexture final
	{
		string m_szTex{};
		string m_szNorm{};
		string m_szSprite{};
	};

	struct CMount final
	{
		std::string m_szType{};
		std::string m_szWpn1{};
		std::string m_szWpn2{};
	};

	struct CStance final
	{
		CMount m_Mount{};
		vector<string> m_rgszPrim{};
		vector<string> m_rgszSec{};
	};

	struct CBattleModel final
	{
		string m_szName{};
		vector<vector<CMesh>> m_rgrgMeshGroup{};
		//unordered_map<string, CTexture> m_UnitTex{};
		vector<pair<string, CTexture>> m_UnitTex{};
		//unordered_map<string, CTexture> m_AttachmentTex{};
		vector<pair<string, CTexture>> m_AttachmentTex{};
		vector<CStance> m_rgStances{};
		int32_t m_iMysteryNum{};
		array<float, 3> m_MysteryVector1{};
		array<float, 3> m_MysteryVector2{};
	};

	struct CFile final
	{
	private:
		long m_iSize = 0;
		char* m_pszBuffer = nullptr;
		char* m_pszCurPos = nullptr;
		char* m_pBufEnd = nullptr;

		void Reset(void) noexcept;
		void Set(const char* pszFilePath = "battle_models.modeldb") noexcept;
		void Skip(int iCount = 1) noexcept;
		void SkipUntilNonspace(void) noexcept;
		void Rewind(int iCount = 1) noexcept;
		void RewindUntilNonspace(void) noexcept;
		string ParseString(void) noexcept;
		string ReadN(int iCount) noexcept;
		string ParseBySpace(void) noexcept;
		bool Eof(void) const noexcept;
		void Seek(int iOffset, int iMode = SEEK_CUR) noexcept;

	public:
		bool m_bSpecialFirst{};
		size_t m_iTotalModels{};
		vector<CBattleModel> m_rgBattleModels{};

		explicit CFile(const char* pszFilePath = "battle_models.modeldb") noexcept { Set(pszFilePath); Initialize(); }

		CFile(CFile const&) noexcept = default;
		CFile(CFile&&) noexcept = default;

		CFile& operator= (CFile const&) noexcept = default;
		CFile& operator= (CFile&&) noexcept = default;

		~CFile(void) noexcept { Reset(); }

		void Initialize(void) noexcept;
		template<bool bSpecial = false> void Parse(void) noexcept;	// NOTE this can pass link phase is it NEVER got called outside of its impl cpp.
		bool Sanity(void) const noexcept;
		void Save(const char* pszFilePath = "battle_models.modeldb") const noexcept;
		size_t Count(void) const noexcept;
	};
}
