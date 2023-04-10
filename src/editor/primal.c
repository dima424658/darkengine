/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/primal.c,v 1.11 2000/02/19 13:11:14 toml Exp $
// primal brush systems

#include <malloc.h>
#include <string.h>
#include <math.h>

#include <lg.h>
#include <r3d.h>
#include <matrix.h>

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>

#include <primal.h>
#include <primals.h>
#include <primshap.h>
#include <primfast.h>

// exposed in primal.h for defines and such - look up an ID here
primalInfo *primals_list[2*(USED_PRIMAL_TYPES)][USED_PRIMAL_SIDES-2];

// for setting up default
static primalInfo default_primal={0,0,0,0,NULL,NULL,NULL,NULL};

////////////////////
// fast and unsafe system for editbr rendering use

// rest of this is in primfast.h macros
int *primalBrs_FU_faceedgebase=NULL;
primalInfo *primalBrs_FastUnsafe=NULL;
void primalBr_SetFastUnsafePrimal(int primal_id)
{
   primalBrs_FastUnsafe=getPrimInfo(primal_id);
}

/////////////////////
// internal utility stuff

// given a primalInfo with points edges and faces set up, allocate us the 3 arrays
BOOL _primalBr_GetMem(primalInfo *us)
{
   us->pt_array=(mxs_vector *)Malloc(sizeof(mxs_vector)*us->points);
   us->edge_list=(int *)Malloc(sizeof(int)*us->edges*2);  // each edge is 2 points
   us->face_pts_list=(int *)Malloc(sizeof(int)*us->faces*us->face_stride);
   us->face_edge_list=(int *)Malloc(sizeof(int)*us->faces*us->face_stride);
   return us->pt_array && us->edge_list && us->face_pts_list && us->face_edge_list;
}

// free the memory of the pointers in
void _primalBr_FreeMem(primalInfo *us)
{
   if (us->pt_array)  Free(us->pt_array);
   if (us->edge_list) Free(us->edge_list);
   if (us->face_pts_list) Free(us->face_pts_list);
   if (us->face_edge_list) Free(us->face_edge_list);
}

BOOL _primalBr_SzCheck(primalInfo *us)
{
   if (us->edges>MAX_PRIMAL_EDGES) return FALSE;
   if (us->faces>MAX_PRIMAL_FACES) return FALSE;
   if (us->points>MAX_PRIMAL_PTS) return FALSE;
   return TRUE;
}

//////////////////////
// startup and shutdown

// @TODO: add a save and load for primals to send them to disk and all

static BOOL cur_setup=FALSE;
static primalInfo cube, line, light,wedge,dodec;

#define PRIMAL_CNT_PER (USED_PRIMAL_SIDES-2)
static primalInfo vcyl[PRIMAL_CNT_PER], fcyl[PRIMAL_CNT_PER];
static primalInfo vpyr[PRIMAL_CNT_PER], fpyr[PRIMAL_CNT_PER];
static primalInfo vcpyr[PRIMAL_CNT_PER], fcpyr[PRIMAL_CNT_PER];

