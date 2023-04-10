/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/reprthlp.cpp,v 1.2 1998/10/21 23:40:49 dc Exp $
// Misc helper functions for better/easier report generation

#include <mprintf.h>
#include <appagg.h>

#include <contain.h>        // for belt report

#include <report.h>
#include <reprthlp.h>

#include <room.h>
#include <rooms.h>
#include <roomsys.h>

#include <objpos.h>
#include <objedit.h>

#include <dbmem.h>

static IContainSys       *_gpContainSys=NULL;
static IIntProperty      *_gpStackCountProp=NULL;

void ReportHelpersStartUse(void)
{
   if (!_gpContainSys)
      _gpContainSys=AppGetObj(IContainSys);
   if (!_gpStackCountProp)
      _gpStackCountProp=_gpContainSys->StackCountProp();
}

void ReportHelpersDoneWith(void)
{
   if (_gpContainSys)
   {
      SafeRelease(_gpContainSys);
      _gpContainSys=NULL;
   }
   if (_gpStackCountProp)
   {
      SafeRelease(_gpStackCountProp);
      _gpStackCountProp=NULL;
   }
}

// for rooms
char *ReportHelperRoomString(ObjID obj, mxs_vector *pos, char *buffer)
{
   static char our_static_buf[64];
   char *p=buffer?buffer:our_static_buf;
   mxs_vector *use_pos=pos?pos:ObjPosGetLocVector(obj);
   cRoom *rm=g_pRooms->RoomFromPoint(*use_pos);
   if (rm)
   {
      sprintf(p," room %s",rm->GetName());
      return p;
   }
   return "";     // if we are here, we have nothing to say
}

// for in room DB?
BOOL ReportHelperInRoom(ObjID obj, mxs_vector *pos, char *p)
{
   mxs_vector *use_pos=pos?pos:ObjPosGetLocVector(obj);
   cRoom *rm=g_pRooms->RoomFromPoint(*use_pos);
   if (rm)
   {
      if (p) sprintf(p," room %s",rm->GetName());
      return TRUE;
   }
   else
   {
      if (p) strcpy(p,"");
      return FALSE;
   }
}

// for "room and pos or what im in"
char *ReportHelperLocationString(ObjID obj, char *buffer)
{
   static char our_static_buf[128];
   char *p=buffer?buffer:our_static_buf;
   mxs_vector *pos=ObjPosGetLocVector(obj);
   ObjID holder=_gpContainSys->GetContainer(obj);
   if (holder)
   {
      pos=ObjPosGetLocVector(holder);
      if (pos!=NULL)
         sprintf(p," at %d %d %d%s in %s",RepVecInts(pos),RHRoomPosS(holder,pos),ObjWarnName(holder));
      else
         sprintf(p," in %s",ObjWarnName(holder));
   }
   else
      if (pos!=NULL)
         sprintf(p," at %d %d %d%s",RepVecInts(pos),RHRoomPosS(obj,pos));
      else
         sprintf(p," lost");
   return p;
}

// for stack counts
char *ReportHelperStackString(ObjID obj, char *buffer)
{
   static char our_static_buf[64];
   char *p=buffer?buffer:our_static_buf;
   int count=1;

   if (_gpStackCountProp->Get(obj,&count))
//      if (count!=1)   // should this be != 1, or > 1
      {               // ie. do we want to be reporting on 0, or not?
         sprintf(p," stack %d",count);
         return p;
      }
   return "";     // if we are here, we have nothing to say
}

BOOL ReportHelperHasStack(ObjID obj)
{
   int count=1;

   if (_gpStackCountProp->Get(obj,&count))
      return TRUE;
   return FALSE;
}
