#include "Canvas.hpp"

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
		glDisable(GL_TEXTURE_2D);
		glColor3f(1, 1, 1);

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

	for (auto &&Rect : m_Gizmos)
	{
		glDisable(GL_TEXTURE_2D);

		glColor3f(1, 0, 0);

		glBegin(GL_LINE_LOOP);
		glVertex2d(Rect.m_left + 1, Rect.m_top + 1);	// #INVESTIGATE weird offset.
		glVertex2d(Rect.m_right, Rect.m_top + 1);
		glVertex2d(Rect.m_right, Rect.m_bottom);
		glVertex2d(Rect.m_left + 1, Rect.m_bottom);
		glEnd();

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

