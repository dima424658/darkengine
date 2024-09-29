#pragma once

#include <grs.h>

struct sOverRectangle
{
    float fX0, fU0, fY0, fV0;
    float fX1, fU1, fY1, fV1;
};

struct sLGD3DOverlayInfo
{
    DWORD dwFlags;
    grs_bitmap* pBitmap;
    float fX0, fY0;
    float fX1, fY1;
    float tu0, tv0;
    float tu1, tv1;
    int nAlpha, nRed, nGreen, nBlue;
};

struct sBaseOverData
{
    float fX0, fY0;
    float fX1, fY1;
    float fU0, fV0;
    float fU1, fV1;
    DWORD dwColor;
    bool bReady;
};

class cD6OverlayHandler;

class cD6OvelayType {
    virtual ~cD6OvelayType() = default;

    void TurnOnOff(BOOL bOn) { m_bTurnedOff = bOn == FALSE; }
    virtual void DrawOverlay() { }

    DWORD m_dwID;
    cD6OverlayHandler * m_pcParent;
    cD6OvelayType * m_pcNext;
    cD6OvelayType * m_pcPrevious;
    BOOL m_bVisible;
    BOOL m_bTurnedOff;
};

class cD6OverlayHandler
{
public:
    cD6OverlayHandler();
    ~cD6OverlayHandler();

    void AddOverlay(cD6OvelayType* pcNewOver, DWORD* phOver);
    void InsertOverlayAfter(cD6OvelayType* pcNewOver, cD6OvelayType* pcAfterOver, DWORD* phOver);
    void RemoveOverlay(cD6OvelayType* pcOver);
    void RemoveAllOverlays();
    void KillBranch(cD6OvelayType* pcOver);
    void SetClipViewport(sOverRectangle* psInRect);
    void GetClipViewport(sOverRectangle* psInRect);
    void DrawOverlays();
    BOOL OverlayFromHandle(DWORD hOver, cD6OvelayType** ppcAlphaOver);
    BOOL AlphaOverlayFromHandle(DWORD hOver, cD6OvelayType** ppcAlphaOver);
    DWORD MakeNewOverlayHandle(cD6OvelayType* pcOver);

private:
    cD6OvelayType m_cListHead;
    sOverRectangle m_viewRect;

    static DWORD s_dwIdGenerator;
};

extern cD6OverlayHandler* pcOverlayHandler;
