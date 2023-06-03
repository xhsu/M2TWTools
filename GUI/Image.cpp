#include <stb_image.h>

#include "Image.hpp"

using std::string_view;

bool Image_t::LoadFromFile(string_view sz) noexcept
{
	// Load from file
	auto const image_data = stbi_load(sz.data(), &m_iWidth, &m_iHeight, nullptr, 4);

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
