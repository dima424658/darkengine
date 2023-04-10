///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdspmode.h $
// $Author: KEVIN $
// $Date: 1997/10/21 16:18:10 $
// $Revision: 1.8 $
//

#ifndef __WDSPMODE_H
#define __WDSPMODE_H

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinDisplayModeOperations
//
// Implements mode-level operations for a given implementation of the display
// device.
//
// Implements (mode specific)
//    OnBeginFrame()
//    OnEndFrame()
//    DoStatHTrace()
//    DoStatVTrace()
//    DoSetPalette()
//    DoFlip()
//    DoLock()
//    DoUnlock()
//

class cWinDisplayModeOperations
{
public:
    cWinDisplayModeOperations(cWinDisplayDevice * pDisplayDevice)
      : m_pDisplayDevice(pDisplayDevice),
        m_ScaleFactor(0)
    {
    }

    // Notification that the window is losing or gaining activation
    virtual BOOL OnTaskSwitch(BOOL);

    //
    // Implementations of low level operations. Default implementations
    // are no-ops.
    //
    virtual eGrDispMode DoGetMode(sGrModeInfoEx *);
    virtual BOOL DoStatHTrace();
    virtual BOOL DoStatVTrace();
    virtual BOOL DoSetPalette(LOGPALETTE & palette, unsigned uStart, unsigned n);
    virtual BOOL DoPageFlip();
    virtual BOOL DoLock(sGrModeCap *) = 0;
    virtual BOOL DoUnlock() = 0;
    virtual void DoFlush();
    virtual void DoFlushRect(int x0, int y0, int x1, int y1);

    const sGrModeInfoEx & GetModeInfoEx() const
    {
        return m_ModeInfoEx;
    }

    int GetSignificantDepth()
    {
        if (m_ModeInfoEx.depth == 16 && !(m_ModeInfoEx.redMask & 0x8000))
            return 15;
        return m_ModeInfoEx.depth;
    }

    int GetScaleFactor() const
    {
        return m_ScaleFactor;
    }

    void SetScaleFactor(int newScale)
    {
        m_ScaleFactor = newScale;
    }

    int Scale(int val)
    {
        return ( (m_ScaleFactor < 0) ? (val >> -m_ScaleFactor) : (val << m_ScaleFactor) );
    }

protected:
    virtual ~cWinDisplayModeOperations();

    cWinDisplayDevice * const   m_pDisplayDevice;
    sGrModeInfoEx               m_ModeInfoEx;
    int                         m_ScaleFactor;

};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WDSPMODE_H */
