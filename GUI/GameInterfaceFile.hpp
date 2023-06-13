#pragma once

#include <imgui.h>
#include <tinyxml2.h>

#include <array>
#include <filesystem>
#include <set>
#include <vector>

#include "Image.hpp"

namespace Config
{
	inline bool ShouldSort = true;
}

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

	inline constexpr auto Width() const noexcept { return m_right - m_left; }
	inline constexpr auto Height() const noexcept { return m_bottom - m_top; }

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

	inline constexpr std::pair<ImVec2, ImVec2> UV() const noexcept
	{
		auto const flWidth = static_cast<float>(m_Image.m_iWidth);
		auto const flHeight = static_cast<float>(m_Image.m_iHeight);

		return std::pair
		{
			ImVec2{ (float)m_Rect.m_left / flWidth, (float)m_Rect.m_top / flHeight },
			ImVec2{ (float)m_Rect.m_right / flWidth, (float)m_Rect.m_bottom / flHeight },
		};
	}
	inline constexpr std::pair<int32_t, int32_t> WH() const noexcept { return std::pair{ m_Rect.Width(), m_Rect.Height() }; }

	// Only name make difference. Just like in game.
	inline constexpr auto operator<=> (Sprite_t const &rhs) const noexcept { return m_Name <=> rhs.m_Name; }
	inline constexpr auto operator== (Sprite_t const &rhs) const noexcept { return m_Name == m_Name; }
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

	std::set<Image_t> Images(bool bUseCultureOverride) const noexcept;
	std::set<std::wstring_view> ReferencedFileStems() const noexcept;

	std::string Decompile(std::filesystem::path const& Path) noexcept;
	void Import(std::filesystem::path const &Path) noexcept;
	void Export(tinyxml2::XMLDocument *xml) const noexcept;

	inline void Clear() noexcept { m_Version = 6; m_EnumerationName.clear(); m_rgSprites.clear(); }
	[[nodiscard]] inline bool Empty() noexcept { return m_EnumerationName.empty() || m_rgSprites.empty(); }
};

namespace UIFolder
{
	namespace fs = std::filesystem;

	using namespace std::literals;

	using std::array;

	inline constexpr array m_rgszCultures
	{
		"eastern_european"sv,
		"greek"sv,
		"mesoamerican"sv,
		"middle_eastern"sv,
		"northern_european"sv,
		"southern_european"sv,
	};

	enum ECulture
	{
		eastern_european,
		greek,
		mesoamerican,
		middle_eastern,
		northern_european,
		southern_european,
	};

	inline constexpr auto CULTURE_COUNT = m_rgszCultures.size();

	inline fs::path m_szUIFolder;
	inline array<fs::path, CULTURE_COUNT> m_rgszOverrideFolders;
	inline array<bool, CULTURE_COUNT> m_rgbOverrideExists;
	inline int32_t m_iSelected = southern_european;

	void Update(fs::path const& UIFolder) noexcept;
}
