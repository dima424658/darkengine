/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/inputbnd/RCS/inpbnd_i.h 1.27 1999/10/21 15:23:18 MAT Exp $
#ifndef __INPBND_I_H
#define __INPBND_I_H
#pragma once


#include <appagg.h>
#include <comtools.h>
#include <inpguid.h>

#include <event.h>
#include <types.h>


#define GAME_VAR 0
#define USER_VAR 1


#define IBFLAG_USER              0x00000001
#define IBFLAG_ALIAS             0x00000002
#define IBFLAG_FALLS_THRU_MOD    0x00000004




//
// Callbacks
//

// Standard UI handler
typedef BOOL (*tBindHandler) (uiEvent *pEvent);

// Passes to you the control string, command string, and user data.
// Return TRUE if you accept the bind request.
typedef BOOL (*tTrapBindFilter) (char *pControl, char *pCmd, void *pUserData);

// For doing any work after a bind trap has occured. bBound will contain
// whether or not the control was bound to the command.
typedef void (*tTrapBindPostFunc) (BOOL bBound);

// Accepts an array of pointers to all channels of a var,
// the number of channels in the array, and a char buffer
// of IB_VARSTRMAX length, for which to stuff the prevailing
// value of the conflicting channels. Should also return
// TRUE if conflict was resolved.
typedef BOOL CDECL (*tBindAggCallback) (struct _intrnl_var_channel **ppChannels, long iNum, char *pBuf);

// Accepts variable name, value in string form, and 
// whether or not the control was already down.
// Most callbacks should return NULL, as this
// is more internal.
typedef char *(*tBindProcCallback) (const char *pCmd, const char *pValStr, BOOL bAlreadyDown);



//provided aggregation callbacks
EXTERN BOOL IBMaxActiveAgg (struct _intrnl_var_channel **, long, char *);
EXTERN BOOL IBAddActiveAgg (struct _intrnl_var_channel **, long, char *);
EXTERN BOOL IBAveActiveAgg (struct _intrnl_var_channel **, long, char *);
EXTERN BOOL IBMRUActiveAgg (struct _intrnl_var_channel **, long, char *);




/*
--------------------------------------------------------------------------------------
Input Binding command variable. Used for registering cmd vars to the Variable Manager
--------------------------------------------------------------------------------------
*/
#define IB_VARSTRMAX 32
#define IB_VALSTRMAX 64
typedef struct _IB_var {
   char name[IB_VARSTRMAX];
   char val[IB_VALSTRMAX];
   ulong flags;
   tBindProcCallback cb;
   tBindAggCallback agg;

   //for internal use. user can set this to anything safely, VarSet will just override it
   char last_val[IB_VARSTRMAX];
} IB_var;


/*
----------------------------------------------------------------------
Input control channel.
An array of these gets passed into a variable's aggregation callback.
----------------------------------------------------------------------
*/
typedef struct _intrnl_var_channel { 
   char val[IB_VARSTRMAX];
   long adj_type;
   BOOL active;
   short stamp;
   unsigned long context;
} intrnl_var_channel;

///////////////////////////////////////////////////////////////////////////////
// Structure used to associate binding contexts. In a bind file, a bind command
// may be prefixed by a string. This structure is used to map the string (case-
// insensitive) to its context. "" for aszStr indicates no prefix.
typedef struct _sBindContext {
   char aszStr[IB_VARSTRMAX];
   ulong iContext;
} sBindContext;

#define kBindContextNull {"", 0}



#ifdef __cplusplus

/*
--------------------------------------------------------------
Base class for processing joystick axis input.
App's derived class must be registered with the input binder.
Expect joystick axis' to be given in -1.0 to 1.0 range,
inclusive.
--------------------------------------------------------------
*/
struct cIBJoyAxisProcess {
public:
   cIBJoyAxisProcess ();

   void SetDeadZoneX (double x) {m_deadzone_x = x;}
   void SetDeadZoneY (double y) {m_deadzone_y = y;}
   void SetDeadZoneZ (double z) {m_deadzone_z = z;}
   void SetDeadZoneR (double r) {m_deadzone_r = r;}

   void GetDeadZoneX (double *x) {*x = m_deadzone_x;}
   void GetDeadZoneY (double *y) {*y = m_deadzone_y;}
   void GetDeadZoneZ (double *z) {*z = m_deadzone_z;}
   void GetDeadZoneR (double *r) {*r = m_deadzone_r;}

