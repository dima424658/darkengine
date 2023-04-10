///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/appcore/RCS/dappcore.cpp $
// $Author: TOML $
// $Date: 1997/08/05 13:13:38 $
// $Revision: 1.2 $
//

#ifndef _WIN32
#include <lg.h>
#include <comtools.h>
#include <comconn.h>
#include <appapi.h>
#include <appagg.h>

EXTERN tResult LGAPI _GenericApplicationCreate(REFIID,
                                               IApplication ** ppApplication,
                                               IUnknown * pOuterUnknown,
                                               int argc, const char *argv[],
                                               const char * pszName,
                                               const char * pszDefaultPath)
{
    USE_EXTENDED_START_UP();
    return E_FAIL;
}

#endif
