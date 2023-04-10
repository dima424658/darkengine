///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/thrmnapi.h $
// $Author: TOML $
// $Date: 1996/11/05 09:08:10 $
// $Revision: 1.1 $
//

#ifndef __THRMNAPI_H
#define __THRMNAPI_H

#include <thrmguid.h>

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IThreadManager
//
//

#undef  INTERFACE
#define INTERFACE IThreadManager

DECLARE_INTERFACE_(IThreadManager, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    //
    //
    STDMETHOD_(void, ManageCurrentThread)(THIS_ int basePriority, int flags) PURE;

    //
    //
    //
    STDMETHOD_(void, UnmanageCurrentThread)(THIS) PURE;

    //
    //
    //
    STDMETHOD_(void, Rebalance)(THIS) PURE;

    //
    //
    //
    STDMETHOD_(void, SuspendOthers)(THIS) PURE;

    //
    //
    //
    STDMETHOD_(void, ResumeOthers)(THIS) PURE;

};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__THRMNAPI_H */