   //be sure to call this base method in your inherited Process,
   //probably before you do your own processing
   virtual void ProcessXY (double *x, double *y);
   virtual void ProcessZR (double *z, double *r);

private:
   void ApplyDeadzone (double *axis, double deadzone);

   double m_deadzone_x, m_deadzone_y, m_deadzone_z, m_deadzone_r;
};

#else
typedef struct cIBJoyAxisProcess cIBJoyAxisProcess;
#endif



/*
************************************************
Interface
************************************************
*/

#undef INTERFACE
#define INTERFACE IInputBinder



#define MAX_CONTEXTS 32
DECLARE_INTERFACE_(IInputBinder, IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Can initialize binding vars and a .bnd file to load upon Init.
   // pVars and pBndFname can both be NULL.
   //
   STDMETHOD_(char *, Init) (THIS_ IB_var *pVars, char *pBndFname) PURE;
   STDMETHOD_(char *, Term) (THIS) PURE;

   //
   // Stuffs ppHandler with the input binder's ui handler.
   //
   STDMETHOD_(void, GetHandler) (THIS_ tBindHandler *ppHandler) PURE;

   //
   // Used to mask which events the binder will trap.
   // These are defined in event.h
   //
   STDMETHOD_(void, SetValidEvents) (THIS_ ulong iEventMask) PURE;
   STDMETHOD_(ulong, GetValidEvents) (THIS) PURE;

   //
   // Binds pCmd to pControl. Multiple worded commands should
   // be wrapped in quotes. Returns TRUE if all went well.
   //
   STDMETHOD_(BOOL, Bind) (THIS_ const char *pControl, const char *pCmd) PURE;

   //
   // Uninds pControl. Returns FALSE if pControl is invalid.
   //
   STDMETHOD_(BOOL, Unbind) (THIS_ const char *pControl) PURE;

   //
   // Stuffs pCmdBuf with whatever command pControl is bound to.
   //
   STDMETHOD_(void, QueryBind) (THIS_ const char *pControl, char *pCmdBuf, long iBufLen) PURE;

   //
   // Stuffs pValBuf with the value of pVarStr. Currently does not take
   // channels into account.
   //
   STDMETHOD_(void, GetVarValue) (THIS_ const char *pVarStr, char *pValBuf, long iBufLen) PURE;

   //
   // Accepts a string which contains commands
   // to interact with the binder. Most users
   // should not have to call this.
   //
   STDMETHOD_(const char *, ProcessCmd) (THIS_ const char *pCmdStr) PURE;

   //
   // Trap a control, and bind it to pCmdStr, if the filter callback allows it.
   // Both callbacks and pUserData may be NULL, however.
   //
   STDMETHOD_(void, TrapBind) (THIS_ const char *pCmdStr, tTrapBindFilter, tTrapBindPostFunc, void *pUserData) PURE;

   //
   // Merely sets the internal mapper to the correct context. 
   // Usually not needed.
   //
   STDMETHOD_(char *, Update) (THIS) PURE;

   //
   // Polls keys and sends out appropriate commands upon change of state.
   //
   STDMETHOD_(void, PollAllKeys) (THIS) PURE;

   //
   // Registers the application's joystick processor, inherited from above.
   //
   STDMETHOD_(void, RegisterJoyProcObj) (cIBJoyAxisProcess *pJoyProc) PURE;


   //
   // This is the preferred method of bind loading. You associate context bitmasks
   // and context strings with the ContextAssociate method, then LoadBndContexted
   // will parse the file only once, and map the contexted commands accordingly.
   //
   STDMETHOD_(void, LoadBndContexted) (THIS_ char *pBndFname) PURE;

   //
   // Accepts a pointer to a kBindContextNull-terminated array of sBindContext's.
   // A call to this method wipes out any previoius context associations. These
   // associations are then used when mapping contexted binds during a call to
   // LoadBndContexted.
   //
   STDMETHOD_(void, ContextAssociate) (THIS_ sBindContext *pBindContext) PURE;

   
   //
   // Loads a .bnd file into the specified iContext. If pPrefix is not NULL,
   // we bind only the matching prefixed binds.
   //
   STDMETHOD_(const char *, LoadBndFile) (THIS_ const char *pBndFname, unsigned long iContext, const char *pPrefix) PURE;

   //
   // Saves a bind file under the current context only. Should undoubtedly
   // change this in the future. If pHeader is non-NULL, the string will
   // be placed at the top of the file.
   //
   STDMETHOD_(char *, SaveBndFile) (THIS_ const char *pBndFname, char *pHeader) PURE;

   //
   // Methods for setting and unsetting bind command variables.
   //
   STDMETHOD_(BOOL, VarSet) (THIS_ IB_var *pVars) PURE;//must be NULL-terminated
   STDMETHOD_(BOOL, VarSetn) (THIS_ IB_var *pVars, long iNum) PURE;
   // if bUser is set, then the variable(s) can be unset by the player.
   STDMETHOD_(const char *, VarUnset) (THIS_ char **ppVarNames, BOOL bUser) PURE;//must be NULL-terminated
   STDMETHOD_(const char *, VarUnsetn) (THIS_ char **ppVarNames, long iNum, BOOL bUser) PURE;
   STDMETHOD_(const char *, VarUnsetAll) (THIS) PURE;


   //
   // For setting and getting out current context. This should be a bitmask.
   // Also, do we want to poll when changing to a different context?
   //
   STDMETHOD_(BOOL, SetContext) (THIS_ unsigned long iContext, BOOL bPoll) PURE;
   // Stuffs it
   STDMETHOD_(void, GetContext) (THIS_ unsigned long *pContext) PURE;

   //
   // Set globally master agg callback.
   // This is also set interally to IBMaxActiveAgg, so most don't need
   // to use this.
   //
   STDMETHOD_(void, SetMasterAggregation) (THIS_ tBindAggCallback) PURE;

   //
   // Set the default variable processing callback. When a command variable
   // doesn't have its own callback, or a bound command isn't an input command
   // this callback will be used. Useful for sending most commands along to
   // a game's command system.
   //
   STDMETHOD_(void, SetMasterProcessCallback) (THIS_ tBindProcCallback) PURE;

   //
   // Iterators for getting which controls are bound to a certain command.
   // pControlBuf really should be at least 32 chars long.
   //
   STDMETHOD_(char *, GetControlFromCmdStart) (THIS_ char *pCmd, char *pControlBuf) PURE;
   STDMETHOD_(char *, GetControlFromCmdNext) (THIS_ char *pControlBuf) PURE;

   //
   // Not currently used.
   //
   STDMETHOD_(void, SetResPath) (THIS_ char *pPath) PURE;

   //
   // Useful for taking a composed control (ie. "p+shift+alt") and separating
   // then into ppControls. Maximum controls is 4 (some control+alt+crtl+shift).
   // pNumControls will be stuffed with the number of controls separated.
   //
   STDMETHOD_(char *, DecomposeControl) (THIS_ char *pControlStr, char ppControls[4][32], long *pNumControls) PURE;

   //
   // Clears all binds in the current context.
   //
   STDMETHOD_(void, Reset)(THIS) PURE;
};



