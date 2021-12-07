/*
* Dec 06 2021
*/

module;

#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <cstdio>

export module export_descr_unit;

import UtlLinearAlgebra;
import UtlString;

using std::string;
using std::vector;

using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;

struct soldier_t
{
	string m_szModel;
	int m_iHumanCount{ 0 };
	int m_iExtraCount{ 0 };
	float m_flCollisionMass{ 0.0f };

	void Parse(string& sz) noexcept
	{
		sz.erase(0, strlen_c("soldier"));
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(4);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_szModel = std::move(rgsz[0]);
		m_iHumanCount = UTIL_StrToNum<int>(rgsz[1]);
		m_iExtraCount = UTIL_StrToNum<int>(rgsz[2]);
		m_flCollisionMass = UTIL_StrToNum<float>(rgsz[3]);
	}

	string ToString(uint8 iIndent = 4) const noexcept
	{
		string ret = "soldier";
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{}, {}, {}, {}", m_szModel, m_iHumanCount, m_iExtraCount, m_flCollisionMass);
		return ret;
	}

	void Clear(void) noexcept
	{
		m_szModel.clear();
		m_iHumanCount = 0;
		m_iExtraCount = 0;
		m_flCollisionMass = 0;
	}
};

struct mount_effect_t
{
	string m_szMount;
	int m_iEffect{ 0 };

	void Parse(const string& sz) noexcept
	{
		vector<string> rgsz;
		rgsz.reserve(2);

		UTIL_Split(sz, rgsz, ' ');

		m_szMount = std::move(rgsz[0]);
		m_iEffect = UTIL_StrToNum<int>(rgsz[1]);
	}

	static void Parse(vector<mount_effect_t>& rg, string& sz) noexcept
	{
		sz.erase(0, strlen_c("mount_effect"));
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(2);

		UTIL_Split(sz, rgsz, string(","));
		for (auto& s : rgsz)
			UTIL_Trim(s);

		rg.resize(rgsz.size());
		for (size_t i = 0; i < rgsz.size(); ++i)
			rg[i].Parse(rgsz[i]);
	}

	string ToString(uint8 iIndent = 2) const noexcept
	{
		string ret = "mount_effect";
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{} {:+}", m_szMount, m_iEffect);
		return ret;
	}

	static string ToString(const vector<mount_effect_t>& rg, uint8 iIndent = 1) noexcept
	{
		string ret = "mount_effect";
		for (; iIndent; --iIndent)
			ret += '\t';

		for (const auto& obj : rg)
			ret += std::format("{} {:+}, ", obj.m_szMount, obj.m_iEffect);

		// Remove the ", " at the end.
		if (!rg.empty())
		{
			ret.pop_back();
			ret.pop_back();
		}

		return ret;
	}
};

struct formation_t
{
	Vector2D m_vecSpacingNormal;
	Vector2D m_vecSpacingLoose;
	int m_iRanks{ 0 };
	vector<string> m_rgszFormations;

	void Parse(string& sz) noexcept
	{
		sz.erase(0, strlen_c("formation"));
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(7);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_vecSpacingNormal.x = UTIL_StrToNum<vec_t>(rgsz[0]);
		m_vecSpacingNormal.y = UTIL_StrToNum<vec_t>(rgsz[1]);
		m_vecSpacingLoose.x = UTIL_StrToNum<vec_t>(rgsz[2]);
		m_vecSpacingLoose.y = UTIL_StrToNum<vec_t>(rgsz[3]);
		m_iRanks = UTIL_StrToNum<int>(rgsz[4]);

		m_rgszFormations.reserve(2);
		for (size_t i = 5; i < rgsz.size(); ++i)
			m_rgszFormations.emplace_back(std::move(rgsz[i]));
	}

	string ToString(uint8 iIndent = 3) const noexcept
	{
		string ret = "formation";
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{}, {}, {}, {}, {}", m_vecSpacingNormal.x, m_vecSpacingNormal.y, m_vecSpacingLoose.x, m_vecSpacingLoose.y, m_iRanks);
		for (auto& s : m_rgszFormations)
			ret += ", " + s;

		return ret;
	}

	void Clear(void) noexcept
	{
		m_vecSpacingNormal.Clear();
		m_vecSpacingLoose.Clear();
		m_iRanks = 0;
		m_rgszFormations.clear();
	}
};

