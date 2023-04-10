/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/bldstair.c,v 1.9 2000/02/19 12:27:41 toml Exp $
// stair building tools

#include <math.h>

#include <lg.h>
#include <matrix.h>
#include <mprintf.h>

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <brundo.h>
#include <primal.h>
#include <primfast.h>
#include <vbrush.h>
#include <status.h>
#include <gedit.h>
#include <brquery.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define STAIRS_STRAIGHT (0)
#define STAIRS_SPIRAL   (1)

static char *stairNames[]={"StraightStairSerf", "SpiralStairSerf"};

/////////////////////////
// shared stair parameters structure

typedef struct {
   char  type;     // STRAIGHT or SPIRAL, for now
   char  pad[3];   // hack hack hack
   BOOL  slats;    // do we have slats or full stairs
   BOOL  use_grid; // try and snap to grid/fit nicely with it
   int   cnt;      // how many stairs you want
   float height;   // if slats, how high each slat should be
   int   face_txt; // face textures
   int   step_txt; // step textures
   int   br_id;    // since we arent modal, we need to know who/where we are
   editBrush *br;  // filled in after the sdesc is entered with the br of our br_id
} sStairParms;

static sFieldDesc stair_fields [] =
{
   { "Step Count",   kFieldTypeInt,   FieldLocation(sStairParms,cnt) },
   { "Slats",        kFieldTypeBool,  FieldLocation(sStairParms,slats) },
   { "Use Grid",     kFieldTypeBool,  FieldLocation(sStairParms,use_grid) },
   { "Face Texture", kFieldTypeInt,   FieldLocation(sStairParms,face_txt) },
   { "Step Texture", kFieldTypeInt,   FieldLocation(sStairParms,step_txt) },
   { "Height",       kFieldTypeFloat, FieldLocation(sStairParms,height) }
};

static sStructDesc stair_struct = StructDescBuild(sStairParms,kStructFlagNone,stair_fields);

// im not gonna bother malloc and freeing this over and over, it just isnt worth it
static sStairParms static_stair_parms;
static BOOL        _stairs_inuse=FALSE;

// this is at the bottom of the code, but called by prop_cb on done, of course
BOOL stair_build(sStairParms *s);

///////////////////////////
// dialog controllers

static void LGAPI stair_prop_cb(sStructEditEvent *event, StructEditCBData data)
{
   sStairParms *s=(sStairParms *)data;
   switch (event->kind)
   {
      case kStructEdOK:
         stair_build(s);
         break;
      case kStructEdCancel:
         _stairs_inuse=FALSE;
         break;
   }
}

static BOOL stair_dialog(sStairParms *stairs)
{
   IStructEditor *sed;
   sStructEditorDesc editdesc;
   BOOL result;

   strncpy(editdesc.title, stairNames[stairs->type], sizeof(editdesc.title));
   editdesc.flags = kStructEditNoApplyButton;
   sed = CreateStructEditor(&editdesc, &stair_struct, stairs);
   if (!sed)
    { _stairs_inuse=FALSE; return FALSE; }
   IStructEditor_SetCallback(sed,stair_prop_cb,stairs);
   result = IStructEditor_Go(sed,kStructEdModal);
   SafeRelease(sed);
   return TRUE;
}

///////////////////////
// setup code, build the sStairParms struct and stuff

// pass 0 to mean i want to do fancy dialog parameter selection
// pass -1 to mean just do something - or a number of stairs

static BOOL stair_startup_common(sStairParms *ssp, int cnt, int br_id)
{
   ssp->slats=TRUE;
   ssp->use_grid=TRUE;
   ssp->cnt=(cnt==0)?8:cnt;
   ssp->height=0.0;
   ssp->br_id=br_id;
   ssp->face_txt=0;
   ssp->step_txt=0;
   _stairs_inuse=TRUE;
   if (cnt==0)
      return stair_dialog(ssp);
   else
      return stair_build(ssp);
}

BOOL straight_stair_startup(int cnt, int br_id)
{
   sStairParms *ssp=&static_stair_parms;
   ssp->type=STAIRS_STRAIGHT;
   return stair_startup_common(ssp,cnt,br_id);
}

BOOL spiral_stair_startup(int cnt, int br_id)
{
   sStairParms *ssp=&static_stair_parms;
   ssp->type=STAIRS_SPIRAL;
   return stair_startup_common(ssp,cnt,br_id);
}

//////////////////////////////
// commands - do validity checking, then hand off to above
// pass 0 to mean i want to do fancy dialog parameter selection
// pass -1 to mean just do something - or a number of stairs

