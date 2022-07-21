/*
* Dec 06 2021
*/

module;

#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include <cstdio>

export module export_descr_unit;

import UtlLinearAlgebra;
import UtlString;
import UtlWinConsole;

using namespace std::string_literals;
using namespace std::string_view_literals;

using std::list;
using std::pair;
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

		if (rgsz.size() != 4) [[unlikely]]
		{
			cout_pink() << "[Error] String " << std::quoted("soldier\t"s + sz) << " has " << rgsz.size() << " argument(s).\n";
			cout_gray() << "[Message] Attribute \"soldier\" expected to have 4 arguments.\n";
			
			if (rgsz.size() < 4)
				return;
		}

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

	bool Empty(void) const noexcept
	{
		return m_szModel.empty();
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

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		rg.resize(rgsz.size());
		for (size_t i = 0; i < rgsz.size(); ++i)
			rg[i].Parse(rgsz[i]);
	}

	string ToString(uint8 iIndent = 2) const noexcept
	{
		if (Empty())
			return "";	// optional field

		string ret = "mount_effect";
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{} {:+}", m_szMount, m_iEffect);
		return ret;
	}

	static string ToString(const vector<mount_effect_t>& rg, uint8 iIndent = 2) noexcept
	{
		if (Empty(rg))
			return "";	// optional field

		string ret = "mount_effect";
		for (; iIndent; --iIndent)
			ret += '\t';

		for (const auto& ins : rg)
			ret += std::format("{} {:+}, ", ins.m_szMount, ins.m_iEffect);

		// Remove the ", " at the end.
		if (!rg.empty())
		{
			ret.pop_back();
			ret.pop_back();
		}

		return ret;
	}

	bool Empty(void) const noexcept
	{
		return m_szMount.empty();
	}

	static bool Empty(const vector<mount_effect_t>& rg) noexcept
	{
		for (const auto& ins : rg)
			if (ins.Empty())
				return true;

		return false;
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

	bool Empty(void) const noexcept
	{
		return m_rgszFormations.empty();
	}
};

template<StringLiteral STR> struct wpn_stat_t
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
	bool m_bMusketShotSet{ false };
	float m_flMinAttakcInterval{ 25 };	// in 0.1s
	short m_iSkeletonCompensationFactorInMelee{ 1 };	// Multiple weapons (?)

	void Parse(string& sz) noexcept
	{
		sz.erase(0, STR.length);
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(11);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		if (rgsz.size() != 11 && rgsz.size() != 12) [[unlikely]]
		{
			cout_r() << "[Error] String " << std::quoted(STR.value + "\t"s + sz) << " has " << rgsz.size() << " argument(s).\n";
			cout_gray() << "[Message] Attribute \"" << STR << "\" expected to have 11 arguments for regular troops, or 12 arguments for gunpowder units.\n";

			if (rgsz.size() < 11)
				return;
		}

		m_iDamage = UTIL_StrToNum<uint16>(rgsz[0]);
		m_iChargingBonus = UTIL_StrToNum<uint16>(rgsz[1]);
		m_szMissileType = std::move(rgsz[2]);
		m_flRange = UTIL_StrToNum<float>(rgsz[3]);
		m_iAmmunition = UTIL_StrToNum<uint16>(rgsz[4]);
		m_szWeaponType = std::move(rgsz[5]);
		m_szTechType = std::move(rgsz[6]);
		m_szDamageType = std::move(rgsz[7]);
		m_szHitSound = std::move(rgsz[8]);
		
		if (rgsz.size() == 12) [[unlikely]]
		{
			m_bMusketShotSet = true;
			m_flMinAttakcInterval = UTIL_StrToNum<float>(rgsz[10]);
			m_iSkeletonCompensationFactorInMelee = UTIL_StrToNum<short>(rgsz[11]);
		}
		else [[likely]]
		{
			m_flMinAttakcInterval = UTIL_StrToNum<float>(rgsz[9]);
			m_iSkeletonCompensationFactorInMelee = UTIL_StrToNum<short>(rgsz[10]);
		}
	}

	string ToString(uint8 iIndent = 3) const noexcept
	{
		string ret = STR;
		for (; iIndent; --iIndent)
			ret += '\t';

		if (!m_bMusketShotSet) [[likely]]
		{
			ret += std::format("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
				m_iDamage, m_iChargingBonus, m_szMissileType, m_flRange, m_iAmmunition,
				m_szWeaponType, m_szTechType, m_szDamageType, m_szHitSound, m_flMinAttakcInterval,
				m_iSkeletonCompensationFactorInMelee);
		}
		else [[unlikely]]
		{
			ret += std::format("{}, {}, {}, {}, {}, {}, {}, {}, {}, musket_shot_set, {}, {}",
				m_iDamage, m_iChargingBonus, m_szMissileType, m_flRange, m_iAmmunition,
				m_szWeaponType, m_szTechType, m_szDamageType, m_szHitSound, m_flMinAttakcInterval,
				m_iSkeletonCompensationFactorInMelee);
		}

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

	bool Empty(void) const noexcept
	{
		return !m_iDamage && !m_iChargingBonus &&
			m_szMissileType == "no" && !m_flRange && !m_iAmmunition &&
			m_szWeaponType == "no" && m_szTechType == "melee_simple" && m_szDamageType == "blunt" && m_szHitSound == "none";
	}
};

