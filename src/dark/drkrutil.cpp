/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkrutil.cpp,v 1.6 1999/11/02 16:17:58 adurant Exp $
// dark rendering utility functions

#include <mprintf.h>

#include <config.h>
#include <osystype.h>
#include <drkinv.h>

#include <drkrutil.h>  // dark rendering utilities

#include <meshatt.h>
#include <meshatts.h>
#include <invtype.h>   // drkinvtype, really
#include <contain.h>   // so we can see what we are containing
#include <appagg.h>
#include <matrixc.h>

#include <meshprop.h>  //for location of attachments

#include <dbmem.h>

//////////////
// horrible link->attachment craziness for meshes

static IContainSys *pContainSys=NULL;

void _darkru_attached_obj_init(void)
{
   pContainSys=AppGetObj(IContainSys);
}

void _darkru_attached_obj_term(void)
{
   SafeRelease(pContainSys);
}

// for now, we support 2 attachments per biped, for ease of coding
// real thing needs multiple, offsets, joints per, etc
static ObjID _am_attached_to_belt(ObjID o)
{
   sContainIter *iter=NULL;
   ObjID attachee=OBJ_NULL;
      
   Assert_(pContainSys);

   iter=pContainSys->IterStartType(o,kContainTypeBelt,kContainTypeBelt);
   for (; !iter->finished; pContainSys->IterNext(iter))
      attachee=iter->containee;
   pContainSys->IterEnd(iter);
   return attachee;
}

static ObjID _am_attached_to_alt(ObjID o)
{
  sContainIter *iter=NULL;
  ObjID attachee=OBJ_NULL;
  
  Assert_(pContainSys); //the attachee case  Sorry.  Couldn't resist.
  
  iter=pContainSys->IterStartType(o,kContainTypeAlt,kContainTypeAlt);
  for (; !iter->finished; pContainSys->IterNext(iter))
    attachee=iter->containee;
  pContainSys->IterEnd(iter);
  return attachee;
}

// setup any attached objects, return final one of them
ObjID _darkru_setup_attached_objs(ObjID o)
{
   ObjID attached_obj=_am_attached_to_belt(o);
   if (attached_obj)
   {
      sMeshAttachInstance mData=GetBeltLinkAttachment(o);
      //must set our own object, so we do.
      mData.m_Object=attached_obj;
      AttachObjToMeshObj(o,&mData);
   }
   
   ObjID alt_attached_obj=_am_attached_to_alt(o);
   if (alt_attached_obj)
     {
       sMeshAttachInstance mData=GetAltLinkAttachment(o);
       //must set our own object, so we do.
       mData.m_Object=alt_attached_obj;
       AttachObjToMeshObj(o,&mData);
       return alt_attached_obj;
     }
   return attached_obj;
 }
   



//      sMeshAttachInstance mData={0,0,{0,0,0},0,{0,0,0}};
//      mx_mk_angvec(&mData.m_Angles, 0,16384,9000);
//      mData.m_Offset = cMxsVector(0.24,0.75,0.10);// watcom workaround
//for thief2, I moved the mesh attach a bit to conform to the newer
//models with slim waists.  Sexy!  AMSD
//      mData.m_Offset = cMxsVector(0.24,0.75,0.0);// watcom workaround
//      mData.m_Object=attached_obj;
//      mData.m_iJoint=8;     // for now, soon need this per joint
//      AttachObjToMeshObj(o,&mData);
//   }
//   return attached_obj;
//}

// remove any object we had just attached
void _darkru_remove_attached_objs(ObjID o)
{
   ObjID attached_obj=_am_attached_to_belt(o);
   if (attached_obj)
      RemoveObjFromMeshObj(o,attached_obj);
   attached_obj=_am_attached_to_alt(o);
   if (attached_obj)
     RemoveObjFromMeshObj(o,attached_obj);
}
