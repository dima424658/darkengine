///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/ddisp.cpp $
// $Author: TOML $
// $Date: 1996/11/18 12:41:44 $
// $Revision: 1.3 $
//

#include <ddisp.h>

///////////////////////////////////////////////////////////////////////////////
//
// Low-level Display Device create method
//

EXTERN
tResult LGAPI _DisplayDeviceCreate(REFIID, IDisplayDevice ** /* ppDisplayDevice */, IUnknown * pOuterUnknown, eDisplayDeviceKind kind, int flags)
{
    if (new cDosDisplayDevice(pOuterUnknown, kind, flags) != 0)
        return NOERROR;
    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
