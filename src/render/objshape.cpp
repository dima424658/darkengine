/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objshape.cpp,v 1.24 2000/03/07 19:58:47 toml Exp $
// this module provides basic shape info about an object, based on its renderer model

#include <math.h>
#include <stdlib.h>

#include <dev2d.h>
#include <objpos.h>
#include <objshape.h>
#include <mnumprop.h>
#include <md.h>
#include <objmodel.h>
#include <matrix.h>
#include <rendprop.h>
#include <objscale.h>
#include <mm.h>
#include <hdskprop.h>
#include <meshapi.h>
#include <creatapi.h>

// Must be last header 
#include <dbmem.h>


#define ONE_OVER_ROOT_3 0.57735

typedef float (*GetRadiusFunc)(ObjID objID, int idx);
// returns type of bbox retrieved, -1 on error
typedef int (*GetBBoxFunc)(ObjID objID, int idx,mxs_vector *, mxs_vector *);

// returns what bbox type of favorite bbox is, where favorite bbox is that
// which takes up least space.
typedef int (*FavoriteBBoxFunc)(ObjID objID, int idx);

// Get a specific vhots location
typedef void (*GetVhotFunc)(ObjID objID, int idx, int vhot,mxs_vector *);

typedef struct
{
   GetRadiusFunc radFunc;
   union {
      struct {
          GetBBoxFunc relBBoxFunc,
                      offBBoxFunc,
                    worldBBoxFunc,
                 favoriteBBoxFunc;};
      // causes watcom internal error (toml 10-09-97) GetBBoxFunc bboxFuncs[4];
   };
   union {
      struct {
         GetVhotFunc relVhotFunc,offVhotFunc;};
   };
} ObjShapeFuncs;

//// TONS OF FUNCTION DECLARATIONS

// default object function declarations
static float GetDefaultRadius(ObjID objID, int idx);
static int GetDefaultObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetDefaultObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetDefaultWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static void GetDefaultObjRelVhot(ObjID objID, int idx, 
                                 int vhot,mxs_vector *pos);
static void GetDefaultObjOffsetVhot(ObjID objID, int idx, 
                                    int vhot,mxs_vector *pos);


// md model object function declarations
static float GetMdRadius(ObjID objID, int idx);
static int GetMdObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetMdObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetMdWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static void GetMdObjRelVhot(ObjID objID, int idx,
                            int vhot,mxs_vector *pos);
static void GetMdObjOffsetVhot(ObjID objID, int idx,
                               int vhot,mxs_vector *pos);

// heat object function declarations
static float GetHeatRadius(ObjID objID, int idx);
static int GetHeatObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetHeatObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetHeatWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);


// spark object function declarations
static float GetSparkRadius(ObjID objID, int idx);
static int GetSparkObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetSparkObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetSparkWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);


// mesh object function declarations
static float GetMeshRadius(ObjID objID, int idx);
static int GetMeshObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetMeshObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetMeshWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetMeshFavoriteBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);

// particle objects
EXTERN float GetParticleRadius(ObjID objID, int idx);
EXTERN int GetParticleObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
EXTERN int GetParticleObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
EXTERN int GetParticleWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
EXTERN int GetParticleFavoriteBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);

// bitmap objects
static float GetBitmapRadius(ObjID objID, int idx);
static int GetBitmapObjRelBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetBitmapObjOffsetBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);
static int GetBitmapWorldBBox(ObjID objID, int idx,
                                mxs_vector *bmin, mxs_vector *bmax);

