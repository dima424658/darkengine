/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/dumbprox.cpp,v 1.2 1998/06/22 02:49:06 dc Exp $
// really dumb prox system for use for now

// this is a onedirectional model
// in which we assume that the number of objs in the world which might have 
//  prox info is very small compared to the number of objects in the world
//  in general, such that it is faster/as fast to search all objects with
//  the property, as opposed to scanning nearby space for objects which might
//  have the property.
// clearly, as the effective prox distance drops, and the number of prox objects
//  grows, this solution becomes a worse and worse fit to the actual problem

// as i see it, the real issue is quickly being able to create multiple prox
//  properties, and that is the part that is bad about the current solution

#include <string.h>

#include <lg.h>
#include <dynarray.h>
#include <matrix.h>

#include <mprintf.h>

#include <objtype.h>
#include <objpos.h>
#include <wrtype.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <dumbprox.h>

#include <dbmem.h>

#define PROX_PROP_IMPL kPropertyImplLlist

//static sPropertyDesc ProxPropDesc = { "Prox", 0, NULL, 0, 0, { "Prox", "Generic"} };

IBoolProperty *ProxBuildProp(const char *prox_name)
{
   sPropertyDesc *ProxPropDesc=new sPropertyDesc;
   memset(ProxPropDesc,0,sizeof(sPropertyDesc));
   strncpy(ProxPropDesc->name,prox_name,sizeof(ProxPropDesc->name));
   ProxPropDesc->flags=kPropertyInstantiate;
   char *tmp_cat=(char *)Malloc(sizeof("Prox")+1);
   strcpy(tmp_cat,"Prox");
   ProxPropDesc->ui.category=tmp_cat;
   char *tmp_name=(char *)Malloc(strlen(prox_name)+1);
   strcpy(tmp_name,prox_name);
   ProxPropDesc->ui.friendly_name=tmp_name;
   IBoolProperty *pProxProp=CreateBoolProperty(ProxPropDesc,PROX_PROP_IMPL);
   delete ProxPropDesc;
   return pProxProp;
}

BOOL ProxCheckLoc(IBoolProperty *pProx, mxs_vector *src_pos, float rad, cDynObjArray *objList, ProxCallback pCB)
{
   sPropertyObjIter iter;
   float rad_2=rad*rad;
   BOOL found=FALSE, cur_val=FALSE;
   ObjID obj;

   if (src_pos==NULL) return FALSE;
   pProx->IterStart(&iter);
   while (pProx->IterNextValue(&iter,&obj,&cur_val))
      if (cur_val&&(OBJ_IS_CONCRETE(obj)))
      {  // is obj in range
         mxs_vector *iter_pos=&ObjPosGet(obj)->loc.vec;
         float dist_2=mx_dist2_vec(src_pos,iter_pos);
         if (dist_2<rad_2)
         {
            found=TRUE;
            if (objList)
               objList->Append(obj);
            if (pCB)
               if ((*pCB)(obj,dist_2))  // if callback returns true, abort out
                  break;
         }
      }
   pProx->IterStop(&iter);
   return found;
}

BOOL ProxCheckObj(IBoolProperty *pProx, ObjID src, float rad, cDynObjArray *objList, ProxCallback pCB)
{
   mxs_vector *src_pos=ObjPosGetLocVector(src);
   return ProxCheckLoc(pProx,src_pos,rad,objList,pCB);
}
