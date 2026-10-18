///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/appcore/RCS/appcore.cpp $
// $Author: TOML $
// $Date: 1997/02/15 17:44:44 $
// $Revision: 1.28 $
//

#include <lg.h>
#include <codewrit.h>

#include <appagg.h>
#include <objcguid.h>
#include <objcoll.h>
#include <platform_services.h>
#include <stdlib.h>

#ifdef __ANDROID__
#define ANDROID_STARTUP_TRACE(message) PlatformDebugOutput("[startup] " message "\n")
#else
#define ANDROID_STARTUP_TRACE(message) ((void)0)
#endif

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
    ANDROID_STARTUP_TRACE("_AppMain begin");

    // Force link of main/WinMain/LibMain
    _g_referenceEntryPoint = 1;

    // Allow our code to be self-modifying
    if (g_fAppStartupFlags & kASF_MakeCodeWritable)
        MakeAllCodeWritable();

    // Increase the number of available handles in the standard buffered i/o
    #ifdef __WATCOMC__
    _grow_handles(60);
    #else
    #pragma message("Need a way to increase available file handles")
    #endif

    // Create the core aggregate entity
    _AppAggregateCreate(IID_IAggregate);
    ANDROID_STARTUP_TRACE("application aggregate created");

    // Tell the app to create the application-level COM objects
    if (AppCreateObjects(argc, argv) != NOERROR)
    {
        ANDROID_STARTUP_TRACE("AppCreateObjects failed");
        return 1;
    }
    ANDROID_STARTUP_TRACE("AppCreateObjects complete");

    // Clean-up is done through exit()
    atexit(AppOnExit);

    // Initialize the components
    if (pAppAggregate)
    {
        ANDROID_STARTUP_TRACE("aggregate initialization begin");
        pAppAggregate->Init();
        ANDROID_STARTUP_TRACE("aggregate initialization complete");
    }

    // Tell the app the components are initialized
    if (AppInit() != NOERROR)
    {
        ANDROID_STARTUP_TRACE("AppInit failed");
        return 1;
    }
    ANDROID_STARTUP_TRACE("AppInit complete");

    // Set flag indicating successful initialization, so should call AppExit()
    g_fCallAppExit = TRUE;

    // Start the application
    ANDROID_STARTUP_TRACE("AppMain begin");
    int mainRetVal = AppMain(argc, argv);
    ANDROID_STARTUP_TRACE("AppMain returned");
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
