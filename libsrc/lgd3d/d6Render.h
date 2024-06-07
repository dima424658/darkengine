#pragma once

#include <types.h>

class cD6Renderer {
private:
	static cD6Renderer* m_RendererInstance;

protected:
	cD6Renderer(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags);
	~cD6Renderer();

public:
	cD6Renderer* Instance(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags);
	cD6Renderer* DeInstance();

private:
	void CreateStatesStack(DWORD dwInitialSize, DWORD dwEntrySize);
	void DeleteStatesStack();

public:
	int SwitchOverlaysOnOff(bool bOn);
	void StartFrame(int nFrame);
	void EndFrame();
	void CleanDepthBuffer(int x1, int y1, int x2, int y2);
	void CleanRenderSurface(bool bDepthBufferToo);
	long InitViewport();

	DWORD m_dwEntrySize;
	DWORD m_dwNoEntries;

	DWORD m_dwStackTop;
	char* m_pStackData;

	DWORD* m_pdwRSCData;
	char* m_pCurrentEntry;
	char* m_pSetEntry;
	bool m_bOverlaysOn;
};