template<StringLiteral STR> struct wpn_stat_ex_t
{
	uint16 m_iAtkBonusVsMounted{ 0 };
	uint16 m_iDefBonusVsMounted{ 0 };
	uint16 m_iArmourPenetration{ 0 };

	void Parse(string& sz) noexcept
	{
		sz.erase(0, STR.length);
		UTIL_Trim(sz);

		auto lastPos = sz.find_first_not_of(',', 0);
		auto pos = sz.find_first_of(',', lastPos);
		uint16* p = (uint16*)this;

		while (sz.npos != pos || sz.npos != lastPos)
		{
			*p++ = UTIL_StrToNum<uint16>(sz.substr(lastPos, pos - lastPos));
			lastPos = sz.find_first_not_of(',', pos);
			pos = sz.find_first_of(',', lastPos);
		}
	}

	string ToString(uint8 iIndent = 2) const noexcept
	{
		string ret = STR;
		for (; iIndent; --iIndent)
			ret += '\t';

		[&] <size_t... I>(std::index_sequence<I...> seq)
		{
			ret += std::format(UTIL_SpaceCommaFormatter<seq.size()>(), *(uint16*)(size_t(this) + (I * sizeof(uint16)))...);
		}
		(std::make_index_sequence<3>{});

		return ret;
	}

	void Clear(void) noexcept
	{
		memset(this, NULL, sizeof(wpn_stat_ex_t<STR>));
	}

	bool Empty(void) const noexcept
	{
		return[&] <size_t... I>(std::index_sequence<I...> seq) -> bool
		{
			return ((*(uint16*)(size_t(this) + (I * sizeof(uint16))) == 0) && ...);
		}
		(std::make_index_sequence<3>{});
	}
};

struct stat_pri_armour_t
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

	bool Empty(void) const noexcept
	{
		return !m_iArmor && !m_iShield && !m_iSkill && m_szHitSound == "flesh";
	}
};

struct stat_sec_armour_t
{
	uint16 m_iArmor{ 0 };
	uint16 m_iSkill{ 0 };
	string m_szHitSound{ "flesh" };

	void Parse(string& sz) noexcept
	{
		sz.erase(0, strlen_c("stat_sec_armour"));
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(3);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_iArmor = UTIL_StrToNum<uint16>(rgsz[0]);
		m_iSkill = UTIL_StrToNum<uint16>(rgsz[1]);
		m_szHitSound = std::move(rgsz[2]);
	}

	string ToString(uint8 iIndent = 2) const noexcept
	{
		string ret = "stat_sec_armour";
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{}, {}, {}", m_iArmor, m_iSkill, m_szHitSound);
		return ret;
	}

	void Clear(void) noexcept
	{
		m_iArmor = 0;
		m_iSkill = 0;
		m_szHitSound = "flesh";
	}

	bool Empty(void) const noexcept
	{
		return !m_iArmor && !m_iSkill && m_szHitSound == "flesh";
	}
};

struct stat_ground_t
{
	short m_iScrub{ 0 };
	short m_iSand{ 0 };
	short m_iForest{ 0 };
	short m_iSnow{ 0 };

	void Parse(string& sz) noexcept
	{
		sz.erase(0, strlen_c("stat_ground"));
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(4);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_iScrub = UTIL_StrToNum<short>(rgsz[0]);
		m_iSand = UTIL_StrToNum<short>(rgsz[1]);
		m_iForest = UTIL_StrToNum<short>(rgsz[2]);
		m_iSnow = UTIL_StrToNum<short>(rgsz[3]);
	}

