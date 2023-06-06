
#include <glad/glad.h>

#include <fmt/color.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <array>
#include <filesystem>
#include <ranges>
#include <set>
#include <string_view>
#include <tuple>
#include <vector>

#include "Canvas.hpp"
#include "GameInterfaceFile.hpp"
#include "Image.hpp"
#include "OpenFile.hpp"

import UtlString;

using std::array;
using std::set;
using std::string;
using std::string_view;
using std::tuple;
using std::vector;

namespace fs = std::filesystem;

set<Image_t> g_rgImages;
GameInterfaceFile_t g_SelectedXML;
fs::path g_CurrentPath;
fs::path::string_type g_ImageFileJmp;

ImVec2 g_vecImgOrigin;	// Left-top conor of the image.
float g_flScope = 1;	// Scoping image

namespace Window
{
	inline bool SpritesList{ false };
}

namespace Config
{
	inline bool EnableAdvSprEditing = false;
}

inline void UpdateScopeValue(float delta = 0.f) noexcept
{
	if (delta != 0.f)
		g_flScope = std::clamp(g_flScope + delta, 0.1f, 1.f);

	auto const flScale = 1.f - g_flScope;
	g_vecImgOrigin.x = std::clamp(g_vecImgOrigin.x, 0.f, flScale);
	g_vecImgOrigin.y = std::clamp(g_vecImgOrigin.y, 0.f, flScale);
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

inline void Helper_HelpMarker(const char *desc) noexcept
{
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void AddSpriteDialog(bool bShow) noexcept
{
	if (!g_pActivatedImage)
		return;

	if (bShow)
		ImGui::OpenPopup("Add a new sprite ...");

	// Always center this window when appearing
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

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

void AboutDialog(bool bShow) noexcept
{
	if (bShow)
		ImGui::OpenPopup("About...");

	// Always center this window when appearing
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("About...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Version 1.1");
		ImGui::TextUnformatted("By: Hydrogenium, @" __DATE__);
		ImGui::TextUnformatted("Website: https://github.com/xhsu/M2TWTools/releases");

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
		{
			ImGui::SetTooltip("Right-click to copy the URL.");

			if (ImGui::IsKeyPressed(ImGuiKey_MouseRight))
				ImGui::SetClipboardText("https://github.com/xhsu/M2TWTools/releases");
		}

		ImGui::NewLine();

		if (Helper_AlignedButton("Close", ImVec2{ 96, 0 }))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}

void DockingSpaceDisplay() noexcept
{
#ifdef _DEBUG
	static bool show_demo_window = false;

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);
#endif

	bool bAddSpr{}, bShowAbout{};

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Application"))
		{
			ImGui::SeparatorText("File");

			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				if (auto pPath = Win32_OpenFileDialog(L"Select M2TW UI XML", L"*.xml", L"Extensible Markup Language (*.xml)"); pPath)
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

				xml.SaveFile(g_CurrentPath.u8string().c_str());
			}
			if (ImGui::MenuItem("Save as...", "Ctrl+Alt+S") && fs::exists(g_CurrentPath))
			{
				auto const DefaultOut = g_CurrentPath.parent_path() / (g_CurrentPath.stem().native() + L"_DEBUG.xml");

				if (auto pOutPath = Win32_SaveFileDialog(L"Save as...", DefaultOut.c_str(), L"*.xml", L"Extensible Markup Language (*.xml)"); pOutPath)
				{
					tinyxml2::XMLDocument xml;
					g_SelectedXML.Export(&xml);

					xml.SaveFile(pOutPath->u8string().c_str());
				}
			}

#ifdef _DEBUG
			ImGui::SeparatorText("Debug");
			ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
#endif
			ImGui::SeparatorText("Application");
			bShowAbout = ImGui::MenuItem("About");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::SeparatorText("Sprites");

			bAddSpr = ImGui::MenuItem("Add new sprite");
			ImGui::MenuItem("Enable advanced editing", nullptr, &Config::EnableAdvSprEditing);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Utilities"))
		{
			ImGui::SeparatorText("XML");

			ImGui::MenuItem("Sort sprites when importing", nullptr, &Config::ShouldSort);

			ImGui::SeparatorText("SD");

			ImGui::MenuItem("Compile", nullptr, nullptr, false);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip(
					R"(This program does not provide such function.

The game engine will do the compilation job at its launch if you make sure both .xml and .sd files exist
and the "last modified" attribute of .xml is NEWER than its .sd counterpart.)"
				);
			}

			if (ImGui::MenuItem("Decompile", "Ctrl+D"))
			{
				if (auto pPath = Win32_OpenFileDialog(L"Select Compiled UI File", L"*.sd", L"M2TW UI (*.sd)"); pPath)
				{
					system("cls");
					fmt::print("File decompiled: {}\n", pPath->u8string());
					g_CurrentPath = g_SelectedXML.Decompile(*pPath);

					g_rgImages = g_SelectedXML.Images();

					g_pActivatedImage = nullptr;
					Canvas::m_SelectedSprites.clear();	// weak_ptr equivalent.
				}
			}

			ImGui::SeparatorText("Window");

			ImGui::MenuItem("Search Sprite", nullptr, &Window::SpritesList);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_AutoHideTabBar);

	AddSpriteDialog(bAddSpr);
	AboutDialog(bShowAbout);
}

