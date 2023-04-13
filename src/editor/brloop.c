/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brloop.c,v 1.23 2000/02/19 12:27:44 toml Exp $

#include <lg.h>
#include <comtools.h>
#include <loopapi.h>
#include <mprintf.h>
#include <command.h>

#include <loopmsg.h>
#include <dbasemsg.h>
#include <dispbase.h>
#include <objnotif.h>

#include <brloop.h>
#include <objloop.h>
#include <uiloop.h>

#include <editbr.h>
#include <brlist.h>
#include <vbrush.h>
#include <property.h>
#include <brprop.h>
#include <editobj.h>
#include <editsave.h>
#include <areapnp.h>
#include <wrloop.h>
#include <backup.h>

#include <editbr_.h>  // for the horror of curBrush save/load hell
#include <brinfo.h>
#include <editobj.h>
#include <brquery.h>

#include <appagg.h>
#include <iobjsys.h>
#include <objquery.H>
#include <osysbase.h>
#include <brestore.h>
#include <edmedmo.h>

#include <editmode.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/////////////////////////////////////////////////////////////
// BRUSH LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//



#define MY_FACTORY BrushListLoopFactory

//
//
#define MY_GUID   LOOPID_BrushList

//
//
// My context data
typedef void Context;

//
// 
// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.

   BOOL edit_mode; 
   // State fields go here
} StateRecord;


////////////////////////////////////////
// DATABASE MESSAGE HANDLER
//

static brush_list *master_bl;

static void db_message(DispatchData* msg)
{
   msgDatabaseData info;

   info.raw = msg->data;

   if (BackupSaving() || BackupLoading())
      return ;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         EdMedMoClearMediumMotion();
         brushClearAll();
         AreaPnP_Names_Reset();
         vBrushReset();
         break;
   
      case kDatabaseLoad:
         vBrush_DontUpdateFocus=TRUE;
         if (msg->subtype & kDBMap)
            editor_LoadCow(info.load);
         break;

      case kDatabasePostLoad:
         if (msg->subtype & kDBMap)
            editor_PostLoadCow();
         vBrush_DontUpdateFocus=FALSE;
         vBrush_SelectBrush(vBrush_editBrush_Get());

         break;
   
      case kDatabaseSave:
         if (msg->subtype & kDBMap)
            editor_SaveCow(info.load);
         break;
   }
}


////////////////////////////////////////
// BRUSH PROPERTY CREATE
//

static PropertyID brush_prop_id = -1; 

static void init_brush_prop(void)
{
   IBrushProperty* prop = BrushPropInit();
   brush_prop_id = IProperty_GetID(prop);
}

////////////////////////////////////////
// OBJECT MESSAGE HANDLER 
//

static void create_obj_brushes(void)
{
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   IObjectQuery* query = IObjectSystem_Iter(pObjSys,kObjectConcrete); 
   for (; !IObjectQuery_Done(query); IObjectQuery_Next(query))
   {
      ObjID obj = IObjectQuery_Object(query); 
      editobjCreateBrushfromObj(obj);   
   }
   SafeRelease(query);
   SafeRelease(pObjSys); 
}

static void destroy_obj_brushes(void)
{
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   IObjectQuery* query = IObjectSystem_Iter(pObjSys,kObjectConcrete); 
   
   CommandExecute ("unlock_all");
   for (; !IObjectQuery_Done(query); IObjectQuery_Next(query))
   {
      ObjID obj = IObjectQuery_Object(query); 
      editObjDeleteObjBrush(obj);   
   }
   SafeRelease(query);
   SafeRelease(pObjSys); 
}

#pragma off(unreferenced)
static void obj_message(ObjID obj, eObjNotifyMsg msg, void* data)
{

   switch (msg)
   {
      case kObjNotifyCreate:
      case kObjNotifyLoadObj:
         editobjCreateBrushfromObj(obj);
         break;
   }
}
#pragma on(unreferenced)

static tObjListenerHandle listenerHandle; 

