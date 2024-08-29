#pragma once

#include <types.h>

class cD6Renderer {
private:
	static cD6Renderer* m_RendererInstance;

protected:
	cD6Renderer(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags);
	~cD6Renderer();

public:
	static cD6Renderer* Instance(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags);
	static cD6Renderer* DeInstance();

private:
	void CreateStatesStack(DWORD dwInitialSize, DWORD dwEntrySize);
	void DeleteStatesStack();

public:
	BOOL SwitchOverlaysOnOff(BOOL bOn);
	void StartFrame(int nFrame);
	void EndFrame();
	void CleanDepthBuffer(int x1, int y1, int x2, int y2);
	void CleanRenderSurface(BOOL bDepthBufferToo);
	HRESULT InitViewport();

	DWORD m_dwEntrySize;
	DWORD m_dwNoEntries;

	DWORD m_dwStackTop;
	char* m_pStackData;

	DWORD* m_pdwRSCData;
	char* m_pCurrentEntry;
	char* m_pSetEntry;
	BOOL m_bOverlaysOn;
};