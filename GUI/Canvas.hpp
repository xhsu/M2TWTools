#pragma once

#include <glad/glad.h>

#include <imgui.h>

#include <vector>

#include "GameInterfaceFile.hpp"
#include "Image.hpp"

inline GLuint g_iFrameTexture = 0, g_iFrameBuffer = 0;
inline constexpr int CANVAS_WIDTH = 512, CANVAS_HEIGHT = 512;
inline Image_t *g_pActivatedImage = nullptr;

namespace Canvas
{
	void Initialize() noexcept;

	// Update flags
	inline bool m_UpdateCursor{};
	inline ImVec2 m_vecCursorPos{};
	inline std::vector<rect_t> m_Gizmos{};

	void Update() noexcept;
}