static void init_obj_message(void)
{
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   sObjListenerDesc desc = { obj_message, NULL }; 
   listenerHandle = IObjectSystem_Listen(pObjSys,&desc);
   SafeRelease(pObjSys); 
}

static void term_obj_message(void)
{
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   listenerHandle = IObjectSystem_Unlisten(pObjSys, listenerHandle);
   SafeRelease(pObjSys); 
}

////////////////////////////////////////
// BRESTORE.H API
//
// and speaking of dirty work, horrible save brush focus code
static int   editorLastBrushID=-1;
static ObjID editorLastObjID=OBJ_NULL;

void SaveBrushSelection(void)
{
   editBrush *cur_br=vBrush_editBrush_Get();
   editorLastObjID=OBJ_NULL;
   editorLastBrushID=-1;
   if (brushGetType(cur_br)==brType_OBJECT)
      editorLastObjID=brObj_ID(cur_br);
   else
      editorLastBrushID=cur_br->br_id;
   //   mprintf("Store for restore %s: %d\n",
   //           editorLastObjID!=OBJ_NULL?"Obj":"Brush",
   //           editorLastObjID!=OBJ_NULL?editorLastObjID:editorLastBrushID);
}

void RestoreBrushSelection(void)
{
   editBrush *focus_br;
   //            mprintf("Want to restore %s: %d\n",
   //               editorLastObjID!=OBJ_NULL?"Obj":"Brush",
   //               editorLastObjID!=OBJ_NULL?editorLastObjID:editorLastBrushID);
   create_obj_brushes(); 
   if (editorLastObjID!=OBJ_NULL)
      focus_br=editObjGetBrushFromObj(editorLastObjID);
   else
      focus_br=brFind(editorLastBrushID);
   if (focus_br==NULL)
      focus_br=vBrush_editBrush_Get();
   vBrush_SelectBrush(focus_br);
}

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//


#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata; 

   switch(msg)
   {
      case kMsgAppInit:
         brushSysInit();
         vBrushInit();
         AreaPnP_Names_Init();
         master_bl=blistAlloc(256);
         blistSetCurrent(master_bl);
         init_brush_prop();
         editObjInit();
         break;

      case kMsgAppTerm:
         editObjTerm();
         vBrushFree();
         AreaPnP_Names_Term();
         brushClearAll();
         blistSetCurrent(NULL);
         blistFree(master_bl);
         brushSysFree();
         break;

      case kMsgResumeMode:
      case kMsgEnterMode:
      {
         state->edit_mode = IsEqualGUID(info.mode->to.pID,&LOOPID_EditMode); 
         if (state->edit_mode)
         {
            // we can't do this here because the mission backup might load
            //            create_obj_brushes();
            init_obj_message(); 
         }
      }
      break; 

      case kMsgExitMode:
      case kMsgSuspendMode:
      {
         if (state->edit_mode)
         {
            destroy_obj_brushes(); 
            vBrush_SelectBrush(vBrush_editBrush_Get());
            term_obj_message();
         }
      }
      break; 

      case kMsgEndFrame:
         if (state->edit_mode)
            vBrush_FrameCallback();
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;

      case kMsgEnd:
         Free(state);
         break;   
   }
   return result;
}

F_DECLARE_INTERFACE(IPropertyManager);

////////////////////////////////////////////////////////////
// 
// Loop client factory function. 
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
// 

sLoopClientDesc BrushListLoopClientDesc =
{
   &MY_GUID,                              // GUID
   "Brush List",                          // NAME        
   kPriorityNormal,                       // PRIORITY          
   kMsgEnd
   | kMsgEndFrame
   | kMsgDatabase 
   | kMsgsAppOuter
   | kMsgsMode, // INTERESTS      

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      { kConstrainBefore, &LOOPID_UI, kMsgDatabase }, 
      // so we get brushes before textures 
      { kConstrainBefore, &LOOPID_Wr, kMsgDatabase }, 
      { kConstrainBefore, &LOOPID_Editor, kMsgsMode }, 



      {kNullConstraint} // terminator
   }
};




