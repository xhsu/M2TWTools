
#include <fmt/color.h>

#include <array>
#include <ranges>
#include <string_view>

#include "OpenFile.hpp"
#include "Preview.hpp"
#include "Window.hpp"

using namespace std::literals;

namespace fs = std::filesystem;

using std::array;
using std::string_view;


static GameInterfaceFile_t s_Battle, s_Shared, s_Strategy;
static array<array<Sprite_t const*, 3>, 3> s_rgrgScroll = {};

enum ERoll
{
	TOP = 0,
	MID,
	BOTTOM,
};

enum EColumn
{
	LEFT = 0,
	CENTER,
	RIGHT,
};

inline void DrawSprite(const Sprite_t& spr, int32_t x, int32_t y) noexcept
{
	auto const [uv0, uv1] = spr.UV();
	auto const [iWidth, iHeight] = spr.WH();

	glBindTexture(GL_TEXTURE_2D, spr.m_Image.m_iTexture);

	glBegin(GL_QUADS);
	glTexCoord2f(uv0.x, uv0.y);
	glVertex2i(x, y);
	glTexCoord2f(uv1.x, uv0.y);
	glVertex2i(x + iWidth, y);
	glTexCoord2f(uv1.x, uv1.y);
	glVertex2i(x + iWidth, y + iHeight);
	glTexCoord2f(uv0.x, uv1.y);
	glVertex2i(x, y + iHeight);
	glEnd();
}

void Preview::Initilize() noexcept
{
	// create a frame buffer like a canvas such that we can draw stuff onto it.

	glGenFramebuffers(1, &m_iFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_iFrameBuffer);

	glGenTextures(1, &m_iTexture);
	glBindTexture(GL_TEXTURE_2D, m_iTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PREVIEW_WIDTH, PREVIEW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_iTexture, 0);

	// tie up loose ends.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Preview::Gather() noexcept
{
	static constexpr array rgrgszNames
	{
		array{ "SCROLL_TOP_LEFT"sv, "SCROLL_TOP_CENTER"sv, "SCROLL_TOP_RIGHT"sv, },
		array{ "SCROLL_MID_LEFT"sv, "SCROLL_MID"sv, "SCROLL_MID_RIGHT"sv, },
		array{ "SCROLL_BOTTOM_LEFT"sv, "SCROLL_BOTTOM_CENTER"sv, "SCROLL_BOTTOM_RIGHT"sv, },
	};

	for (auto&& [rgsz, rgpspr] : std::views::zip(rgrgszNames, s_rgrgScroll))
	{
		for (auto&& [sz, pSpr] : std::views::zip(rgsz, rgpspr))
		{
			if (auto const it = std::ranges::find_if(s_Shared.m_rgSprites, [&](Sprite_t const& spr) noexcept { return spr.m_Name == sz; }); it != s_Shared.m_rgSprites.end())
				pSpr = &(*it);
			else
			{
				pSpr = nullptr;
				fmt::print(fg(fmt::color::light_golden_rod_yellow), "Sprite no found: '{}'\n", sz);
			}
		}
	}
}

void Preview::Draw() noexcept
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_TEXTURE_2D);
	glBindFramebuffer(GL_FRAMEBUFFER, m_iFrameBuffer);

	// size of FBO.
	glViewport(0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(
		0,		// left
		PREVIEW_WIDTH,	// right
		0,		// bottom
		PREVIEW_HEIGHT,	// top
		-1,		// z_near
		1		// z_far
	);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1, 1, 1);

	if (Ready())
	{
		int32_t x{}, y{};

		for (auto&& rgpSpr : s_rgrgScroll)
		{
			DrawSprite(*rgpSpr[LEFT], x, y);

			x += rgpSpr[LEFT]->m_Rect.Width();

			for (int i = 0; i < 2; ++i)
			{
				DrawSprite(*rgpSpr[CENTER], x, y);
				x += rgpSpr[CENTER]->m_Rect.Width();
			}

			DrawSprite(*rgpSpr[RIGHT], x, y);
			x = 0;
			y += rgpSpr[RIGHT]->m_Rect.Height();
		}
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);

		glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(PREVIEW_WIDTH, 0);
		glVertex2i(PREVIEW_WIDTH, PREVIEW_HEIGHT);
		glVertex2i(0, PREVIEW_HEIGHT);
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	// LUNA: unbind FBO such that we can render other things.
	// https://stackoverflow.com/questions/9742840/what-are-the-steps-necessary-to-render-my-scene-to-a-framebuffer-objectfbo-and
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Preview::GUI() noexcept
{
	Draw();

	if (!Window::Preview)
		return;

	ImGui::Begin("Preview", &Window::Preview, ImGuiWindowFlags_MenuBar);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open 'battle.sd.xml'"))
			{
				if (auto const p = Win32_OpenFileDialog(L"Open 'battle.sd.xml'", L"battle.sd.xml", L"Battle UI def (battle.sd.xml)"); p)
				{
					system("cls");
					s_Battle.Import(p->u8string());
					Gather();
				}
			}
			if (ImGui::MenuItem("Open 'shared.sd.xml'"))
			{
				if (auto const p = Win32_OpenFileDialog(L"Open 'shared.sd.xml'", L"shared.sd.xml", L"Shared UI def (shared.sd.xml)"); p)
				{
					system("cls");
					s_Shared.Import(p->u8string());
					Gather();
				}
			}
			if (ImGui::MenuItem("Open 'strategy.sd.xml'"))
			{
				if (auto const p = Win32_OpenFileDialog(L"Open 'strategy.sd.xml'", L"strategy.sd.xml", L"Strategy UI def (strategy.sd.xml)"); p)
				{
					system("cls");
					s_Strategy.Import(p->u8string());
					Gather();
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Select '<mod>/data/ui/' folder"))
			{
				if (auto const p = Win32_SelectFolderDialog(L"Select <mod>/data/ui/ folder"); p)
				{
					system("cls");

					auto const FBattle = *p / "battle.sd.xml";
					auto const FShared = *p / "shared.sd.xml";
					auto const FStrategy = *p / "strategy.sd.xml";

					if (fs::exists(FBattle) && fs::exists(FShared) && fs::exists(FStrategy))
					{
						s_Battle.Import(FBattle.u8string());
						s_Shared.Import(FShared.u8string());
						s_Strategy.Import(FStrategy.u8string());
						Gather();
					}
					else
						fmt::print(fg(fmt::color::light_golden_rod_yellow), "Not all following files can be found under this path: 'battle.sd.xml', 'shared.sd.xml' and 'strategy.sd.xml'\n");
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	ImGui::SeparatorText("Status");

	ImGui::BulletText(fmt::format("battle.sd.xml - {}", s_Battle.Empty() ? "Not Loaded" : "Ready").c_str());
	ImGui::BulletText(fmt::format("shared.sd.xml - {}", s_Shared.Empty() ? "Not Loaded" : "Ready").c_str());
	ImGui::BulletText(fmt::format("strategy.sd.xml - {}", s_Strategy.Empty() ? "Not Loaded" : "Ready").c_str());

	ImGui::SeparatorText("Illustration");

	ImGui::Image((ImTextureID)m_iTexture, ImVec2{ PREVIEW_WIDTH, PREVIEW_HEIGHT });

	ImGui::End();
}

bool Preview::Ready() noexcept
{
	return !s_Battle.Empty() && !s_Strategy.Empty() && !s_Strategy.Empty();
}
