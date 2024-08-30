#include <lgassert.h>
#include <dbg.h>
#include "d6Render.h"
#include "d6States.h"
#include "d6Prim.h"
#include "d6Over.h"

extern cD6Renderer*			pcRenderer;
extern IDirect3DViewport3*	g_lpViewport;
extern DWORD				g_dwScreenWidth;
extern DWORD				g_dwScreenHeight;
extern float				g_XOffset;
extern float				g_YOffset;


cD6Renderer::cD6Renderer(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags)
{
	m_bOverlaysOn = TRUE;

	if (dwInitialBufferType == 0)
	{
		pcRenderBuffer = cImBuffer::Instance();
		pcStates = cImStates::Instance();
	}
	else if (dwInitialBufferType == 1)
	{
		pcRenderBuffer = cMSBuffer::Instance();
		pcStates = cMSStates::Instance();
	}

	DWORD RenderStatesSize = pcStates->GetRenderStatesSize();
	CreateStatesStack(10, RenderStatesSize);
	if (!pcStates->Initialize(*pdwRequestedFlags))
	{
		if (dwInitialBufferType != 1)
		{
			lgd3d_g_bInitialized = FALSE;
			Warning(("Could not Initialize D3D states\n"));
		}

		pcStates->DeInstance();
		pcRenderBuffer->DeInstance();

		DeleteStatesStack();

		// bit flag?
		pdwRequestedFlags ~= 0x100;
	
		pcRenderBuffer = cImBuffer::Instance();
		pcStates = cImStates::Instance();

		CreateStatesStack(10, pcStates->GetRenderStatesSize());

		if (pcStates->Initialize(*pdwRequestedFlags))
		{
			Warning(("Can not use multitextures.  Reverting to single texture mode.\n"));
		}
		else
		{
			lgd3d_g_bInitialized = FALSE;
			Warning(("Could not Initialize D3D states\n"));
		}
	}
}

cD6Renderer::~cD6Renderer()
{
	pcStates->DeInstance();
	pcRenderBuffer->DeInstance();

	DeleteStatesStack();

	if (pcOverlayHandler)
	{
		delete pcOverlayHandler;
		pcOverlayHandler = nullptr;
	}
}

cD6Renderer* cD6Renderer::Instance(DWORD dwInitialBufferType, DWORD* pdwRequestedFlags)
{
	if (!m_RendererInstance)
	{
		m_RendererInstance = new cD6Renderer(dwInitialBufferType, pdwRequestedFlags);
	}

	return m_RendererInstance;
}

cD6Renderer* cD6Renderer::DeInstance()
{
	if (m_RendererInstance)
	{
		delete m_RendererInstance;
		m_RendererInstance = nullptr;
	}

	return m_RendererInstance;
}

void cD6Renderer::CreateStatesStack(DWORD dwInitialSize, DWORD dwEntrySize)
{
	m_dwEntrySize = dwEntrySize;
	m_dwNoEntries = dwInitialSize;

	m_pStackData = (char*)malloc(dwInitialSize * dwEntrySize); // #TODO: should be void* ?
	m_pSetEntry = (char*)malloc(dwEntrySize);
	m_pdwRSCData = (DWORD*)malloc(sizeof(DWORD) * dwInitialSize);

	if (!m_pStackData || !m_pSetEntry || !m_pdwRSCData)
		CriticalMsg("Memory Allocation failure!");

	m_dwStackTop = 0;
	m_pCurrentEntry = m_pStackData;

	pcStates->SetPointerToCurrentStates(m_pCurrentEntry);
	pcStates->SetPointerToSetStates(m_pSetEntry);
}

void cD6Renderer::DeleteStatesStack()
{
	free(m_pSetEntry);
	free(m_pStackData);
	free(m_pdwRSCData);

	m_pSetEntry = nullptr;
	m_pStackData = nullptr;
	m_pdwRSCData = nullptr;
}

BOOL cD6Renderer::SwitchOverlaysOnOff(BOOL bOn)
{
	BOOL bWasOn = m_bOverlaysOn;
	m_bOverlaysOn = bOn;
	return bWasOn;
}

