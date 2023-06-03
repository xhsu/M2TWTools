#pragma once

#include <tinyxml2.h>

#include <filesystem>
#include <set>
#include <vector>

#include "Image.hpp"

struct rect_t final
{
	int32_t m_left{}, m_right{}, m_top{}, m_bottom{};
};

struct Sprite_t final
{
	std::string m_Name{};
	std::filesystem::path m_Path{};
	rect_t m_Rect{};
	int32_t m_OfsX{}, m_OfsY{};
	bool m_IsAlpha{ true }, m_IsCursor{ false };
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

	std::set<std::filesystem::path> ImageFiles() const noexcept;

	void Import(std::filesystem::path const &Path) noexcept;
	void Export(tinyxml2::XMLDocument *xml) const noexcept;

	inline void Clear() noexcept { m_Version = 6; m_EnumerationName.clear(); m_rgSprites.clear(); }
};
