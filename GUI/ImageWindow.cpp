
#include <glad/glad.h>

#include <fmt/core.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <filesystem>
#include <ranges>
#include <set>
#include <string_view>
#include <vector>

#include "Canvas.hpp"
#include "GameInterfaceFile.hpp"
#include "Image.hpp"
#include "OpenFile.hpp"

using std::string;
using std::string_view;
using std::vector;
using std::set;

namespace fs = std::filesystem;

set<Image_t> g_rgImages;
GameInterfaceFile_t g_SelectedXML;
ImVec2 g_vecScope(0, 0);
fs::path g_CurrentPath;

inline void Helper_SpritePreviewTooltip(Sprite_t const &spr) noexcept
{
	if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
	{
		auto const flWidth = static_cast<float>(spr.m_Image.m_iWidth);
		auto const flHeight = static_cast<float>(spr.m_Image.m_iHeight);

		ImGui::Image(
			(void *)spr.m_Image.m_iTexture,
			ImVec2{ (float)spr.m_Rect.Width(), (float)spr.m_Rect.Height() },
			ImVec2{ (float)spr.m_Rect.m_left / flWidth, (float)spr.m_Rect.m_top / flHeight },
			ImVec2{ (float)spr.m_Rect.m_right / flWidth, (float)spr.m_Rect.m_bottom / flHeight }
		);

		ImGui::EndTooltip();
	}
}

void DockingSpaceDisplay() noexcept
{
	static bool show_demo_window = false;

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Application"))
		{
			ImGui::SeparatorText("File");

			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				if (auto pPath = Win32_OpenFileDialog(L"M2TW UI (*.xml)\0*.xml\0"); pPath)
				{
					g_CurrentPath = *pPath;

					fmt::print("File selected: {}\n", pPath->u8string());
					g_SelectedXML.Import(*pPath);

					g_rgImages = g_SelectedXML.Images();

					g_pActivatedImage = nullptr;
					Canvas::m_SelectedSprites.clear();	// weak_ptr equivalent.
				}
			}

			if (ImGui::MenuItem("Save", "Ctrl+S") && fs::exists(g_CurrentPath))
			{
				tinyxml2::XMLDocument xml;
				g_SelectedXML.Export(&xml);

				auto const OutPath = g_CurrentPath.parent_path() / (g_CurrentPath.stem().native() + L"_DEBUG.xml");
				xml.SaveFile(OutPath.u8string().c_str());
			}

#ifdef _DEBUG
			ImGui::SeparatorText("Debug");
			ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
#endif
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_AutoHideTabBar);

}

void ImageWindowDisplay() noexcept
{
	static ImVec2 vecContentRegionAvail{};
	static bool bUpdateSelectInfo{ false };

	ImGui::Begin("GUI Pages");

	if (ImGui::BeginTabBar("Images", ImGuiTabBarFlags_FittingPolicyScroll))
	{
		for (auto &&Img : g_rgImages)
		{
			if (ImGui::BeginTabItem(Img.Name().c_str()))
			{
				g_pActivatedImage = &Img;
				Canvas::m_vecCursorPos = ImGui::GetMousePos() - ImGui::GetCursorScreenPos() - ImVec2{ ImGui::GetScrollX(), ImGui::GetScrollY() };

				vecContentRegionAvail = ImGui::GetContentRegionAvail();

				ImGui::Image(
					(void *)(intptr_t)g_iFrameTexture,
					ImVec2((float)CANVAS_WIDTH, (float)CANVAS_HEIGHT),
					g_vecScope, g_vecScope + ImVec2{ 0.5f, 0.5f }
				);

				if (Canvas::m_UpdateCursor = ImGui::IsItemHovered())
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);

					// Drag the image

					// Sign reversed for human sense.
					g_vecScope.x = std::clamp(g_vecScope.x - ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).x * 0.5f / CANVAS_WIDTH, 0.f, 0.5f);
					g_vecScope.y = std::clamp(g_vecScope.y - ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).y * 0.5f / CANVAS_WIDTH, 0.f, 0.5f);

					// Clear drag data.
					ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Middle] = ImGui::GetIO().MousePos;

					// Select rect
					bUpdateSelectInfo = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				}

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::End();

	rect_t const RecCurrentCanvas{
		(int32_t)std::roundf(g_vecScope.x * CANVAS_WIDTH),	(int32_t)std::roundf(std::min(g_vecScope.x + 0.5f, 1.f) * CANVAS_WIDTH),		// l, r
		(int32_t)std::roundf(g_vecScope.y * CANVAS_HEIGHT),	(int32_t)std::roundf(std::min(g_vecScope.y + 0.5f, 1.f) * CANVAS_HEIGHT),	// t, b
	};

	Canvas::m_vecCursorPos *= 0.5f;
	Canvas::m_vecCursorPos += ImVec2{(float)RecCurrentCanvas.m_left, (float)RecCurrentCanvas.m_top};

	// Update selection info.

	if (bUpdateSelectInfo)
	{
		if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
			Canvas::m_SelectedSprites.clear();

		Canvas::m_SelectedSprites.append_range(
			g_SelectedXML.m_rgSprites
			| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return spr.m_Rect.IsPointIn(Canvas::m_vecCursorPos); })
			| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return g_pActivatedImage && spr.m_Image.m_Path == g_pActivatedImage->m_Path; })
			| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return !std::ranges::contains(Canvas::m_SelectedSprites, &spr); })
			| std::views::transform([](Sprite_t &spr) noexcept -> Sprite_t *{ return &spr; })
		);
	}

	// Canvas will update post-render on itself.

	Canvas::m_Gizmos =
		g_SelectedXML.m_rgSprites
		| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return g_pActivatedImage && spr.m_Image.m_Path == g_pActivatedImage->m_Path; })
		| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return !std::ranges::contains(Canvas::m_SelectedSprites, &spr); })
		| std::views::transform([](Sprite_t const &spr) noexcept { return spr.m_Rect; })
		| std::ranges::to<vector>();

	Canvas::Resize(static_cast<int>(vecContentRegionAvail.x), static_cast<int>(vecContentRegionAvail.y));
	Canvas::Update();
}