// hacks till we have real code design for primal types
#define STAIR_CODE_CUBE (0)    // some cube type
#define STAIR_CODE_CYL  (1)    // current brush is solo ngon cylinder (or trunc pyramid, somehow?)

// verify the brush in question is ok
static int stair_brush_verify(editBrush *us, int primal_type)
{
   if (brSelect_Flag(us)==brSelect_VIRTUAL)
      return -1;
   else if (brushGetType(us)!=brType_TERRAIN)
      return -1;
   else if (us->media!=1)
      return -1;
   else
   {
      switch (primal_type)
      {
         case STAIR_CODE_CUBE:
            if (us->primal_id==PRIMAL_CUBE_IDX)
               return us->br_id;
            break;
         case STAIR_CODE_CYL:
            if (primalID_GetType(us->primal_id)==PRIMAL_TYPE_CYLINDER)
               return us->br_id;
            break;
      }
   }
   return -1;
}

static void do_stair_cmd(int cnt, int primal_type, BOOL (*startup_call)(int cnt, int br_id))
{
   int br_id=stair_brush_verify(vBrush_editBrush_Get(),primal_type);
   if (br_id==-1)
      Status("Illegal initial brush");
   else
      if ((*startup_call)(cnt,br_id))
         Status("Have some stairs");
      else
         Status("Couldnt startup stairSerf");
}

// actual command calls
void straight_stair_cmd(int cnt)
{
   do_stair_cmd(cnt,STAIR_CODE_CUBE,straight_stair_startup);
}

void spiral_stair_cmd(int cnt)
{
   do_stair_cmd(cnt,STAIR_CODE_CYL,spiral_stair_startup);
}

//////////////////////////
// shared utilities for all stair building

#define STAIR_EPS (0.00001)

// given pt_list (len cnt) as a clockwise face,
//  picks normal and center (from all) and src_pt (midway from 1st to last pt)
static BOOL get_face_vecs(mxs_vector *norm, mxs_vector *src_pt, mxs_vector *center_pt, mxs_vector *width_vec, mxs_vector *pt_list, int cnt)
{
   mxs_vector edge[2];
   int i;
   
   if (cnt<3) return FALSE;
   
   mx_sub_vec(&edge[0],&pt_list[0],&pt_list[1]);      // build the first two edges of the face
   mx_sub_vec(&edge[1],&pt_list[1],&pt_list[2]);
   mx_cross_norm_vec(norm,&edge[0],&edge[1]);         // and cross to get the normal
   
   mx_add_vec(src_pt,&pt_list[0],&pt_list[cnt-1]);    // and then get the "face base point"
   mx_scaleeq_vec(src_pt,0.5);                        //   by averaging the first and last points in the poly

   mx_sub_vec(width_vec,&pt_list[cnt-1],&pt_list[0]); // and then get the width vec by assuming 

   mx_zero_vec(center_pt);
   for (i=0; i<cnt; i++)
      mx_addeq_vec(center_pt,&pt_list[i]);
   mx_scaleeq_vec(center_pt,1.0/(float)cnt);
   
   return TRUE;
}

static BOOL decode_face(editBrush *br, int face, mxs_vector *norm, mxs_vector *src_pt, mxs_vector *center_pt, mxs_vector *width_vec)
{
   int pt_cnt, *pt_id_list;  // cnt, ids of points on this face
   mxs_vector *pt_raw_vecs;  // raw transformed point vectors
   BOOL rv=FALSE;

   pt_id_list=primalBr_FacePts(br->primal_id,face,&pt_cnt);
   if (pt_id_list==NULL) return FALSE;
   pt_raw_vecs=(mxs_vector *)Malloc(pt_cnt*sizeof(mxs_vector));
   if (pt_raw_vecs==NULL) return FALSE;
   primalRawPart(br->primal_id,&br->sz,pt_raw_vecs,pt_id_list,pt_cnt);
   rv=get_face_vecs(norm,src_pt,center_pt,width_vec,pt_raw_vecs,pt_cnt);
   Free(pt_raw_vecs);
   return rv;
}

static int find_primary_axis(mxs_vector *step)
{
   int i, axis;
   for (axis=-1, i=0; i<3; i++)  // find axis
      if (fabs(step->el[i])>STAIR_EPS)
         if (axis==-1)
            axis=i;
         else
         {
            Warning(("Multiple valued primary axis (%d %d) - (%g %g %g)???\n",axis,i,step->x,step->y,step->z));
            return -1;
         }
   return axis;
}

static BOOL finalize_stair_brush(editBrush *br, sStairParms *s)
{
   br->ang=s->br->ang;                            // set angles
   br->media=0;                                   // this means fill solid, sneakily
   if (s->face_txt==0) s->face_txt=s->br->tx_id;  // inherit from brush, i guess
   br->tx_id=s->face_txt;
   if (s->step_txt==0) s->step_txt=s->face_txt;
   br->txs[5].tx_id=br->txs[4].tx_id=s->step_txt; // and the step top and bottom
   return TRUE;
}

