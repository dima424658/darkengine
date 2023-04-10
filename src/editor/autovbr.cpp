/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/autovbr.cpp,v 1.3 2000/02/19 12:27:40 toml Exp $

// COM fun
#include <appagg.h>
#include <aggmemb.h>

#include <mprintf.h>
#include <matrix.h>

// sdesc
#include <sdesc.h>
#include <sdesbase.h>
#include <propbase.h>
#include <propface.h>

#include <command.h>
#include <iobjsys.h>
#include <objdef.h>
#include <objnotif.h>
#include <objpos.h>

#include <wrtype.h>
#include <editbr_.h>
#include <editbr.h>
#include <editbrs.h>
#include <vbrush.h>
#include <autovbr.h>
#include <vbr_math.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

ILabelProperty *gAutoVBrushProp;
EXTERN void br_update_obj(editBrush *us);

static void ObjSysListener(ObjID obj, eObjNotifyMsg msg, void*)
{
   AutoAppIPtr_(ObjectSystem,pObjSys);
   char cmd[255];
   Label *vbrname;

   if ((msg == kObjNotifyCreate) && (OBJ_IS_CONCRETE(obj)))
   {
      if (gAutoVBrushProp->IsRelevant(obj))
      {
         gAutoVBrushProp->Get(obj, &vbrname);
         sprintf(cmd,"load_group %s",vbrname->text);

         mprintf("loading group %s from obj %d\n",vbrname->text,obj);

         // load up the multibrush
         CommandExecute(cmd);

         // move the multibrush to this location
         ObjPos *p1;
         editBrush *br=vBrush_GetSel();
         p1 = ObjPosGet(obj);
         mx_sub_vec(br_trans_vec,&p1->loc.vec,&br->pos);
         vBrush_GroupOp(FALSE,br_translate);    // zero relative them all
         vBrush_GroupOp(FALSE,br_update_obj);    

         // delete this poor object
         pObjSys->Destroy(obj);
      }
   }

}

static sPropertyDesc AutoVBrushDesc =
{
   "AutoVBR", 0, 
   NULL, 0, 0, // constraints, versions
   { "Editor", "Auto-Multibrush" },
};

void AutoVBrushInit(void)
{
   gAutoVBrushProp = CreateLabelProperty(&AutoVBrushDesc, kPropertyImplDense);

   // listen to obj sys changes 
   AutoAppIPtr_(ObjectSystem,pObjSys);
   sObjListenerDesc listener = { ObjSysListener, NULL}; 
   pObjSys->Listen(&listener); 
}
