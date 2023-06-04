
#include <glad/glad.h>

#include <fmt/core.h>

#include <imgui.h>

#include <filesystem>
#include <ranges>
#include <string_view>
#include <vector>

#include "Canvas.hpp"
#include "GameInterfaceFile.hpp"
#include "Image.hpp"
#include "OpenFile.hpp"

using std::string;
using std::string_view;
using std::vector;

namespace fs = std::filesystem;

vector<Image_t> g_rgImages;
GameInterfaceFile_t g_SelectedXML;
ImVec2 g_vecScope(0, 0);

void DockingSpaceDisplay() noexcept
{
	static bool show_demo_window = false;

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Application"))
		{
			ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
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
		for (auto &&Image : g_rgImages)
		{
			if (ImGui::BeginTabItem(Image.Name().c_str()))
			{
				g_pActivatedImage = &Image;
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
		Canvas::m_SelectedSprites =
			g_SelectedXML.m_rgSprites
			| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return spr.m_Rect.IsPointIn(Canvas::m_vecCursorPos); })
			| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return g_pActivatedImage && spr.m_Path == g_pActivatedImage->m_Path; })
			| std::views::transform([](Sprite_t &spr) noexcept -> Sprite_t *{ return &spr; })
			| std::ranges::to<vector>();
	}

	// Canvas will update post-render on itself.

	Canvas::m_Gizmos =
		g_SelectedXML.m_rgSprites
		| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return g_pActivatedImage && spr.m_Path == g_pActivatedImage->m_Path; })
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
		if (Canvas::m_SelectedSprites.empty())
		{
			ImGui::TextUnformatted("Hovering items: ");

			bool bAny{ false };

			for (auto &&sz :
				g_SelectedXML.m_rgSprites
				| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return spr.m_Rect.IsPointIn(Canvas::m_vecCursorPos); })
				| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return g_pActivatedImage && spr.m_Path == g_pActivatedImage->m_Path; })
				| std::views::transform([](Sprite_t const &spr) noexcept -> string const &{ return spr.m_Name; })
				)
			{
				ImGui::BulletText(sz.c_str());
				bAny = true;
			}

			if (!bAny)
				ImGui::BulletText("[None]");
		}
		else
		{
			ImGui::TextUnformatted("Selected items: ");

			for (auto &&pSpr : Canvas::m_SelectedSprites)
			{
				ImGui::BulletText(pSpr->m_Name.c_str());
			}
		}

		if (ImGui::Button("Open"))
		{
			if (auto pPath = Win32_OpenFileDialog(L"M2TW UI (*.xml)\0*.xml\0"); pPath)
			{
				fmt::print("File selected: {}\n", pPath->u8string());
				g_SelectedXML.Import(*pPath);

				g_rgImages =
					g_SelectedXML.ImageFiles()
					| std::views::transform([](fs::path const &Path) noexcept { return Image_t{ Path }; })
					| std::ranges::to<vector>();

				g_pActivatedImage = nullptr;
				Canvas::m_SelectedSprites.clear();	// weak_ptr equivalent.

				tinyxml2::XMLDocument xml;
				g_SelectedXML.Export(&xml);

				auto const OutPath = pPath->parent_path() / (pPath->stem().native() + L"_DEBUG.xml");
				xml.SaveFile(OutPath.u8string().c_str());
			}
		}

		ImGui::Checkbox("Show gizmos", &Canvas::m_ShouldDrawGizmos);

		ImGui::SliderFloat("X", &g_vecScope.x, 0.f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat("Y", &g_vecScope.y, 0.f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	}

	ImGui::End();
}

void SpriteWindowDisplay() noexcept
{

}