// warning - actual code that does work starts here
////////////////////////////////
// straight stair tool
// given a cube air brush, generate a bunch of new stair brushes

static BOOL build_straight_stairs(sStairParms *s)
{
   mxs_vector norm, src_pt, center_pt, face_width_vec;
   mxs_vector h_step, v_step, cur_pos;
   mxs_matrix rot;
   int h_axis, v_axis;
   int i, j;

   if (!decode_face(s->br,s->br->cur_face==-1?0:s->br->cur_face,&norm,&src_pt,&center_pt,&face_width_vec))
       return FALSE;      // go find axis of the brushes
       
   // so, by symmetry arguments, we know h_total is -center_pt*2 and v_total is (center_pt-src_pt)*2
   mx_scale_vec(&h_step,&center_pt,-2.0/(float)s->cnt);
   mx_sub_vec(&v_step,&center_pt,&src_pt);
   mx_scaleeq_vec(&v_step,2.0/(float)s->cnt);
   h_axis=find_primary_axis(&h_step);   // go get the primary axis'
   v_axis=find_primary_axis(&v_step);   
   if ((h_axis==-1)||(v_axis==-1)) return FALSE;
   if (h_axis==v_axis) { Warning(("horiz and vert axis the same???\n")); return FALSE; }
   
   mx_scale_add_vec(&cur_pos,&src_pt,&h_step,0.5);   // get to right initial pos (1/2 step from base, really)
   mx_scale_addeq_vec(&cur_pos,&v_step,0.5);
   mx_ang2mat(&rot,&s->br->ang);
   editUndoStoreBlock(TRUE);
   for (i=0; i<s->cnt; i++)
   {
      editBrush *tmp=brushInstantiate(PRIMAL_CUBE_IDX); // get a brush to work with...
      mxs_vector real_pos=cur_pos;
      mx_mat_mul_vec(&real_pos,&rot,&cur_pos);       // fix position up by rotation
      mx_add_vec(&tmp->pos,&real_pos,&s->br->pos);   
      for (j=0; j<3; j++)                            // size based on axis fun...
      {
//this code appears to do the right thing except for making sure size gets
//abs value.  So I'm making it all use abs.  AMSD
         if (j==h_axis)
            tmp->sz.el[j]=fabs(h_step.el[j]/2.0);
         else if (j==v_axis)
            if (s->slats)
               if (s->height>STAIR_EPS)              // 0.0 means use size based on step size
                  tmp->sz.el[j]=s->height; //guaranteed positive
               else
                  tmp->sz.el[j]=fabs(v_step.el[j]/4.0);
            else
               tmp->sz.el[j]=fabs(cur_pos.el[j]-src_pt.el[j]);
         else
            tmp->sz.el[j]=fabs(s->br->sz.el[j]);
      }
      finalize_stair_brush(tmp,s);
      gedit_full_create_brush(tmp,NULL,GEDIT_CREATE_AT_END,GEDIT_DEFAULT_BRTYPE);
      vBrush_NewBrush(tmp);                          // build and notify about the brush
      mx_addeq_vec(&cur_pos,&h_step);                // update cur_pos to next step
      mx_addeq_vec(&cur_pos,&v_step);
      if (!s->slats)
         cur_pos.el[v_axis]-=v_step.el[v_axis]/2.0;  // downstep the base 
   }
   editUndoStoreBlock(FALSE);
   return TRUE;
}

///////////////////////////
// spiral stair tool
// given an ngon cyl air brush, generate a bunch of new stair brushes

static BOOL build_spiral_stairs(sStairParms *s)
{
   
   return TRUE;
}

/////////////////////////////
// base call/dispatch
// actually to the building of each type
static BOOL stair_build(sStairParms *s)
{
   _stairs_inuse=FALSE;
   s->br=brFind(s->br_id);
   if (s->br) // make sure it is an air brush
   {
      switch (s->type)
      {
         case STAIRS_STRAIGHT:
            if (stair_brush_verify(s->br,STAIR_CODE_CUBE))
               return build_straight_stairs(s);
            break;
         case STAIRS_SPIRAL:
            if (stair_brush_verify(s->br,STAIR_CODE_CYL))
               return build_spiral_stairs(s);
            break;
      }
      Warning(("Bad media or brush data type for genesis brush\n"));
   }
   else
      Warning(("Brush you started the serf on is missing\n"));
   Status("Cant build stairs with this brush");
   return FALSE;
}