void ImageWindowDisplay() noexcept
{
	static ImVec2 vecContentRegionAvail{};
	static bool bUpdateSelectInfo{ false };

	ImGui::Begin("GUI Pages");

	if (ImGui::BeginTabBar("Images", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs))
	{
		for (auto &&Img : g_rgImages)
		{
			auto const bitsFlags =
				(!g_ImageFileJmp.empty() && Img.m_Path.native() == g_ImageFileJmp) ?
				ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

			if (ImGui::BeginTabItem(Img.Name().c_str(), nullptr, bitsFlags))
			{
				g_pActivatedImage = &Img;
				Canvas::m_vecCursorPos = ImGui::GetMousePos() - ImGui::GetCursorScreenPos() - ImVec2{ ImGui::GetScrollX(), ImGui::GetScrollY() };

				vecContentRegionAvail = ImGui::GetContentRegionAvail();

				ImGui::Image(
					(void *)(intptr_t)g_iFrameTexture,
					ImVec2((float)CANVAS_WIDTH, (float)CANVAS_HEIGHT),
					g_vecImgOrigin, g_vecImgOrigin + ImVec2{ g_flScope, g_flScope }
				);

				if (Canvas::m_UpdateCursor = ImGui::IsItemHovered())
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);

					// Drag the image

					// Sign reversed for human sense.
					auto const flScale = 1.f - g_flScope;
					g_vecImgOrigin.x = std::clamp(g_vecImgOrigin.x - ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).x * flScale / CANVAS_WIDTH, 0.f, flScale);
					g_vecImgOrigin.y = std::clamp(g_vecImgOrigin.y - ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).y * flScale / CANVAS_WIDTH, 0.f, flScale);

					// Clear drag data.
					ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Middle] = ImGui::GetIO().MousePos;

					// Select rect
					bUpdateSelectInfo = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

					if (ImGui::GetIO().MouseWheel < 0)
						UpdateScopeValue(0.1f);
					else if (ImGui::GetIO().MouseWheel > 0)
						UpdateScopeValue(-0.1f);
				}

				ImGui::EndTabItem();
			}
		}

		// Enroll new image
		if (ImGui::TabItemButton(/*u8"\uFF0B"*/"+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoReorder))
		{
			if (g_CurrentPath.empty())
			{
				fmt::print(fg(fmt::color::light_golden_rod_yellow), "You may only be able to edit existing UI file, not creating one from scratch.");
			}
			else if (auto pPath = Win32_OpenFileDialog(L"Add a new UI page", L"*.tga", L"TARGA (*.tga)"); pPath)
			{
				g_rgImages.emplace(*pPath);
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::End();

	rect_t const RecCurrentCanvas{
		(int32_t)std::roundf(g_vecImgOrigin.x * CANVAS_WIDTH),	(int32_t)std::roundf(std::min(g_vecImgOrigin.x + g_flScope, 1.f) * CANVAS_WIDTH),	// l, r
		(int32_t)std::roundf(g_vecImgOrigin.y * CANVAS_HEIGHT),	(int32_t)std::roundf(std::min(g_vecImgOrigin.y + g_flScope, 1.f) * CANVAS_HEIGHT),	// t, b
	};

	Canvas::m_vecCursorPos *= g_flScope;
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

		if (ImGui::SliderFloat("Scope", &g_flScope, 0.1f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			UpdateScopeValue();
		Helper_HelpMarker("You may also scroll the mouse wheel when hovering on the image.");

		auto const flScale = 1.f - g_flScope;
		ImGui::SliderFloat("X", &g_vecImgOrigin.x, 0.f, flScale, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat("Y", &g_vecImgOrigin.y, 0.f, flScale, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		Helper_HelpMarker("You may also drag the image by click and hold the mouse wheel when hovering on the image.");
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

void SearchSpriteWindowDisplay() noexcept
{
	if (!Window::SpritesList)
		return;

	ImGui::Begin("Search Sprite...", &Window::SpritesList, ImGuiWindowFlags_AlwaysAutoResize);

	static int iFunctionIndex{ 0 };
	ImGui::BeginDisabled(iFunctionIndex == 0);

	static string szSearch{};
	ImGui::InputText("Search", &szSearch, ImGuiInputTextFlags_CharsUppercase);

	if (iFunctionIndex == 0 && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("All sprites are listed when switched to 'NONE' mode.");

	ImGui::EndDisabled();

	static vector<string_view> rgszSearchCell{};
	rgszSearchCell = UTIL_Split(szSearch, " \t\f\v\n\r") | std::ranges::to<vector>();

	static constexpr array rgfnFilters
	{
		tuple{ 0, "NONE", +[](Sprite_t const& spr) constexpr noexcept -> bool { return true; }, },	// everything
		tuple{ 1, "OR", +[](Sprite_t const& spr) noexcept -> bool { for (auto&& sz : rgszSearchCell) if (spr.m_Name.contains(sz)) return true; return false; },},	// OR
		tuple{ 2, "AND", +[](Sprite_t const& spr) noexcept -> bool { for (auto&& sz : rgszSearchCell) if (!spr.m_Name.contains(sz)) return false; return true; },},	// AND
	};

	static bool(*pfnFilter)(Sprite_t const&) { std::get<2>(rgfnFilters[0]) };
	for (auto&& [idx, psz, pfn] : rgfnFilters)
	{
		if (ImGui::RadioButton(psz, &iFunctionIndex, idx))
			pfnFilter = pfn;

		ImGui::SameLine();
	}

	ImGui::NewLine();
	ImGui::Separator();

	if (ImGui::BeginListBox("##search_result_list_box", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y)))
	{
		for (auto&& spr :
			g_SelectedXML.m_rgSprites
			| std::views::filter(pfnFilter)
			)
		{
			if (auto const bAlreadySelected = std::ranges::contains(Canvas::m_SelectedSprites, &spr);
				ImGui::Selectable(spr.m_Name.c_str(), bAlreadySelected))
			{
				if (!bAlreadySelected)
					Canvas::m_SelectedSprites.emplace_back(&spr);
				else
					std::erase(Canvas::m_SelectedSprites, &spr);

				g_ImageFileJmp = spr.m_Image.m_Path.native();
			}

			Helper_SpritePreviewTooltip(spr);
		}

		ImGui::EndListBox();
	}

	ImGui::End();
}