// The array of function structures based on model type
ObjShapeFuncs g_aObjShapeFuncs[] =  { \
   { GetDefaultRadius,
        GetDefaultObjRelBBox, GetDefaultObjOffsetBBox,
        GetDefaultWorldBBox, GetDefaultObjRelBBox,
        GetDefaultObjRelVhot,GetDefaultObjOffsetVhot },
   { GetMdRadius,
        GetMdObjRelBBox, GetMdObjOffsetBBox,
        GetMdWorldBBox, GetMdObjRelBBox,
        GetMdObjRelVhot,GetMdObjOffsetVhot },
   { GetMeshRadius,
        GetMeshObjRelBBox, GetMeshObjOffsetBBox,
        GetMeshWorldBBox, GetMeshFavoriteBBox,
        GetDefaultObjRelVhot,GetDefaultObjOffsetVhot },
   { GetHeatRadius,
        GetHeatObjRelBBox, GetHeatObjOffsetBBox,
        GetHeatWorldBBox, GetHeatObjRelBBox,
        GetDefaultObjRelVhot,GetDefaultObjOffsetVhot },
   { GetSparkRadius,
        GetSparkObjRelBBox, GetSparkObjOffsetBBox,
        GetSparkWorldBBox, GetSparkObjRelBBox,
        GetDefaultObjRelVhot,GetDefaultObjOffsetVhot },
   { GetParticleRadius,
        GetParticleObjRelBBox, GetParticleObjOffsetBBox,
        GetParticleWorldBBox, GetParticleFavoriteBBox,
        GetDefaultObjRelVhot,GetDefaultObjOffsetVhot },
   { GetBitmapRadius,
        GetBitmapObjRelBBox, GetBitmapObjOffsetBBox,
        GetBitmapWorldBBox, GetBitmapObjRelBBox,
        GetDefaultObjRelVhot,GetDefaultObjOffsetVhot },
};

//// UTILITY FUNCTIONS
//


// trans is rotation matrix from bbox coord space to desired bounding coordinate space
EXTERN void BoundBBox(mxs_matrix *trans,mxs_vector *bmin,mxs_vector *bmax)
{
   mxs_vector dest_min={0,0,0},dest_max={0,0,0};
   int i,j;

   // graphics gems I, pg 548
   for (i=0; i < 3; ++i) {
      for (j=0; j < 3; ++j) {
         double a,b;
         a = trans->vec[j].el[i] * bmin->el[j];
         b = trans->vec[j].el[i] * bmax->el[j];
         if (a < b) {
            dest_min.el[i] += a;
            dest_max.el[i] += b;
         } else {
            dest_min.el[i] += b;
            dest_max.el[i] += a;
         }
      }
   }
   *bmin = dest_min;
   *bmax = dest_max;
}

// externed so particle system can be external
EXTERN void ObjRel2ObjScaledBBox(ObjID obj,mxs_vector *bmin,mxs_vector *bmax)
{
   mxs_vector scale;

   // scale bbox by object's scale
   if(ObjGetScale(obj,&scale))
   {
      mx_elmuleq_vec(bmin,&scale);
      mx_elmuleq_vec(bmax,&scale);
   }
}

// externed so particle system can be external
EXTERN void ObjRel2ObjOffsetBBox(ObjID obj,mxs_vector *bmin,mxs_vector *bmax)
{
   mxs_vector scale;
   mxs_matrix trans;

   // scale bbox by object's scale
   if(ObjGetScale(obj,&scale))
   {
      mx_elmuleq_vec(bmin,&scale);
      mx_elmuleq_vec(bmax,&scale);
   }
   // bound obj bbox in world space
   mx_ang2mat(&trans, &(ObjPosGet(obj)->fac));
   BoundBBox(&trans,bmin,bmax);
}

// externed so particle system can be external
EXTERN void ObjRel2WorldBBox(ObjID obj,mxs_vector *bmin,mxs_vector *bmax)
{
   mxs_vector scale;
   mxs_matrix trans;
   Position *pos=ObjPosGet(obj);

   AssertMsg1(pos, "ObjRel2WorldBBox() called for positionless object %d", obj);
   
   if (!pos)
      return;
      
   // scale bbox by object's scale
   if(ObjGetScale(obj,&scale))
   {
      mx_elmuleq_vec(bmin,&scale);
      mx_elmuleq_vec(bmax,&scale);
   }
   // bound obj bbox in world space
   mx_ang2mat(&trans, &(pos->fac));
   BoundBBox(&trans,bmin,bmax);

   // xlat bbox to world
   mx_addeq_vec(bmin,&pos->loc.vec);
   mx_addeq_vec(bmax,&pos->loc.vec);
}


