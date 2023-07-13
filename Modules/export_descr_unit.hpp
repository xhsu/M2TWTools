#pragma once

#include <array>
#include <map>
#include <vector>

#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include "String.hpp"

using namespace std::literals;

namespace Units
{
	using std::array;
	using std::map;
	using std::optional;
	using std::string;
	using std::string_view;
	using std::tuple;
	using std::vector;

	template <typename K, typename V>
	using Dictionary = std::map<K, V, CaseIgnoredLess>;

	using CUnit = map<string, vector<string>>;
	using CFile = vector<CUnit>;

	string Serialize(CUnit const& unit) noexcept;
	CFile Deserialize(const char* file = "export_descr_unit.txt") noexcept;

	// soldier
	using CSoldier = tuple<string_view, int32_t, int32_t, float>;
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
	using CMountEffect = tuple<string_view, int32_t>;
	string Serialize(vector<CMountEffect> const& mount_effect, int16_t iIndent) noexcept;
	namespace Deserializer { vector<CMountEffect> MountEffect(string_view sz) noexcept; }

	enum EMountEffect
	{
		Mount = 0,
		Effect,
	};

	// formation
	using CFormation = tuple<array<float, 2>, array<float, 2>, int32_t, vector<string_view>>;
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
	//															                5                                                         10
	using CWeaponStat = tuple<uint16_t, uint16_t, string_view, float, uint16_t, string_view, string_view, string_view, string_view, bool, float, int16_t>;
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
	using CPrimaryArmour = tuple<uint16_t, uint16_t, uint16_t, string_view>;
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
	using CSecondaryArmour = tuple<uint16_t, uint16_t, string_view>;
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
	using CStatMental = tuple<int16_t, string_view, string_view, bool>;
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
		optional<string_view>			m_type{};
		optional<string_view>			m_dictionary{};

		//; --	Category and class define the rough type of the unit. They're used for setting some default attributes and for
		//		determining where units go in formation amongst other things such as tags to support AI army formation
		optional<string_view>			m_category{};
		optional<string_view>			m_class{};
		optional<string_view>			m_voice_type{};
		optional<string_view>			m_accent{};
		optional<vector<array<string_view, 2>>>	m_banner{};
		optional<CSoldier>				m_soldier{};
		optional<vector<string_view>>	m_officer{};
		optional<string_view>			m_ship{};
		optional<string_view>			m_engine{};
		optional<string_view>			m_animal{};
		optional<string_view>			m_mount{};
		optional<vector<CMountEffect>>	m_mount_effect{};
		optional<vector<string_view>>	m_attributes{};
		optional<CFormation>			m_formation{};
		optional<array<uint16_t, 2>>	m_stat_health{};	// Human health, animal health

		// ; -- Details of unit's primary weapon. If the unit has a missile weapon it must be the primary
		optional<CWeaponStat>				m_stat_pri{};
		optional<array<uint16_t, 3>>		m_stat_pri_ex{};
		optional<vector<string_view>>		m_stat_pri_attr{};
		optional<CWeaponStat>				m_stat_sec{};
		optional<array<uint16_t, 3>>		m_stat_sec_ex{};
		optional<vector<string_view>>		m_stat_sec_attr{};
		optional<CWeaponStat>				m_stat_ter{};
		optional<array<uint16_t, 3>>		m_stat_ter_ex{};
		optional<vector<string_view>>		m_stat_ter_attr{};
		optional<CPrimaryArmour>			m_stat_pri_armour{};
		optional<CSecondaryArmour>			m_stat_sec_armour{};
		optional<int16_t>					m_stat_heat{};
		optional<array<int16_t, 4>>			m_stat_ground{};
		optional<CStatMental>				m_stat_mental{};
		optional<int32_t>					m_stat_charge_dist{};
		optional<float>						m_stat_fire_delay{};	// Type unknown #UNDONE
		optional<array<uint16_t, 8>>		m_stat_cost;
		optional<uint16_t>					m_stat_stl{};	// Number of soldiers needed for unit to count as alive
		optional<vector<uint16_t>>			m_armour_ug_levels;
		optional<vector<string_view>>		m_armour_ug_models;
		optional<vector<string_view>>		m_ownership;
		optional<array<vector<string_view>, 3>>	m_era{};	// #UPDATE_AT_CPP23 std::views::enumerate 17.7
		optional<float>						m_recruit_priority_offset{};
		optional<float>						m_move_speed_mod{};	// Although if you really want to have this value displayed, it should be 1.0f

		void ParseLine(string_view sz) noexcept;
		string Serialize() const noexcept;
	};

	struct CEDU final : public IBaseFile
	{
		explicit CEDU(std::filesystem::path const& Path) noexcept : IBaseFile{ Path } { Deserialize(); }

		void Deserialize() noexcept override;	// #INVESTIGATE linker error here?
		string Serialize() const noexcept override;

		inline decltype(auto) operator[] (string_view k) noexcept { return m_Info[k]; }

		Dictionary<string_view, Units::unit_t_ver_2> m_Info{};
	};
}