void OperationWindowDisplay() noexcept
{
	ImGui::Begin("Actions");

	{
		ImGui::SeparatorText("Hovering Items");

		bool bAny{ false };

		for (auto &&sz :
			g_SelectedXML.m_rgSprites
			| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return spr.m_Rect.IsPointIn(Canvas::m_vecCursorPos); })
			| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return g_pActivatedImage && spr.m_Image.m_Path == g_pActivatedImage->m_Path; })
			| std::views::transform([](Sprite_t const &spr) noexcept -> string const &{ return spr.m_Name; })
			)
		{
			ImGui::BulletText(sz.c_str());
			bAny = true;
		}

		if (!bAny)
			ImGui::BulletText("[NONE]");

		ImGui::SeparatorText("Selected Items");

		for (auto &&pSpr : Canvas::m_SelectedSprites)
		{
			ImGui::BulletText(pSpr->m_Name.c_str());
			Helper_SpritePreviewTooltip(*pSpr);
		}

		if (Canvas::m_SelectedSprites.empty())
			ImGui::BulletText("[NONE]");

		ImGui::SeparatorText("Image Manipulation");

		ImGui::Checkbox("Show all gizmos", &Canvas::m_ShouldDrawGizmos);

		ImGui::SliderFloat("X", &g_vecScope.x, 0.f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat("Y", &g_vecScope.y, 0.f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	}

	ImGui::End();
}

void SpriteWindowDisplay() noexcept
{
	ImGui::Begin("Sprites");

	if (!Canvas::m_SelectedSprites.empty())
	{
		for (auto it = Canvas::m_SelectedSprites.begin(); it != Canvas::m_SelectedSprites.end(); /* Does nothing*/)
		{
			auto &pSpr = *it;
			bool bAllowedToExist = pSpr->m_Image == *g_pActivatedImage;

			if (ImGui::CollapsingHeader(pSpr->m_Name.c_str(), &bAllowedToExist, ImGuiTreeNodeFlags_DefaultOpen))
			{
				Helper_SpritePreviewTooltip(*pSpr);

				//ImGui::SliderInt("Left", &pSpr->m_Rect.m_left, 0, pSpr->m_Rect.m_right, "%d", ImGuiSliderFlags_AlwaysClamp);
				//ImGui::SliderInt("Right", &pSpr->m_Rect.m_right, pSpr->m_Rect.m_left, pSpr->m_Image.m_iWidth, "%d", ImGuiSliderFlags_AlwaysClamp);
				//ImGui::SliderInt("Top", &pSpr->m_Rect.m_top, 0, pSpr->m_Rect.m_bottom, "%d", ImGuiSliderFlags_AlwaysClamp);
				//ImGui::SliderInt("Bottom", &pSpr->m_Rect.m_bottom, pSpr->m_Rect.m_top, pSpr->m_Image.m_iHeight, "%d", ImGuiSliderFlags_AlwaysClamp);

				ImGui::InputText("Identifier", &pSpr->m_Name);

				ImGui::DragIntRange2("X", &pSpr->m_Rect.m_left, &pSpr->m_Rect.m_right, 1, 0, pSpr->m_Image.m_iWidth, "Left: %d", "Right: %d", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragIntRange2("Y", &pSpr->m_Rect.m_top, &pSpr->m_Rect.m_bottom, 1, 0, pSpr->m_Image.m_iHeight, "Top: %d", "Bottom: %d", ImGuiSliderFlags_AlwaysClamp);

				auto const sz = fmt::format("Width: {}, Height: {}", pSpr->m_Rect.m_right - pSpr->m_Rect.m_left, pSpr->m_Rect.m_bottom - pSpr->m_Rect.m_top);
				ImGui::TextUnformatted(sz.c_str());
			}
			else
				Helper_SpritePreviewTooltip(*pSpr);	// Only way to get the hover-tooltip when collapsed.

			if (!bAllowedToExist)
				it = Canvas::m_SelectedSprites.erase(it);
			else
				++it;
		}
	}
	else
	{
		ImGui::TextUnformatted("Click on image to select sprites!");
	}

	ImGui::End();
}