struct wpn_stat_t
{
	uint16 m_iDamage{ 0 };
	uint16 m_iChargingBonus{ 0 };
	string m_szMissileType{ "no" };
	float m_flRange{ 0.0f };
	uint16 m_iAmmunition{ 0 };
	string m_szWeaponType{ "no" };
	string m_szTechType{ "melee_simple" };
	string m_szDamageType{ "blunt" };
	string m_szHitSound{ "none" };
	float m_flMinAttakcInterval{ 25 };	// in 0.1s
	int m_iSkeletonCompensationFactorInMelee{ 1 };	// Multiple weapons (?)

	template<StringLiteral STR> void Parse(string& sz) noexcept
	{
		sz.erase(0, STR.length);
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(11);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_iDamage = UTIL_StrToNum<uint16>(rgsz[0]);
		m_iChargingBonus = UTIL_StrToNum<uint16>(rgsz[1]);
		m_szMissileType = std::move(rgsz[2]);
		m_flRange = UTIL_StrToNum<float>(rgsz[3]);
		m_iAmmunition = UTIL_StrToNum<uint16>(rgsz[4]);
		m_szWeaponType = std::move(rgsz[5]);
		m_szTechType = std::move(rgsz[6]);
		m_szDamageType = std::move(rgsz[7]);
		m_szHitSound = std::move(rgsz[8]);
		m_flMinAttakcInterval = UTIL_StrToNum<float>(rgsz[9]);
		m_iSkeletonCompensationFactorInMelee = UTIL_StrToNum<int>(rgsz[10]);
	}

	template<StringLiteral STR> string ToString(uint8 iIndent = 3) const noexcept
	{
		string ret = STR;
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
			m_iDamage, m_iChargingBonus, m_szMissileType, m_flRange, m_iAmmunition,
			m_szWeaponType, m_szTechType, m_szDamageType, m_szHitSound, m_flMinAttakcInterval,
			m_iSkeletonCompensationFactorInMelee);

		return ret;
	}

	void Clear(void) noexcept
	{
		m_iDamage = 0;
		m_iChargingBonus = 0;
		m_szMissileType = "no";
		m_flRange = 0.0f;
		m_iAmmunition = 0;
		m_szWeaponType = "no";
		m_szTechType = "melee_simple";
		m_szDamageType = "blunt";
		m_szHitSound = "none";
		m_flMinAttakcInterval = 25;
		m_iSkeletonCompensationFactorInMelee = 1;
	}
};

struct wpn_stat_ex_t
{
	uint16 m_iAtkBonusVsMounted{ 0 };
	uint16 m_iDefBonusVsMounted{ 0 };
	uint16 m_iArmourPenetration{ 0 };

	template<StringLiteral STR> void Parse(string& sz) noexcept
	{
		sz.erase(0, STR.length);
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(3);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_iAtkBonusVsMounted = UTIL_StrToNum<uint16>(rgsz[0]);
		m_iDefBonusVsMounted = UTIL_StrToNum<uint16>(rgsz[1]);
		m_iArmourPenetration = UTIL_StrToNum<uint16>(rgsz[2]);
	}

	template<StringLiteral STR> string ToString(uint8 iIndent = 2) const noexcept
	{
		string ret = STR;
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{}, {}, {}", m_iAtkBonusVsMounted, m_iDefBonusVsMounted, m_iArmourPenetration);
		return ret;
	}

	void Clear(void) noexcept
	{
		m_iAtkBonusVsMounted = 0;
		m_iDefBonusVsMounted = 0;
		m_iArmourPenetration = 0;
	}
};

export struct stat_pri_armour_t
{
	uint16 m_iArmor{ 0 };
	uint16 m_iSkill{ 0 };
	uint16 m_iShield{ 0 };
	string m_szHitSound{ "flesh" };

	void Parse(string& sz) noexcept
	{
		sz.erase(0, strlen_c("stat_pri_armour"));
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(4);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_iArmor = UTIL_StrToNum<uint16>(rgsz[0]);
		m_iSkill = UTIL_StrToNum<uint16>(rgsz[1]);
		m_iShield = UTIL_StrToNum<uint16>(rgsz[2]);
		m_szHitSound = std::move(rgsz[3]);
	}