// NEED TO SET UP WHEN THIS IS CALLED/CLEARED
// setup the primal brush system
void primalBr_Init(void)
{
   int i;
   BOOL worked=TRUE;
   if (cur_setup) { Warning(("Yo - PrimalInit Again")); return; }
   worked&=PrimShape_CreateCube(&cube);    // setup the master cube of destruction
   worked&=PrimShape_CreateLine(&line);   
   worked&=PrimShape_CreateLight(&light);  // build the custom light shape
   worked&=PrimShape_CreateWedge(&wedge);
   worked&=PrimShape_CreateDodecahedron(&dodec);
   for (i=0; i<USED_PRIMAL_SIDES-3; i++)   // all the basics default to cube
      primals_list[0][i]=primals_list[1][i]=&cube;
   getPrimInfo(PRIMAL_LIGHT_IDX)=getPrimInfo(PRIMAL_LIGHT_IDX|PRIMAL_ALIGN_FACE)=&light;
   getPrimInfo(PRIMAL_LINE_IDX)=getPrimInfo(PRIMAL_LINE_IDX|PRIMAL_ALIGN_FACE)=&line;   
   // hm, are these actually the same when face aligned?
   getPrimInfo(PRIMAL_WEDGE_IDX)=getPrimInfo(PRIMAL_WEDGE_IDX|PRIMAL_ALIGN_FACE)=&wedge;
   getPrimInfo(PRIMAL_DODEC_IDX)=getPrimInfo(PRIMAL_DODEC_IDX|PRIMAL_ALIGN_FACE)=&dodec;
   for (i=0; i<PRIMAL_CNT_PER; i++)
   {
      worked&=PrimShape_CreateNGonCyl(&vcyl[i],i+3,FALSE);
      primals_list[PRIMAL_TYPE_CYLINDER*2+0][i]=&vcyl[i];
      worked&=PrimShape_CreateNGonCyl(&fcyl[i],i+3,TRUE);
      primals_list[PRIMAL_TYPE_CYLINDER*2+1][i]=&fcyl[i];

      worked&=PrimShape_CreateNGonPyr(&vpyr[i],i+3,FALSE,FALSE);
      primals_list[PRIMAL_TYPE_PYRAMID*2+0][i]=&vpyr[i];
      worked&=PrimShape_CreateNGonPyr(&fpyr[i],i+3,TRUE,FALSE);
      primals_list[PRIMAL_TYPE_PYRAMID*2+1][i]=&fpyr[i];

      worked&=PrimShape_CreateNGonPyr(&vcpyr[i],i+3,FALSE,TRUE);
      primals_list[PRIMAL_TYPE_CORNERPYR*2+0][i]=&vcpyr[i];
      worked&=PrimShape_CreateNGonPyr(&fcpyr[i],i+3,TRUE,TRUE);
      primals_list[PRIMAL_TYPE_CORNERPYR*2+1][i]=&fcpyr[i];
   }
   default_primal=cube;            // for now, to be safe
   cur_setup=TRUE;
   if (!worked)
      Warning(("Failed to correctly initialize all primal shapes\n"));
}

void primalBr_Term(void)
{
   int i;
   if (!cur_setup) { Warning(("Yo - PrimalTerm Again")); return; }   
   _primalBr_FreeMem(&cube);
   _primalBr_FreeMem(&line);
   _primalBr_FreeMem(&light);
   _primalBr_FreeMem(&wedge);
   _primalBr_FreeMem(&dodec);
   for (i=0; i<PRIMAL_CNT_PER; i++)
   {
      _primalBr_FreeMem(&vcyl[i]);
      _primalBr_FreeMem(&fcyl[i]);
      _primalBr_FreeMem(&vpyr[i]);
      _primalBr_FreeMem(&fpyr[i]);
      _primalBr_FreeMem(&vcpyr[i]);
      _primalBr_FreeMem(&fcpyr[i]);
      // ick - this is all super icky, argh
      primals_list[0][i]=primals_list[1][i]=
         primals_list[2][i]=primals_list[3][i]=
         primals_list[4][i]=primals_list[5][i]=
         primals_list[6][i]=primals_list[7][i]=NULL;
   }
   cur_setup=FALSE;   
}

/////////////////////
// actual accessors

// return the two brush points to use for this edge
int *primalBr_EdgePts(int primal, int edge_id)
{
   primalInfo *prim=getPrimInfo(primal);
   if (prim->edges>edge_id)
      return &prim->edge_list[edge_id*2];
   return NULL;
}

static int *primalBr_FaceData(primalInfo *prim, int *base_pt, int face_id, int *cnt)
{
   *cnt=0;
   if (prim->faces>face_id)
   {
      int i=0, *next_pt;
      base_pt+=face_id*prim->face_stride;
      next_pt=base_pt;
      while (((*cnt)<prim->face_stride)&&(*next_pt!=-1))
       {  next_pt++; (*cnt)++; }
      return base_pt;
   }
   return NULL;
}

// return the _cnt_ face points to use for this edge
int *primalBr_FacePts(int primal, int face_id, int *cnt)
{
   primalInfo *prim=getPrimInfo(primal);
   return primalBr_FaceData(prim,prim->face_pts_list,face_id,cnt);
}

