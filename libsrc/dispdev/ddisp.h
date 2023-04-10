///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/ddisp.h $
// $Author: TOML $
// $Date: 1996/11/18 12:42:18 $
// $Revision: 1.3 $
//

#ifndef __DDISP_H
#define __DDISP_H

#include <dispbase.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDosDisplayDevice
//

class cDosDisplayDevice : public cDisplayDevice
{
public:
    cDosDisplayDevice(IUnknown * pOuterUnknown, eDisplayDeviceKind kind, int flags)
      : cDisplayDevice(pOuterUnknown, kind, flags) {}
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__DDISP_H */