// Converts a vector from object space to offset space, ie, rotated, but
// not translated into the world space
static void ObjRel2ObjOffsetVector(ObjID obj,mxs_vector *vec)
{
   // Rotate vector into obj space
   mxs_matrix mat;
   mx_ang2mat(&mat, &(ObjPosGet(obj)->fac));
   mx_mat_muleq_vec(&mat,vec);
}




///////////// MODEL TYPE-SPECIFIC FUNCTIONS
//

///// default functions
//

static float GetDefaultRadius(ObjID objID, int /*idx*/ )
{
   mxs_vector scale;

   if (ObjGetScale(objID,&scale))
   {
      // find max increase
      float max=scale.x;

      if(scale.y>max)
      max=scale.y;
      if(scale.z>max)
      max=scale.z;

      return 2.0*max*ONE_OVER_ROOT_3;
   }
   return 2.0*ONE_OVER_ROOT_3;
}

static int GetDefaultObjRelBBox(ObjID /*objID*/, int /*idx*/,
                                   mxs_vector *bmin, mxs_vector *bmax)
{
   mx_mk_vec(bmin,-2.0,-2.0,-2.0);
   mx_mk_vec(bmax,2.0,2.0,2.0);
   return OBJ_BBOX_TYPE_OBJREL;
}

static int GetDefaultObjOffsetBBox(ObjID obj, int idx,
                                   mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetDefaultObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2ObjOffsetBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_OBJOFF;
   }
   return OBJ_BBOX_TYPE_NONE;
}

static int GetDefaultWorldBBox(ObjID obj, int idx,
                                   mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetDefaultObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2WorldBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_WORLD;
   }
   return OBJ_BBOX_TYPE_NONE;
}


static void GetDefaultObjRelVhot(ObjID objID,int idx, int vhot,mxs_vector *pos)
{
   mx_zero_vec(pos);
}

static void GetDefaultObjOffsetVhot(ObjID objID,int idx, int vhot,mxs_vector *pos)
{
   // zero rotated is still zero
   mx_zero_vec(pos);
}


///// md functions
//

static float GetMdRadius(ObjID obj, int idx)
{
   mds_model *m=(mds_model *)objmodelGetModel(idx);

   if (m==NULL)
      return 0;
   else
   {
      float rad=m->radius;
      mxs_vector scale;

      if (ObjGetScale(obj,&scale))
      {
          // find max increase
          float max=scale.x;

          if(scale.y>max)
             max=scale.y;
          if(scale.z>max)
             max=scale.z;
          rad *= max;
      }
      return rad;
   }
}

static int GetMdObjRelBBox(ObjID /*obj*/, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   mds_model *m=(mds_model *)objmodelGetModel(idx);

   if (m!=NULL)
   {
      *bmin = m->bmin;
      *bmax = m->bmax;
      return OBJ_BBOX_TYPE_OBJREL;
   }
   return OBJ_BBOX_TYPE_NONE;
}

static int GetMdObjOffsetBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetMdObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2ObjOffsetBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_OBJOFF;
   }
   return OBJ_BBOX_TYPE_NONE;
}

static int GetMdWorldBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetMdObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2WorldBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_WORLD;
   }
   return OBJ_BBOX_TYPE_NONE;
}

static void GetMdObjRelVhot(ObjID objID, int idx,int vhot,mxs_vector *pos)
{
   mds_model *m=(mds_model *)objmodelGetModel(idx);

   if (m!=NULL)
   {
      int i;
      mds_vhot *v;      
      v = md_vhot_list(m);
      for (i=0;i<m->vhots;++i) {
         if (v->id==vhot) {
            mxs_vector scale;
            *pos = v->v;
            if(ObjGetScale(objID,&scale)) {
               mx_elmuleq_vec(pos,&scale);
            }
            return;
         }
         ++v;
      }
   }
   mx_zero_vec(pos);
}

static void GetMdObjOffsetVhot(ObjID objID, int idx, int vhot,mxs_vector *pos)
{       
   GetMdObjRelVhot(objID,idx,vhot,pos);
   ObjRel2ObjOffsetVector(objID,pos);
}


///// mesh functions
//

// XXX THIS IS AN ETCS HACK
// non-biped mesh objects have no way of getting accurate bboxes.
extern void stupid_jointpos_callback(mms_model *m, int joint_id, mxs_trans *t);

