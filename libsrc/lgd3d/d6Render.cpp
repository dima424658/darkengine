#include "d6Render.h"

cD6Renderer::cD6Renderer(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags)
{
}

cD6Renderer::~cD6Renderer()
{
}

cD6Renderer* cD6Renderer::Instance(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags)
{
	return nullptr;
}

cD6Renderer* cD6Renderer::DeInstance()
{
	return nullptr;
}

void cD6Renderer::CreateStatesStack(DWORD dwInitialSize, DWORD dwEntrySize)
{
}

void cD6Renderer::DeleteStatesStack()
{
}

int cD6Renderer::SwitchOverlaysOnOff(bool bOn)
{
	return 0;
}

void cD6Renderer::StartFrame(int nFrame)
{
}

void cD6Renderer::EndFrame()
{
}

void cD6Renderer::CleanDepthBuffer(int x1, int y1, int x2, int y2)
{
}

void cD6Renderer::CleanRenderSurface(bool bDepthBufferToo)
{
}

long cD6Renderer::InitViewport()
{
	return 0;
}
