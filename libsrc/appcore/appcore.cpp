///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/appcore/RCS/appcore.cpp $
// $Author: TOML $
// $Date: 1997/02/15 17:44:44 $
// $Revision: 1.28 $
//

#ifdef _WIN32
#include <windows.h>
#endif

#include <lg.h>
#include <codewrit.h>

#include <appagg.h>
#include <objcguid.h>
#include <objcoll.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

static IAggregate * pAppAggregate;

///////////////////////////////////////////////////////////////////////////////

tResult _AppAggregateCreate(REFIID)
{
    pAppAggregate = CreateGenericAggregate("Application Aggregate", kISetKeyLGGUIDs);
    AssertMsg(pAppAggregate, "Failed to create application aggregate!");
    _AppSetAggregate(pAppAggregate);
    return (pAppAggregate) ? NOERROR : E_FAIL;
}

///////////////////////////////////////

static BOOL g_fCallAppExit;
static BOOL g_fReturnedFromMain;

void AppOnExit()
{
    // Tell the app the components are about to go away
    if (g_fCallAppExit)
        AppExit();
    if (pAppAggregate)
    {
        pAppAggregate->End();

        //
        // This module always retains 1 reference on the aggregate. However, this
        // function must handle the (not too unusual) case of closing without cleaning
        // up all reference counts.
        //
        ulong ulRefsRemaining = pAppAggregate->ReleaseAll();
        //AssertMsg1(!g_fReturnedFromMain || ulRefsRemaining == 1, "Reference of application object is incorrect on exit (was %d, expected 1)", ulRefsRemaining);
        pAppAggregate = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////

extern "C" {

int _g_referenceExtendedStartup = 0;
extern int _g_referenceEntryPoint;

};

///////////////////////////////////////////////////////////////////////////////
//
// This is the primary client entry-point
//

extern "C"
int LGAPI _AppMain(int argc, const char *argv[])
{
    // Force link of main/WinMain/LibMain
    _g_referenceEntryPoint = 1;

#ifdef _WIN32
    // Allow our code to be self-modifying
    if (GetPrivateProfileInt("AppCore", "MakeAllCodeWritable", !!(g_fAppStartupFlags & kASF_MakeCodeWritable), "lg.ini"))
        MakeAllCodeWritable();
#endif

    // Increase the number of available handles in the standard buffered i/o
    #ifdef __WATCOMC__
    _grow_handles(60);
    #else
    #pragma message("Need a way to increase available file handles")
    #endif

    // Create the core aggregate entity
    _AppAggregateCreate(IID_IAggregate);

    // Tell the app to create the application-level COM objects
    if (AppCreateObjects(argc, argv) != NOERROR)
        return 1;

    // Clean-up is done through exit()
    atexit(AppOnExit);

    // Initialize the components
    if (pAppAggregate)
        pAppAggregate->Init();

    // Tell the app the components are initialized
    if (AppInit() != NOERROR)
        return 1;

    // Set flag indicating successful initialization, so should call AppExit()
    g_fCallAppExit = TRUE;

    // Start the application
    int mainRetVal = AppMain(argc, argv);
    g_fReturnedFromMain = TRUE;
    return mainRetVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// Symbol to Force inclusion of this extended startup code
//

int __ExtendedStartup(void)
{
    return 0;
}