static void DippyMeshGetBBox(ObjID /*objID*/, int /*idx*/, mxs_vector *bmax, mxs_vector *bmin)
{
   mx_mk_vec(bmin,-1.0,-1.0,-1.0);
   mx_mk_vec(bmax,1.0,1.0,1.0);
   mx_scaleeq_vec(bmin,2.4);
   mx_scaleeq_vec(bmax,2.4);
}

// XXX ignore scale on mesh objects
static float GetMeshRadius(ObjID objID, int idx)
{
   float rad;
   IMesh *pMesh;

   // first assume a creature.  this is dippy.
   if((pMesh=CreatureGetMeshInterface(objID))!=NULL)
   {
      rad=pMesh->GetRadius();
   } else
   {
      mxs_vector bmin,bmax,bigun;
      int i;

      // slow but functional
      DippyMeshGetBBox(objID,idx,&bmin,&bmax);
      for(i=0;i<3;i++)
      {
         if(fabs(bmax.el[i])>fabs(bmin.el[i]))
            bigun.el[i]=bmax.el[i];
         else
            bigun.el[i]=bmin.el[i];
      }
      rad=mx_mag_vec(&bigun);
   }
   return rad;
}

// ignore scale since mesh objects cannot be scaled
static int GetMeshObjRelBBox(ObjID objID, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   IMesh *pMesh;

   // first assume a creature.  this is dippy.
   if((pMesh=CreatureGetMeshInterface(objID))!=NULL)
   {
      mxs_matrix trans;
      Position *pos=ObjPosGet(objID);

      pMesh->GetWorldBBox(bmin,bmax);
      // creature bbox in world coords, so untranslate ..
      mx_subeq_vec(bmin,&pos->loc.vec);
      mx_subeq_vec(bmax,&pos->loc.vec);

      // .. and bound world bbox in object space
      mx_ang2mat(&trans, &(pos->fac));
      mx_transeq_mat(&trans); // inverse of orthonormal matrix is its transpose
      BoundBBox(&trans,bmin,bmax);
   } else {
      DippyMeshGetBBox(objID,idx,bmin,bmax);
   }
   return OBJ_BBOX_TYPE_OBJREL;
}

static int GetMeshObjOffsetBBox(ObjID objID, int idx,
                             mxs_vector *bmin, mxs_vector *bmax)
{
   IMesh *pMesh;

   // first assume a creature.  this is dippy.
   if((pMesh=CreatureGetMeshInterface(objID))!=NULL)
   {
      mxs_vector *pos=&ObjPosGet(objID)->loc.vec;

      pMesh->GetWorldBBox(bmin,bmax);

      // creature bbox in world coords, so untranslate
      mx_subeq_vec(bmin,pos);
      mx_subeq_vec(bmax,pos);
      
   } else
   {
      mxs_matrix trans;
      DippyMeshGetBBox(objID,idx,bmin,bmax);
      // ignore scale since mesh objects cannot be scaled
      //  bound world bbox in object space
      mx_ang2mat(&trans, &(ObjPosGet(objID)->fac));
      BoundBBox(&trans,bmin,bmax);
   }
   return OBJ_BBOX_TYPE_OBJOFF;
}

// ignore scale since mesh objects cannot be scaled
static int GetMeshWorldBBox(ObjID objID, int idx,
                             mxs_vector *bmin, mxs_vector *bmax)
{
   IMesh *pMesh;

   if((pMesh=CreatureGetMeshInterface(objID))!=NULL)
   {
      pMesh->GetWorldBBox(bmin,bmax);
   } else
   {
      Position *pos=ObjPosGet(objID);
      mxs_matrix trans;

      DippyMeshGetBBox(objID,idx,bmin,bmax);
      // ignore scale since mesh objects cannot be scaled
      //  bound world bbox in object space
      mx_ang2mat(&trans, &(pos->fac));
      BoundBBox(&trans,bmin,bmax);

      // translate to world space
      mx_addeq_vec(bmin,&pos->loc.vec);
      mx_addeq_vec(bmax,&pos->loc.vec);
   }
   return OBJ_BBOX_TYPE_WORLD;
}

