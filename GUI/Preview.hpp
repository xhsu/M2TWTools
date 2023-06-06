#pragma once

#include <glad/glad.h>

inline constexpr auto PREVIEW_WIDTH = 512, PREVIEW_HEIGHT = 512;

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
