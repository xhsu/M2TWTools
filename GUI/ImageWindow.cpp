
#include <glad/glad.h>
#include <stb_image.h>

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

using std::string_view;
using std::vector;

namespace fs = std::filesystem;

vector<Image_t> g_rgImages;
GameInterfaceFile_t g_SelectedXML;

void ImageWindowDisplay() noexcept
{
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});

	ImGui::Begin("OpenGL Texture Text");

	if (ImGui::BeginTabBar("Images", ImGuiTabBarFlags_FittingPolicyScroll))
	{
		for (auto &&Image : g_rgImages)
		{
			if (ImGui::BeginTabItem(Image.Name().c_str()))
			{
				g_pActivatedImage = &Image;
				Canvas::m_vecCursorPos = ImGui::GetMousePos() - ImGui::GetCursorScreenPos();

				ImGui::Image((void *)(intptr_t)g_iFrameTexture, ImVec2(CANVAS_WIDTH, CANVAS_HEIGHT), ImVec2{}, ImVec2{0.5f, 0.5f});
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::End();

	//ImGui::PopStyleVar();

	// Canvas will update post-render on itself.
	Canvas::m_UpdateCursor = Canvas::m_vecCursorPos.x > 0 && Canvas::m_vecCursorPos.x < CANVAS_WIDTH && Canvas::m_vecCursorPos.y > 0 && Canvas::m_vecCursorPos.y < CANVAS_HEIGHT;
	Canvas::m_Gizmos =
		g_SelectedXML.m_rgSprites
		| std::views::filter([](Sprite_t const &spr) noexcept -> bool { return g_pActivatedImage && spr.m_Path == g_pActivatedImage->m_Path; })
		| std::views::transform([](Sprite_t const &spr) noexcept { return spr.m_Rect; })
		| std::ranges::to<vector>();
	
	Canvas::Update();
}

void OperationWindowDisplay() noexcept
{
	ImGui::Begin("Actions");

	{
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

				tinyxml2::XMLDocument xml;
				g_SelectedXML.Export(&xml);

				auto const OutPath = pPath->parent_path() / (pPath->stem().native() + L"_DEBUG.xml");
				xml.SaveFile(OutPath.u8string().c_str());
			}
		}
	}

	ImGui::End();
}
