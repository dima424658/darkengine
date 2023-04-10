///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/looptype.h $
// $Author: TOML $
// $Date: 1998/04/24 13:09:56 $
// $Revision: 1.12 $
//
// Structures and typedefs used by the loop dispatch system
//

#ifndef __LOOPTYPE_H
#define __LOOPTYPE_H

#include <comtools.h>
#include <prikind.h>
#include <constrid.h>

typedef struct sLoopClientDesc sLoopClientDesc;
DECLARE_HANDLE(tLoopClientData);

///////////////////////////////////////////////////////////////////////////////
//
// Loop Dispatch Messages
//

#ifndef NO_LOOP_ENUMS

///////////////////////////////////////
//
// Type to represent or'd sets of loop messages
//

typedef unsigned long tLoopMessageSet;
#define kNumLoopMessages 32


///////////////////////////////////////
//
// Loop messages
//

enum eLoopMessageEnum
{

   //
   // Constant                     Value           Purpose                     Data
   //

   kMsgNull                =       0,

   // Over-arching state messages
   kMsgStart               =       0x01,        // Loop is firing up           none
   kMsgEnd                 =       0x02,        // Loop is closing             none
   kMsgExit                =       0x04,        // Exiting game                none

   kMsgStartBlock          =       0x08,        // External block starting     none
   kMsgBlocked             =       0x10,        // External block ongoing      none
   kMsgEndBlock            =       0x20,        // External block ending       none

   // Frame based messages
   kMsgBeginFrame          =       0x40,        // Frame is beginning          struct sLoopFrameInfo *
   kMsgNormalFrame         =       0x80,        // Do frame (normal)           struct sLoopFrameInfo *
   kMsgPauseFrame          =     0x0100,        // Do frame (paused)           struct sLoopFrameInfo *
   kMsgEndFrame            =     0x0200,        // Frame is ending             struct sLoopFrameInfo *

   kMsgFrameReserved1      =     0x0400,
   kMsgFrameReserved2      =     0x0800,

   // Mode messages
   kMsgEnterMode           =     0x1000,        // Mode is starting            struct sLoopTransition * 
   kMsgSuspendMode         =     0x2000,        // Mode is being suspended     struct sLoopTransition * 
   kMsgResumeMode          =     0x4000,        // Mode is being resumed       struct sLoopTransition * 
   kMsgExitMode            =     0x8000,        // Mode is ending              struct sLoopTransition * 
   kMsgMinorModeChange     = 0x00010000,        // Minor mode is starting      int, game defined minor mode

   kMsgModeReserved2       = 0x00020000,
   kMsgModeReserved3       = 0x00040000,

   // Common user event messages
   kMsgLoad                = 0x00080000,        // File load                   ?
   kMsgSave                = 0x00100000,        // File save                   ?

   kMsgUserReserved1       = 0x00200000,
   kMsgUserReserved2       = 0x00400000,
   kMsgUserReserved3       = 0x00800000,

   // Client Defined
   kMsgApp1                = 0x01000000,
   kMsgApp2                = 0x02000000,
   kMsgApp3                = 0x04000000,
   kMsgApp4                = 0x08000000,
   kMsgApp5                = 0x10000000,
   kMsgApp6                = 0x20000000,
   kMsgApp7                = 0x40000000,
   kMsgApp8                = 0x80000000

};


///////////////////////////////////////
//
// Loop message kinds/ranges
//

enum eLoopMessageKindsEnum
{
   kMsgsNone               =          0,

   kMsgsAll                = 0xffffffff,

   kMsgsState              = (kMsgStart | kMsgEnd | kMsgExit |
                              kMsgStartBlock | kMsgBlocked | kMsgEndBlock),

   kMsgsFrame              = (kMsgBeginFrame | kMsgNormalFrame |
                              kMsgPauseFrame | kMsgEndFrame),

   kMsgsMode               = (kMsgEnterMode | kMsgSuspendMode |
                              kMsgResumeMode | kMsgExitMode),

   kMsgsSaveLoad           = (kMsgSave | kMsgLoad),
   kMsgsUser               = (kMsgSave | kMsgLoad),

   kMsgsPredefined         = (kMsgsState | kMsgsFrame | kMsgsMode | kMsgsUser),

   kMsgsApp                = 0xff000000
};


///////////////////////////////////////
//
// Message data type
//

DECLARE_HANDLE(tLoopMessageData);


///////////////////////////////////////
//
// Return values from client dispatch handler functions
//

enum eLoopMessageResultEnum
{
   kLoopDispatchContinue,
   kLoopDispatchHalt
};


///////////////////////////////////////
//
// Dispatch ordering
//

enum eLoopMessageDispatchFlagsEnum
{
   kDispatchForward    = 0x01,
   kDispatchReverse    = 0x02
};


///////////////////////////////////////
//
// Loop diagnostic flags
//

#ifndef SHIP

enum eLoopDiagnosticsEnum
{
   kLoopDiagTracking              = 0x01,
   kLoopDiagTimings               = 0x02,
   kLoopDiagShuffleUnconstrained  = 0x04,
   kLoopDiagFrameHeapchk          = 0x08,
   kLoopDiagClientHeapchk         = 0x10
};

