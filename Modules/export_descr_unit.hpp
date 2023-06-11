#pragma once

#include <array>
#include <map>
#include <tuple>
#include <tuple>
#include <vector>

#include <fmt/ranges.h>

using namespace std::literals;

namespace Units
{
	using std::array;
	using std::map;
	using std::string;
	using std::string_view;
	using std::tuple;
	using std::vector;

	using CUnit = map<string, vector<string>>;
	using CFile = vector<CUnit>;

	string Serialize(CUnit const& unit) noexcept;
	CFile Deserialize(const char* file = "export_descr_unit.txt") noexcept;

	// soldier
	using CSoldier = tuple<string, int32_t, int32_t, float>;
	inline string Serialize(CSoldier const& soldier, int16_t iIndent) noexcept { return fmt::format("{0:<{2}}{1}\n", "soldier"sv, fmt::join(soldier, ", "), iIndent); }
	namespace Deserializer { CSoldier Soldier(string_view sz) noexcept; }

	enum ESoldier
	{
		Model = 0,
		HumanCount,
		ExtraCount,
		CollisionMass,
	};

	// mount_effect
	using CMountEffect = tuple<string, int32_t>;
	string Serialize(vector<CMountEffect> const& mount_effect, int16_t iIndent) noexcept;
	namespace Deserializer { vector<CMountEffect> MountEffect(string_view sz) noexcept; }

	enum EMountEffect
	{
		Mount = 0,
		Effect,
	};

	// formation
	using CFormation = tuple<array<float, 2>, array<float, 2>, int32_t, vector<string>>;
	string Serialize(CFormation const& formation, int16_t iIndent) noexcept;
	namespace Deserializer { CFormation Formation(string_view sz) noexcept; }

	enum EFormation
	{
		SpacingNormal,
		SpacingLoose,
		Ranks,
		Formations,
	};


	// stat_[pri/sec]
	//															           5                                     10
	using CWeaponStat = tuple<uint16_t, uint16_t, string, float, uint16_t, string, string, string, string, bool, float, int16_t>;
	inline string Serialize(string_view key, CWeaponStat const& wpnstat, int16_t iIndent) noexcept;
	namespace Deserializer { CWeaponStat WeaponStat(string_view sz) noexcept; }

	enum EWeaponStat
	{
		Damage = 0,
		ChargingBonus,
		MissileType,
		Range,
		Ammo,
		WeaponType,
		TechType,
		DamageType,
		HitSound,
		MusketShotSet,
		MinAttackInterval,
		SkeletonCompensationFactorInMelee,
	};

	// stat_[]_ex
	enum EWeaponStatEx
	{
		AttackBonusVsMounted = 0,
		DefenceBonusVsMounted,
		ArmourPenetration,
	};

	// stat_pri_armour
	using CPrimaryArmour = tuple<uint16_t, uint16_t, uint16_t, string>;
	inline string Serialize(CPrimaryArmour const& stat_pri_armour, int16_t iIndent) noexcept { return fmt::format("{0:<{2}}{1}\n", "stat_pri_armour"sv, fmt::join(stat_pri_armour, ", "), iIndent); }
	namespace Deserializer { CPrimaryArmour PrimaryArmour(string_view sz) noexcept; }

	enum EPrimaryArmour
	{
		Armour = 0,
		Skill,
		Shield,
		HitSound_
	};

	// stat_sec_armour
	using CSecondaryArmour = tuple<uint16_t, uint16_t, string>;
	inline string Serialize(CSecondaryArmour const& stat_sec_armour, int16_t iIndent) noexcept { return fmt::format("{0:<{2}}{1}\n", "stat_sec_armour"sv, fmt::join(stat_sec_armour, ", "), iIndent); }
	namespace Deserializer { CSecondaryArmour SecondaryArmour(string_view sz) noexcept; }

	enum ESecondaryArmour
	{
		Armour_ = 0,
		Skill_,
		HitSound__,
	};

	// stat_ground
	enum EStatGround
	{
		Scrub = 0,
		Sand,
		Forest,
		Snow,
	};

	// stat_mental
	using CStatMental = tuple<int16_t, string, string, bool>;
	inline string Serialize(CStatMental const& stat_mental, int16_t iIndent) noexcept;
	namespace Deserializer { CStatMental StatMental(string_view sz) noexcept; }

	enum EStatMental
	{
		Morale = 0,
		Discipline,
		Training,
		LockMorale,
	};

	// stat_cost
	enum EStatCost
	{
		RecruitTurns = 0,
		Cost,
		Upkeep,
		UpgradeingWeapon,
		UpgradingArmor,

		CustomBattlesCost,
		CustomBattlesPenaltyCount,
		CustomBattlesPenaltyStacks,
	};

	struct unit_t_ver_2 final
	{
		// ; -- Data entries are as follows
		string					m_type{};
		string					m_dictionary{};

		//; --	Category and class define the rough type of the unit. They're used for setting some default attributes and for
		//		determining where units go in formation amongst other things such as tags to support AI army formation
		string					m_category{};
		string					m_class{};
		string					m_voice_type{};
		string					m_accent{};
		string					m_banner_faction{};
		string					m_banner_holy{};
		CSoldier				m_soldier{};
		vector<string>			m_officer{};
		string					m_ship{};
		string					m_engine{};
		string					m_animal{};
		string					m_mount{};
		vector<CMountEffect>	m_mount_effect{};
		vector<string>			m_attributes{};
		CFormation				m_formation{};
		array<uint16_t, 2>		m_stat_health{};	// Human health, animal health

		// ; -- Details of unit's primary weapon. If the unit has a missile weapon it must be the primary
		CWeaponStat			m_stat_pri{};
		array<uint16_t, 3>	m_stat_pri_ex{};
		vector<string>		m_stat_pri_attr{};
		CWeaponStat			m_stat_sec{};
		array<uint16_t, 3>	m_stat_sec_ex{};
		vector<string>		m_stat_sec_attr{};
		CWeaponStat			m_stat_ter{};
		array<uint16_t, 3>	m_stat_ter_ex{};
		vector<string>		m_stat_ter_attr{};
		CPrimaryArmour		m_stat_pri_armour{};
		CSecondaryArmour	m_stat_sec_armour{};
		int16_t				m_stat_heat{};
		array<int16_t, 4>	m_stat_ground{};
		CStatMental			m_stat_mental{};
		int32_t				m_stat_charge_dist{};
		float				m_stat_fire_delay{};	// Type unknown #UNDONE
		array<uint16_t, 8>	m_stat_cost;
		uint16_t			m_stat_stl{ 0 };	// Number of soldiers needed for unit to count as alive
		vector<uint16_t>	m_armour_ug_levels;
		vector<string>		m_armour_ug_models;
		vector<string>		m_ownership;
		array<vector<string>, 3> m_era{};
		int16_t				m_recruit_priority_offset{ 0 };
		float				m_move_speed_mod{ 0 };	// Although if you really want to have this value displayed, it should be 1.0f

		void ParseLine(string_view sz) noexcept;
		string Serialize() const noexcept;
	};
}