static int GetMeshFavoriteBBox(ObjID objID, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   IMesh *pMesh;

   if((pMesh=CreatureGetMeshInterface(objID))!=NULL)
   {
      pMesh->GetWorldBBox(bmin,bmax);
      return OBJ_BBOX_TYPE_WORLD;
   } else
   {
      DippyMeshGetBBox(objID,idx,bmin,bmax);
      return OBJ_BBOX_TYPE_OBJREL;
   }
}


//// heat object functions--we could try something more sophisticated,like writing some code
//

#define min(x,y) (((x)<(y))?(x):(y))
#define max(x,y) (((x)>(y))?(x):(y))

static float GetHeatRadius(ObjID obj, int idx)
{
   sHeatDiskCluster *cluster;

   if (ObjHeatDiskGet(obj, &cluster)) {

      mxs_real bounding_radius = max(cluster->start_radius, cluster->end_radius) + cluster->height;

      mxs_vector start_minus_radius, end_plus_radius;
      mxs_vector plus, minus, diagonal;

      mx_mk_vec(&plus, bounding_radius, bounding_radius, bounding_radius);
      mx_mk_vec(&minus, -bounding_radius, -bounding_radius, -bounding_radius);

      mx_add_vec(&start_minus_radius, &cluster->start_offset, &minus);
      mx_add_vec(&end_plus_radius, &cluster->end_offset, &plus);

      mx_sub_vec(&diagonal, &start_minus_radius, &end_plus_radius);

      return mx_mag_vec(&diagonal) * .5;

   } else
      return GetDefaultRadius(obj, idx);
}

// There's nothing more boring than a bounding box.  Even two bounding
// boxes aren't as boring as one.

// @TODO: If this is something we're using much we should add the bbox
// and radius to the structure proper.
static int GetHeatObjRelBBox(ObjID obj, int /*idx*/,
                                mxs_vector *bmin, mxs_vector *bmax)
{
   sHeatDiskCluster *cluster;

   if (ObjHeatDiskGet(obj, &cluster)) {

      // This radius can be too large, but should be pretty close
      // since we expect the height to be small.
      mxs_real bounding_radius = max(cluster->start_radius, cluster->end_radius) + cluster->height;
      mxs_vector start_minus_radius, start_plus_radius;
      mxs_vector end_minus_radius, end_plus_radius;
      mxs_vector plus, minus;

      mx_mk_vec(&plus, bounding_radius, bounding_radius, bounding_radius);
      mx_mk_vec(&minus, -bounding_radius, -bounding_radius, -bounding_radius);

      mx_add_vec(&start_plus_radius, &cluster->start_offset, &plus);
      mx_add_vec(&start_minus_radius, &cluster->start_offset, &minus);

      mx_add_vec(&end_plus_radius, &cluster->end_offset, &plus);
      mx_add_vec(&end_minus_radius, &cluster->end_offset, &minus);

      bmin->x = min(start_minus_radius.x, end_minus_radius.x);
      bmin->y = min(start_minus_radius.y, end_minus_radius.y);
      bmin->z = min(start_minus_radius.z, end_minus_radius.z);

      bmax->x = max(start_plus_radius.x, end_plus_radius.x);
      bmax->y = max(start_plus_radius.y, end_plus_radius.y);
      bmax->z = max(start_plus_radius.z, end_plus_radius.z);
      return OBJ_BBOX_TYPE_OBJREL;
   }
   return OBJ_BBOX_TYPE_NONE;
}

int GetHeatObjOffsetBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetHeatObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2ObjOffsetBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_OBJOFF;
   }
   return OBJ_BBOX_TYPE_NONE;
}

int GetHeatWorldBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetHeatObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2WorldBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_WORLD;
   }
   return OBJ_BBOX_TYPE_NONE;
}


///// spark object functions (verrrrrrry small)
//

static float GetSparkRadius(ObjID /*obj*/, int /*idx*/)
{
   return 0.0;
}

static int GetSparkObjRelBBox(ObjID /*obj*/, int /*idx*/,
                          mxs_vector *bmin, mxs_vector *bmax)
{
   mx_mk_vec(bmin, 0.0, 0.0, 0.0);
   mx_mk_vec(bmax, 0.0, 0.0, 0.0);
   return OBJ_BBOX_TYPE_OBJREL;
}

