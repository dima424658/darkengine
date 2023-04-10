///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/appagg.cpp $
// $Author: TOML $
// $Date: 1996/10/22 13:17:26 $
// $Revision: 1.3 $
//

#ifdef _WIN32
#include <windows.h>
#endif
#include <lg.h>
#include <comtools.h>
#include <appagg.h>

///////////////////////////////////////////////////////////////////////////////

static IUnknown * pAppAggregate;

///////////////////////////////////////

IUnknown * LGAPI _AppGetAggregated(REFIID iid)
{
    if (pAppAggregate)
    {
        IUnknown * pReturn;
        if (pAppAggregate->QueryInterface(iid, (void**)(&pReturn)) == 0)
            return pReturn;
    }
    return NULL;
}

///////////////////////////////////////

void LGAPI _AppSetAggregate(IUnknown * pUnknown)
{
    AssertMsg(!pAppAggregate, "Application aggregate already set!");
    pAppAggregate = pUnknown;
}

///////////////////////////////////////////////////////////////////////////////