	string ToString(uint8 iIndent = 3) const noexcept
	{
		string ret = "stat_ground";
		for (; iIndent; --iIndent)
			ret += '\t';

		ret += std::format("{}, {}, {}, {}", m_iScrub, m_iSand, m_iForest, m_iSnow);
		return ret;
	}

	void Clear(void) noexcept
	{
		m_iScrub = 0;
		m_iSand = 0;
		m_iForest = 0;
		m_iSnow = 0;
	}

	bool Empty(void) const noexcept
	{
		return !m_iForest && !m_iSand && !m_iScrub && !m_iSnow;
	}
};

struct stat_mental_t
{
	short m_iMorale{ 0 };
	string m_szDiscipline{ "normal" };
	string m_szTraining{ "trained" };
	bool m_bLockMorale{ false };

	void Parse(string& sz) noexcept
	{
		sz.erase(0, strlen_c("stat_mental"));
		UTIL_Trim(sz);

		vector<string> rgsz;
		rgsz.reserve(4);

		UTIL_Split(sz, rgsz, ',');
		for (auto& s : rgsz)
			UTIL_Trim(s);

		m_iMorale = UTIL_StrToNum<short>(rgsz[0]);
		m_szDiscipline = std::move(rgsz[1]);
		m_szTraining = std::move(rgsz[2]);

		if (rgsz.size() > 3) [[unlikely]]	// This one is optional
			m_bLockMorale = true;
	}

	string ToString(uint8 iIndent = 3) const noexcept
	{
		string ret = "stat_mental";
		for (; iIndent; --iIndent)
			ret += '\t';

		if (m_bLockMorale) [[unlikely]]
			ret += std::format("{}, {}, {}, lock_morale", m_iMorale, m_szDiscipline, m_szTraining);
		else [[likely]]
			ret += std::format("{}, {}, {}", m_iMorale, m_szDiscipline, m_szTraining);

		return ret;
	}

	void Clear(void) noexcept
	{
		m_iMorale = 0;
		m_szDiscipline = "normal";
		m_szTraining = "trained";
		m_bLockMorale = false;
	}

	bool Empty(void) const noexcept
	{
		return !m_iMorale && m_szDiscipline == "normal" && m_szTraining == "trained";
	}
};

struct stat_cost_t
{
	uint16 m_iRecruitTurns{ 0 };
	uint16 m_iCost{ 0 };
	uint16 m_iUpkeep{ 0 };
	uint16 m_iUpgradingWpn{ 0 };
	uint16 m_iUpgradingArm{ 0 };
	uint16 m_iCustomBattlesCost{ 0 };
	uint16 m_iCustomBattlesPenaltyCount{ 0 };
	uint16 m_iCustomBattlesPenaltyStacks{ 0 };

	void Parse(string& sz) noexcept
	{
		sz.erase(0, strlen_c("stat_cost"));
		UTIL_Trim(sz);

		auto lastPos = sz.find_first_not_of(',', 0);
		auto pos = sz.find_first_of(',', lastPos);
		uint16* p = (uint16*)this;

		while (sz.npos != pos || sz.npos != lastPos)
		{
			*p++ = UTIL_StrToNum<uint16>(sz.substr(lastPos, pos - lastPos));
			lastPos = sz.find_first_not_of(',', pos);
			pos = sz.find_first_of(',', lastPos);
		}
	}

	string ToString(uint8 iIndent = 3) const noexcept
	{
		string ret = "stat_cost";
		for (; iIndent; --iIndent)
			ret += '\t';

		[&] <size_t... I>(std::index_sequence<I...> seq)
		{
			ret += std::format(UTIL_SpaceCommaFormatter<seq.size()>(), *(uint16*)(size_t(this) + (I * sizeof(uint16)))...);
		}
		(std::make_index_sequence<8>{});

		return ret;
	}

	void Clear(void) noexcept
	{
		memset(this, NULL, sizeof(stat_cost_t));
	}

	bool Empty(void) const noexcept
	{
		return [&] <size_t... I>(std::index_sequence<I...> seq) -> bool
		{
			return ((*(uint16*)(size_t(this) + (I * sizeof(uint16))) == 0) && ...);
		}
		(std::make_index_sequence<8>{});
	}
};

export struct Unit_t
{
	// ; -- Data entries are as follows
	string					m_type;
	string					m_dictionary;

