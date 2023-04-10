/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendobj.cpp,v 1.5 2000/02/28 16:50:59 MAT Exp $

// Render an object
#include <objarray.h>

#include <math.h>
#include <stdlib.h>

extern "C" {
#include <r3d.h>
#include <g2pt.h>
#include <2d.h>
#include <lgd3d.h>
#include <md.h>
#include <md_.h>
#include <mds.h>
#include <mdutil.h>
#include <mm.h>
#include <mmd.h>
#include <qt.h>
}

#include <mprintf.h>

#include <mlight.h>

#include <globalid.h>
#include <wr.h>
#include <portal.h>
#include <objlight.h>

#include <editbr.h>
#include <editobj.h>
#include <objmodel.h>
#include <objspace.h>
#include <rendobj.h>
#include <robjbase.h>
#include <guistyle.h>
#include <objscale.h>
#include <objshape.h>
#include <osysbase.h>
#include <objedit.h>
#include <objpos.h>
#include <hdskprop.h>
#include <sprkprop.h>
#include <rendprop.h>
#include <aidebug.h>
#include <aiapi.h>     // ObjIsAI()
#include <refsys.h>
#include <meshatt.h>
#include <meshtex.h>
#include <corona.h>
#include <plycbllm.h>  // PlayerArm()
#include <weapbow.h>   // PlayerArrow()
#include <playrobj.h>
#include <roomsys.h>
#include <pgroup.h>
#include <scrnman.h>
#include <fas.h>
#include <fasprop.h>
#include <packflag.h>

#ifdef NEW_NETWORK_ENABLED
#include <appagg.h>    // for AppGetObj()
#include <iobjnet.h>
#endif

#include <creatext.h>
#include <propface.h>

// since there seems to be no font on the 3d canvas
#include <editor.h>
#include <res.h>

#include <texmem.h>
#include <config.h>
#include <objdef.h>

#include <mnumprop.h>
#include <rendphys.h>
#include <physapi.h>
#include <psndapi.h>
#include <ambient.h>

#ifdef DBG_ON
#include <cfgdbg.h>
#endif

#include <gfxprop.h>
#include <simtime.h>