#define InputBinderCreate(ppInputBinder) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _CreateInputBinder(IID_TO_REFIID(IID_IInputBinder), ppInputBinder, pAppUnknown); \
    COMRelease(pAppUnknown); \
}

// this is internal--use the above macro
EXTERN tResult LGAPI _CreateInputBinder(REFIID, IInputBinder **ppInputBinder, IUnknown *pOuter);



#define IInputBinder_QueryInterface(p, a, b)  COMQueryInterface(p, a, b)
#define IInputBinder_AddRef(p)  COMAddRef(p)
#define IInputBinder_Release(p)  COMRelease(p)
#define IInputBinder_Init(p, a, b)  COMCall2(p, Init, a, b)
#define IInputBinder_Term(p)  COMCall0(p, Term)
#define IInputBinder_GetHandler(p, a)  COMCall1(p, GetHandler, a)
#define IInputBinder_ProcessCmd(p, a)  COMCall1(p, ProcessCmd, a)
#define IInputBinder_Update(p)  COMCall0(p, Update)
#define IInputBinder_LoadBndFile(p, a, b, c)  COMCall3(p, LoadBndFile, a, b, c)
#define IInputBinder_LoadBndFile(p, a, b, c)  COMCall3(p, LoadBndFile, a, b, c)
#define IInputBinder_SaveBndFile(p, a)  COMCall1(p, SaveBndFile, a)
#define IInputBinder_VarSet(p, a)  COMCall1(p, VarSet, a)
#define IInputBinder_VarSetn(p, a, b)  COMCall2(p, VarSetn, a, b)
#define IInputBinder_VarUnset(p, a, b)  COMCall2(p, VarUnset, a, b)
#define IInputBinder_VarUnsetn(p, a, b, c)  COMCall3(p, VarUnsetn, a, b, c)
#define IInputBinder_SetContext(p, a, b)  COMCall2(p, SetContext, a, b)
#define IInputBinder_GetContext(p, a)  COMCall1(p, GetContext, a)



#endif