static int GetSparkObjOffsetBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetSparkObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2ObjOffsetBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_OBJOFF;
   }
   return OBJ_BBOX_TYPE_NONE;
}

static int GetSparkWorldBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetSparkObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2WorldBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_WORLD;
   }
   return OBJ_BBOX_TYPE_NONE;
}

///// bitmap object functions
//

static float GetBitmapSize(ObjID obj, int idx)
{
   grs_bitmap *bm = objmodelSetupBitmapTexture(idx, 0); // get first frame
   float size = (bm->w > bm->h ? bm->w : bm->h) / 16.0; // 16 texels per foot
   objmodelReleaseBitmapTexture(idx, 0);
   return size;
}

static float GetBitmapRadius(ObjID obj, int idx)
{
   float sz = GetBitmapSize(obj, idx)/2;
   mxs_vector scale;
   if (ObjGetScale(obj,&scale))
   {
      // find max increase
      float max=scale.x;

      if(scale.y>max)
      max=scale.y;
      if(scale.z>max)
      max=scale.z;

      return sz*max/ONE_OVER_ROOT_3;
   }
   return sz / ONE_OVER_ROOT_3;
}

static int GetBitmapObjRelBBox(ObjID obj, int idx,
                               mxs_vector *bmin, mxs_vector *bmax)
{
   sBitmapWorldspace *pBWS;
   if (ObjBitmapWorldspace(obj, &pBWS)) {
      mx_mk_vec(bmin, -.5 * pBWS->m_fXSize, -.5 * pBWS->m_fYSize, 0);
      mx_mk_vec(bmax, .5 * pBWS->m_fXSize, .5 * pBWS->m_fYSize, 0);
   } else {
      float sz = GetBitmapSize(obj, idx) / 2;
      mx_mk_vec(bmin, -sz, -sz, -sz);
      mx_mk_vec(bmax,  sz,  sz,  sz);
   }
   return OBJ_BBOX_TYPE_OBJREL;
}

static int GetBitmapObjOffsetBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetBitmapObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2ObjOffsetBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_OBJOFF;
   }
   return OBJ_BBOX_TYPE_NONE;
}

static int GetBitmapWorldBBox(ObjID obj, int idx, mxs_vector *bmin, mxs_vector *bmax)
{
   if(GetBitmapObjRelBBox(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)
   {
      ObjRel2WorldBBox(obj,bmin,bmax);
      return OBJ_BBOX_TYPE_WORLD;
   }
   return OBJ_BBOX_TYPE_NONE;
}


/////////////////////////////////////////////////
///// EXTERNALLY VISIBLE FUNCTIONS

float ObjGetRadius(ObjID obj)
{
   int idx=MIDX_INVALID,type;

   if(!ObjGetModelNumber(obj,&idx))
      return g_aObjShapeFuncs[OM_TYPE_DEFAULT].radFunc(obj,idx);
   type=objmodelGetModelType(idx);
#ifdef DBG_ON
   AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
      "ObjGetRadius: model type %d not supported\n",type);
#endif
   return g_aObjShapeFuncs[type].radFunc(obj,idx);
}

BOOL ObjGetObjRelBBoxUnscaled(ObjID obj, mxs_vector* bmin, mxs_vector* bmax)
{
   int idx=MIDX_INVALID,type;

   Assert_(bmin != NULL && bmax != NULL);
   if(!ObjGetModelNumber(obj,&idx))
      return(g_aObjShapeFuncs[OM_TYPE_DEFAULT].relBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL);

   type=objmodelGetModelType(idx);
#ifdef DBG_ON
   AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
      "ObjGetUnscaledBBox: model type %d not supported\n",type);
#endif
   return (g_aObjShapeFuncs[type].relBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL);
}

BOOL ObjGetObjRelBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax)
{
   int idx=MIDX_INVALID,type;

   Assert_(bmin != NULL && bmax != NULL);
   if(!ObjGetModelNumber(obj,&idx))
      return(g_aObjShapeFuncs[OM_TYPE_DEFAULT].relBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL);

   type=objmodelGetModelType(idx);
#ifdef DBG_ON
   AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
      "ObjGetUnscaledBBox: model type %d not supported\n",type);
