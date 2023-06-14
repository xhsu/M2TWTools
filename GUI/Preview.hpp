#pragma once

#include <glad/glad.h>

// Width: whichever is larger.
inline constexpr auto PREVIEW_WIDTH = /*(60 + 75 * 5 + 60) + */(280 * 2 + 461), PREVIEW_HEIGHT = (60 + 85 * 5 + 60) + (180);

namespace Preview
{
	inline GLuint m_iFrameBuffer = 0;
	inline GLuint m_iTexture = 0;

	void Initilize() noexcept;
	void Gather() noexcept;
	void Draw() noexcept;
	void GUI() noexcept;

	[[nodiscard]] bool Ready() noexcept;
}