#endif

///////////////////////////////////////

typedef int eLoopMessage;
typedef int eLoopMessageKinds;
typedef int eLoopMessageResult;
typedef int eLoopMessageDispatchFlags;

///////////////////////////////////////

#endif /* !NO_LOOP_ENUMS */


///////////////////////////////////////////////////////////////////////////////
//
// Loop ID types
//

typedef const GUID tLoopClientID;
typedef const GUID tLoopModeID;


///////////////////////////////////////////////////////////////////////////////
//
// Loop dispatch client types
//

#ifndef NO_LOOP_STRUCTS

///////////////////////////////////////
//
// STRUCT: sLoopConstraint, sAbsoluteLoopConstraint
//
// Describes constraining of a client
//

struct sLoopConstraint
{
   sRelativeConstraint constraint;
   tLoopMessageSet     messages;
};

struct sAbsoluteLoopConstraint
{
   sAbsoluteConstraint constraint;
   tLoopMessageSet     messages;
};

typedef struct sLoopConstraint         sLoopConstraint;
typedef struct sLoopConstraint         sRelativeLoopConstraint;
typedef struct sAbsoluteLoopConstraint sAbsoluteLoopConstraint;


///////////////////////////////////////
//
// Factory types
//

enum eLoopClientFactoryType
{
   kLCF_None,
   kLCF_Singleton,
   kLCF_Callback,
   kLCF_FactObj,

   kLCF_IntMax = 0xffffffff                      // force it use an int
};

typedef enum eLoopClientFactoryType eLoopClientFactoryType;

///////////////////////////////////////

F_DECLARE_INTERFACE(ILoopClient);
F_DECLARE_INTERFACE(ILoopClientFactory);
typedef ILoopClient * (LGAPI * tLoopClientFactoryFunc)(const sLoopClientDesc *, tLoopClientData data);

///////////////////////////////////////
//
// STRUCT: sLoopClientInfo
//
// Detailed information about a loop client
//

struct sLoopClientDesc
{
   // Client info
   tLoopClientID * pID;                          // Name
   char            szName[40];                   // Human-readable name
   int             priority;                     //
   tLoopMessageSet interests;                    // Desired messages

   // Creation pattern
   eLoopClientFactoryType      factoryType;
   union 
   {
      void *                 p;
      ILoopClient *          pClient;          // Singleton
      tLoopClientFactoryFunc pfnFactory;       // Callback
      ILoopClientFactory *   pFactory;         // Factory object
   };

   // Client data
   DWORD           clientData[4];

   // Client constraints
   sLoopConstraint dispatchConstraints[];        // Null-terminated contraints array
};

typedef struct sLoopClientDesc sLoopClientDesc;

#define NO_LC_DATA { 0, 0, 0, 0 }

///////////////////////////////////////////////////////////////////////////////
//
// Loop mode types
//

///////////////////////////////////////
//
// Loop initialization parameter types
//

struct sLoopModeInitParm
{
   tLoopClientID *     pID;
   tLoopClientData     data;
};

typedef struct sLoopModeInitParm *  sLoopModeInitParmList;
typedef struct sLoopModeInitParm    sLoopModeInitParm;

struct sLoopInstantiator
{
   tLoopModeID *pID;
   int minorMode;
   sLoopModeInitParmList init;
};

typedef struct sLoopInstantiator sLoopInstantiator;
   


///////////////////////////////////////
//
// STRUCT: sLoopModeDesc
//

struct sLoopModeName
{
   tLoopModeID *       pID;        // Unique identifier
   char                szName[32]; // Human readable name
};

typedef struct sLoopModeName sLoopModeName;


struct sLoopModeDesc
{
   sLoopModeName       name;
   tLoopClientID **    ppClientIDs; // Client IDs
   unsigned            nClients;   // Number of clients
};

typedef struct sLoopModeDesc sLoopModeDesc;


///////////////////////////////////////////////////////////////////////////////
//
// Loop manager types
//

enum eLoopModeChangeKindEnum
{
   kLoopModePush,
   kLoopModeSwitch,
   kLoopModeUnwindTo
};

typedef int eLoopModeChangeKind;

///////////////////////////////////////////////////////////////////////////////
//
// Loop message added data types
//


//
// Information about the current frame
//
struct sLoopFrameInfo
{
   int         fMinorMode;
   unsigned    nCount;
   unsigned long  nTicks; // current timer ticks
   unsigned long  dTicks; // time since last frame
};

typedef struct sLoopFrameInfo sLoopFrameInfo;

//
// Information about mode transition
//

struct sLoopTransition
{
   sLoopInstantiator from;
   sLoopInstantiator to;
};

typedef struct sLoopTransition sLoopTransition;

///////////////////////////////////////

#endif /* !NO_LOOP_STRUCTS */

///////////////////////////////////////////////////////////////////////////////

#endif /* !__LOOPTYPE_H */
