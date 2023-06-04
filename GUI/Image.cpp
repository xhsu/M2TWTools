#include <stb_image.h>

#include "Image.hpp"

bool Image_t::LoadFromFile(std::filesystem::path const& Path) noexcept
{
	// Load from file
	auto const image_data = stbi_load(Path.u8string().c_str(), &m_iWidth, &m_iHeight, nullptr, 4);

	if (!image_data)
		return false;

	// Create a OpenGL texture identifier
	glGenTextures(1, &m_iTexture);
	glBindTexture(GL_TEXTURE_2D, m_iTexture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_iWidth, m_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	if (!m_Precache.contains(Path))
		m_Precache[Path] = Cache_t{ m_iTexture, m_iWidth, m_iHeight };

	return true;
}

void Image_t::Draw() const noexcept
{
	glBindTexture(GL_TEXTURE_2D, m_iTexture);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2i(0, 0);
	glTexCoord2f(1, 0);
	glVertex2i(m_iWidth, 0);
	glTexCoord2f(1, 1);
	glVertex2i(m_iWidth, m_iHeight);
	glTexCoord2f(0, 1);
	glVertex2i(0, m_iHeight);
	glEnd();
}

void Image_t::DrawCentered(int iCanvasWidth, int iCanvasHeight) const noexcept
{
	auto const iOriginX = (iCanvasWidth - 1) / 2 - (m_iWidth - 1) / 2;
	auto const iOriginY = (iCanvasHeight - 1) / 2 - (m_iHeight - 1) / 2;

	glBindTexture(GL_TEXTURE_2D, m_iTexture);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2i(iOriginX, iOriginY);
	glTexCoord2f(1, 0);
	glVertex2i(iOriginX + m_iWidth, iOriginY);
	glTexCoord2f(1, 1);
	glVertex2i(iOriginX + m_iWidth, iOriginY + m_iHeight);
	glTexCoord2f(0, 1);
	glVertex2i(iOriginX, iOriginY + m_iHeight);
	glEnd();
}
