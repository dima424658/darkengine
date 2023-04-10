///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdispdd.h $
// $Author: KEVIN $
// $Date: 1998/02/02 12:46:29 $
// $Revision: 1.13 $
//
// @TBD (toml 05-16-96): Must do a flippable DD mode op
// implementation
//

#ifndef __WDISPDD_H
#define __WDISPDD_H

///////////////////////////////////////////////////////////////////////////////

class cDDCritMsgHandler;
class cDDModeOpsBase;

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#include <mprintf.h>
#endif

const char * WhatDDError(HRESULT error);

#ifdef DEBUG
#define DebugMsgIfErr(msg, err) { if (result != DD_OK) { mprintf("%s, result == \"%s\"\n", msg, WhatDDError(err)); } }
#define DebugDumpModeInfo(t, p) DebugMsgTrue5(t, "Mode info is (%d, %d, %d, 0x%lx, 0x%lx)", p->w, p->h, p->rowBytes, p->vbase, p->vbase2)
#define DebugMsgDesc(d) \
do { \
   DebugMsg2("Surface Descriptor " #d ": dwSize is %u, dwFlags is 0x%x,", d.dwSize, d.dwFlags); \
   DebugMsg4("  h is %u, w is %u, p is %d, ddsCaps is 0x%x", d.dwHeight, d.dwWidth, d.lPitch, d.ddsCaps); \
} while(0)
#else
#define DebugMsgIfErr(msg, err)
#define DebugDumpModeInfo(t, p)
#define DebugMsgDesc(fPrimaryDesc)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDDProvider
//
// Implements DirectDraw based version of modeless display device operations
//

class cDDProvider : public cWinDisplayProvider
{
public:
    cDDProvider(cWinDisplayDevice * pDisplayDevice);
    virtual ~cDDProvider();

    enum
    {
        kDDFullScreenCoopFlags  = (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX),
        kDDWindowedCoopFlags    = DDSCL_NORMAL
    };

    ///////////////////////////////////

    HRESULT SetCooperativeLevel(DWORD);

private:

    ///////////////////////////////////
    //
    // Hooks from cWinDisplayProvider
    //

    virtual BOOL DoProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal);
    virtual void DoGetInfo(sGrDeviceInfo *, sGrModeInfo *);
    virtual BOOL DoOpen(sGrModeCap *, int flags);
    virtual BOOL DoClose();
    virtual cWinDisplayModeOperations * DoSetMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL DoGetDirectDraw(IDirectDraw **);
    virtual BOOL DoGetBitmapSurface(sGrBitmap *, IDirectDrawSurface **);

    ///////////////////////////////////

    friend class cDDModeOpsBase;
    cDDModeOpsBase * m_pModeOps;

    ///////////////////////////////////

    LPDIRECTDRAW        m_pDD;          // DirectDraw object

    DWORD               m_DDCoopFlags;
    DWORD               m_PalFlags;
    DDSCAPS             m_SurfaceCaps;
    long                m_lTotalVideoMemory;
    long                m_NormalWindowStyle;
    long                m_NormalWindowExStyle;

    enum eFlags
    {
        kAlwaysEmulate  = 0x01
    };

    int                 m_Flags;

    static HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd, LPVOID Context);
    friend class cDDCritMsgHandler;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WDISPDD_H */
