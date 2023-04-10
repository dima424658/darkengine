///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/appagg.h $
// $Author: TOML $
// $Date: 1997/02/15 17:48:23 $
// $Revision: 1.16 $
//
// Basic implementation independent description of application object aggregation
//

#ifndef __APPAGG_H
#define __APPAGG_H

#include <comtools.h>

///////////////////////////////////////////////////////////////////////////////
//
// Convenience creation macros for common targets
//

//
// Core game libraries
//
#define CoreGameLibrariesCreate(name, argc, argv, dispDevKind, gameShellOpt) \
    do \
    { \
    GenericApplicationCreate(argc, argv, name, NULL); \
    DisplayDeviceCreate(dispDevKind); \
    GameShellCreate(gameShellOpt); \
    } while (0)


#define CoreGameLibrariesCreate2(name, argc, argv, dispDevKind, dispDevCreateFlags, gameShellOpt) \
    do \
    { \
    GenericApplicationCreate(argc, argv, name, NULL); \
    DisplayDeviceCreate2(dispDevKind, dispDevCreateFlags); \
    GameShellCreate(gameShellOpt); \
    } while (0)


#define CoreGameLibrariesCreate3(name, argc, argv, dispDevKind, dispDevCreateFlags, gameShellOpt, msecBlockSignal) \
    do \
    { \
    GenericApplicationCreate(argc, argv, name, NULL); \
    DisplayDeviceCreate2(dispDevKind, dispDevCreateFlags); \
    GameShellCreate(gameShellOpt, msecBlockSignal); \
    } while (0)


// @TBD (toml 06-19-96): Remove references to this
#define AppAggregateCreate()

///////////////////////////////////////////////////////////////////////////////
//
// Access an application object (e.g. "IFoo * pFoo = AppGetObj(IFoo)")
//

#define AppGetObj(INTERFACE)     ((INTERFACE *)(_AppGetAggregated(IID_TO_REFIID( IID_ ## INTERFACE ))))

EXTERN IUnknown * LGAPI _AppGetAggregated(REFIID);
EXTERN void       LGAPI _AppSetAggregate(IUnknown *);


///////////////////////////////////////////////////////////////////////////////
//
// Functions the application should implement
//

//
// Call to notify start-up code is complete, COM initialization is
// pending.
// The application uses this to call the COM "Create" functions
//
EXTERN tResult  LGAPI AppCreateObjects(int argc, const char *argv[]);

//
// Call to notify COM initialization is complete
//
EXTERN tResult  LGAPI AppInit();

//
// Called to run the program
//
EXTERN int      LGAPI AppMain(int argc, const char *argv[]);

//
// Call to notify COM close-down is pending
//
EXTERN tResult  LGAPI AppExit();

///////////////////////////////////////////////////////////////////////////////
//
// App may optionally define a flags data item to control pre-AppMaom()
// initialization
//

enum eAppStartupFlags
{
    kASF_MakeCodeWritable = 0x01,

    kASF_Default = (kASF_MakeCodeWritable)
};

EXTERN unsigned g_fAppStartupFlags;

///////////////////////////////////////////////////////////////////////////////
//
// Simple applications may need to force a reference of the extended start-up
// code.  By placing this in the file that contains AppMain, this macro will assure
// proper linking.
//

EXTERN int _g_referenceExtendedStartup;

#define USE_EXTENDED_START_UP() _g_referenceExtendedStartup = 1


///////////////////////////////////////////////////////////////////////////////

#endif /* __APPAGG_H */
