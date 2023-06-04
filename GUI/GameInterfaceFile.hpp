#pragma once

#include <imgui.h>
#include <tinyxml2.h>

#include <filesystem>
#include <set>
#include <vector>

#include "Image.hpp"

struct rect_t final
{
	int32_t m_left{}, m_right{}, m_top{}, m_bottom{};

	inline constexpr bool IsPointIn(ImVec2 pos) const noexcept
	{
		return
			m_left <= pos.x && pos.x <= m_right
			&&
			m_top <= pos.y && pos.y <= m_bottom	// reversed Y axis.
			;
	}

	// Thanks, C++20.
	inline constexpr auto operator<=> (rect_t const &) const noexcept = default;
};

struct Sprite_t final
{
	std::string m_Name{};
	Image_t m_Image{};
	rect_t m_Rect{};
	int32_t m_OfsX{}, m_OfsY{};
	bool m_IsAlpha{ true }, m_IsCursor{ false };

	// Only name make difference. Just like in game.
	inline constexpr auto operator<=> (Sprite_t const &rhs) const noexcept { return m_Name <=> rhs.m_Name; }
};

struct GameInterfaceFile_t final
{
	GameInterfaceFile_t() noexcept = default;
	explicit GameInterfaceFile_t(std::filesystem::path const& Path) noexcept { Import(Path); }

	GameInterfaceFile_t(GameInterfaceFile_t &&) noexcept = default;
	GameInterfaceFile_t(GameInterfaceFile_t const&) noexcept = default;

	GameInterfaceFile_t &operator=(GameInterfaceFile_t &&) noexcept = default;
	GameInterfaceFile_t &operator=(GameInterfaceFile_t const&) noexcept = default;

	~GameInterfaceFile_t() noexcept = default;

	// Members

	int32_t m_Version{ 6 };
	std::string m_EnumerationName{};
	//std::vector<std::filesystem::path> m_rgImagePaths{};	// Concluded from all texture cells.
	std::vector<Sprite_t> m_rgSprites{};

	// Methods

	std::set<Image_t> Images() const noexcept;

	void Import(std::filesystem::path const &Path) noexcept;
	void Export(tinyxml2::XMLDocument *xml) const noexcept;

	inline void Clear() noexcept { m_Version = 6; m_EnumerationName.clear(); m_rgSprites.clear(); }
	[[nodiscard]] inline bool Empty() noexcept { return m_EnumerationName.empty() || m_rgSprites.empty(); }
};
