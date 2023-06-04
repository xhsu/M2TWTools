#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <map>

struct Image_t final
{
	inline explicit Image_t(std::filesystem::path const& Path) noexcept
		: m_Path{ Path }
	{
		if (m_Precache.contains(Path))
			*this = Image_t{ m_Precache.at(Path), Path };
		else if (std::filesystem::exists(m_Path))
			LoadFromFile(m_Path);
	}

	Image_t(Image_t const &) noexcept = default;
	Image_t(Image_t &&) noexcept = default;

	Image_t &operator=(Image_t const &) noexcept = default;
	Image_t &operator=(Image_t &&) noexcept = default;

	~Image_t() noexcept = default;

	bool LoadFromFile(std::filesystem::path const &Path) noexcept;

	inline bool Valid() const noexcept { return m_iTexture != 0 && m_iWidth > 0 && m_iHeight > 0; }
	inline operator bool() const noexcept { return Valid(); }

	void Draw() const noexcept;
	void DrawCentered(int iCanvasWidth, int iCanvasHeight) const noexcept;

	inline auto Name() const noexcept { return m_Path.stem().u8string(); }

	GLuint m_iTexture{};
	int m_iWidth{}, m_iHeight{};
	std::filesystem::path m_Path{};

	// Cache
private:
	struct Cache_t final
	{
		GLuint m_iTexture{};
		int m_iWidth{}, m_iHeight{};
	};

	inline Image_t(Cache_t const &cache, std::filesystem::path const &Path) noexcept
		: m_iTexture{ cache.m_iTexture }, m_iHeight{ cache.m_iHeight }, m_iWidth{ cache.m_iWidth }, m_Path{ Path } {}

	inline static std::map<std::filesystem::path, Cache_t> m_Precache{};
};
