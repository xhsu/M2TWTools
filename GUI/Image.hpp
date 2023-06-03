#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <string_view>

struct Image_t final
{
	inline explicit Image_t(std::string_view sz) noexcept
		: m_Path{ sz }
	{
		if (std::filesystem::exists(m_Path))
			LoadFromFile(m_Path.u8string());
	}

	bool LoadFromFile(std::string_view sz) noexcept;

	inline bool Valid() const noexcept { return m_iTexture != 0 && m_iWidth > 0 && m_iHeight > 0; }
	inline operator bool() const noexcept { return Valid(); }

	void Draw() const noexcept;

	inline auto Name() const noexcept { return m_Path.stem().u8string(); }

	GLuint m_iTexture{};
	int m_iWidth{}, m_iHeight{};
	std::filesystem::path m_Path{};
};