int *primalBr_FaceEdges(int primal, int face_id, int *cnt)
{
   primalInfo *prim=getPrimInfo(primal);
   return primalBr_FaceData(prim,prim->face_edge_list,face_id,cnt);
}

// get the raw initial untransed point data for this primal brush point
mxs_vector *primalBr_GetRawPt(int primal, int pt_id)
{
   primalInfo *prim=getPrimInfo(primal);
   if (prim->points>pt_id)
      return &prim->pt_array[pt_id];
   return FALSE;
}

#ifdef DBG_ON
#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
void primalBr_mprint(int primal_id)
{
   primalInfo *prim=getPrimInfo(primal_id);
   int i, j, next_val;
   
   mprintf("primal %d> pts %d edges %d faces %d stride %d\n",primal_id,prim->points,prim->edges,prim->faces,prim->face_stride);
   for (i=0; i<prim->points; i++)
      mprintf("     point %d> %g %g %g\n",i,prim->pt_array[i].x,prim->pt_array[i].y,prim->pt_array[i].z);
   for (i=0; i<prim->edges; i++)
      mprintf("      edge %d> %d %d\n",i,prim->edge_list[i*2],prim->edge_list[i*2+1]);
   for (i=0; i<prim->faces; i++)
   {
      mprintf(" face edge %d> ",i);
      j=0;
      while ((next_val=prim->face_edge_list[prim->face_stride*i+(j++)])!=-1)
         mprintf("%d ",next_val);
      mprintf("\n");
   }

   for (i=0; i<prim->faces; i++)
   {
      mprintf("   face pt %d> ",i);
      j=0;
      while ((next_val=prim->face_pts_list[prim->face_stride*i+(j++)])!=-1)
         mprintf("%d ",next_val);
      mprintf("\n");
   }
}
#endif

///////////////////////
// wacky get sets of points in world or local space stuff

// this whole in/out two arrays thing is super ugly
// but on the other hand dont want secret statics in here which you have to remember to expire
// or mallocs you have to free, so, for now, oh well, this is it

//#define DO_POS_QUANTIZE
#ifdef DO_POS_QUANTIZE
#define QUANTIZE 32
static mxs_vector *size_quantize(mxs_vector *in, mxs_vector *out)
{
   out->x = ((float) (int) (in->x*QUANTIZE)) / QUANTIZE;
   out->y = ((float) (int) (in->y*QUANTIZE)) / QUANTIZE;
   out->z = ((float) (int) (in->z*QUANTIZE)) / QUANTIZE;
   return out;
}
#else
static mxs_vector *size_quantize(mxs_vector *in, mxs_vector *out)
{
   *out=*in;
   return out;
}
#endif

void primalRawPart(int primal_id, mxs_vector *scale, mxs_vector *pts, int *pt_list, int cnt)
{
   primalInfo *prim=getPrimInfo(primal_id);
#ifdef DO_POS_QUANTIZE
   mxs_vector sz_mem;  // this is ifdefed to avoid unrefed var warnings if you make size_quantize deal
   mxs_vector *sz=size_quantize(scale,&sz_mem);
#else
   mxs_vector *sz=scale;
#endif
   int i;

   for (i=0; i<cnt; i++)
      mx_elmul_vec(&pts[i],sz,&prim->pt_array[pt_list[i]]);
}

// build the entire brushes point list, basically
int primalRawFull(int primal_id, mxs_vector *scale, mxs_vector *pts)
{
   primalInfo *prim=getPrimInfo(primal_id);
#ifdef DO_POS_QUANTIZE
   mxs_vector sz_mem;
   mxs_vector *sz=size_quantize(scale,&sz_mem);
#else
   mxs_vector *sz=scale;
#endif
   int i;

   for (i=0; i<prim->points; i++)
      mx_elmul_vec(&pts[i],sz,&prim->pt_array[i]);
   return i;
}
  
// world position build
void primalQuantizePos(editBrush *br, mxs_vector *pos)
{
   size_quantize(&br->pos,pos);
}

#define DO_ANG_QUANTIZE