#endif
   if ((g_aObjShapeFuncs[type].relBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL)) {
      ObjRel2ObjScaledBBox(obj,bmin,bmax);
      return TRUE;
   } else
      return FALSE;
}

BOOL ObjGetObjOffsetBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax)
{
   int idx=MIDX_INVALID,type;

   Assert_(bmin != NULL && bmax != NULL);
   if(!ObjGetModelNumber(obj,&idx))
      return(g_aObjShapeFuncs[OM_TYPE_DEFAULT].offBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL);

   type=objmodelGetModelType(idx);
#ifdef DBG_ON
   AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
      "ObjGetUnscaledBBox: model type %d not supported\n",type);
#endif
   return (g_aObjShapeFuncs[type].offBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJOFF);
}

BOOL ObjGetWorldBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax)
{
   int idx=MIDX_INVALID,type;

   if(!ObjGetModelNumber(obj,&idx))
      return(g_aObjShapeFuncs[OM_TYPE_DEFAULT].worldBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_OBJREL);

   type=objmodelGetModelType(idx);
#ifdef DBG_ON
   AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
      "ObjGetUnscaledBBox: model type %d not supported\n",type);
#endif
   return (g_aObjShapeFuncs[type].worldBBoxFunc(obj,idx,bmin,bmax)==OBJ_BBOX_TYPE_WORLD);
}


void ObjGetObjRelVhot(ObjID obj, int vhot,mxs_vector* pos)
{
   int idx=MIDX_INVALID,type;

   Assert_(pos != NULL);
   if(!ObjGetModelNumber(obj,&idx)) {
      g_aObjShapeFuncs[OM_TYPE_DEFAULT].relVhotFunc(obj,idx,vhot,pos);
      return;
   }

   type=objmodelGetModelType(idx);
#ifdef DBG_ON
   AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
      "ObjGetObjRelVhot: model type %d not supported\n",type);
#endif
   g_aObjShapeFuncs[type].relVhotFunc(obj,idx,vhot,pos);
}

void ObjGetObjOffsetVhot(ObjID obj, int vhot, mxs_vector* pos)
{
   int idx=MIDX_INVALID,type;

   Assert_(pos != NULL);
   if(!ObjGetModelNumber(obj,&idx)) {
      g_aObjShapeFuncs[OM_TYPE_DEFAULT].offVhotFunc(obj,idx,vhot,pos);
      return;
   }

   type=objmodelGetModelType(idx);
#ifdef DBG_ON
   AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
      "ObjGetObjOffsetVhot: model type %d not supported\n",type);
#endif
   g_aObjShapeFuncs[type].offVhotFunc(obj,idx,vhot,pos);
}




BOOL ObjGetUnscaledDims(ObjID obj, mxs_vector* dims)
{
   mxs_vector lo,hi;
   if(ObjGetObjRelBBoxUnscaled(obj,&lo,&hi))
   {
      mx_sub_vec(dims,&hi,&lo);
      return TRUE;
   }
   return FALSE;
}

int ObjGetFavoriteBBox(ObjID obj, mxs_vector* bmin, mxs_vector* bmax)
{
   int idx=MIDX_INVALID, type, bbox;

   if(!ObjGetModelNumber(obj,&idx)) {
      bbox = g_aObjShapeFuncs[OM_TYPE_DEFAULT].favoriteBBoxFunc(obj,idx,bmin,bmax);
   } else {
      type=objmodelGetModelType(idx);
#ifdef DBG_ON
      AssertMsg1(type<(sizeof(g_aObjShapeFuncs)/sizeof(ObjShapeFuncs)),\
         "ObjGetFavoriteBBox: model type %d not supported\n",type);
#endif
      bbox = g_aObjShapeFuncs[type].favoriteBBoxFunc(obj,idx,bmin,bmax);
   }

   if (bbox == OBJ_BBOX_TYPE_OBJREL)
      ObjRel2ObjScaledBBox(obj,bmin,bmax);
   return bbox;
     
}