	string ToString(uint8 iIndent = 2) const noexcept
	{
		string ret = "stat_pri_armour";
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{}, {}, {}, {}", m_iArmor, m_iSkill, m_iShield, m_szHitSound);
		return ret;
	}

	void Clear(void) noexcept
	{
		m_iArmor = 0;
		m_iSkill = 0;
		m_iShield = 0;
		m_szHitSound = "flesh";
	}
};

struct stat_sec_armour_t
{
	uint16 m_iArmor{ 0 };
	uint16 m_iSkill{ 0 };
	string m_szHitSound;
};

struct stat_ground_t
{
	short m_iScrub{ 0 };
	short m_iSand{ 0 };
	short m_iForest{ 0 };
	short m_iSnow{ 0 };
};

struct stat_mental_t
{
	short m_iMorale{ 0 };
	string m_szDiscipline;
	string m_szTraining;
	bool m_bLockMorale{ false };
};

struct stat_cost_t
{
	uint16 m_iRecruitTurns{ 1 };
	uint16 m_iCost{ 0 };
	uint16 m_iUpkeep{ 0 };
	uint16 m_iUpgradingWpn{ 0 };
	uint16 m_iUpgradingArm{ 0 };
	uint16 m_iCustomBattlesCost{ 0 };
	uint16 m_iCustomBattlesPenaltyCount{ 0 };
	uint16 m_iCustomBattlesPenaltyStacks{ 0 };
};

export struct Unit_t
{
	// ; -- Data entries are as follows
	string m_szType;
	string m_szDictionary;

	//; --	Category and class define the rough type of the unit. They're used for setting some default attributes and for
	//		determining where units go in formation amongst other things such as tags to support AI army formation
	string m_szCategory;
	string m_szClass;
	string m_szVoiceType;
	string m_szAccent;
	string m_szBannerFaction;
	string m_szBannerHoly;
	soldier_t m_Soldier;
	string m_szOfficer;
	string m_szShip;
	string m_szEngine;
	string m_szAnimal;
	string m_szMount;
	vector<mount_effect_t> m_rgMountEffect;
	vector<string> m_rgszAttributes;
	formation_t m_Formation;
	vector<unsigned> m_rgiHealth;

	// ; -- Details of unit's primary weapon. If the unit has a missile weapon it must be the primary
	wpn_stat_t m_StatPri;
	wpn_stat_ex_t m_StatPriEx;
	vector<string> m_rgszStatPriAttr;
	wpn_stat_t m_StatSec;
	wpn_stat_ex_t m_StatSecEx;
	vector<string> m_rgszStatSecAttr;
	wpn_stat_t m_StatTer;
	wpn_stat_ex_t m_StatTerEx;
	vector<string> m_rgszStatTerAttr;
	stat_pri_armour_t m_StatPriArmour;
	stat_sec_armour_t m_StatSecArmour;
	short m_iStatHeat{ 0 };
	stat_ground_t m_StatGround;
	int m_iStatChargeDist{ 0 };
	float m_iStatFireDelay{ 0 };	// Type unknown #UNDONE
	stat_cost_t m_StatCost;
	uint16 m_iMinimalSoldiersToConsiderAlive{ 0 };	// stat_stl
	vector<uint8> m_rgArmourUgLevels;
	vector<string> m_rgszArmourUgModels;
	vector<string> m_rgszOwnership;
	vector<string> m_rgszEra0, m_rgszEra1, m_rgszEra2;
	int m_iRecruitPriorityOffset{ 0 };
	float m_flMoveSpeedMod{ 1.0f };
};

export struct edu_file_t
{
public:
	long m_iSize = 0;
	char* m_pszBuffer = nullptr;
	char* m_pszCurPos = nullptr;
	char* m_pBufEnd = nullptr;