void cD6Renderer::StartFrame(int nFrame)
{
	if (FAILED(g_lpD3Ddevice->BeginScene()))
		CriticalMsg("BeginScene failed");

	pcRenderBuffer->StartFrame();

	if (g_tmgr)
		g_tmgr->start_frame(nFrame);
}

void cD6Renderer::EndFrame()
{
	pcRenderBuffer->EndFrame();

	if (m_bOverlaysOn && pcOverlayHandler)
		pcOverlayHandler->DrawOverlays();

	g_lpD3Ddevice->EndScene();

	if (g_tmgr)
		g_tmgr->end_frame();
}

void cD6Renderer::CleanDepthBuffer(int x1, int y1, int x2, int y2)
{
	int width = 0, height = 0;

	float fX1 = x1 + g_XOffset;
	float fX2 = x2 + g_XOffset;
	float fY1 = y1 + g_YOffset;
	float fY2 = y2 + g_YOffset;

	if (fX1 < 0.0)
		fX1 = 0.0;
	
	if (g_dwScreenWidth < fX2)
	{
		width = g_dwScreenWidth;
		fX2 = g_dwScreenWidth;
	}
	
	if (fY1 < 0.0)
		fY1 = 0.0;

	if ((double)g_dwScreenHeight < fY2)
	{
		height = g_dwScreenHeight;
		fY2 = (float)g_dwScreenHeight;
	}

	pcRenderBuffer->FlushPrimitives();

	BOOL bZWriteOn = pcStates->IsZWriteOn();
	BOOL bZCompareOn = pcStates->IsZCompareOn();

	if (!bZWriteOn)
		pcStates->SetZWrite(TRUE);

	if (bZCompareOn)
		pcStates->SetZCompare(FALSE);

	BOOL bAlphaOn = pcStates->IsAlphaBlendingOn();
	pcStates->EnableAlphaBlending(TRUE);

	D3DTLVERTEX* pTLVerts = pcRenderBuffer->ReservePolySlots(4);
	memset(pTLVerts, 0, sizeof(D3DTLVERTEX) * 4);

	for (int i = 0; i < 4; ++i)
	{
		pTLVerts[i].sz = 1.0;
		pTLVerts[i].rhw = 1.0;
	}
	pTLVerts->sx = fX1;
	pTLVerts->sy = fY1;
	pTLVerts[1].sx = fX2;
	pTLVerts[1].sy = fY1;
	pTLVerts[2].sx = fX2;
	pTLVerts[2].sy = fY2;
	pTLVerts[3].sx = fX1;
	pTLVerts[3].sy = fY2;
	pcRenderBuffer->DrawPoly(TRUE);

	if (!bZWriteOn)
		pcStates->SetZWrite(FALSE);

	if (bZCompareOn)
		pcStates->SetZCompare(TRUE);

	if (!bAlphaOn)
		pcStates->EnableAlphaBlending(FALSE);
}

void cD6Renderer::CleanRenderSurface(BOOL bDepthBufferToo)
{
	DWORD dwFlags = D3DCLEAR_TARGET;
	if (bDepthBufferToo)
		dwFlags |= D3DCLEAR_ZBUFFER;

	D3DRECT sRect;
	sRect.x1 = 0;
	sRect.y1 = 0;
	sRect.x2 = g_dwScreenWidth;
	sRect.y2 = g_dwScreenHeight;
	HRESULT hr = g_lpViewport->Clear(1, &sRect, dwFlags);
	if (FAILED(hr))
	{
		CriticalMsg2("%s: error %d\n%s", "CleanRenderSurface failed", hr, GetDDErrorMsg(hr));
	}
}

HRESULT cD6Renderer::InitViewport()
{
	Assert_(g_lpViewport);

	D3DVIEWPORT2 vdData;
	memset(&vdData, 0, sizeof(vdData));
	vdData.dwSize = 44;
	vdData.dwWidth = g_dwScreenWidth;
	vdData.dwHeight = g_dwScreenHeight;
	vdData.dvClipX = -1.0;
	vdData.dvClipWidth = 2.0;
	vdData.dvClipY = -1.0;
	vdData.dvClipHeight = 2.0;
	vdData.dvMaxZ = 1.0;
	return g_lpViewport->SetViewport2(&vdData);
}
