
#include "Preview.hpp"
#include "Window.hpp"

static Sprite_t* s_Window;

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
	//g_CurrentXml.m_rgSprites
}

void Preview::Draw() noexcept
{
}