	void Reset(void) noexcept
	{
		m_iSize = 0;

		if (m_pszBuffer)
			free(m_pszBuffer);
		m_pszBuffer = nullptr;

		m_pszCurPos = nullptr;
		m_pBufEnd = nullptr;
	}
	void Set(const char* pszFilePath = "export_descr_unit.txt") noexcept
	{
		Reset();

		FILE* f = fopen(pszFilePath, "rb");
		if (!f)
			return;

		fseek(f, 0, SEEK_END);
		m_iSize = ftell(f);

		m_pszBuffer = (char*)calloc(m_iSize + 1, sizeof(char));
		fseek(f, 0, SEEK_SET);
		fread(m_pszBuffer, sizeof(char), m_iSize, f);

		fclose(f);

		m_pszCurPos = m_pszBuffer;
		m_pBufEnd = m_pszBuffer + m_iSize;	// Landed at the last '\0'
	}
	bool Eof(void) const noexcept
	{
		return m_pszCurPos == m_pBufEnd;
	}
	void Seek(int iOffset, int iMode = SEEK_CUR) noexcept
	{
		switch (iMode)
		{
		case SEEK_SET:
			m_pszCurPos = m_pszBuffer;
			goto LAB_HANDLE_OFS;

		case SEEK_END:
			m_pszCurPos = m_pBufEnd;
			[[fallthrough]];

		case SEEK_CUR:
		LAB_HANDLE_OFS:;
			m_pszCurPos += iOffset;

		default:
			break;
		}
	}
	void Skip(size_t iCount = 1) noexcept
	{
		while (iCount && (++m_pszCurPos) != m_pBufEnd)
		{
			switch (*m_pszCurPos)
			{
			case '\r':
			case '\n':	// To use before Initialize()
			case '\0':
				--iCount;
				SkipUntilNonspace();
				[[fallthrough]];

			default:
				break;
			}
		}
	}
	void SkipUntilNonspace(void) noexcept
	{
		while (m_pszCurPos != m_pBufEnd && (isspace(*m_pszCurPos) || *m_pszCurPos == '\0')) { ++m_pszCurPos; }
	}
	void Rewind(size_t iCount = 1) noexcept
	{
		while (iCount && (--m_pszCurPos) != m_pszBuffer)	// m_pszBuffer represent the first element of this string.
		{
			switch (*m_pszCurPos)
			{
			case '\r':
			case '\n':	// To use before Initialize()
			case '\0':
				--iCount;
				RewindUntilNonspace();
				RewindToSectionBeginning();
				[[fallthrough]];

			default:
				break;
			}
		}
	}
	void RewindUntilNonspace(void) noexcept
	{
		while (m_pszCurPos != m_pszBuffer && (isspace(*m_pszCurPos) || *m_pszCurPos == '\0')) { --m_pszCurPos; }
	}
	void RewindToSectionBeginning(void) noexcept
	{
		while (m_pszCurPos != m_pszBuffer && *m_pszCurPos != '\0') { --m_pszCurPos; }
		
		if (*m_pszCurPos == '\0')
			++m_pszCurPos;	// Shouldn't stop at the terminator.
	}
	string GetLine(void) noexcept
	{
		string ret = m_pszCurPos;
		Skip();
		return ret;
	}

public:
	edu_file_t(const char* pszFile = "export_descr_unit.txt") noexcept { Set(pszFile); }
	virtual ~edu_file_t(void) noexcept { Reset(); }

	void Initialize(void) noexcept
	{
		Seek(0, SEEK_SET);

		for (; m_pszCurPos != m_pBufEnd; ++m_pszCurPos)
		{
			switch (*m_pszCurPos)
			{
			case '\n':
			case '\r':
				*m_pszCurPos = '\0';
				[[fallthrough]];

			default:
				break;
			}
		}

		Seek(0, SEEK_SET);
	}

	void Parse(void) noexcept
	{
	}

	int Count(void) noexcept
	{
		int iCount = 0;
		Seek(0, SEEK_SET);

		for (char* p = m_pszCurPos; m_pszCurPos != m_pBufEnd; ++m_pszCurPos)
		{
			switch (*m_pszCurPos)
			{
			case '\r':
			case '\n':

			default:
				break;
			}
		}

		return iCount;
	}
};

namespace EDU
{
	int Count(void) noexcept
	{
		int iCount = 0;

		if (std::ifstream f("export_descr_unit.txt"); f)
		{
			while (!f.eof())
			{
				string sz;
				std::getline(f, sz);

				if (sz.starts_with(';'))
					continue;

				if (sz.starts_with("type"))
					iCount++;
			}

			std::cout << "There are " << iCount << " units presented.\n";
		}
		else
			std::cout << "Could not open file: export_descr_unit.txt\n";

		return iCount;
	}
}
