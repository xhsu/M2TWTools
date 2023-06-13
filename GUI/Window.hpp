#pragma once

#include <imgui.h>

#include <set>
#include <filesystem>

#include "GameInterfaceFile.hpp"

inline std::set<Image_t> g_rgImages;
inline Image_t const* g_pActivatedImage = nullptr;	// NOT owning, sourced above.
inline GameInterfaceFile_t g_CurrentXml;
inline std::filesystem::path g_CurrentPath;
inline std::filesystem::path::string_type g_ImageFileJmp;

inline ImVec2 g_vecImgOrigin;	// Left-top conor of the image.
inline float g_flScope = 1;	// Scoping image

namespace Window
{
	inline bool SpritesList{ true };
	inline bool Preview{ false };
}

namespace Config
{
	inline bool EnableAdvSprEditing = false;
}
