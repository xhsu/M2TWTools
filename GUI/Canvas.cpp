#include "Canvas.hpp"
#include "Timer.hpp"

#include <tuple>

#include <fmt/color.h>

using std::tuple;

tuple<double, double, double> HueToRGB(double h) noexcept
{
	if (h >= 360.0)
		h = 0.0;

	h /= 60.0;

	auto i = static_cast<long>(h);
	auto t = h - i;
	auto q = 1.0 - t;

	switch (i)
	{
	default:
	case 0:
		return tuple{ 1, t, 0 };

	case 1:
		return tuple{ q, 1, 0 };

	case 2:
		return tuple{ 0, 1, t };

	case 3:
		return tuple{ 0, q, 1 };

	case 4:
		return tuple{ t, 0, 1 };

	case 5:
		return tuple{ 1, 0, q };
	}
}

void Canvas::Initialize() noexcept
{
	// create a frame buffer like a canvas such that we can draw stuff onto it.

	glGenFramebuffers(1, &g_iFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, g_iFrameBuffer);

	glGenTextures(1, &g_iFrameTexture);
	glBindTexture(GL_TEXTURE_2D, g_iFrameTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CANVAS_WIDTH, CANVAS_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_iFrameTexture, 0);

	// tie up loose ends.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Canvas::Resize(int iWidth, int iHeight) noexcept
{
	if (iWidth == CANVAS_WIDTH && iHeight == CANVAS_HEIGHT)
		return;

	CANVAS_WIDTH = iWidth;
	CANVAS_HEIGHT = iHeight;

	glDeleteTextures(1, &g_iFrameTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, g_iFrameBuffer);

	glGenTextures(1, &g_iFrameTexture);
	glBindTexture(GL_TEXTURE_2D, g_iFrameTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iWidth, iHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_iFrameTexture, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Canvas::Update() noexcept
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_TEXTURE_2D);
	glBindFramebuffer(GL_FRAMEBUFFER, g_iFrameBuffer);

	// size of FBO.
	glViewport(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(
		0,		// left
		CANVAS_WIDTH,	// right
		0,		// bottom
		CANVAS_HEIGHT,	// top
		-1,		// z_near
		1		// z_far
	);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1, 1, 1);

	// Draw image of selected .tga

	if (g_pActivatedImage)
		g_pActivatedImage->Draw();
	else
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);

		glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(CANVAS_WIDTH, 0);
		glVertex2i(CANVAS_WIDTH, CANVAS_HEIGHT);
		glVertex2i(0, CANVAS_HEIGHT);
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	// Update user cursor.

	if (m_UpdateCursor)
	{
		auto const s = (std::sin(Timer::Now()) + 1) / 2.0;
		auto const [r, g, b] = HueToRGB(s * 360);

		glDisable(GL_TEXTURE_2D);
		glColor3d(r, g, b);

		glBegin(GL_QUADS);
		glVertex2d(m_vecCursorPos.x - 1, m_vecCursorPos.y - 16);
		glVertex2d(m_vecCursorPos.x + 1, m_vecCursorPos.y - 16);
		glVertex2d(m_vecCursorPos.x + 1, m_vecCursorPos.y + 16);
		glVertex2d(m_vecCursorPos.x - 1, m_vecCursorPos.y + 16);
		glEnd();

		glBegin(GL_QUADS);
		glVertex2d(m_vecCursorPos.x - 16, m_vecCursorPos.y - 1);
		glVertex2d(m_vecCursorPos.x + 16, m_vecCursorPos.y - 1);
		glVertex2d(m_vecCursorPos.x + 16, m_vecCursorPos.y + 1);
		glVertex2d(m_vecCursorPos.x - 16, m_vecCursorPos.y + 1);
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}

	if (m_ShouldDrawGizmos)
	{
		glDisable(GL_TEXTURE_2D);

		for (auto &&Rect : m_Gizmos)
		{
			glColor3d(1, 0, 0);

			glBegin(GL_LINE_LOOP);
			glVertex2d(Rect.m_left, Rect.m_top);	// #INVESTIGATE weird offset.
			glVertex2d(Rect.m_right + 1, Rect.m_top);
			glVertex2d(Rect.m_right + 1, Rect.m_bottom + 1);
			glVertex2d(Rect.m_left, Rect.m_bottom + 1);
			glEnd();
		}

		glEnable(GL_TEXTURE_2D);
	}

	if (!m_SelectedSprites.empty())
	{
		glDisable(GL_TEXTURE_2D);

		static constexpr auto omega = 2.0;
		auto const s = (std::sin(Timer::Now() * omega) + 1) / 2.0;
		auto const [r, g, b] = HueToRGB(s * 360);

		for (auto &&pSpr : m_SelectedSprites)
		{
			glColor3d(r,g, b);

			glBegin(GL_LINE_LOOP);
			glVertex2d(pSpr->m_Rect.m_left, pSpr->m_Rect.m_top);	// #INVESTIGATE weird offset.
			glVertex2d(pSpr->m_Rect.m_right + 1, pSpr->m_Rect.m_top);
			glVertex2d(pSpr->m_Rect.m_right + 1, pSpr->m_Rect.m_bottom + 1);
			glVertex2d(pSpr->m_Rect.m_left, pSpr->m_Rect.m_bottom + 1);
			glEnd();
		}

		glEnable(GL_TEXTURE_2D);
	}

	//glDisable(GL_TEXTURE_2D);

	//glColor3f(1, 0, 0);

	//glBegin(GL_LINE_LOOP);
	//glVertex2i(1, 1);
	//glVertex2i(CANVAS_WIDTH, 1);
	//glVertex2i(CANVAS_WIDTH, CANVAS_HEIGHT);
	//glVertex2i(1, CANVAS_HEIGHT);
	//glEnd();

	//glEnable(GL_TEXTURE_2D);

	// Clear then re-add every frame.
	m_Gizmos.clear();

	// LUNA: unbind FBO such that we can render other things.
	// https://stackoverflow.com/questions/9742840/what-are-the-steps-necessary-to-render-my-scene-to-a-framebuffer-objectfbo-and
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

