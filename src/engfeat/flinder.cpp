/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/flinder.cpp,v 1.2 1999/04/28 13:49:42 XEMU Exp $

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <matrix.h>
#include <rand.h>

#include <iobjsys.h>
#include <objedit.h>
#include <objpos.h>
#include <objdef.h>
#include <objshape.h>

#include <sphrcst.h>
#include <sphrcsts.h>
#include <wrtype.h>
#include <port.h>

#include <phprop.h>
#include <physapi.h>

#include <flinder.h>

// Must be last header
#include <dbmem.h>

void CreateFlinders(ObjID victim, ObjID flinder, int count, BOOL scatter, float impulse, const mxs_vector &offset)
{
   if (OBJ_IS_CONCRETE(flinder))
   {
      Warning(("%s attempts to flinderize with concrete flinders?\n", ObjWarnName(victim)));
      return;
   }

   if (count == 0)
   {
      Warning(("%s attempt to flinderize into %s, but 0 count?\n", ObjWarnName(victim), ObjWarnName(flinder)));
      return;
   }

   mxs_vector flinder_loc;
   Position  *pPos;
   int i, j;

   if ((pPos = ObjPosGet(victim)) == NULL)
   {
      Warning(("Attempt to flinderize object %d w/o position!\n", victim)); 
      return;
   }

   if (!scatter)
   {
      mxs_matrix obj_rot;

      mx_ang2mat(&obj_rot, &pPos->fac);
      mx_mat_mul_vec(&flinder_loc, &obj_rot, &offset);
      mx_addeq_vec(&flinder_loc, &pPos->loc.vec);
   }

   AutoAppIPtr_(ObjectSystem, pObjSys);

   for (i=0; i<count; i++)
   {
      // create the flinder
      ObjID flinder_obj = pObjSys->BeginCreate(flinder, kObjectConcrete);

      // find a valid location for this flinder, if we're scattering
      if (scatter)
      {
         cPhysDimsProp *pDimsProp;

         if (!g_pPhysDimsProp->Get(flinder_obj, &pDimsProp))
         {
            Warning(("Non-physical flinder obj %s?\n", ObjWarnName(flinder)));
            pObjSys->EndCreate(flinder_obj);
            pObjSys->Destroy(flinder_obj);
            return;
         }

         // this probably should be the object-relative bbox, and then we should
         // rotate it ourselves, but that forces us to ang2mat for each flinderize
         // link, and we just don't care that much.         
         mxs_vector bmin, bmax;
         ObjGetWorldBBox(victim, &bmin, &bmax);

         for (j=0; j<3; j++)
         {
            mxs_real rand_amt = ((float)(Rand() % 1024) / 1024.0);
            flinder_loc.el[j] = ((bmax.el[j] - bmin.el[j]) * rand_amt) + bmin.el[j];
         }

         Location start, end;

         MakeHintedLocationFromVector(&start, &pPos->loc.vec, &pPos->loc);
         MakeHintedLocationFromVector(&end, &flinder_loc, &pPos->loc);

         if (pDimsProp->radius[0] == 0.0)
         {
            Location hit;

            if (!PortalRaycast(&start, &end, &hit, FALSE))
               mx_interpolate_vec(&flinder_loc, &start.vec, &hit.vec, 0.95);
            else
               flinder_loc = end.vec;
         }
         else
         {
            int contact_count;

            if ((contact_count = SphrSpherecastStatic(&start, &end, pDimsProp->radius[0], 0)) > 0)
            {
               sSphrContact *pContact = &gaSphrContact[0];
               for (j=1; j<contact_count; j++)
               {
                  if (gaSphrContact[j].time < pContact->time)
                     pContact = &gaSphrContact[j];
               }

               mx_interpolate_vec(&flinder_loc, &start.vec, &pContact->point_on_ray, 0.95);
            }
            else
               flinder_loc = end.vec;
         }
      }

      // update its position
      ObjPosUpdate(flinder_obj, &flinder_loc, &pPos->fac);
      pObjSys->EndCreate(flinder_obj);

      // apply a velocity away
      mxs_vector velocity;
      mxs_vector rot_velocity;
      mxs_real   obj_impulse;

      mx_sub_vec(&velocity, &flinder_loc, &pPos->loc.vec);
      if (mx_mag2_vec(&velocity) > 0.0001)
         mx_normeq_vec(&velocity);

      obj_impulse = impulse + (((float)(Rand() % 1024) / 1024.0) * impulse * 0.5) - impulse * 0.25;

      mx_scaleeq_vec(&velocity, impulse); 
      PhysSetVelocity(flinder_obj, &velocity);

      // apply a rotational velocity, too
      for (j=0; j<3; j++)
         rot_velocity.el[j] = ((float)(Rand() % 1024) / 512.0) - 1.0;

      mx_scaleeq_vec(&rot_velocity, impulse * 0.6);
      PhysSetRotationalVelocity(flinder_obj, &rot_velocity);
   }
}



