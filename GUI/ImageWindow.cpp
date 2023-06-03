
#include <glad/glad.h>
#include <stb_image.h>

#include <fmt/core.h>

#include <imgui.h>

#include <filesystem>
#include <string_view>
#include <vector>

#include "Image.hpp"
#include "OpenFile.hpp"
#include "GameInterfaceFile.hpp"

using std::string_view;
using std::vector;

namespace fs = std::filesystem;

GLuint g_iFrameTexture = 0, g_iFrameBuffer = 0;
inline constexpr int FRAME_WIDTH = 512, FRAME_HEIGHT = 512;

vector<Image_t> g_rgImages;
Image_t *g_pActivatedImage = nullptr;

void ImageWindowInit() noexcept
{
	g_rgImages = vector<Image_t>
	{
		Image_t{R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\vanilla_data\ui\southern_european\interface\stratpage_01.tga)"},
		Image_t{R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\vanilla_data\ui\southern_european\interface\stratpage_02.tga)"},
		Image_t{R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\vanilla_data\ui\southern_european\interface\stratpage_03.tga)"},
		Image_t{R"(D:\SteamLibrary\steamapps\common\Medieval II Total War\Unpacked\vanilla_data\ui\southern_european\interface\stratpage_04.tga)"},
	};

	// create a frame buffer like a canvas such that we can draw stuff onto it.

	glGenFramebuffers(1, &g_iFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, g_iFrameBuffer);

	glGenTextures(1, &g_iFrameTexture);
	glBindTexture(GL_TEXTURE_2D, g_iFrameTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_iFrameTexture, 0);

	// tie up loose ends.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void fn1(bool bUpdateCursor, float x, float y) noexcept
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_TEXTURE_2D);
	glBindFramebuffer(GL_FRAMEBUFFER, g_iFrameBuffer);

	// size of FBO.
	glViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(
		0,		// left
		FRAME_WIDTH,	// right
		0,		// bottom
		FRAME_HEIGHT,	// top
		-1,		// z_near
		1		// z_far
	);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1, 1, 1);

	if (g_pActivatedImage)
		g_pActivatedImage->Draw();
	else
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);

		glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(FRAME_WIDTH, 0);
		glVertex2i(FRAME_WIDTH, FRAME_HEIGHT);
		glVertex2i(0, FRAME_HEIGHT);
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	if (bUpdateCursor)
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);
	
		glBegin(GL_QUADS);
		glVertex2f(x - 1, y - 16);
		glVertex2f(x + 1, y - 16);
		glVertex2f(x + 1, y + 16);
		glVertex2f(x - 1, y + 16);
		glEnd();
	
		glBegin(GL_QUADS);
		glVertex2f(x - 16, y - 1);
		glVertex2f(x + 16, y - 1);
		glVertex2f(x + 16, y + 1);
		glVertex2f(x - 16, y + 1);
		glEnd();
	
		glEnable(GL_TEXTURE_2D);
	}

	// LUNA: unbind FBO such that we can render other things.
	// https://stackoverflow.com/questions/9742840/what-are-the-steps-necessary-to-render-my-scene-to-a-framebuffer-objectfbo-and
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ImageWindowDisplay() noexcept
{
	static ImVec2 vecCalibratedPos{};

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});

	ImGui::Begin("OpenGL Texture Text");

	if (ImGui::BeginTabBar("Images", ImGuiTabBarFlags_FittingPolicyScroll))
	{
		for (auto &&Image : g_rgImages)
		{
			if (ImGui::BeginTabItem(Image.Name().c_str()))
			{
				g_pActivatedImage = &Image;
				vecCalibratedPos = ImGui::GetMousePos() - ImGui::GetCursorScreenPos();

				ImGui::Image((void *)(intptr_t)g_iFrameTexture, ImVec2(FRAME_WIDTH, FRAME_HEIGHT));
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::End();

	ImGui::PopStyleVar();

	fn1(vecCalibratedPos.x > 0 && vecCalibratedPos.x < FRAME_WIDTH && vecCalibratedPos.y > 0 && vecCalibratedPos.y < FRAME_HEIGHT,
		vecCalibratedPos.x, vecCalibratedPos.y
	);
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
				GameInterfaceFile_t file{ *pPath };

				tinyxml2::XMLDocument xml;
				file.Export(&xml);

				auto const OutPath = pPath->parent_path() / (pPath->stem().native() + L"_DEBUG.xml");
				xml.SaveFile(OutPath.u8string().c_str());
			}
		}
	}

	ImGui::End();
}