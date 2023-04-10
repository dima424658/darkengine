/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkvhot.cpp,v 1.5 1999/11/19 14:55:09 adurant Exp $

This file has been moved to gunvhot.cpp AMSD

#include <matrix.h>
extern "C" 
{
#include <r3d.h>
#include <md.h>
}
#include <objpos.h>
#include <rendprop.h>
#include <mnamprop.h>
#include <objmodel.h>
#include <objscale.h>
#include <portal.h>
#include <rendobj.h>

#include <mprintf.h>

// must be last header
#include <dbmem.h>

// from rendobj.c
EXTERN mds_parm *getRendParms(ObjID obj, mds_model *m);

// This is almost entirely copied from a function in litprop.cpp
// but I wanted to add the ability to set the actual joint positions
EXTERN void VHotGetRaw(mxs_vector *pOffset, ObjID objID, int vhotNum)
{
   char model_name[80];

   if (ObjGetModelName(objID, model_name)) 
   {
      int idx = objmodelGetIdx(model_name);

      if (objmodelGetModelType(idx) == OM_TYPE_MD) 
      {
         mds_model *model = (mds_model *) objmodelGetModel(idx);
         mds_vhot* pVhots = md_vhot_list(model);
         *pOffset = pVhots[vhotNum].v;
      }
   }
}

EXTERN void VHotGetOffset(mxs_vector *pOffset, ObjID objID, int vhotNum)
{
   char model_name[80];
   mxs_vector scale;   
   mds_parm *pParms;

   pOffset->x = 0;
   pOffset->y = 0;
   pOffset->z = 0;
   // vhot time.  We get the model type from the model idx, which we
   // get from the model name.  That will tell us whether it's an md
   // model, which is the only kind with vhots.  There is almost
   // certainly a simpler way to do this.
   if (ObjGetModelName(objID, model_name)) 
   {
      int idx = objmodelGetIdx(model_name);

      if (objmodelGetModelType(idx) == OM_TYPE_MD) 
      {
         mds_model *model = (mds_model *) objmodelGetModel(idx);
         if (model->vhots<=vhotNum)
            return;
         // get joint posns
         pParms = getRendParms(objID, model);
         r3_start_frame();
         md_eval_vhots(model, pParms);
         r3_end_frame();
         
         mx_addeq_vec(pOffset, &mdd_vhot_tab[vhotNum]);
      }
   }

   // Of course, the object may have a scale property.
   if (ObjGetScale(objID, &scale))
      mx_elmuleq_vec(pOffset, &scale);
}

// This is almost entirely copied from a function in litprop.cpp
// but I wanted to add the ability to set the actual joint positions
EXTERN void VHotGetLoc(mxs_vector *pLoc, ObjID objID, int vhotNum)
{
   mxs_vector offset;
   mxs_matrix trans;
   ObjPos *pos = ObjPosGet(objID);

   VHotGetOffset(&offset, objID, vhotNum);

   // And, at last, we put the offset into worldspace and add in the
   // object's coordinates.
   mx_ang2mat(&trans, &pos->fac);
   mx_mat_mul_vec(pLoc, &trans, &offset);

   mx_addeq_vec(pLoc, &pos->loc.vec);
}