	//; --	Category and class define the rough type of the unit. They're used for setting some default attributes and for
	//		determining where units go in formation amongst other things such as tags to support AI army formation
	string					m_category;
	string					m_class;
	string					m_voice_type;
	string					m_accent;
	string					m_banner_faction;
	string					m_banner_holy;
	soldier_t				m_soldier;
	list<string>			m_officer;
	string					m_ship;
	string					m_engine;
	string					m_animal;
	string					m_mount;
	vector<mount_effect_t>	m_mount_effect;
	vector<string>			m_attributes;
	formation_t				m_formation;
	vector<uint16>			m_stat_health;	// Human health, animal health

	// ; -- Details of unit's primary weapon. If the unit has a missile weapon it must be the primary
	wpn_stat_t<"stat_pri">			m_stat_pri;
	wpn_stat_ex_t<"stat_pri_ex">	m_stat_pri_ex;
	vector<string>					m_stat_pri_attr;
	wpn_stat_t<"stat_sec">			m_stat_sec;
	wpn_stat_ex_t<"stat_sec_ex">	m_stat_sec_ex;
	vector<string>					m_stat_sec_attr;
	wpn_stat_t<"stat_ter">			m_stat_ter;
	wpn_stat_ex_t<"stat_ter_ex">	m_stat_ter_ex;
	vector<string>					m_stat_ter_attr;
	stat_pri_armour_t		m_stat_pri_armour;
	stat_sec_armour_t		m_stat_sec_armour;
	short					m_stat_heat{ 0 };
	stat_ground_t			m_stat_ground;
	stat_mental_t			m_stat_mental;
	int						m_stat_charge_dist{ 0 };
	float					m_stat_fire_delay{ 0 };	// Type unknown #UNDONE
	stat_cost_t				m_stat_cost;
	uint16					m_stat_stl{ 0 };	// Number of soldiers needed for unit to count as alive
	vector<uint16>			m_armour_ug_levels;
	vector<string>			m_armour_ug_models;
	vector<string>			m_ownership;
	vector<string>			m_era_0, m_era_1, m_era_2;
	int						m_recruit_priority_offset{ 0 };
	float					m_move_speed_mod{ 0 };	// Although if you really want to have this value displayed, it should be 1.0f

