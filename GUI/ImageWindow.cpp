
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

namespace Config
{
	inline bool EnableAdvSprEditing = false;
}

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

inline bool Helper_AlignedButton(const char *label, ImVec2 const &vecSize = {}, float alignment = 0.5f) noexcept
{
	auto const &style = ImGui::GetStyle();

	auto const size = std::max(ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f, vecSize.x);
	auto const &avail = ImGui::GetContentRegionAvail().x;

	if (auto const off = (avail - size) * alignment; off > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

	return ImGui::Button(label, vecSize);
}

void AddSpriteDialog(bool bShow) noexcept
{
	if (!g_pActivatedImage)
		return;

	if (bShow)
		ImGui::OpenPopup("Add a new sprite ...");

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Add a new sprite ...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::BeginDisabled();
		if (ImGui::BeginCombo("Page", g_pActivatedImage->Name().c_str()))
		{
			for (auto &&Img : g_rgImages)
				ImGui::Selectable(Img.Name().c_str(), Img == *g_pActivatedImage);

			ImGui::EndCombo();
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip("The page must be the current page!");

		ImGui::EndDisabled();

		static string szName{};
		ImGui::InputText("Identifier", &szName, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Name must be unique and consists of:\n - '_' symbol\n - [A..Z]");

		ImGui::BeginDisabled();

		static rect_t Rect{ 0, g_pActivatedImage->m_iWidth / 2, 0, g_pActivatedImage->m_iHeight / 2 };
		ImGui::InputInt4("Rectangle", &Rect.m_left);

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip("Must use default value now.\nYou can edit them later!");

		ImGui::EndDisabled();

		static bool bShowAdv{ false };
		ImGui::Checkbox("Show advance settings", &bShowAdv);

		static bool bAlpha{ true }, bCursor{ false };
		static int32_t ofs_x{}, ofs_y{};
		if (bShowAdv)
		{
			ImGui::Checkbox("Alpha", &bAlpha); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Purpose unknown. Everything in vanilla is set to 'true' expect one.\nDefault: true"); ImGui::SameLine();
			ImGui::Checkbox("Cursor", &bCursor); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Could be an indicator that this is used as mouse cursor replacement.\nDefault: false");

			ImGui::InputInt("X Offset", &ofs_x); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Some coord used if this sprite were to used as mouse cursor???\nDefault: 0");
			ImGui::InputInt("Y Offset", &ofs_y); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Some coord used if this sprite were to used as mouse cursor???\nDefault: 0");
		}

		ImGui::NewLine();
		if (Helper_AlignedButton("Accept", ImVec2{ 96, 0 }))
		{
			auto &Sprite = g_SelectedXML.m_rgSprites.emplace_back(Sprite_t{
				.m_Name{ std::move(szName) },
				.m_Image{ *g_pActivatedImage },
				.m_Rect{ Rect },
				.m_OfsX{ ofs_x },
				.m_OfsY{ ofs_y },
				.m_IsAlpha{ bAlpha },
				.m_IsCursor{ bCursor },
				}
			);

			Canvas::m_SelectedSprites.emplace_back(&Sprite);

			ImGui::CloseCurrentPopup();

			szName = "";
			Rect = { 0, g_pActivatedImage->m_iWidth / 2, 0, g_pActivatedImage->m_iHeight / 2 };
			bShowAdv = false;
			bAlpha = true;
			bCursor = false;
			ofs_x = 0;
			ofs_y = 0;
		}
		if (Helper_AlignedButton("Decline", ImVec2{ 96, 0 }))
		{
			ImGui::CloseCurrentPopup();

			szName = "";
			Rect = { 0, g_pActivatedImage->m_iWidth / 2, 0, g_pActivatedImage->m_iHeight / 2 };
			bShowAdv = false;
			bAlpha = true;
			bCursor = false;
			ofs_x = 0;
			ofs_y = 0;
		}

		ImGui::EndPopup();
	}
}

void DockingSpaceDisplay() noexcept
{
	static bool show_demo_window = false, bAddSpr{};

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

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::SeparatorText("Sprites");

			bAddSpr = ImGui::MenuItem("Add new sprite");
			ImGui::MenuItem("Enable advanced editing", nullptr, &Config::EnableAdvSprEditing);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			ImGui::SeparatorText("XML");

			ImGui::MenuItem("Sort sprites when importing", nullptr, &Config::ShouldSort);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_AutoHideTabBar);

	AddSpriteDialog(bAddSpr);
	bAddSpr = false;
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

				ImGui::InputText("Identifier", &pSpr->m_Name, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);

				auto const& flSpacing = ImGui::GetStyle().ItemInnerSpacing.x;
				auto const fl = ImGui::GetFrameHeight();	// copy from ArrowButtonEx
				ImGui::PushButtonRepeat(true);
				ImGui::InvisibleButton("placeholder1", ImVec2{ fl, fl }); ImGui::SameLine(0.0f, flSpacing);
				if (ImGui::ArrowButton("##up", ImGuiDir_Up) && pSpr->m_Rect.m_top > 0)
				{
					pSpr->m_Rect.m_top = std::clamp(pSpr->m_Rect.m_top - 1, 0, pSpr->m_Image.m_iHeight);
					pSpr->m_Rect.m_bottom = std::clamp(pSpr->m_Rect.m_bottom - 1, 0, pSpr->m_Image.m_iHeight);
				}
				ImGui::SameLine(0.0f, flSpacing);
				ImGui::InvisibleButton("placeholder3", ImVec2{ fl, fl });
				if (ImGui::ArrowButton("##left", ImGuiDir_Left) && pSpr->m_Rect.m_left > 0)
				{
					pSpr->m_Rect.m_left = std::clamp(pSpr->m_Rect.m_left - 1, 0, pSpr->m_Image.m_iWidth);
					pSpr->m_Rect.m_right = std::clamp(pSpr->m_Rect.m_right - 1, 0, pSpr->m_Image.m_iWidth);

				}
				ImGui::SameLine(0.0f, flSpacing);
				ImGui::InvisibleButton("placeholder4", ImVec2{ fl, fl }); ImGui::SameLine(0.0f, flSpacing);
				if (ImGui::ArrowButton("##right", ImGuiDir_Right) && pSpr->m_Rect.m_right < pSpr->m_Image.m_iWidth)
				{
					pSpr->m_Rect.m_left = std::clamp(pSpr->m_Rect.m_left + 1, 0, pSpr->m_Image.m_iWidth);
					pSpr->m_Rect.m_right = std::clamp(pSpr->m_Rect.m_right + 1, 0, pSpr->m_Image.m_iWidth);
				}
				ImGui::SameLine(0.0f, flSpacing);
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("Translation");
				ImGui::InvisibleButton("placeholder5", ImVec2{ fl, fl }); ImGui::SameLine(0.0f, flSpacing);
				if (ImGui::ArrowButton("##down", ImGuiDir_Down) && pSpr->m_Rect.m_bottom < pSpr->m_Image.m_iHeight)
				{
					pSpr->m_Rect.m_top = std::clamp(pSpr->m_Rect.m_top + 1, 0, pSpr->m_Image.m_iHeight);
					pSpr->m_Rect.m_bottom = std::clamp(pSpr->m_Rect.m_bottom + 1, 0, pSpr->m_Image.m_iHeight);
				}
				ImGui::PopButtonRepeat();

				if (ImGui::InputInt("Left", &pSpr->m_Rect.m_left))
					pSpr->m_Rect.m_left = std::clamp(pSpr->m_Rect.m_left, 0, pSpr->m_Rect.m_right);
				if (ImGui::InputInt("Right", &pSpr->m_Rect.m_right))
					pSpr->m_Rect.m_right = std::clamp(pSpr->m_Rect.m_right, pSpr->m_Rect.m_left, pSpr->m_Image.m_iWidth);
				if (ImGui::InputInt("Top", &pSpr->m_Rect.m_top))
					pSpr->m_Rect.m_top = std::clamp(pSpr->m_Rect.m_top, 0, pSpr->m_Rect.m_bottom);
				if (ImGui::InputInt("Bottom", &pSpr->m_Rect.m_bottom))
					pSpr->m_Rect.m_bottom = std::clamp(pSpr->m_Rect.m_bottom, pSpr->m_Rect.m_top, pSpr->m_Image.m_iHeight);

				//ImGui::DragIntRange2("##X", &pSpr->m_Rect.m_left, &pSpr->m_Rect.m_right, 1, 0, pSpr->m_Image.m_iWidth, "Left: %d", "Right: %d", ImGuiSliderFlags_AlwaysClamp);
				//ImGui::DragIntRange2("##Y", &pSpr->m_Rect.m_top, &pSpr->m_Rect.m_bottom, 1, 0, pSpr->m_Image.m_iHeight, "Top: %d", "Bottom: %d", ImGuiSliderFlags_AlwaysClamp);

				auto const sz = fmt::format("Width: {}, Height: {}", pSpr->m_Rect.m_right - pSpr->m_Rect.m_left, pSpr->m_Rect.m_bottom - pSpr->m_Rect.m_top);
				ImGui::TextUnformatted(sz.c_str());

				if (Config::EnableAdvSprEditing)
				{
					ImGui::Checkbox("Alpha", &pSpr->m_IsAlpha); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Purpose unknown. Everything in vanilla is set to 'true' expect one.\nDefault: true"); ImGui::SameLine();
					ImGui::Checkbox("Cursor", &pSpr->m_IsCursor); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Could be an indicator that this is used as mouse cursor replacement.\nDefault: false");

					ImGui::InputInt("X Offset", &pSpr->m_OfsX); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Some coord used if this sprite were to used as mouse cursor???\nDefault: 0");
					ImGui::InputInt("Y Offset", &pSpr->m_OfsY); if (ImGui::IsItemHovered()) ImGui::SetTooltip("Some coord used if this sprite were to used as mouse cursor???\nDefault: 0");
				}

				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0, 0.6f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0, 0.8f, 0.8f));
				if (ImGui::Button("DELETE", ImVec2{ ImGui::GetContentRegionAvail().x, 0 }))
				{
					bAllowedToExist = false;
					//std::erase(g_SelectedXML.m_rgSprites, *pSpr);	// Weird.
					std::erase_if(g_SelectedXML.m_rgSprites, [&](Sprite_t const &spr) noexcept -> bool { return spr.m_Name == pSpr->m_Name; });
				}
				ImGui::PopStyleColor(3);
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