#ifdef DO_ANG_QUANTIZE
// ok, legal angles for creation are 15/64ths of a degree (ANG_QUANTIZE_VAL)
// thus, we solve n(15/64)=360*(x/65536), or n=3x/128
// so, the angle we want is ang = floor(3x/128)*(15/64)
mxds_vector *ang_quantize(mxs_angvec *ang_in, mxds_vector *ang_out)
{
   ang_out->x=floor(((3.0*(double)ang_in->tx)/128.0)+0.5)*(15.0/64.0)*(MX_REAL_PI/180.0);
   ang_out->y=floor(((3.0*(double)ang_in->ty)/128.0)+0.5)*(15.0/64.0)*(MX_REAL_PI/180.0);
   ang_out->z=floor(((3.0*(double)ang_in->tz)/128.0)+0.5)*(15.0/64.0)*(MX_REAL_PI/180.0);
   return ang_out;
}
#else  // DO_ANG_QUANTIZE
mxds_vector *ang_quantize(mxs_angvec *ang_in, mxds_vector *ang_out)
{
   ang_out->x=((double)ang_in->tx/65536.0)*MX_REAL_2PI;
   ang_out->y=((double)ang_in->ty/65536.0)*MX_REAL_2PI;
   ang_out->z=((double)ang_in->tz/65536.0)*MX_REAL_2PI;
   return ang_out;
}
#endif // DO_ANG_QUANTIZE

void primalQuantizeAng(mxs_angvec *ang_in, mxds_vector *ang_out)
{
   ang_quantize(ang_in,ang_out);
}

///////////////////////////////////////////
///////////////////////////////////////////

void primalStartTransGroup(void)
 { r3_start_frame(); }

void primalFinishTransGroup(void)
 { r3_end_frame(); }

// setup a transform for this brush
static void _primalSetupTrans(editBrush *br, mxs_vector *pos)
{
   r3_start_object_angles(pos,&br->ang,R3_DEFANG);
}

// transform the cnt pts_in vectors to pts_out, grid snap on brush has been disabled for now
static void _primalDoTrans(editBrush *br, int cnt, mxs_vector *pts_in, mxs_vector *pts_out)
{
   int i;
   for (i=0; i<cnt; i++)
   {
      r3_transform_o2w(&pts_out[i],&pts_in[i]);
      if (br->grid.grid_enabled)                // who knows what is the point of this
         gedit_vertex_snap(&br->grid,&pts_out[i],&pts_out[i]);
   }
   r3_end_object();
}

// do primal build of world coordinates for a brush list
static void _primalBuildWorldPos(editBrush *br, int cnt, mxs_vector *pts_in, mxs_vector *pts_out)
{
   mxs_vector pos;

   primalQuantizePos(br,&pos);
   _primalSetupTrans(br,&pos);
   _primalDoTrans(br,cnt,pts_in,pts_out);
}

void primalTransBrush(editBrush *br, mxs_vector *world_pts)
{

}

// call these for standalone build of a given face or point
//
// These 2 Worlds get all points for the brush or part in world coordinates
// ie. one calls and pts is filled with the correct world space points
//
// a single element edge list is built and transformed
void primalWorldPart(editBrush *br, mxs_vector *pts, int *pt_list, int cnt)
{
   mxs_vector *wrld_pts;

   wrld_pts=(mxs_vector *)Malloc(cnt*sizeof(mxs_vector));
   primalRawPart(br->primal_id,&br->sz,wrld_pts,pt_list,cnt);
   _primalBuildWorldPos(br,cnt,wrld_pts,pts);
   Free(wrld_pts);
}

void primalWorldFull(editBrush *br, mxs_vector *pts)
{
   primalInfo *prim=getPrimInfo(br->primal_id);
   mxs_vector *wrld_pts;
   int pt_cnt=prim->points;

   wrld_pts=(mxs_vector *)Malloc(pt_cnt*sizeof(mxs_vector));
   primalRawFull(br->primal_id,&br->sz,wrld_pts);
   _primalBuildWorldPos(br,pt_cnt,wrld_pts,pts);
   Free(wrld_pts);      
}
