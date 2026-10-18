///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dynfunc.cpp $
// $Author: TOML $
// $Date: 1996/10/21 14:14:52 $
// $Revision: 1.2 $
//
// (c) Copyright 1994-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#include "dynfunc.h"
#include <platform_services.h>

///////////////////////////////////////
BOOL cDynFunc::Load()
    {
    if (!fTriedToLoad)
        {
        fTriedToLoad = TRUE;
        // Assert(pszLibName && pszFuncSig);
        hInstLib = PlatformLoadLibrary(pszLibName);
        if (LoadedDLL(hInstLib))
            {
            pfnFunc = PlatformFindSymbol(hInstLib, pszFuncSig);
            //DebugMsgTrue3(pfnFunc && HIWORD(pszFuncSig), "Loaded function %s from %s (%p)", pszFuncSig, pszLibName, pfnFunc);
            //DebugMsgTrue3(pfnFunc && !HIWORD(pszFuncSig), "Loaded function %d from %s (%p)", int(LOWORD(pszFuncSig)), pszLibName, pfnFunc);
            }
        else
            ;//DebugMsg1("Failed to load %s", pszLibName);
        }
    if (pfnFunc)
        return TRUE;
    if (pfnFail)
        pfnFunc = pfnFail;
    return FALSE;
    }

///////////////////////////////////////

cDynFunc::~cDynFunc()
    {
    Unload();
    }

///////////////////////////////////////

void cDynFunc::Unload()
    {
    pfnFunc = NULL;
    fTriedToLoad = FALSE;
    if (LoadedDLL(hInstLib))
        {
        PlatformUnloadLibrary(hInstLib);
        }
    hInstLib = 0;
    }

///////////////////////////////////////

void * cDynFunc::FindFunc()
    {
    if (pfnFunc)
        return pfnFunc;

    Load();

    return pfnFunc;
    }
