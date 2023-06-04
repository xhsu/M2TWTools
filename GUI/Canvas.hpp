#pragma once

#include <glad/glad.h>

#include <imgui.h>

#include <vector>

#include "GameInterfaceFile.hpp"
#include "Image.hpp"

inline GLuint g_iFrameTexture = 0, g_iFrameBuffer = 0;
inline int CANVAS_WIDTH = 512, CANVAS_HEIGHT = 512;
inline Image_t const *g_pActivatedImage = nullptr;

namespace Canvas
{
	void Initialize() noexcept;

	// Update flags
	inline bool m_UpdateCursor{};
	inline ImVec2 m_vecCursorPos{};
	inline bool m_ShouldDrawGizmos{ true };
	inline std::vector<rect_t> m_Gizmos{};
	inline std::vector<Sprite_t *> m_SelectedSprites{};	// NOT OWNING

	void Resize(int iWidth, int iHeight) noexcept;
	void Update() noexcept;
}