	void Parse(string& sz) noexcept
	{
#define PARSE_STRING(x)	else if (!strnicmp_c<#x>(sz.c_str()))	\
						{										\
							sz.erase(0, strlen_c(#x));			\
							UTIL_Trim(sz);						\
							m_##x = std::move(sz);				\
						}

#define PARSE_OBJECT(x)	else if (!strnicmp_c<#x>(sz.c_str()))	\
						{										\
							m_##x.Parse(sz);					\
						}

#define PARSE_VARY_STR(x)	else if (!strnicmp_c<#x>(sz.c_str()))	\
							{										\
								sz.erase(0, strlen_c(#x));			\
								UTIL_Trim(sz);						\
																	\
								m_##x.clear();						\
								UTIL_Split(sz, m_##x, ',');			\
								for (auto& s : m_##x)				\
									UTIL_Trim(s);					\
							}

#define PARSE_VARY_STR2(key, var)	else if (!strnicmp_c<key>(sz.c_str()))	\
									{										\
										sz.erase(0, strlen_c(key));			\
										UTIL_Trim(sz);						\
																			\
										m_##var.clear();					\
										UTIL_Split(sz, m_##var, ',');		\
										for (auto& s : m_##var)				\
											UTIL_Trim(s);					\
									}

#define PARSE_VARY_NUM(x)	else if (!strnicmp_c<#x>(sz.c_str()))	\
							{										\
								sz.erase(0, strlen_c(#x));			\
								UTIL_Trim(sz);						\
																	\
								m_##x.clear();						\
								UTIL_SplitIntoNums<decltype(m_##x)::value_type>(sz, m_##x, ',');\
							}

#define PARSE_NUMBER(x)	else if (!strnicmp_c<#x>(sz.c_str()))	\
						{										\
							sz.erase(0, strlen_c(#x));			\
							UTIL_Trim(sz);						\
																\
							m_##x = UTIL_StrToNum<decltype(m_##x)>(sz);\
						}

		if (false) [[unlikely]]
			return;

		PARSE_STRING(type)
		PARSE_STRING(dictionary)
		PARSE_STRING(category)
		PARSE_STRING(class)
		PARSE_STRING(voice_type)
		PARSE_STRING(accent)

		else if (!strnicmp_c<"banner">(sz.c_str()))
		{
			sz.erase(0, strlen_c("banner"));
			UTIL_Trim(sz);

			if (!strnicmp_c<"faction">(sz.c_str()))
			{
				sz.erase(0, strlen_c("faction"));
				UTIL_Trim(sz);

				m_banner_faction = std::move(sz);
			}
			else if (!strnicmp_c<"holy">(sz.c_str()))
			{
				sz.erase(0, strlen_c("holy"));
				UTIL_Trim(sz);

				m_banner_holy = std::move(sz);
			}
			else
			{
				cout_w() << "[Error] String " << std::quoted("banner "s + sz) << " cannot be prased.\n";
			}
		}

		PARSE_OBJECT(soldier)

		else if (!strnicmp_c<"officer">(sz.c_str()))	// "officer" attribute is not separated by comma, but new line.
		{
			sz.erase(0, strlen_c("officer"));
			UTIL_Trim(sz);
			m_officer.emplace_back(std::move(sz));
		}

		PARSE_STRING(ship)
		PARSE_STRING(engine)
		PARSE_STRING(animal)

		else if (!strnicmp_c<"mount_effect">(sz.c_str()))
			mount_effect_t::Parse(m_mount_effect, sz);
		PARSE_STRING(mount)

		PARSE_VARY_STR(attributes)
		PARSE_OBJECT(formation)
		PARSE_VARY_NUM(stat_health)
		PARSE_OBJECT(stat_pri_armour)
		PARSE_OBJECT(stat_sec_armour)

		PARSE_VARY_STR(stat_pri_attr)
		PARSE_OBJECT(stat_pri_ex)
		PARSE_OBJECT(stat_pri)
		PARSE_VARY_STR(stat_sec_attr)
		PARSE_OBJECT(stat_sec_ex)
		PARSE_OBJECT(stat_sec)
		PARSE_VARY_STR(stat_ter_attr)
		PARSE_OBJECT(stat_ter_ex)
		PARSE_OBJECT(stat_ter)

		PARSE_NUMBER(stat_heat)
		PARSE_OBJECT(stat_ground)
		PARSE_OBJECT(stat_mental)
		PARSE_NUMBER(stat_charge_dist)
		PARSE_NUMBER(stat_fire_delay)

		else if (!strnicmp_c<"stat_food">(sz.c_str()))
			return;	// Obsolete

		PARSE_OBJECT(stat_cost)
		PARSE_NUMBER(stat_stl)
		PARSE_VARY_NUM(armour_ug_levels)
		PARSE_VARY_STR(armour_ug_models)
		PARSE_VARY_STR(ownership)
		PARSE_VARY_STR2("era 0", era_0)
		PARSE_VARY_STR2("era 1", era_1)
		PARSE_VARY_STR2("era 2", era_2)
		PARSE_NUMBER(recruit_priority_offset)
		PARSE_NUMBER(move_speed_mod)

		else
			cout_w() << "[Error] String " << std::quoted(sz) << " cannot be parsed.\n";

#undef PARSE_STRING
#undef PARSE_OBJECT
#undef PARSE_VARY_STR
#undef PARSE_VARY_STR2
#undef PARSE_VARY_NUM
#undef PARSE_NUMBER
	}
};

export auto operator<< (OStream auto& lhs, const Unit_t& rhs) noexcept -> decltype(lhs)&
{
	using namespace std;

#define WRITE_STRING(x, indent, opt)	if (!rhs.m_##x.empty()) lhs << #x << UTIL_Indent<indent>() << rhs.m_##x << endl; else if (!opt) lhs << #x << UTIL_Indent<indent>() << "no" << endl
#define WRITE_NUMBER(x, indent, opt)	if (rhs.m_##x != 0) lhs << #x << UTIL_Indent<indent>() << rhs.m_##x << endl; else if (!opt) lhs << #x << UTIL_Indent<indent>() << 0 << endl
#define WRITE_STRING2(key, var, indent)	if (!rhs.m_##var.empty()) lhs << key << UTIL_Indent<indent>() << rhs.m_##var << endl	// Only used for banner field. And it's optional.
#define WRITE_OBJECT(x, opt)			if (!opt || !rhs.m_##x.Empty()) lhs << rhs.m_##x.ToString() << endl
#define WRITE_VAR_RNG(x, indent, opt)	if (!rhs.m_##x.empty())													\
										{																		\
											lhs << #x << UTIL_Indent<indent>();									\
											for (auto it = rhs.m_##x.begin(); it != rhs.m_##x.end(); ++it)		\
												lhs << *it << (distance(it, rhs.m_##x.end()) == 1 ? "" : ", ");	\
											lhs << endl;														\
										}																		\
										else if (!opt)	/* #UNTESTED is this the default value? */				\
											lhs << #x << UTIL_Indent<indent>() << "no" << endl
#define WRITE_VAR_RNG2(key, var, indent)	if (!rhs.m_##var.empty())													\
											{																			\
												lhs << key << UTIL_Indent<indent>();									\
												for (auto it = rhs.m_##var.begin(); it != rhs.m_##var.end(); ++it)		\
													lhs << *it << (distance(it, rhs.m_##var.end()) == 1 ? "" : ", ");	\
												lhs << endl;															\
											}else

	WRITE_STRING(type, 4, false);
	WRITE_STRING(dictionary, 3, false);
	WRITE_STRING(category, 3, false);
	WRITE_STRING(class, 4, false);
	WRITE_STRING(voice_type, 3, false);
	WRITE_STRING(accent, 4, true);
	WRITE_STRING2("banner faction", banner_faction, 2);
	WRITE_STRING2("banner holy", banner_holy, 3);
	WRITE_OBJECT(soldier, false);

	if (!rhs.m_officer.empty())
		for (const auto& ins : rhs.m_officer)
			lhs << "officer" << UTIL_Indent<4>() << ins << endl;

	WRITE_STRING(ship, 4, true);
	WRITE_STRING(engine, 4, true);
	WRITE_STRING(animal, 4, true);
	WRITE_STRING(mount, 4, true);

	if (!rhs.m_mount_effect.empty())
		lhs << mount_effect_t::ToString(rhs.m_mount_effect) << endl;

	WRITE_VAR_RNG(attributes, 3, false);
	WRITE_OBJECT(formation, false);
	WRITE_VAR_RNG(stat_health, 3, false);

	WRITE_OBJECT(stat_pri, false);
	WRITE_OBJECT(stat_pri_ex, true);
	WRITE_VAR_RNG(stat_pri_attr, 2, false);
	WRITE_OBJECT(stat_sec, false);
	WRITE_OBJECT(stat_sec_ex, true);
	WRITE_VAR_RNG(stat_sec_attr, 2, false);
	WRITE_OBJECT(stat_ter, true);
	WRITE_OBJECT(stat_ter_ex, true);
	WRITE_VAR_RNG(stat_ter_attr, 2, true);

	WRITE_OBJECT(stat_pri_armour, false);
	WRITE_OBJECT(stat_sec_armour, false);
	WRITE_NUMBER(stat_heat, 3, false);
	WRITE_OBJECT(stat_ground, false);
	WRITE_OBJECT(stat_mental, false);
	WRITE_NUMBER(stat_charge_dist, 1, false);
	WRITE_NUMBER(stat_fire_delay, 2, false);
	lhs << "stat_food			60, 300" << endl;	// dummy.
	WRITE_OBJECT(stat_cost, false);
	WRITE_NUMBER(stat_stl, 3, true);
	WRITE_VAR_RNG(armour_ug_levels, 1, false);
	WRITE_VAR_RNG(armour_ug_models, 1, false);
	WRITE_VAR_RNG(ownership, 3, false);
	WRITE_VAR_RNG2("era 0", era_0, 4);
	WRITE_VAR_RNG2("era 1", era_1, 4);
	WRITE_VAR_RNG2("era 2", era_2, 4);
	WRITE_NUMBER(recruit_priority_offset, 1, true);
	WRITE_NUMBER(move_speed_mod, 2, true);

#undef WRITE_STRING
#undef WRITE_STRING2
#undef WRITE_OBJECT

	return lhs;
}

export struct edu_file_t
{
private:
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
	list<Unit_t> m_Units{};

	explicit edu_file_t(const char* pszFile = "export_descr_unit.txt") noexcept { Set(pszFile); }
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
		Unit_t* pCurrent = nullptr;

		Seek(0, SEEK_SET);
		while (!Eof())
		{
			string sz = GetLine();
			if (!sz.empty() && sz[0] == ';')	// Comment line.
				continue;

			if (auto pos = sz.find_first_of(';'); pos != sz.npos)	// Remove attached comment line.
				sz.erase(pos, sz.npos);

			if (!strnicmp_c<"type">(sz.c_str()))
			{
				m_Units.emplace_back();
				pCurrent = &m_Units.back();
			}

			pCurrent->Parse(sz);
		}
	}
	auto Count(void) noexcept
	{
		return m_Units.size();
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

			cout_w() << "There are " << iCount << " units presented.\n";
		}
		else
			cout_w() << "Could not open file: export_descr_unit.txt\n";

		return iCount;
	}
}