extern "C" { 

// so we can draw cell ids
extern void AIPathFindDrawPathCellIds();
extern BOOL g_AIPathDBDrawCellIds;

extern BOOL g_lgd3d;
extern BOOL g_zbuffer;

extern BOOL portal_allow_object_splitting;
extern void portal_render_water_polys(int n);
extern void (*portal_post_render_cback)(void);
extern void (*portal_queue_water_poly)();
extern int mCellRefSys;
extern mxs_vector portal_camera_loc;
extern void set_mm_sort(bool);

#ifdef RGB_LIGHTING
#define RGB_MD()   (g_lgd3d)
#endif

// This should always be commented out on the backing tree, since it
// adds overhead.
//#define CTIMER_ON
#include <ctimer.h>

#include <dbmem.h>



// for now, this seems easiest
#ifdef PLAYTEST
#define NAME_SUPPORT
#define OBJ_VECTOR_TESTER
#endif

#ifdef OBJ_VECTOR_TESTER
mxs_vector rendobj_vector={1,0,0};
ObjID      rendobj_vector_obj=OBJ_NULL;
#endif

// publicly available what obj we last saw - 0 is none
int  rendobj_seen;

#ifdef PLAYTEST
// do we light objects
BOOL obj_lighting_on=TRUE;
BOOL g_ShowAmbients=FALSE;
#else
#define obj_lighting_on TRUE
#endif

#ifdef NAME_SUPPORT
// show names in objects
int     rendobj_name_color=0;
int     rendobj_name_types=kRendObjNameAll;
#ifdef NEW_NETWORK_ENABLED
// Object networking manager
static IObjectNetworking *g_pObjNet=NULL;
// show names, with color indicating network status
int     rendobj_local_name_color, rendobj_local_name_use_color;
int     rendobj_hosted_name_color, rendobj_hosted_name_use_color;
int     rendobj_proxy_name_color, rendobj_proxy_name_use_color;
BOOL    rendobj_net_name_coloring=FALSE;
#else
#define rendobj_net_name_coloring FALSE
#endif
#endif

#ifndef SHIP
BOOL always_slow_split=FALSE;
#endif

#ifndef SHIP
BOOL g_bShowPhysModels=FALSE;
BOOL g_bShowPhysBBox=FALSE;
BOOL g_bShowCreatureJoints=FALSE;
#endif

float g_obj_zbias = 4.0/65536.0; // 4 bits, in a 16 bit zbuffer

/////////////////////////////
// And now, for a moment of weirdness to help us track which objects
// were visisble last frame...

cPackedBoolSet g_RendObjVisible(1); 

BOOL rendobj_object_visible(ObjID obj)
{
   Assert_(obj < gMaxObjID); 
   return g_RendObjVisible.IsSet(obj); 
}

//
// Sink object to listen to gMaxObjID changes resize visible vector
//

class cRendObjIDSink: public cObjIDSink
{
   void OnObjIDSpaceResize(const sObjBounds& bounds)
   {
      g_RendObjVisible.Resize(bounds.max); 
   }
}; 


/////////////////////////////
// silly system for wedges

#define NO_PT (-1)

static r3s_point r3_pts[6];

static mxs_vector pts[] =
{
   {  3, 1  , -.5 }, {  3,   -1, -.5 },
   { -3, 1.5,  -1 }, { -3, -1.5,  -1 },
   { -3, 1.5,   1 }, { -3, -1.5,   1 }
};

static int polys[][4] =
{
   { 1,0,2,3 },
   { 0,1,5,4 },
   { 3,2,4,5 },
   { 2,0,4,NO_PT },
   { 1,3,5,NO_PT }
};


////////////////////////////////////
// translucent object support

int rendobj_type(ObjID obj_id);

static bool is_tluc(ObjID obj)
{
   float alpha;
   int type;
   if (PROPERTY_GET(gAlphaRenderProp, obj, &alpha))
      return TRUE;
   type = rendobj_type(obj);
   if (type == OM_TYPE_BITMAP)
      return TRUE;
   if (type == OM_TYPE_PARTICLES && ParticleGroupTranslucent(obj))
      return TRUE;
   return FALSE;
}

// if we're zbuffering, we draw all the objects
// at once, so we just need to set the alpha.
// if we're not zbuffering, we draw as normal,
// so we have to turn blending on and off around the object
static void rendobj_start_translucency(ObjID obj)
{
   if (g_lgd3d) {
      float alpha;
      if (PROPERTY_GET(gAlphaRenderProp, obj, &alpha)) {
         lgd3d_set_alpha(alpha);
       //zb  if (!g_zbuffer)
		 {
            lgd3d_set_blend(TRUE);
         }
      }
   }
}

static void rendobj_stop_translucency(ObjID obj)
{
   if (g_lgd3d /*zb&& !g_zbuffer*/) {
      float alpha;
      if (PROPERTY_GET(gAlphaRenderProp, obj, &alpha)) {
         lgd3d_set_blend(FALSE);
         lgd3d_set_alpha(1.0);
      }
   }
}

/////////////////////////////
// for now, this is just a global to set
float       rendobjScaleFac=1.0;
mxs_vector  rendobjUnitScale={1.0,1.0,1.0};
mxs_vector *rendobjScaleVec=&rendobjUnitScale;

static void render_pgon(mds_pgon *p, r3s_phandle *vlist, grs_bitmap *bm, ulong color, ulong type)
{
   int n;

   r3_set_clip_flags(R3_CLIP_UV | R3_CLIP_I);
   n = r3_clip_polygon(p->num, vlist, &vlist);

   if (n) {
      int i;

      if (bm==NULL) {
         // har har! 'portal' mapper can't do straight gouraud shading!

         static grs_bitmap *hack_bm=NULL;
         if (hack_bm==NULL)
            hack_bm = gr_alloc_bitmap(BMT_FLAT8, 0, 1, 1);

         hack_bm->bits[0] = (uchar )grd_gc.fcolor;
         for (i=0; i<n; i++)
            vlist[i]->grp.u = vlist[i]->grp.v = 0;

         g2pt_poly_affine(hack_bm, n, vlist, !!(type & MD_PGON_LIGHT_ON));
      } else {
         // rescale u,v values
         for (i=0; i < n; ++i) {
            vlist[i]->grp.u *= bm->w;
            vlist[i]->grp.v *= bm->h;
         }

         g2pt_poly_affine(bm, n, vlist, !!(type & MD_PGON_LIGHT_ON));

         for (i=0; i < n; ++i) {
            vlist[i]->grp.u /= bm->w;
            vlist[i]->grp.v /= bm->h;
         }
      }
   }
}


// We can't render 8888 textures in software, and bad things happen
// if we try.  So in these functions, we don't try.
#ifdef EDITOR
static void editor_safe_pgon_callback(mds_pgon *p, r3s_phandle *vlist,
                                      grs_bitmap *bm, ulong color, ulong type)
{
   int context = R3_PL_POLYGON;
   if (type & MD_PGON_LIGHT_ON)
      context |= R3_PL_GOURAUD;

   if (bm) {
      context |= R3_PL_TEXTURE;

      if (bm->flags & BMF_RGB_8888) {
         static grs_bitmap *hack_bm = NULL;
         if (hack_bm==NULL) {
            hack_bm = gr_alloc_bitmap(BMT_FLAT8, 0, 1, 1);
            hack_bm->bits[0] = (uchar) grd_gc.fcolor;
         }
         bm = hack_bm;
      }
   }
   r3_set_polygon_context(context);
   r3_set_texture(bm);
   r3_draw_poly(p->num, vlist);
}


// This is an even more terrible thing than the above.  Since it's
// patterned after md_render_pgon it could easily get out of synch
// with it.
static void editor_safe_render_pgon(mds_pgon *p)
{
   int i;
   int j;
   ulong flag=0;
   ulong type;
   float alpha=0;       // material translucency

   // Bail if it isn't facing you
   // use the cached normal dot prod
   if ((mdd.buff_norms[p->norm] + p->d) <= 0.0) return;


   // Set mdd_vlist to contain the list of verts
   for (i=0;i<p->num;++i) {
      mdd_vlist[i] = md_buff_point(p->verts[i]);
   }

   // Set the type, accounting for rendering mode,
   // ie, no lighting, or a particular prim type
   type = (p->type&mdd_type_and)|mdd_type_or;

   // lighting on
   if (type&MD_PGON_LIGHT_ON) {
      // Set lighting values
      if (!mdd_rgb_lighting) {
         flag |= R3_PL_GOURAUD;
         for (i=0,j=p->num;i<p->num;++i,++j) {
            mdd_vlist[i]->grp.i = mdd.buff_lights[p->verts[j]];
         }
      } else {
         flag |= R3_PL_RGB_GOURAUD;
         for (i=0,j=p->num;i<p->num;++i,++j) {
            int offset = p->verts[j] * 3;
            g2s_point *g2p = (g2s_point *) &mdd_vlist[i]->grp;
            g2p->i = mdd.buff_lights[offset];
            g2p->h = mdd.buff_lights[offset+1];
            g2p->d = mdd.buff_lights[offset+2];
         }
      }
   }


   // Mea culpa.  If only we'd thought of a per polygon
   // material field sooner...
   // Next modeler for sure!
   if (mdd.use_alpha) {
      int num_verts = p->num * ((type&MD_PGON_PRIM_MASK)==MD_PGON_PRIM_TMAP?3:2);
      mds_pgon_aux *a = (mds_pgon_aux *)(&p->verts[num_verts]);
      alpha = md_amat(mdd.model,a->mat)->trans;
      if (alpha>0) {
         lgd3d_set_alpha(alpha * mdd.alpha_scale);
         lgd3d_set_blend(TRUE);
      }
   }

   switch (type&MD_PGON_COLOR_MASK) {
      case MD_PGON_COLOR_PAL:
      {
         // punt stupid encoding
         r3_set_color(p->data);
         break;
      }
      case MD_PGON_COLOR_VCOL:
      {
         r3_set_color(mdd_vcolor_tab[p->data]);
         break;
      }
   }

   switch(type&MD_PGON_PRIM_MASK) {
      case MD_PGON_PRIM_SOLID:
      {
         r3_set_polygon_context(flag | R3_PL_POLYGON);
         r3_draw_poly(p->num,mdd_vlist);
         break;
      }
      case MD_PGON_PRIM_WIRE:
      {
         j = p->num-1;
         for (i=0;i<p->num;++i) {
            r3_draw_line(mdd_vlist[j],mdd_vlist[i]);
            j = i;
         }
         break;
      }
      case MD_PGON_PRIM_TMAP:
      {
         ///////////////////////////////////////////////
         // @NOTE: This is the start of the code that's different from
         // what's in md_render_pgon.

         // Set the requested texture map and context--or not.
         grs_bitmap *bm = mdd_vtext_tab[p->data];

         for (i=0,j=2*p->num;i<p->num;++j,++i) {
            mdd_vlist[i]->grp.u = mdd.uvs[p->verts[j]].u;
            mdd_vlist[i]->grp.v = mdd.uvs[p->verts[j]].v;
         }

         if (!g_lgd3d && (bm->flags & BMF_RGB_8888)) {
            static grs_bitmap *hack_bm = NULL;
            if (hack_bm==NULL) {
               hack_bm = gr_alloc_bitmap(BMT_FLAT8, 0, 1, 1);
               hack_bm->bits[0] = (uchar) grd_gc.fcolor;
            }
            bm = hack_bm;
         }
         r3_set_texture(bm);
         // @NOTE: This is the end of the bit that's different from
         // the code in md_render_pgon.
         ///////////////////////////////////////////////

         r3_set_polygon_context(flag | R3_PL_POLYGON | mdd_tmap_mode);
         r3_draw_poly(p->num,mdd_vlist);
         break;
      }
   }

   if (alpha>0)
   {
       lgd3d_set_blend(FALSE);
       lgd3d_set_alpha( 1.0 );
   }
}
#endif // EDITOR


#define SECRET_WEDGE_FACTOR 0.5           // antilles, that is

#ifndef SHIP
void render_wedge(uchar color, uchar *clut)
{
   int i,j;
   r3s_phandle vp[4];
   mxs_vector scaled_pts[6];

   r3_set_clipmode(R3_CLIP);
   r3_start_block();
   r3_set_polygon_context(R3_PL_CHECK_FACING);
   for (i=0; i<6; i++)
      for (j=0; j<3; j++)
         scaled_pts[i].el[j]=rendobjScaleVec->el[j]*pts[i].el[j]*SECRET_WEDGE_FACTOR;
   r3_transform_block(6, r3_pts, scaled_pts);

   for (j=0; j < 5; ++j)
   {
      for (i=0; i < 4; ++i)
         if (polys[j][i]!=NO_PT)
            vp[i] = &r3_pts[polys[j][i]];
      if (clut)
         r3_set_color(clut[(uchar) (color+j)]);
      else
         r3_set_color(color+j);
      r3_draw_poly(polys[j][3] == NO_PT ? 3 : 4, vp);
   }
   r3_end_block();
}
#else
#define render_wedge(color,clut)
#endif

////////////////////////////////////
// shared functions

#ifdef NAME_SUPPORT
// show floating names
static void show_name(ObjID obj, int color)
{
   mxs_vector *pos=ObjPosGetLocVector(obj);
   char buf[64];
   r3s_point pt;

   if (pos==NULL) return;
   guiStyleSetupFont(NULL,StyleFontNormal);
   r3_start_block();
   r3_transform_point(&pt,pos);
   r3_end_block();
   gr_set_fcolor(color);
   gr_string((char *)ObjEditName(obj),fix_int(pt.grp.sx),fix_int(pt.grp.sy));
   sprintf(buf,"%d %d %d",(int)pos->el[0],(int)pos->el[1],(int)pos->el[2]);
   gr_string(buf,fix_int(pt.grp.sx),fix_int(pt.grp.sy)+10);
   guiStyleCleanupFont(NULL,StyleFontNormal);
}

static void pick_color_and_show_name(ObjID obj)
{
   int color=rendobj_name_color;  // default to old way

   // first, the type filter
   if (rendobj_name_types!=kRendObjNameAll)
   {
      BOOL draw=FALSE;
      if (rendobj_name_types&kRendObjNameAI)
         draw|=ObjIsAI(obj);
      if (rendobj_name_types&kRendObjNameAnyPhys)
         draw|=PhysObjHasPhysics(obj);
      if (rendobj_name_types&kRendObjNameActPhys)
         draw|=(PhysObjHasPhysics(obj)&&(!PhysIsSleeping(obj)));
      if (!draw)
         return;  // dont draw, didnt pass the filter
   }

   // now, choose the color
#ifdef NEW_NETWORK_ENABLED
   if (rendobj_net_name_coloring)
   {  // Show the object's network status as the name's color
      if (IObjNet_ObjHostedHere(g_pObjNet, obj))
         color=rendobj_hosted_name_use_color;
      else if (IObjNet_ObjIsProxy(g_pObjNet, obj))
         color=rendobj_proxy_name_use_color;
      else
         color=rendobj_local_name_use_color;
   }
#endif

   // now draw
   show_name(obj,color);
}
#endif

#ifndef SHIP

#define MAX_REND_PHYS_SUBMODELS 10
#define MAX_NUM_CONSTRAINTS     12

void rendobj_draw_phys_bbox(ObjID obj)
{
   mxs_vector pos;
   mxs_angvec fac;
   mxs_vector bbox;

   gr_set_fcolor(253);
   if (RendPhysGetPhysBBox(obj,&pos,&fac,&bbox))
   {
      mxs_vector bmin, bmax;

      mx_scale_vec(&bmin, &bbox, -0.5);
      mx_scale_vec(&bmax, &bbox,  0.5);

      draw_bbox(&pos,&fac,&bmin,&bmax);
   }
}

static void rendobj_draw_phys_models(ObjID obj)
{
   mxs_vector pos[MAX_REND_PHYS_SUBMODELS];
   mxs_real   size[MAX_REND_PHYS_SUBMODELS];
   r3s_point  pts[MAX_REND_PHYS_SUBMODELS];
   mxs_angvec fac;
   mxs_vector bbox;
   mxs_vector position;
   mxs_vector velocity, forces;
   mxs_vector constraints[MAX_NUM_CONSTRAINTS];
   int num_constraints = MAX_NUM_CONSTRAINTS;
   int numModels = MAX_REND_PHYS_SUBMODELS;
   int i,ocol;
   int s;

   ocol=gr_get_fcolor();

   // Models
   gr_set_fcolor(254);
   if(RendPhysGetSphereModelInfo(obj,pos,size,&numModels))
   {
      r3_set_clipmode(R3_CLIP);
      r3_set_clip_flags(0);

      r3_start_block();
      r3_transform_block(numModels,pts,pos);
      for(i=0;i<numModels;i++)
      {
         fix rad;

         if(pts[i].ccodes)
            continue;
         if(size[i]==0) // it a point or non-spherical
            size[i]=0.1; // make it visible
         s=(int)r3_get_hsize(pts[i].p.z,size[i]);
         rad=fix_div(s,grd_bm.w);
         g2_circle(pts[i].grp.sx,pts[i].grp.sy,rad);
      }
      r3_end_block();
   }
   else
   if(RendPhysGetOBBModelInfo(obj,pos,&fac,&bbox))
   {
      mxs_vector bmin, bmax;

      mx_scale_vec(&bmin, &bbox, -0.5);
      mx_scale_vec(&bmax, &bbox,  0.5);

      draw_bbox(pos,&fac,&bmin,&bmax);
   }

   r3_start_block();
   // Vectors
   //   if (!g_lgd3d && RendPhysGetVectors(obj, &position, &velocity, &forces, constraints, &num_constraints))
   if (RendPhysGetVectors(obj, &position, &velocity, &forces, constraints, &num_constraints))
   {
      r3s_point from, to_vel, to_force, to;

      mx_addeq_vec(&velocity, &position);
      if (mx_mag2_vec(&forces) > 0)
         mx_normeq_vec(&forces);
      mx_scaleeq_vec(&forces, 2);
      mx_addeq_vec(&forces, &position);

      r3_set_clipmode(R3_CLIP);
      r3_set_clip_flags(0);

      r3_transform_point(&from, &position);
      r3_transform_point(&to_vel, &velocity);
      r3_transform_point(&to_force, &forces);

      // velocity
      gr_set_fcolor(253);
      r3_draw_line(&from, &to_vel);

      // forces
      gr_set_fcolor(45);
      r3_draw_line(&from, &to_force);

      // constraints
      gr_set_fcolor(241);
      for (i=0; i<num_constraints; i++)
      {
         mx_addeq_vec(&constraints[i], &position);

         r3_set_clipmode(R3_CLIP);
         r3_set_clip_flags(0);

         r3_transform_point(&to, &constraints[i]);

         r3_draw_line(&from, &to);
      }
   }
   r3_end_block();
   gr_set_fcolor(ocol);
}

#define MAX_CREATURE_REND_JOINTS 32

void rendobj_draw_creature_joints(ObjID obj)
{
   mxs_vector pos[MAX_CREATURE_REND_JOINTS];
   r3s_point  pts[MAX_CREATURE_REND_JOINTS];
   int num_joints;
   int old_color;
   int i, s;

   old_color = gr_get_fcolor();
   gr_set_fcolor(127);

   num_joints = MAX_CREATURE_REND_JOINTS;

   if(RendCreatureGetJointInfo(obj, pos, &num_joints))
   {
      r3_set_clipmode(R3_CLIP);
      r3_set_clip_flags(0);

      r3_start_block();
      r3_transform_block(num_joints, pts, pos);
      for(i=0; i<num_joints; i++)
      {
         fix rad;

         if(pts[i].ccodes)
            continue;

         s=(int)r3_get_hsize(pts[i].p.z,0.1);
         rad=fix_div(s,grd_bm.w);

         g2_circle(pts[i].grp.sx,pts[i].grp.sy,rad);
      }
      r3_end_block();
   }

   gr_set_fcolor(old_color);
}

#endif


// this is really slow to compute, so it should be done only once per frame!
static r3s_point box_pts[8];
static r3s_phandle box_vlist[6][4] =
{
   { box_pts+0, box_pts+1, box_pts+3, box_pts+2 },
   { box_pts+0, box_pts+1, box_pts+5, box_pts+4 },
   { box_pts+0, box_pts+2, box_pts+6, box_pts+4 },
   { box_pts+1, box_pts+3, box_pts+7, box_pts+5 },
   { box_pts+2, box_pts+3, box_pts+7, box_pts+6 },
   { box_pts+4, box_pts+5, box_pts+7, box_pts+6 }
};

void draw_bbox(mxs_vector *loc, mxs_angvec *fac,
       mxs_vector *bmin, mxs_vector *bmax)
{
   mxs_vector vec[8];
   int i,j,n,k;
   r3s_phandle *vl;

   if (loc && fac)
      r3_start_object_angles(loc, fac, R3_DEFANG);

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(0);

   vec[0].x = vec[2].x = vec[4].x = vec[6].x = bmin->x;
   vec[1].x = vec[3].x = vec[5].x = vec[7].x = bmax->x;

   vec[0].y = vec[1].y = vec[4].y = vec[5].y = bmin->y;
   vec[2].y = vec[3].y = vec[6].y = vec[7].y = bmax->y;

   vec[0].z = vec[1].z = vec[2].z = vec[3].z = bmin->z;
   vec[4].z = vec[5].z = vec[6].z = vec[7].z = bmax->z;

   r3_transform_block(8, box_pts, vec);

   for (i=0; i < 6; ++i) {
      n = r3_clip_polygon(4, box_vlist[i], &vl);
      k = n-1;
      for (j=0; j < n; ++j) {
         r3_draw_line(vl[j], vl[k]);
         k = j;
      }
   }

   r3_end_block();

   if (loc && fac)
      r3_end_object();
}

static r3s_point line_pts[2];
static r3s_phandle line_vlist[2] = 
{
   line_pts+0, 
   line_pts+1,
};

void draw_line(mxs_vector *start, mxs_vector *end)
{
   r3s_phandle *vl;
   mxs_vector vec[2];

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(0);

   vec[0] = *start;
   vec[1] = *end;

   r3_transform_block(2, line_pts, vec);
   
   r3_clip_line(line_vlist, &vl);

   r3_draw_line(vl[0], vl[1]);
   
   r3_end_block();
}

void draw_poly(int n, mxs_vector *vert)
{
   int i;
   r3s_point results[64];
   r3s_phandle points[64];
   if (n > 64) return;

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   r3_set_clip_flags(0);
   r3_transform_block(n, results, vert);
   for (i=0; i < n; ++i) points[i] = &results[i];
   r3_draw_poly(n, points);
   r3_end_block();
}


void draw_world_bbox(ObjID obj)
{
   mxs_vector bmin, bmax;

   ObjGetWorldBBox(obj, &bmin, &bmax);
   r3_set_color(6);
   draw_bbox(0, 0, &bmin, &bmax);
}

void draw_obj_bbox(ObjID obj)
{
   mxs_vector bmin, bmax;
   Position *pos = ObjPosGet(obj);
   int boxType=ObjGetFavoriteBBox(obj,&bmin,&bmax);
   if(boxType==OBJ_BBOX_TYPE_NONE)
      return;
   r3_set_color(6);
   if(boxType!=OBJ_BBOX_TYPE_WORLD)
      draw_bbox(&pos->loc.vec, &pos->fac, &bmin, &bmax);
   else
      draw_bbox(0,0, &bmin, &bmax);
}

enum bounds
{
   BOUNDING_RECTANGLE,
   BOUNDING_OCTAGON
};

// move to objshape.c ?
// XXX NOTE: this does not work for scaled models.  Do we care?
// it is because objrel bboxes do not include scale factors.
// how expensive is checking for scale property?
void GetObjScreenBounds(ObjID obj, fix *mn, fix *mx, enum bounds bound)
{
   mxs_vector vec[8], bmin, bmax;
   int i,j,n,boxType;
   r3s_phandle *vl;
   Position *pos = ObjPosGet(obj);
   BOOL startObj;
   int saveflags;

   boxType=ObjGetFavoriteBBox(obj,&bmin,&bmax);
   if(boxType==OBJ_BBOX_TYPE_NONE)
   {
      *mx=*mn=0;
      return;
   }
   if(TRUE==(startObj=(boxType!=OBJ_BBOX_TYPE_WORLD)))
      r3_start_object_angles(&pos->loc.vec, &pos->fac, R3_DEFANG);

   r3_start_block();
   r3_set_clipmode(R3_CLIP);
   saveflags = r3d_state.cur_con->clip.clip_flags;
   r3_set_clip_flags(0);

   // note that if we used delta transformations, we could do this faster!

   vec[0].x = vec[2].x = vec[4].x = vec[6].x = bmin.x;
   vec[1].x = vec[3].x = vec[5].x = vec[7].x = bmax.x;

   vec[0].y = vec[1].y = vec[4].y = vec[5].y = bmin.y;
   vec[2].y = vec[3].y = vec[6].y = vec[7].y = bmax.y;

   vec[0].z = vec[1].z = vec[2].z = vec[3].z = bmin.z;
   vec[4].z = vec[5].z = vec[6].z = vec[7].z = bmax.z;

   r3_transform_block(8, box_pts, vec);

   mn[0] = mn[1] = 0x7fffffff;
   mx[0] = mx[1] = 0x80000000;
   if (bound == BOUNDING_OCTAGON) {
      mn[2] = mn[3] = 0x7fffffff;
      mx[2] = mx[3] = 0x80000000;
   }

   for (i=0; i < 6; ++i) {
      n = r3_clip_polygon(4, box_vlist[i], &vl);
      for (j=0; j < n; ++j) {
         grs_point *p = &vl[j]->grp;
         if (p->sx < mn[0]) mn[0] = p->sx;
         if (p->sx > mx[0]) mx[0] = p->sx;
         if (p->sy < mn[1]) mn[1] = p->sy;
         if (p->sy > mx[1]) mx[1] = p->sy;
         if (bound == BOUNDING_OCTAGON) {
            fix xy,yx;
            xy = p->sx + p->sy;
            yx = p->sx - p->sy;
            if (xy < mn[2]) mn[2] = xy;
            if (xy > mx[2]) mx[2] = xy;
            if (yx < mn[3]) mn[3] = yx;
            if (yx > mx[3]) mx[3] = yx;
         }
      }
   }

   r3_end_block();

   if(startObj)
      r3_end_object();

   r3_set_clip_flags(saveflags);
}

struct obj_bounds
{
   mxs_vector bmin, bmax;  // 24
   fix mn[4], mx[4];        // 32
};

static cObjArray<obj_bounds*,kOAF_NoAbstracts|kOAF_Static> obj_cache;

 EXTERN ParticleGroup *ObjGetParticleGroup(ObjID obj);
#include<parttype.h>

// callback from renderer
void portal_obj_bounds(ObjID obj, mxs_vector **mn, mxs_vector **mx,
                       mxs_vector **center, float *radius, fix *mn2d, fix *mx2d)
{
   static mxs_vector bmin, bmax;
   struct obj_bounds *o;

   o = obj_cache[obj];

#if 0 //the good old, well thought, correct, kosher way
   if (!o) {
      o = obj_cache[obj] = new obj_bounds;
      ObjGetWorldBBox(obj, &o->bmin, &o->bmax);
      GetObjScreenBounds(obj, o->mn, o->mx,
         BOUNDING_OCTAGON);
   }

#else //a hack from The Cold Place


   if( !o )
   {
      o = obj_cache[obj] = new obj_bounds;

      if( rendobj_type(obj) == OM_TYPE_PARTICLES )
      {  // we are counting on the fact that this is the ACTUAL property data
         // we are too cool to ObjSet and ObjGet it again
         // because we dont want to let the listener do extra work
         // we should go make sure that ObjGetPGroup returns the real thing
         // @HACK: get rid of this ASAP, fix the internal bbox generator to do the right thing
         ParticleGroup     *pg = ObjGetParticleGroup(obj);
         float             org_scale;
         bool              b_spin_group;

         // scale up the bounding box
         if( pg )
         {
            //remember...
            b_spin_group = pg->spin_group;
            org_scale    = pg->cur_scale;

            if( b_spin_group )
            {
               pg->cur_scale *= 2.0f;
            }
            else
            {
               pg->spin_group = 1;
               pg->cur_scale = 2.0f;
            }
         }

         ObjGetWorldBBox(obj, &o->bmin, &o->bmax);
         GetObjScreenBounds(obj, o->mn, o->mx, BOUNDING_OCTAGON );

         // restore the size of the bounding box
         if( pg )
         {
            pg->spin_group = b_spin_group;
            pg->cur_scale  = org_scale;
         }
      }
      else //plain simple way
      {
         ObjGetWorldBBox(obj, &o->bmin, &o->bmax);
         GetObjScreenBounds(obj, o->mn, o->mx, BOUNDING_OCTAGON );
      }
   }

#endif //ouch!!!

   *center = NULL;
   *radius = 0;

   *mn = &o->bmin;
   *mx = &o->bmax;
   memcpy(mn2d, o->mn, sizeof(o->mn));
   memcpy(mx2d, o->mx, sizeof(o->mx));
}

void cam_push_clip_planes(ObjID obj, BOOL md_model)
{
   mxs_vector bmin,bmax, *sphere=NULL;
   float radius=0;

   ObjGetWorldBBox(obj, &bmin, &bmax);

   // this should just be GetObjSphere
   if (md_model) {
      int idx;
      mds_model *m;
      ObjGetModelNumber(obj, &idx);
      m = (mds_model *)objmodelGetModel(idx);
      if (m) {
         ObjPos* pos = ObjPosGetUnsafe(obj);
         sphere = &pos->loc.vec;
         radius = m->radius;
      }
   }

   portal_push_clip_planes(&bmin, &bmax, sphere, radius);
}

void cam_pop_clip_planes(void)
{
   portal_pop_clip_planes();
}

//////////////////////////////////////////////////////////
//
// arbitary (post-clipping) polygon renderer:
//
//  this data structure lets us store any polygon, not
//  just one directly out of an md or mesh model

typedef struct
{
   grs_bitmap *tex;
   int n, type;
   r3s_point ptlist[1];
} clipped_poly;

void draw_clipped_poly(clipped_poly *hp, int tmap_mode)
{
   r3s_phandle vlist[64];
   int mode, i;

   for (i=0; i < hp->n; ++i)
      vlist[i] = &hp->ptlist[i];

   if (hp->type&MD_PGON_LIGHT_ON)
      mode = R3_PL_POLYGON | R3_PL_GOURAUD;
   else
      mode = R3_PL_POLYGON;

   if (hp->tex) {
      r3_set_polygon_context(mode | tmap_mode);
      r3_set_texture(hp->tex);
   } else
      r3_set_polygon_context(mode);

   r3_draw_poly(hp->n, vlist);
}

///////////////////////////////////////////////////////////////////
//
//   HACKED CACHE polygon queueing system
//
// This caches the model from a particular viewpoint, etc; it lets
// us determine our potential performance assuming basically no 3d
// overhead

//#define HACK_CACHE

#ifdef HACK_CACHE

enum {
   HACK_NONE,
   HACK_RECORD,
   HACK_PLAYBACK
} hacked_cache;

int hack_pgon_count;
int hack_model_count;

#define MAX_HACK_POLYS   8192    // polys after splitting
#define MAX_HACK_MODELS  1024    // model fragments

int hack_model_start[MAX_HACK_MODELS+1];
clipped_poly *hack_pgon[MAX_HACK_POLYS];

static void hacked_record_pgon(mds_pgon *p, r3s_phandle *vlist,
    grs_bitmap *bm, ulong color, ulong type)
{
   int n = p->num, i;
   r3_set_clip_flags(R3_CLIP_UV | R3_CLIP_I);
   n = r3_clip_polygon(n, vlist, &vlist);
   if (n) {
      clipped_poly *hp;
      r3e_clipmode temp_mode=r3_get_clipmode();
      hp = (clipped_poly *)Malloc(sizeof(*hp) + (n-1)*sizeof(r3s_point));
      hp->tex = bm;
      hp->type = type;
      hp->n = n;
      for (i=0; i < n; ++i)
         hp->ptlist[i] = *vlist[i];
      Assert_(hack_pgon_count < MAX_HACK_POLYS);
      hack_pgon[hack_pgon_count++] = hp;
   }
}

void playback_from_hacked_cache(int tmap_mode)
{
   int n,k;
   k = hack_model_start[hack_model_count++];
   n = hack_model_start[hack_model_count];
   r3_set_clipmode(R3_NO_CLIP);
   r3_start_block();
   for(; k < n; ++k)
      draw_clipped_poly(hack_pgon[k], tmap_mode);
   r3_end_block();
}
#endif

///////////////////////////////////////////////////////////////////
//
//   polygon queueing system

extern ulong mdd_tmap_mode;

typedef struct // typedefed in robjbase.h
{
   ulong tmap_mode;
   void *model;
   void *buf;
   int first_pgon, num_pgon;
   int idx;
   ObjID obj_id;
   uchar clipmode;
   uchar free_model;
} queued_model_info;

// #define QUEUE_SIZE  4096
// Ack, let's give Terri's complex trees a little more breathing space.
#define OLD_QUEUE_SIZE 4096
#define QUEUE_SIZE  12288

#define MAX_MODELS  256

static void *pgon_queue[QUEUE_SIZE];
static int num_pgon_in_queue;
static queued_model_info queued_models[MAX_MODELS];
static int num_models_in_queue;


static void render_queue(ObjID id, ulong fragment, uchar *clut,
                         void (*qrendFunc)(void *, uchar *))
{
   // search for a matching model
   int i;
   for (i=0; i < num_models_in_queue; ++i)
      if (queued_models[i].obj_id == id) {
         if (queued_models[i].num_pgon)
            qrendFunc(&queued_models[i], clut);
         return;
      }
   Warning(("render_queue: object %d not found in queue.\n", id));
}


////////////////////////////////////
// model type default functions


static BOOL render_default(int idx, ObjID obj, int fragment,
                           int color, uchar *clut)
{
   BOOL rv = TRUE;

   if (color == 0)
      color = 3 + (idx * 3);

   // @BUG: Since color is hard-coded as 0 in the caller, we never
   // have these invisible wedges.  Well, not that we can see.
   if (color != -1)
      render_wedge((uchar)color, clut);
   else
      rv = FALSE;

   return rv;
}

static void setup_default(ObjID obj_id)
{
   Position *p = ObjPosGet(obj_id);
   r3_start_object_angles(&p->loc.vec, &p->fac, R3_DEFANG);
}

// also holds the code for naming objects on screen, which should get moved
static void finish_default(ObjID obj)
{
   r3_end_object();
}

static void enter_default(ObjID obj)
{
   return;
}

static void leave_default(ObjID obj)
{
   return;
}


////////////////////////////////////
// model type md functions

// we should really change this so it's not so cliphappy, as it were
#define loc_render_model md_fancy_cliphappy_render_model
BOOL    portal_model;

// @TODO: OPTIMIZE
// we should just plain know this by fiat - this wastes time
// i mean, it is per model, not per frame, that we need to figure this out

static mds_parm zero_parms[8];
static mds_parm use_parms[8];

mds_parm *getRendParms(ObjID obj, mds_model *m)
{
   float *parms=ObjJointPos(obj);
   mds_subobj *sub_list=md_subobj_list(m);
   if (parms)
   {
      int i, parm_ang=0, mask, max=0;
      for (i=0; i<m->subobjs; i++)
      {  // find out which parms are angle - this is idiotic
         mds_subobj *ms=&sub_list[i];
         if (ms->type==MD_SUB_ROT)
            parm_ang|=(1<<ms->parm);
         if (ms->parm>=max)
            max=ms->parm+1;
      }              // i totally dont understand what m->parms is about?
      for (i=0, mask=1; i<max; mask<<=1, i++)  // angle convert, when we know how
         if (parm_ang&mask)
            use_parms[i].ang=degrees_to_fixang(parms[i]);
         else
            use_parms[i].slide=parms[i];
      return use_parms;
   }
   else
      return zero_parms;
}

static void start_md_stride(void)
{
#ifdef RGB_LIGHTING
   if (RGB_MD()) {
      r3d_glob.cur_stride = sizeof(r3s_point) + 2*sizeof(float);
      mdd_rgb_lighting = TRUE;
      mld_multi_rgb  = TRUE;
      return;
   }
   mld_multi_rgb  = FALSE;
#endif
   mdd_rgb_lighting = FALSE;
   r3d_glob.cur_stride = sizeof(r3s_point);
}

static void end_md_stride(void)
{
   r3d_glob.cur_stride = sizeof(r3s_point);
   mld_multi_rgb = FALSE;
}

static char new_clut[256];
static void set_lit_clut(uchar *clut, ObjID o)
{
   // use arbitrary scale factor to compensate for lack of normal-based
   float brightness = compute_object_lighting(o) * 0.7;
   int i;
   uchar *light;

   if (brightness >= .99) brightness = 0.99;
   light = &grd_light_table[((int) (brightness*16)) << 8];

   for (i=0; i < 256; ++i)
      new_clut[i] = clut[light[i]];

   gr_set_fill_parm(new_clut);
}

static void start_md_clut(uchar *clut, ObjID o)
{
   md_set_render_light(FALSE);
   gr_set_fill_type(FILL_CLUT);
   set_lit_clut(clut, o);
}

static void end_md_clut()
{
   md_set_render_light(TRUE);
   gr_set_fill_type(FILL_NORM);
}

static void start_mm_clut(uchar *clut, ObjID o)
{
   gr_set_fill_type(FILL_CLUT);
   set_lit_clut(clut, o);
}

static void end_mm_clut()
{
   gr_set_fill_type(FILL_NORM);
}

static void start_generic_clut(uchar *clut)
{
   gr_set_fill_type(FILL_CLUT);
   gr_set_fill_parm(clut);
}

static void end_generic_clut()
{
   gr_set_fill_type(FILL_NORM);
}

// do model, if not there, do a wedge
static BOOL render_md(int idx, ObjID obj, int fragment, int color, uchar *clut)
{
   mds_model *m;
   mdf_pgon_cback old;
   BOOL rv=TRUE;

   int      nZBias;
   BOOL     bZBias;
  // BOOL     bBumpIt;

   CTIMER_START(kCTimerRendMD);

   start_md_stride();

   md_render_back_to_front(!g2pt_span_clip);

   if (portal_model)
      old = md_set_render_pgon_callback(render_pgon);
#ifdef EDITOR
   else if (!g_lgd3d)
      old = md_set_render_pgon_callback(editor_safe_pgon_callback);
#endif // EDITOR

   if (color==0) color=3+(idx*3);

   md_set_render_light(obj_lighting_on);

   if ((m=(mds_model *)objmodelGetModel(idx))!=NULL)
   {
      mds_parm *parms=NULL;         // this should all be in a function call!
      if (m->parms)
         parms=getRendParms(obj,m);
      md_mat_colors_set(m);
      objmodelSetupMdTextures(obj,m,idx);

      if (clut) start_md_clut(clut, obj);

      // Set Self illumination
      if (md_has_illum(m)) {
         PROPERTY_GET(g_pSelfIlluminationProp,obj,&mld_illum_scale);
      }

      nZBias = 0;
      bZBias = ObjGetZBiasProp( obj, &nZBias );

      if( bZBias )
          lgd3d_push_zbias_i( nZBias );

    //  bBumpIt = ObjGetBumpProp( obj );

      //if( bBumpIt )
      //    do something smart with it

      if (rendobjScaleVec==&rendobjUnitScale)
         loc_render_model(m,parms);
      else
      {
          mds_model *new_md=md_scale_model(NULL,m,rendobjScaleVec,TRUE);
          if (new_md!=NULL)
          {  // NEED TO ADD CACHING CODE/CONTROL HERE
              loc_render_model(new_md,parms);
              if (new_md!=m)
                  Free(new_md);
          }
      }

      if( bZBias )
          lgd3d_pop_zbias();

      if (clut) end_md_clut();

   } else if (color!=-1)
      render_wedge((uchar)color,clut);
   else
      rv=FALSE;

   if (portal_model)
      md_set_pgon_callback(old);
#ifdef EDITOR
   else if (!g_lgd3d)
      md_set_pgon_callback(old);
#endif // EDITOR

   if (g2pt_span_clip)
      md_render_back_to_front(TRUE);

   // Reset state for next guy
   mld_illum_scale = 1.0;

   end_md_stride();

   CTIMER_RETVAL(kCTimerRendMD, rv);
}

static BOOL poly_clip_codes_and(mds_pgon *poly)
{
   ushort *vlist = poly->verts;
   ulong cc_and;
   int n=poly->num;

   switch(n) {
      case 3:
         return md_buff_point(vlist[0])->ccodes
              & md_buff_point(vlist[1])->ccodes
              & md_buff_point(vlist[2])->ccodes;
      case 4:
         return md_buff_point(vlist[0])->ccodes
              & md_buff_point(vlist[1])->ccodes
              & md_buff_point(vlist[2])->ccodes
              & md_buff_point(vlist[3])->ccodes;
      case 5:
         return md_buff_point(vlist[0])->ccodes
              & md_buff_point(vlist[1])->ccodes
              & md_buff_point(vlist[2])->ccodes
              & md_buff_point(vlist[3])->ccodes
              & md_buff_point(vlist[4])->ccodes;
   }

   cc_and = 0xffffffff;
   while (n--)
      cc_and &= md_buff_point(*vlist++)->ccodes;

   return cc_and;
}

static void record_pgon(mds_pgon *p)
{
   if ((num_pgon_in_queue >= OLD_QUEUE_SIZE) && !((num_pgon_in_queue-OLD_QUEUE_SIZE)%500))
      Warning(("Wow. There are a lot of object polys in this scene (%d)\n", num_pgon_in_queue));

   if (num_pgon_in_queue >= QUEUE_SIZE)
      Error(1, "Too many polygons in queue.\n");

   pgon_queue[num_pgon_in_queue++] = (void *)p;
}

EXTERN md_render_pgon_render_callback(mds_pgon *p);

static void queue_render_md(void *q, uchar *clut)
{
   int i, count;
   queued_model_info *qm;
   mds_model *m;
   r3s_point *buf;
   mds_pgon **poly;

   CTIMER_START(kCTimerRendMD);

   start_md_stride();

   qm=(queued_model_info *)q;
   m = (mds_model *)qm->model;
   buf = (r3s_point *) qm->buf;

   md_mat_colors_set(m);
   objmodelSetupMdTextures(qm->obj_id,m,qm->idx);

#ifdef HACK_CACHE
   mdf_pgon_cback old;

   if (hacked_cache == HACK_RECORD) {
      old = md_set_render_pgon_callback(hacked_record_pgon);
      Assert_(hack_model_count < MAX_HACK_MODELS);
      hack_model_start[hack_model_count++] = hack_pgon_count;
   } else if (hacked_cache == HACK_PLAYBACK) {
      playback_from_hacked_cache(qm->tmap_mode);
      CTIMER_RETURN(kCTimerRendMD);
   }
#endif

   if (!m)
      Error(1, "render_queue_raw: Bad model pointer\n");

   // setup pointers to use this prepared model
   cam_push_clip_planes(qm->obj_id, TRUE);

#define TRY_OPTIMAL_CLIPPING

#ifdef TRY_OPTIMAL_CLIPPING
   if (qm->clipmode == R3_NO_CLIP) {
      r3_set_clipmode(R3_USER_CLIP);
      for (i=0; i < m->verts; ++i)
         buf[i].ccodes = 0;
   } else
#endif // OPT_CLIPPING - if we just want to be safe, always clip
      r3_set_clipmode(R3_CLIP);

   mdd_tmap_mode = qm->tmap_mode;

   if (clut) start_md_clut(clut, qm->obj_id);

#ifndef SHIP
   if (always_slow_split) {
      r3_start_block();

      r3_std_code_points(m->verts, buf);

      md_set_buff(m, qm->buf);
      md_render_only(m,NULL);

      r3_end_block();
   } else
#endif
   {

      r3_start_block();

#ifdef HACK_CACHE
         if (hacked_cache != HACK_PLAYBACK)
#endif
            r3_std_code_points(m->verts, buf);

         md_set_buff(m, qm->buf);
         md_set_globals(m);

         poly = (mds_pgon **)&pgon_queue[qm->first_pgon];
         count = qm->num_pgon;

#ifdef HACK_CACHE
         if (hacked_cache == HACK_RECORD)
         for (i=0; i < count; ++i) {
            if ((*poly)->num == 0)
               Error(1, "Badness in render_queue_raw!\n");
            md_render_pgon_render_callback(*poly);
           ++poly;
         }
         else
#endif
         for (i=0; i < count; ++i, ++poly) {
            if (!poly_clip_codes_and(*poly))
#ifdef EDITOR
               editor_safe_render_pgon(*poly);
#else // EDITOR
               md_render_pgon(*poly);
#endif // EDITOR
         }

      r3_end_block();
   }

   cam_pop_clip_planes();

#ifdef HACK_CACHE
   if (hacked_cache == HACK_RECORD)
      md_set_pgon_callback(old);
#endif

   end_md_stride();

   if (clut) end_md_clut();

   CTIMER_RETURN(kCTimerRendMD);
}

BOOL queue_store_md(int idx, ObjID obj, int fragment, int color, uchar *clut)
{
   mds_model *m;
   BOOL rv=TRUE;
   CTIMER_START(kCTimerRendMD);

   if (num_models_in_queue >= MAX_MODELS)
      CTIMER_RETVAL(kCTimerRendMD, FALSE);

   start_md_stride();

   md_render_back_to_front(!g2pt_span_clip);

   if (color==0) color=3+(idx*3);

   md_set_render_light(obj_lighting_on);

   if ((m=(mds_model *)objmodelGetModel(idx))!=NULL)
   {
      queued_model_info *qm;
      mdf_pgon_cback old;
      mds_model *new_md = NULL;
      int bufsize;

      mds_parm *parms=NULL;
      if (m->parms)
         parms=getRendParms(obj,m);
      md_mat_colors_set(m);
      objmodelSetupMdTextures(obj,m,idx);

      qm = &queued_models[num_models_in_queue++];
      qm->idx = idx;

      if (rendobjScaleVec!=&rendobjUnitScale)
         new_md=md_scale_model(NULL,m,rendobjScaleVec,TRUE);
      if (new_md == NULL) {
         new_md = m;
         qm->free_model = FALSE;
      } else
         qm->free_model = TRUE;

      if (!md_fancy_setup_model(new_md, parms)) {
         --num_models_in_queue;
         CTIMER_RETVAL(kCTimerRendMD, FALSE);
      }

      bufsize = md_buffsize(new_md);

      qm->buf = Malloc(bufsize);
      qm->clipmode = r3_get_clipmode();
      qm->tmap_mode = mdd_tmap_mode;
      qm->model = (void *)new_md;
      qm->first_pgon = num_pgon_in_queue;

      md_set_buff(new_md, qm->buf);
      md_transform_only(new_md, parms);
#ifndef SHIP
      if (always_slow_split)
         qm->num_pgon = 1;
      else
#endif
      {
         old = md_set_pgon_callback(record_pgon);
         md_render_only(new_md, parms);
         md_set_pgon_callback(old);
         qm->num_pgon = num_pgon_in_queue - qm->first_pgon;
      }
   } else
      rv = FALSE;

   if (g2pt_span_clip)
      md_render_back_to_front(TRUE);

   end_md_stride();

   CTIMER_RETVAL(kCTimerRendMD, rv);
}

static void setup_md(ObjID obj_id)
{
   Position *pos;
   CTIMER_START(kCTimerRendMD);

   pos = ObjPosGet(obj_id);
   r3_start_object_angles(&pos->loc.vec, &pos->fac, R3_DEFANG);
   CTIMER_RETURN(kCTimerRendMD);
}

// also holds the code for naming objects on screen, which should get moved
static void finish_md(ObjID obj)
{
   CTIMER_START(kCTimerRendMD);
   r3_end_object();
   CTIMER_RETURN(kCTimerRendMD);
}

static void enter_md(ObjID obj)
{
   return;
}

static void leave_md(ObjID obj)
{
   return;
}

////////////////////////////////////
// model type mesh functions

#if 0
static float jt_angle=0;
#endif

static long g_iMeshAppData=0;

void stupid_jointpos_callback(mms_model *m, int joint_id, mxs_trans *t)
{
   ObjPos *pos = ObjPosGetUnsafe((ObjID)m->app_data);

   Assert_(pos);

   mx_ang2mat(&t->mat,&pos->fac);
   mx_copy_vec(&t->vec,&pos->loc.vec);
}

void stupid_stretchy_joint_callback(mms_model *m,int joint_id,mxs_trans *t,quat *rot)
{
   ObjPos *pos = ObjPosGetUnsafe((ObjID)m->app_data);

   Assert_(pos);

   mx_ang2mat(&t->mat,&pos->fac);
   mx_copy_vec(&t->vec,&pos->loc.vec);
   quat_identity(rot);
}

static void enter_mesh(ObjID obj)
{
   ObjPos *pos = ObjPosGetUnsafe(obj);
   Assert_(pos);
   r3_start_object_angles(&pos->loc.vec, &pos->fac, R3_DEFANG);
   return;
}

static void leave_mesh(ObjID obj)
{
   r3_end_object();
   return;
}

static void start_mm_stride(void)
{
#ifdef RGB_LIGHTING
   if (RGB_MD()) {
      r3d_glob.cur_stride = sizeof(r3s_point) + 2*sizeof(float);
      mmd_rgb_lighting = TRUE;
      mld_multi_rgb  = TRUE;
      return;
   }
#endif
   r3d_glob.cur_stride = sizeof(r3s_point);
}

static void end_mm_stride(void)
{
   r3d_glob.cur_stride = sizeof(r3s_point);
   mld_multi_rgb = FALSE;
   mmd_rgb_lighting = FALSE;
}

static void record_pgon_block(mms_pgon *p, int num)
{
   if (num_pgon_in_queue > QUEUE_SIZE)
      Error(1, "Too many polygons in queue.\n");
   // only really need to record first pgon in queue, since that's all
   // queue_render_mesh need look at, since pgons must be adjacent in memory
   // for mesh objects.
   pgon_queue[num_pgon_in_queue++] = (void *)p;
}

BOOL queue_store_mesh(int idx, ObjID obj, int fragment, int color, uchar *clut)
{
   mms_model *m;
   BOOL rv=TRUE;

   r3_set_clip_flags(R3_CLIP_UV | R3_CLIP_I);

   if (num_models_in_queue >= MAX_MODELS)
      return FALSE;

   if ((m=(mms_model *)objmodelGetModel(idx))!=NULL)
   {
      queued_model_info *qm;
      mmf_block_render_cback old;
      int bufsize;

      m->app_data=g_iMeshAppData;
      mm_set_tmap_mode(R3_PL_TEXTURE_LINEAR);

      qm = &queued_models[num_models_in_queue++];
      qm->idx = idx;

      qm->free_model = FALSE;

      bufsize = mm_buffsize(m, NULL);

      qm->buf = Malloc(bufsize);
      qm->clipmode = r3_get_clipmode();
      qm->tmap_mode = mmd_tmap_mode;
      qm->model = (void *)m;
      qm->first_pgon = num_pgon_in_queue;

      //if (clut) start_mm_clut(clut, o);
      mm_set_buff(m, NULL, qm->buf);
      mm_transform_only(m, NULL);
      mm_sort_only(m, NULL);
#ifndef SHIP
      if (always_slow_split)
         qm->num_pgon = 1;
      else
#endif
      {
         old = mm_set_block_render_cback(record_pgon_block);
         mm_render_only(m);
         mm_set_block_render_cback(old);
         qm->num_pgon = m->pgons;
      }
      //if (clut) end_mm_clut();
   } else
      rv = FALSE;

   return rv;
}


static void setup_mesh(ObjID obj_id)
{
   CTIMER_START(kCTimerRendMesh);
   // check if creature
   if(CreatureGetRenderHandle(obj_id,&g_iMeshAppData))
   {
      mm_set_joint_pos_cback((mmf_joint_pos_cback)CreatureMeshJointPosCallback);
      mm_set_stretchy_joint_cback((mmf_stretchy_joint_cback)CreatureMeshStretchyJointCallback);
   }
   else
   {
      g_iMeshAppData=(long)obj_id;
      mm_set_joint_pos_cback(stupid_jointpos_callback);
      mm_set_stretchy_joint_cback(stupid_stretchy_joint_callback);
   }
   CTIMER_RETURN(kCTimerRendMesh);
}


static void finish_mesh(ObjID obj)
{  // ok, this is silly
#ifdef THIS_CANNOT_BE_USEFUL
   CTIMER_START(kCTimerRendMesh);
   CTIMER_RETURN(kCTimerRendMesh);
#endif
}

#ifndef SHIP

BOOL g_bDrawMeshVerts=FALSE;
BOOL g_bDrawMeshJoints=FALSE;
int  g_DbgMeshSegIndex=-1;

#endif

ulong g_MeshRenderFlags=0;
float g_MeshRenderLowDetailDist2;
BOOL g_bRenderMesh = TRUE;

static BOOL render_mesh(int idx, ObjID obj, int fragment,
                        int color, uchar *clut)
{
    BOOL        bGlowing;
    BOOL        bAlphaUsed;
    float       fAlpha;
    float       fOldAlpha;
    float       fGlow;
    float       fOldGlow;
    BOOL        bBumpIt;

    mms_model *m;
    mms_custom_data *pcustom;
    BOOL rv=TRUE;

    CTIMER_START(kCTimerRendMesh);

#ifdef DBG_ON
   if (config_is_defined("phantom_wedge")) {
      setup_default(obj);
      render_default(idx, obj, fragment, color, clut);
      finish_default(obj);
    }
#endif // DBG_ON

    r3_set_clipmode(R3_CLIP);

    // THIS IS VERY BAD!  IF YOU STUFF THE FLAGS DIRECTLY,
    // THE R3D CAN GET OUT OF SYNCH WITH ITSELF!
    //r3_set_clip_flags(R3_CLIP_UV | R3_CLIP_I);

    if ((m=(mms_model *)objmodelGetModel(idx))!=NULL)
    {
        start_mm_stride();

        pcustom = MeshAttachRenderSetup(obj, m, clut);

        m->app_data=g_iMeshAppData;
        objmodelSetupMeshTextures(obj,m,idx);
        mm_set_tmap_mode(R3_PL_TEXTURE_LINEAR);
        if (clut) start_mm_clut(clut, obj);

        if (g_bRenderMesh)
        {
            if (PlayerObjectExists())
            {
                if (mx_dist2_vec(&ObjPosGet(PlayerObject())->loc.vec, &ObjPosGet(obj)->loc.vec) > g_MeshRenderLowDetailDist2)
                    g_MeshRenderFlags |= MMF_XFORM_STRETCHY_UNWEIGHTED;
            }

            //zb: Set Self illumination
            bGlowing = PROPERTY_GET( g_pSelfIlluminationProp, obj, &fGlow);
            if( bGlowing )
                fOldGlow = mm_set_global_glow_level( fGlow );

            bAlphaUsed = PROPERTY_GET(gAlphaRenderProp, obj, &fAlpha);

            if( bAlphaUsed )
                fOldAlpha = mm_set_global_alpha( fAlpha );

            bBumpIt = ObjGetBumpProp( obj );
            MeshTexPrerender(obj, m);

            //if( bBumpIt )
            //    do something smart with it

            mm_render_model(m, pcustom, g_MeshRenderFlags);

            MeshTexPostrender();
            if( bGlowing )
                mm_set_global_glow_level( fOldGlow );

            if( bAlphaUsed )
                mm_set_global_alpha( fOldAlpha );


            g_MeshRenderFlags &= ~MMF_XFORM_STRETCHY_UNWEIGHTED;
        }

        if (clut) end_mm_clut();

#ifndef SHIP
        if(g_bDrawMeshVerts)
        {
           int index;
           double old_bias;

           if (g_lgd3d) {
              // make sure points are in front of model
              old_bias = lgd3d_set_zbias(0);
              lgd3d_set_zbias(old_bias + 2.0/65536.0);
           }

           if(g_DbgMeshSegIndex>=0)
              index=g_DbgMeshSegIndex%(m->segs+1);
           else
              index=g_DbgMeshSegIndex;
           if(index==m->segs)
              mm_dbg_draw_seg_colored_verts(m,-1,fix_div(2,grd_bm.w));
           else
              mm_dbg_draw_seg_colored_verts(m,index,fix_div(2,grd_bm.w));
           //         mm_dbg_draw_verts(m,0,254,fix_div(2,grd_bm.w));

           if (g_lgd3d)
              lgd3d_set_zbias(old_bias);
        }
        if(g_bDrawMeshJoints)
        {
           // The joints are well inside the model, so a subtle z bias won't
           // help us see them.  So we slam the joints to the foreground.
           double old_bias;
           if (g_lgd3d)
              old_bias = lgd3d_set_zbias(1.0);
           mm_dbg_draw_joints(m);
           if (g_lgd3d)
              lgd3d_set_zbias(old_bias);
        }
#endif
        end_mm_stride();

    } else if (color!=-1)
        render_wedge((uchar)color,clut);
    else
        rv=FALSE;

    CTIMER_RETVAL(kCTimerRendMesh, rv);
}


////////////////////////////////////

// heat effect functions

static BOOL render_heat(int idx, ObjID obj, int fragment,
                        int color, uchar *clut)
{
   sHeatDiskCluster *cluster;

   // @HACK: We only draw the effect on its nearest cell.  This will
   // sometimes sort badly with terrain, so if we want this effect in
   // the long run we should mess with this.
   if (fragment != OBJ_SPLIT_OTHER && ObjHeatDiskGet(obj, &cluster))
      ObjHeatDiskRender(obj, cluster);
#ifndef SHIP
   else
   {
      Position *p = ObjPosGet(obj);
      Warning(("Heatobj %d sans heat prop (%g %g %g) frag %d\n",
               obj, p->loc.vec.x, p->loc.vec.y, p->loc.vec.z, fragment));
   }
#endif // ~SHIP

   return TRUE;
}

static void setup_heat(ObjID obj)
{
}

static void finish_heat(ObjID obj)
{
}


////////////////////////////////////
// little sparky things functions

static BOOL render_sparks(int idx, ObjID obj, int fragment,
                          int color, uchar *clut)
{
   sSpark *spark;

   if (ObjSparkGet(obj, &spark)) {
      ObjSparkRender(obj, spark, (grs_bitmap *) objmodelGetModel(idx));
      return TRUE;
   } else
      return FALSE;
}


////////////////////////////////////

// particle system interface (implemented in particle.c)

static BOOL render_particle(int idx, ObjID obj, int fragment,
                        int color, uchar *clut)
{
   r3_set_clipmode(R3_CLIP);
   if (clut) { start_generic_clut(clut); }
   ParticleGroupRender(obj);
   if (clut) { end_generic_clut(); }
   return TRUE;
}

static void setup_null(ObjID obj)
{
}

static void finish_null(ObjID obj)
{
}


////////////////////////////////////

// animating bitmaps
//   first we have the generic function which can be used for
//   lightweight particles

void RenderBitmapGeneric(Position *pos, int idx, int frame, mxs_vector *scale)
{
   r3s_point pt[4], *vlist[4];
   r3s_texture bm = objmodelSetupBitmapTexture(idx, frame);

   r3_set_clipmode(R3_CLIP);
   r3_start_block();
   r3_set_polygon_context(R3_PL_TEXTURE | R3_PL_POLYGON);
   r3_set_texture(bm);
   r3_set_prim();

   // you know, the following code belongs in the r3d


   r3_transform_point(&pt[0], &pos->loc.vec);

   // now compute the surrounding vertices by displacing x&y
   // how much do we displace x&y by?  that depends on the space!

   // #define cx (r3d_state.cur_con->trans)
   //    cx.cspace_vec.x = 1.0;
   //    cx.cspace_vec.y = 1.0;
   //    cx.cspace_vec.z = 1.0;

   // compute the object extents in worldspace

   {
      float w,h;
      if (scale != NULL) {
         w = bm->w / 16.0 * scale->x / 2;
         h = bm->h / 16.0 * scale->y / 2;
      } else {
         w = bm->w / 16.0 / 2;
         h = bm->h / 16.0 / 2;
      }

      w *= r3d_state.cur_con->trans.cspace_vec.x;
      h *= r3d_state.cur_con->trans.cspace_vec.y;

      pt[3].p.z = pt[2].p.z = pt[1].p.z = pt[0].p.z;

      pt[3].p.x = pt[0].p.x - w;
      pt[3].p.y = pt[0].p.y + h;

      pt[2].p.x = pt[0].p.x + w;
      pt[2].p.y = pt[0].p.y + h;

      pt[1].p.x = pt[0].p.x + w;
      pt[1].p.y = pt[0].p.y - h;

      pt[0].p.x = pt[0].p.x - w;
      pt[0].p.y = pt[0].p.y - h;

      // @HACK: if we're not in project space, we'll never clip code!!!
      r3_project_block(4, pt);
   }

   // Now we should find out how big it is and mipmap!  But we never will.
   pt[0].grp.u = pt[3].grp.u = 0;
   pt[1].grp.u = pt[2].grp.u = 0.9999;
   pt[0].grp.v = pt[1].grp.v = 0;
   pt[2].grp.v = pt[3].grp.v = 0.9999;

   vlist[0] = &pt[0];
   vlist[1] = &pt[1];
   vlist[2] = &pt[2];
   vlist[3] = &pt[3];
   r3_draw_poly(4, vlist);
   r3_end_block();

   objmodelReleaseBitmapTexture(idx, frame);
}


EXTERN IVectorProperty      *g_pPhysConveyorVelProp;

void RenderBitmapWorldspace(Position *pos, int idx, int frame, ObjID obj,
                            mxs_vector *scale, sBitmapWorldspace *pBWS)
{
   mxs_vector *pScrollSpeed;
   mxs_matrix orient;
   mxs_vector xaxis, yaxis;
   mxs_vector v[4];
   r3s_point pt[4], *vlist[4] = {pt, pt + 1, pt + 2, pt + 3};

   float fXTilesPerObject = pBWS->m_fXSize / pBWS->m_fXFeetPerTile;
   float fYTilesPerObject = pBWS->m_fYSize / pBWS->m_fYFeetPerTile;

   r3s_texture bm = objmodelSetupBitmapTexture(idx, frame);

   r3_set_clipmode(R3_CLIP);
   r3_start_block();
   r3_set_polygon_context(R3_PL_TEXTURE | R3_PL_POLYGON);
   r3_set_texture(bm);
   r3_set_prim();

   // set up vertices
   mx_ang2mat(&orient, &pos->fac);
   mx_scale_vec(&xaxis, &orient.vec[0], .5 * pBWS->m_fXSize);
   mx_scale_vec(&yaxis, &orient.vec[1], .5 * pBWS->m_fYSize);

   mx_sub_vec(&v[0], &pos->loc.vec, &xaxis);
   mx_addeq_vec(&v[0], &yaxis);

   mx_add_vec(&v[1], &pos->loc.vec, &xaxis);
   mx_addeq_vec(&v[1], &yaxis);

   mx_add_vec(&v[2], &pos->loc.vec, &xaxis);
   mx_subeq_vec(&v[2], &yaxis);

   mx_sub_vec(&v[3], &pos->loc.vec, &xaxis);
   mx_subeq_vec(&v[3], &yaxis);

   if (pBWS->m_fXSize
    && PROPERTY_GET(g_pPhysConveyorVelProp, obj, &pScrollSpeed)
    && pScrollSpeed->x) {
      // pScrollSpeed->x is conveyor belt speed in feet/second
      // objects/second = feet/second / feet/object
      float fObjectsPerSecond = pScrollSpeed->x / pBWS->m_fXSize;
      // tiles/second = tiles/object * objects/second
      float fTilesPerSecond = fXTilesPerObject * fObjectsPerSecond;
      float fTimeSec = GetSimTime() * (1.0 / SIM_TIME_SECOND);
      pt[0].grp.u = pt[3].grp.u = -fmod(fTimeSec * fTilesPerSecond, 1.0);
      pt[1].grp.u = pt[2].grp.u = pt[0].grp.u + fXTilesPerObject;
   } else {
      pt[0].grp.u = pt[3].grp.u = 0;
      pt[1].grp.u = pt[2].grp.u = fXTilesPerObject;
   }

   pt[0].grp.v = pt[1].grp.v = 0;
   pt[2].grp.v = pt[3].grp.v = fYTilesPerObject;

   r3_transform_block(4, pt, v);
   r3_draw_poly(4, vlist);
   r3_end_block();

   objmodelReleaseBitmapTexture(idx, frame);
}


static BOOL render_bitmap(int idx, ObjID obj, int fragment,
                          int color, uchar *clut)
{
   Position *p = ObjPosGet(obj);
   int i,n = objmodelBitmapNumFrames(idx);
   FrameAnimationState *fas = ObjGetFrameAnimationState(obj);
   FrameAnimationConfig *cfg = ObjGetFrameAnimationConfig(obj);
   sBitmapWorldspace *pBWS;

   if (fas)
   {
      i = FrameAnimationGetFrame(fas, cfg, n);
      ObjSetFrameAnimationState(obj, fas);
   }
   else
      i = 0;

   if (clut)
      start_generic_clut(clut);
   if (ObjBitmapWorldspace(obj, &pBWS))
      RenderBitmapWorldspace(p, idx, i, obj, rendobjScaleVec, pBWS);
   else
      RenderBitmapGeneric(p, idx, i, rendobjScaleVec);
   if (clut)
      end_generic_clut();
   return TRUE;
}

BOOL rendobj_bitmap_retire(ObjID obj)
{
   FrameAnimationState *fas = ObjGetFrameAnimationState(obj);
   FrameAnimationConfig *cfg = ObjGetFrameAnimationConfig(obj);
   int idx, n;

   if (rendobj_get_model_funcs(obj, &idx)->render != render_bitmap)
      return FALSE;

   n = objmodelBitmapNumFrames(idx);
   FrameAnimationGetFrame(fas, cfg, n);  // since if off screen, we didnt render
   return FrameAnimationIsDone(fas);     // thus didnt advance, so we wont be done
}

////////////////////////////////////
// actual model call

// model type dependent functions

// model_funcs defined in robjtype.h (also look in robjbase.h, objmodel.h)
static model_funcs g_atModelFuncs[OM_NUM_TYPES] =
{
   { render_default, setup_default, finish_default, enter_default,
     leave_default, NULL, NULL },

   { render_md, setup_md, finish_md, enter_md,
     leave_md, queue_store_md, queue_render_md },

   { render_mesh, setup_mesh, finish_mesh, enter_mesh,
     leave_mesh, NULL, NULL },

   { render_heat, setup_heat, finish_heat, enter_default,
     leave_default, NULL, NULL },

   { render_sparks, setup_heat, finish_heat, enter_default,
     leave_default, NULL, NULL },

   { render_particle, setup_null, finish_null, enter_default,
     leave_default, NULL, NULL },

   { render_bitmap, setup_null, finish_null, enter_default,
     leave_default, NULL, NULL },
};


// always returns something valid
model_funcs *rendobj_get_model_funcs(ObjID obj_id, int *idx)
{
   *idx = -1;

   if (!ObjGetModelNumber(obj_id, idx))
   {
      return &g_atModelFuncs[OM_TYPE_DEFAULT];
   }
   else
   {
      int type=objmodelGetModelType(*idx);

      return &g_atModelFuncs[type]; // XXX what error checking is appropriate?
   }
}

int rendobj_type(ObjID obj_id)
{
   int idx;
   if (!ObjGetModelNumber(obj_id, &idx))
      return OM_TYPE_DEFAULT;
   else
      return objmodelGetModelType(idx);
}

// just get ready to do obj detect, and do the startobjectangles blah blah
void rendobj_setup_obj(ObjID obj_id, int type)
{
   g_atModelFuncs[type].setup(obj_id);
}

// check the test block for if we are the "hot spot"
// also holds the code for naming objects on screen, which should get moved

BOOL rendobj_core_render_object(ObjID obj, int fragment, int idx, uchar *clut,
                                BOOL (*rendfunc)(int, ObjID, int, int, uchar*),
                                model_funcs *ptmf)
{  // ignore the clut for now!
   mxs_vector scale;
   BOOL rval;

   setup_object_lighting(obj, 0, 0);

   ptmf->setup(obj);

   if (ObjGetScale(obj, &scale))
      rendobjScaleVec = &scale;
   else
      rendobjScaleVec = &rendobjUnitScale;

   rendobj_start_translucency(obj);
   rval = rendfunc(idx, obj, fragment, 0, clut);
   rendobj_stop_translucency(obj);

   ptmf->finish(obj);
   return rval;
}

extern bool obj_dealt[1024];  // HACK: need real object dealt flags
bool show_split;
BOOL show_bbox, show_bbox_2d;

BOOL rendobj_object_is_visible(ObjID obj)
{
   if (ObjRenderType(obj) == kRenderNotAtAll)
      return FALSE;
   return TRUE;
}

#ifndef SHIP
void rendobj_nonship_aftereffects(ObjID obj, model_funcs *ptmf)
{
   BOOL any_visuals=show_bbox||g_show_sounds;
   any_visuals=any_visuals||g_bShowCreatureJoints||g_bShowPhysBBox||g_bShowPhysModels;
#ifdef NAME_SUPPORT
   any_visuals=any_visuals||(rendobj_name_color!=0)||rendobj_net_name_coloring||g_AIPathDBDrawCellIds;
#endif
#ifdef OBJ_VECTOR_TESTER
   any_visuals=any_visuals||(obj==rendobj_vector_obj);
#endif
#ifdef EDITOR
   any_visuals=any_visuals||gAIDebugDrawInGame;
#endif
#ifdef PLAYTEST
   any_visuals=any_visuals||g_ShowAmbients;
#endif

   if (!any_visuals)
      return;

   // should only be here if one of the tests above triggered
   // ie. lets predetect if we are doing anything
   // and only Lock/Unlock the Draw Canvas once
   ScrnLockDrawCanvas();

   // need to do something about g_lgd3d / primitives as well

   if (show_bbox)
      draw_obj_bbox(obj);

   if (g_show_sounds)
      DrawSounds();

   if (g_bShowPhysModels)
      rendobj_draw_phys_models(obj);

   if (g_bShowPhysBBox)
      rendobj_draw_phys_bbox(obj);

   if (g_bShowCreatureJoints)
      rendobj_draw_creature_joints(obj);

#ifdef PLAYTEST
   if (g_ShowAmbients)
      AmbientDrawRadii(TRUE);
#endif

#ifdef NAME_SUPPORT
   if (rendobj_name_color||rendobj_net_name_coloring)
      pick_color_and_show_name(obj);
#endif

#ifdef OBJ_VECTOR_TESTER
   if (obj==rendobj_vector_obj)
   {
      r3s_point pt[2];
      mxs_vector *vec=ObjPosGetLocVector(obj);
      mxs_vector newpt;
      mx_add_vec(&newpt,vec,&rendobj_vector);
      r3_start_block();
      r3_transform_point(&pt[0],vec);
      r3_transform_point(&pt[1],&newpt);
      gr_set_fcolor(253);
      r3_draw_line(&pt[0],&pt[1]);
      r3_end_block();
   }
#endif

#ifdef EDITOR
   if(gAIDebugDrawInGame)
      AIDebugDrawObjAIInfo(obj);
#endif

   ScrnUnlockDrawCanvas();
}
#else
#define rendobj_nonship_aftereffects(obj,ptmf)
#endif

#define PLAYER_BOW_HACK

void rendobj_render_object(ObjID obj, uchar *clut, ulong fragment)
{
   model_funcs *ptmf;
   int idx;

   CTIMER_START(kCTimerRendObjTotal);

   if (!rendobj_object_is_visible(obj))
      CTIMER_RETURN(kCTimerRendObjTotal);

   if (show_bbox_2d || (show_split && fragment)) {
      fix mn[2],mx[2];
      GetObjScreenBounds(obj, mn, mx, BOUNDING_RECTANGLE);
      ScrnLockDrawCanvas();
      gr_set_fcolor(1);
      gr_fix_line(mn[0],mn[1],mx[0],mn[1]);
      gr_fix_line(mn[0],mn[1],mn[0],mx[1]);
      gr_fix_line(mn[0],mx[1],mx[0],mx[1]);
      gr_fix_line(mx[0],mn[1],mx[0],mx[1]);
      ScrnUnlockDrawCanvas();
   }

   ptmf = rendobj_get_model_funcs(obj,&idx);

   if (fragment != OBJ_SPLIT_OTHER) {
      g_RendObjVisible.Set(obj); 
      CoronaCheckObj(obj);
   }

   if (fragment == OBJ_NO_SPLIT) {
#ifdef PLAYER_BOW_HACK
      // check if obj has position, since overlay (namely player arrow) may not
      if (ObjPosGet(obj))
         rendobj_core_render_object(obj, fragment, idx, clut, ptmf->render, ptmf);
      else
         Warning(("needed PlayerBowHack for %d\n",obj));
#endif
      goto done;
   }

#ifdef HACK_CACHE
#ifndef SHIP
   if (always_slow_split)
   {
      always_slow_split = FALSE;
      if (hacked_cache == HACK_PLAYBACK)
       { hacked_cache = HACK_NONE; mprintf("Disabling hacked cache.\n"); }
      else
       { hacked_cache = HACK_RECORD; mprintf("Recording hacked cache.\n"); }
   }
#endif
#endif

   // fragment is non-zero, so the object needs splitting
   if (!ptmf->queueStore || !ptmf->queueRender) {
      // use r3 clip planes
      cam_push_clip_planes(obj, FALSE); // push the planes to clip to this cell
      rendobj_core_render_object(obj, fragment, idx, clut, ptmf->render, ptmf);
      cam_pop_clip_planes();
   } else {
      if (obj_dealt[obj]) {        // if obj_dealt TRUE, this is first time so
         if (!rendobj_core_render_object(obj, fragment, idx, 0, ptmf->queueStore, ptmf))
         {  // if we don't draw it, put an empty entry in queue
            queued_models[num_models_in_queue].buf = 0;
            queued_models[num_models_in_queue].free_model = 0;
            queued_models[num_models_in_queue].num_pgon = 0;
            queued_models[num_models_in_queue++].obj_id = obj;
         } else
            queued_models[num_models_in_queue-1].obj_id = obj;
      }
#ifndef SHIP
      if (always_slow_split)
      {
         ptmf->setup(obj);
         render_queue(obj, fragment, clut, ptmf->queueRender);
         ptmf->finish(obj);
      }
      else
#endif
         render_queue(obj, fragment, clut, ptmf->queueRender);
   }

done:
   rendobj_nonship_aftereffects(obj,ptmf);

   CTIMER_RETURN(kCTimerRendObjTotal);
}

void rendobj_done_frame(void);

// free up memory currently in use...
void rendobj_finish_object_rendering(void)
{
   int i;

   for (i=0; i < num_models_in_queue; ++i) {
      if (queued_models[i].buf)
         Free(queued_models[i].buf);
      if (queued_models[i].free_model)
         Free(queued_models[i].model);
   }

   num_pgon_in_queue = 0;
   num_models_in_queue = 0;

   for (i=0; i < obj_cache.Bounds().max; ++i) 
   {
      delete obj_cache[i]; 
      obj_cache[i] = NULL; 
   }

#ifdef HACK_CACHE
   if (hacked_cache == HACK_RECORD) {
      hack_model_start[hack_model_count] = hack_pgon_count;
      hacked_cache = HACK_PLAYBACK;
      mprintf("Playing back from hack cache.\n");
   }
   hack_pgon_count = 0;
   hack_model_count = 0;
#endif

   rendobj_done_frame();
}

// object sorting test used by portal
// maybe we should cache these tests since
// they're done several times?
BOOL rendobj_object_blocks(ObjID blocker, ObjID blockee)
{
   mxs_vector *mn,*mx,*center;
   mxs_vector min1,max1, min2,max2;
   fix mn2d1[4], mx2d1[4];
   fix mn2d2[4], mx2d2[4];
   float radius;
   int i;

   /* //zb
   if (g_lgd3d)
      if (rendobj_type(blocker) == OM_TYPE_PARTICLES ||
          rendobj_type(blockee) == OM_TYPE_PARTICLES)
         return FALSE;
         */

   portal_obj_bounds(blocker, &mn, &mx, &center, &radius, mn2d1, mx2d1);
   min1 = *mn;
   max1 = *mx;
   portal_obj_bounds(blockee, &mn, &mx, &center, &radius, mn2d2, mx2d2);
   min2 = *mn;
   max2 = *mx;

   // check if we overlap along every 2d axis

   for (i=0; i < 4; ++i)
      if (!(mx2d1[i] > mn2d2[i] && mx2d2[i] > mn2d1[i]))
         return FALSE;

   // ok, we do overlap... now try to find a separating plane
   // between the two objects

   // if, along some axis,
   //    viewer < blockee_max < blocker_min, we're ok
   // if, along some axis,
   //    blockee_max < viewer < blocker_min, we're ok

   // if, along some axis,
   //    viewer > blockee_min > blocker_max, we're ok
   // if, along some axis,
   //    blockee_min > viewer > blocker_max, we're ok

   // so this is:
   // if  blocker_min > max(blockee_max, viewer)
   // if  blocker_max < min(blockee_min, viewer)

#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#define pcl       portal_camera_loc

   for (i=0; i < 3; ++i) {
      if (min1.el[i] > MAX(max2.el[i], pcl.el[i])) return FALSE;
      if (max1.el[i] < MIN(min2.el[i], pcl.el[i])) return FALSE;
   }
#undef pcl
#undef MAX
#undef MIN

   // no axis-aligned separating planes, so punt

   return TRUE;
}

// The following functions are for use with zbuffering.

typedef struct rqs_elem {
   ObjID obj;
   uchar *clut;
   ulong fragment;
   uchar flags;
   uchar type;
   ushort count;
   float distance2;     // for sorting alpha objects
} rqs_elem;

#define RQT_OBJECT 0
#define RQT_POLY 1

#define RQ_FLAG_FOG 1

#define RQ_MAX_OBJS 128
static int rqd_num_objs;
static int rqd_tluc_objs;
static rqs_elem rqd_queue[RQ_MAX_OBJS];

static void (*rq_real_render_call)(ObjID o, uchar *c, ulong f)=NULL;
static void (*next_post_render_cback)(void) = NULL;

// Place an object on the queue to be rendered at frame end.
//
// Tluc objects were originally intended for objects with an alpha,
// but since transparent texture share the same issues, we pretend
// they have alpha, too, even though their alphas are 1.
//
static void rq_queue_object(ObjID obj, uchar *clut, ulong fragment)
{
   rqs_elem *p_elem;
   if (rqd_num_objs == rqd_tluc_objs) {
      Warning(("rq_queue_object(): Too many objects/polys to queue!\n"));
      return;
   }

   if (is_tluc(obj)) {
      p_elem = &rqd_queue[--rqd_tluc_objs];

      // Distance to camera only matters for sorting alpha objects.
      mxs_vector *pos = ObjPosGetLocVector(obj);
      Assert_(pos);
      p_elem->distance2 = mx_dist2_vec(pos, &portal_camera_loc);
   } else
      p_elem = &rqd_queue[rqd_num_objs++];

   p_elem->type = RQT_OBJECT;
   p_elem->obj = obj;
   p_elem->clut = clut;
   p_elem->fragment = fragment;
   p_elem->flags = lgd3d_is_fog_on()? RQ_FLAG_FOG : 0;
}

static void rq_queue_water_poly()
{
   rqs_elem *p_elem;

   if ((rqd_tluc_objs != RQ_MAX_OBJS) &&
       (rqd_queue[rqd_tluc_objs].type == RQT_POLY))
   {
      rqd_queue[rqd_tluc_objs].count++;
      return;
   }

   if (rqd_num_objs == rqd_tluc_objs) {
      Warning(("rq_queue_water_poly(): Too many objects/polys to queue!\n"));
      return;
   }

   p_elem = &rqd_queue[--rqd_tluc_objs];
   p_elem->type = RQT_POLY;
   p_elem->count = 1;
}


// This is the qsort callback for comparing queued objects.
// Is queued object i further from the camera than queued object j?
static int rq_alpha_compare(const void *elem_i, const void *elem_j)
{
   rqs_elem *i = (rqs_elem *) elem_i;
   rqs_elem *j = (rqs_elem *) elem_j;
   return (i->distance2 > j->distance2)? 1 : -1;
}


static void qr_sort_alpha_objects()
{
   // Do at least two items need to be sorted?
   if ((RQ_MAX_OBJS - rqd_tluc_objs) < 2)
      return;

   // find runs of objects between polygons & sort 'em by distance
   int i, j;
   for (i = rqd_tluc_objs; i < RQ_MAX_OBJS; ++i) {
      if (rqd_queue[i].type != RQT_OBJECT)
         continue;
      for (j = i + 1; j < RQ_MAX_OBJS; ++j)
         if (rqd_queue[j].type != RQT_OBJECT)
            break;
      if (j - i > 1)
         qsort(rqd_queue + i, j - i, sizeof(rqs_elem), rq_alpha_compare);
      i = j;
   }
}


// Render all objects placed on the queue this frame.
// only run if zbuffering is enabled

static void rq_render_queue(void)
{
   int i;
   double old_bias = lgd3d_set_zbias(g_obj_zbias);

   lgd3d_set_zcompare(TRUE);
   lgd3d_set_zwrite(TRUE);

   lgd3d_use_linear_table_fog(TRUE);//temp 
   
   // We iterate up so the queue can grow as we add objects attached
   // to mesh objects.
   for (i = 0; i < rqd_num_objs; ++i) {
      AssertMsg(rqd_queue[i].type == RQT_OBJECT, "rq_render_queue(): Invalid queue entry!");
      rq_real_render_call(
         rqd_queue[i].obj, rqd_queue[i].clut, rqd_queue[i].fragment);
   }

   lgd3d_set_zwrite(FALSE);

   lgd3d_set_blend(TRUE);

   set_mm_sort(TRUE);

   // wsf: OK, here's something new. portal used to do a topological
   // sort - supposedly fast, but incorrect sort on ALL objects. Now,
   // we're hardware only, and rely on zbuffer. No need to sort all
   // objects anymore - just ones that have alpha. So, we removed the
   // portal sort, and do our own distance-based sort on alpha
   // objects: last in list is furthest away, and rendered first.
   qr_sort_alpha_objects();

   for (i = RQ_MAX_OBJS-1; i >= rqd_tluc_objs; --i) {
      rqs_elem *p_elem = &rqd_queue[i];
      if (portal_fog_on)
         lgd3d_set_fog_enable(!!(p_elem->flags & RQ_FLAG_FOG));
      if (p_elem->type == RQT_OBJECT)
         rq_real_render_call(p_elem->obj, p_elem->clut, p_elem->fragment);
      else {
         portal_render_water_polys(p_elem->count);
      }
   }

   set_mm_sort(FALSE);

   lgd3d_set_blend(FALSE);
   lgd3d_set_zwrite(TRUE);
   lgd3d_set_alpha(1.0);

   lgd3d_set_zbias(old_bias);

   if (next_post_render_cback)
      next_post_render_cback();
}


void render_overlays(void)
{

   #ifndef SHIP
   if (PlayerObjectExists())
   {
      int ocol = gr_get_fcolor();

      DrawRoom(PlayerObject());
      DoRoomTest(PlayerObject());

      gr_set_fcolor(ocol);
   }
   #endif
}

void init_object_rendering(void)
{
   extern void (*portal_render_overlays_cback)(void);
   int objlight_percentage = 48;  // default percentage

   ml_init_multi_lighting();
   md_light_set_type(MD_LT_AMB|MD_LT_DIFF);


   g_RendObjVisible.Resize(gMaxObjID); 
   // attach rendobj sink
   cAutoIPtr<IObjIDSink> pSink = new cRendObjIDSink; 
   AutoAppIPtr(ObjIDManager); 
   pObjIDManager->Connect(pSink); 
   obj_cache.AutoConnect(); 

   g2pt_span_clip = FALSE;
   portal_render_object = rendobj_render_object;
   portal_object_visible = rendobj_object_is_visible;
   portal_object_blocks = rendobj_object_blocks;
   config_get_int("highlight_level",&objlight_percentage);
   objlight_highlight_level=((float)objlight_percentage)/100.0;

   portal_render_overlays_cback = render_overlays;

   g_MeshRenderLowDetailDist2=3600.0;
   if (config_get_float("mesh_detail_dist", &g_MeshRenderLowDetailDist2))
       g_MeshRenderLowDetailDist2 *= g_MeshRenderLowDetailDist2;

#ifdef DBG_ON // check all model types are supported in function table
   AssertMsg2(sizeof(g_atModelFuncs)/sizeof(model_funcs)>=OM_NUM_TYPES,\
      "rendobj.c: %d model types supported, need %d\n",\
      sizeof(g_atModelFuncs)/sizeof(model_funcs), OM_NUM_TYPES);
#endif

#if defined(NEW_NETWORK_ENABLED) && defined(NAME_SUPPORT)
   g_pObjNet = AppGetObj(IObjectNetworking);
#endif
}

// We need to clear the visibility array before each frame.
void rendobj_init_frame(void)
{
   if (g_lgd3d && g_zbuffer) {
      // Put objects on queue when they would normally be rendered;
      // instead render them at the end of the frame after all terrain
      // has been rendered.  Objects don't need to be sorted or split- Yay!
      rqd_num_objs = 0;
      rqd_tluc_objs = RQ_MAX_OBJS;
      rq_real_render_call=portal_render_object;
      portal_allow_object_splitting = FALSE;
      portal_render_object = rq_queue_object;
      next_post_render_cback = portal_post_render_cback;
      portal_post_render_cback = rq_render_queue;
      portal_queue_water_poly = rq_queue_water_poly;

      // set up for terrain rendering.  No zcompare means
      // we don't have to clear the z buffer.  Also faster on some hw.
      lgd3d_set_zwrite(TRUE);
      lgd3d_set_zcompare(FALSE);
   } else {
      // Do the usual thing: split objects to get correct sort order;
      // render them as we come to them.  Don't need any any post terrain
      // render callback.
      portal_allow_object_splitting = TRUE;
      if (g_lgd3d) { // set up for terrain rendering
         lgd3d_set_zwrite(FALSE);
         lgd3d_set_zcompare(FALSE);
      }
   }

   g_RendObjVisible.ClearAll(); 

   // @TODO: If we ever use this timing system for something other
   // than rendobj this should go someplace more general.
#ifndef SHIP
   CTimerFrame();
#endif // ~SHIP
#ifdef NAME_SUPPORT
#ifdef NEW_NETWORK_ENABLED
   rendobj_local_name_use_color=guiScreenColor(rendobj_local_name_color);
   rendobj_hosted_name_use_color=guiScreenColor(rendobj_hosted_name_color);
   rendobj_proxy_name_use_color=guiScreenColor(rendobj_proxy_name_color);
#endif
#endif
}

// this frame is done
void rendobj_done_frame(void)
{
   if (g_lgd3d && g_zbuffer) {
      portal_render_object=rq_real_render_call;
      portal_post_render_cback = next_post_render_cback;
   }
}

// terminate app
void rendobj_exit(void)
{
#if defined(NEW_NETWORK_ENABLED) && defined(NAME_SUPPORT)
   SafeRelease(g_pObjNet);
#endif
}


}  // extern "C"

