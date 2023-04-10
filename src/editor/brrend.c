/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brrend.c,v 1.29 2000/02/19 12:27:47 toml Exp $

// actual brush renderer, picker, so on

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <lg.h>
#include <g2.h>
#include <r3d.h>
#include <matrix.h>
#include <mprintf.h>

#include <uiedit.h>     // get color fixup code

#include <viewmgr.h>    // sad, isnt it
#include <editbr.h>
#include <brlist.h>
#include <editbr_.h>
#include <brinfo.h>
#include <brobjpro.h>
#include <brrend.h>
#include <primal.h>
#include <primfast.h>
#include <vbrush.h>
#include <gridsnap.h>
#include <hilight.h>

#include <csg.h>
#include <rand.h>
#include <config.h>

#include <appagg.h>
#include <comtools.h>
#include <iobjsys.h>
#include <traitman.h>

// this controls whether portal conversion will be doubles or floats
#define PORTAL_CONVERT_USE_DOUBLES   

/////////////////////
// color horribleness

// this is hideous, fix it - (color horribleness, see end)
typedef enum {
   terrCur,    
   terrNorm,   
   terrFace,   
   lightCur,   
   lightNorm,  
   areaCur,    
   areaMeOnly, 
   areaActive,
   areaNorm,   
   objCur,     
   objNorm,
   camCur,
   camNorm,
   terrMulti,
   lightMulti,
   areaMulti,
   objMulti,
   flowCur,
   flowMulti,
   flowNorm,
   roomCur,
   roomMulti,
   roomNorm,
   highlight1,
   NumColors
} brushColorNames;

#define NUM_GREY_SHADES 12

int brushColors[NumColors];
int greyRange[NUM_GREY_SHADES];

int brushRaws[]=
{
   uiRGB(0xd5,0xd5,0xd5),  // terr cur
   uiRGB(0x63,0xa3,0xb8),
   uiRGB(0xa1,0x66,0x40),
   uiRGB(0xc0,0xc0,0x2a),  // light
   uiRGB(0x90,0x90,0x1a),   
   uiRGB(0xc4,0x78,0x4a),  // area
   uiRGB(0xa1,0x66,0x40),
   uiRGB(0x83,0x55,0x36),
   uiRGB(0x6e,0x44,0x2d),
   uiRGB(0xd5,0xa5,0xa5),  // obj, past here is not done
   uiRGB(0xa5,0x25,0x25),  
   uiRGB(0xd5,0xd5,0xd5),  // cam
   uiRGB(0xd5,0xd5,0xd5),  //
   uiRGB(0xa4,0xa4,0x1a),  // multi-terr
   uiRGB(0xa4,0xa4,0x1a),  // multi-light
   uiRGB(0xa4,0xa4,0x1a),  // multi-area
   uiRGB(0xa4,0xa4,0x1a),  // multi-obj
   uiRGB(0x88,0x10,0xc4),  // flow cur, multi, norm
   uiRGB(0xa4,0xa4,0x1a),  // 
   uiRGB(0x58,0x10,0x80),
   uiRGB(0x88,0x10,0xc4),  // room cur, multi, norm
   uiRGB(0xa4,0xa4,0x1a),  
   uiRGB(0x48,0x48,0x48),
   uiRGB(0x80,0xa0,0x20),  // highlight
};
#define NUM_BRUSH_COLORS (sizeof(brushRaws)/sizeof(brushRaws[0]))

// pick weight for grey for i, with i/NGS
#define PikR(i) (i*0x70/(NUM_GREY_SHADES-1))
#define PikG(i) (0x28+(i*0xa8/(NUM_GREY_SHADES-1)))
#define PikB(i) (0x30+(i*0xcf/(NUM_GREY_SHADES-1)))

void brushPickColors(void)
{
   int i;
   if (config_is_defined ("hens_changes"))
   {
      brushRaws[highlight1] = uiRGB(0x00,0xFF,0x00);  //	the new (green) highlight
   }

   for (i=0; i<NumColors; i++)
      brushColors[i]=uieditFixupColor(brushRaws[i]);
   for (i=0; i<NUM_GREY_SHADES; i++)
      greyRange[i]=uieditFixupColor(uiRGB(PikR(i),PikG(i),PikB(i)));
}

//////////////////////////////
// brush render/convert/pick craziness

//////////////////////////////
// "global" state for the brush which we are currently drawing
// this is untrue, but works for now
#define MAX_PTS_PER_BRUSH 32

// static state for the setup/transform/draw brush pipeline
static r3s_point   brush_pts[MAX_PTS_PER_BRUSH]; // transformed points
static mxs_vector  obj_pts[MAX_PTS_PER_BRUSH], world_pts[MAX_PTS_PER_BRUSH];
static int         brush_pt_cnt=-1;
static BOOL        no_rotations=FALSE;
static mxs_vector  obj_offset;
static int         brush_primal;

//////////////////////////////
// "global" current filter/scale/active list for which brushes to draw and how

// default filter is show all, default color is distance based
int editbr_color_mode=brFlag_COLOR_DIST;
int editbr_filter=brFlag_FILTERMASK;

static int brush_color_mode;                    // for use during the frame
static float cur_scale=1.0, cur_lo=0.0;         // for color scaling
static mxs_vector _cur_cam_vec, _cur_cam_pos;   // space to store them
static mxs_vector *cur_cam_vec, *cur_cam_pos;   // pointers, null if not using

static editBrush  *cur_cursor;
static BOOL       isIso=FALSE;

// hot region filter fun
#define MAX_HOTS  32
static int        hot_cnt=0;
static editBrush *hots[MAX_HOTS];

int editbr_filter_time_lo=0, editbr_filter_time_hi=0;
float editbr_filter_size=0.0;
bool renderHilightOnly=FALSE;
bool renderHotsOnly=FALSE;

// set global draw context, color and filters
void brushDrawContext(int filter, int color_mode)
{
   if (color_mode!=-1) editbr_color_mode=color_mode;
   if (filter!=-1) editbr_filter=filter;
}

// MOVE THIS ALL SOMEWHERE ELSE TOO
#define vec_prin(v) (v)->el[0],(v)->el[1],(v)->el[2]

// controls for the fast renderer
static BOOL       fastRender=FALSE; // are we in fastrender (2d) mode
static mxs_vector base, scale;      // upper left of view, scale factor 
static int        scr_x_axis, scr_y_axis;

BOOL fastrendererSetup(int camera, mxs_vector *cam_pos)
{
   mxs_vector scr_unit_x, scr_unit_y; // unit pixel in world space on each axis

   vm_get_camera_axis(camera,&scr_x_axis,&scr_y_axis);
   vm_map_screen_to_world(camera,&base,0,0);
   vm_screen_axes(camera,&scr_unit_x,&scr_unit_y);
   mx_zero_vec(&scale);
   scale.el[scr_x_axis]=1.0/scr_unit_x.el[scr_x_axis];
   scale.el[scr_y_axis]=1.0/scr_unit_y.el[scr_y_axis];

   // @TODO: once we have 6 drawers, pick the correct function pointer here...
   
   return TRUE;
}

#ifdef MOO
// do correct splufty clip coding here... conservative due to line rules...
// @OPTIMIZE: assemblize
static uchar fastrenderClipCode(r3s_point *us)
{
   uchar ccode=CLIP_NONE;
   
   if (us->grp.sx<fix_make(0,0x8000))
      ccode|=CLIP_LEFT;
   else if (us->grp.sx>fix_make(grd_canvas->bm.w-1,0x8000))
      ccode|=CLIP_RIGHT;
   
   if (us->grp.sy<fix_make(0,0x8000))
      ccode|=CLIP_TOP;
   else if (us->grp.sy>fix_make(grd_canvas->bm.h-1,0x8000))
      ccode|=CLIP_BOT;
   
   return ccode;
}
#endif

static uchar fastrenderFloatClipCode(float x, float y)
{
   uchar ccode=CLIP_NONE;
   
   if (x<0.5)
      ccode|=CLIP_LEFT;
   else if (x>(float)(grd_canvas->bm.w-1)-0.5)
      ccode|=CLIP_RIGHT;
   
   if (y<0.5)
      ccode|=CLIP_TOP;
   else if (y>(float)(grd_canvas->bm.h-1)-0.5)
      ccode|=CLIP_BOT;
   
   return ccode;
}

// @OPTIMIZE: do 6 versions of this, function table them
// @OPTIMIZE: assemblize
// given the 2d setup in fastrender, do the 2d axis scale/clip for this point list
void fastrenderTransformBlock(int cnt, r3s_point *view_pts, mxs_vector *pt_list)
{
   int i;

   for (i=0; i<cnt; i++)
   {
      mxs_vector diff;
      mx_sub_vec(&diff,&pt_list[i],&base);

      mx_elmuleq_vec(&diff,&scale);  // 0 relative to up left of window, i guess

      if ((fabs(diff.el[scr_x_axis])>fix_int(FIX_MAX))||
          (fabs(diff.el[scr_y_axis])>fix_int(FIX_MAX)))
         view_pts[i].ccodes=CLIP_ALL;
      else
         view_pts[i].ccodes=fastrenderFloatClipCode(diff.el[scr_x_axis],diff.el[scr_y_axis]);

      view_pts[i].grp.sx=fix_from_float(diff.el[scr_x_axis]);
      view_pts[i].grp.sy=fix_from_float(diff.el[scr_y_axis]);

      r3d_ccodes_or |=view_pts[i].ccodes;
      r3d_ccodes_and&=view_pts[i].ccodes;
   }
}

// @J4FIX: get this out of brinfo.h, clean this shit up
//   maybe have brrend as a new source file, eh?
// setup to draw, uses global context
BOOL brushSetupDraw(int flags, int camera)
{
   float dist, lo=100000.0, hi=0.0;
   mxs_vector tmp, *cam_pos;
   editBrush *us;
   int hIter;

   flags&=brFlag_EDITBR_MASK;  // mask out gedit controls
   vm_get_3d_camera_loc(&cam_pos,NULL);
   cur_cam_vec=cur_cam_pos=NULL; // dont need to do special distance stuff
   brush_color_mode=flags&brFlag_COLOR_MASK;
   if (brush_color_mode==brFlag_COLOR_GLOB)
      brush_color_mode=editbr_color_mode;
   isIso=(flags&brFlag_IsoView)!=0;
   if (flags&brFlag_NO_SELECTION)
      cur_cursor=NULL;
   else
      cur_cursor=blistGet();
   fastRender=FALSE;
   if (isIso&&!vm_get_3d(camera))
      fastRender=fastrendererSetup(camera,cam_pos);  // activate the Fast Renderer
   switch (brush_color_mode)
   {
   case brFlag_COLOR_TIME:
      cur_scale=(float)max(blistCount()-blistGetPos(),blistGetPos());
      cur_lo=0.0;       // scale is max distance from current time
      break;
   case brFlag_COLOR_DIST:
      _cur_cam_vec=*r3_get_forward_slew();   // store off vec
      _cur_cam_pos=*cam_pos;
      if (isIso)
      {                                // set cam vec to the storage
         cur_cam_vec=&_cur_cam_vec;    // so the non-NULL will mean
         cur_cam_pos=&_cur_cam_pos;    // we need to do special distance
      }                                // processing
      us=blistIterStart(&hIter);
      while (us!=NULL)
      {
         mx_sub_vec(&tmp,&us->pos,cam_pos);        // do our own distance
         if (isIso)  
            dist=fabs(mx_dot_vec(&tmp,cur_cam_vec));     // cant use z
         else
            dist=mx_mag_vec(&tmp);                       // can use z
         if (dist<lo) lo=dist;
         if (dist>hi) hi=dist;
         us=blistIterNext(hIter);
      }                    // for now, we ignore lo, so if you are far from
      cur_scale=hi;        // something, it is darker.  if you set cur_lo to lo
      cur_lo=0.0;          // and scale to hi-lo, then you darken the range
      break;               // but it wont get darker as you get far away
   default:                
      cur_scale=1.0;
      cur_lo=0.0;
      break;
   }
   hot_cnt=0;

   if (renderHotsOnly)
   {  // and now, the real fun, the scan hot regions stuff
      us=blistIterStart(&hIter);
      while (us!=NULL)
      {
         if (brushGetType(us)==brType_HOTREGION)
         {
            if (brHot_IsMEONLY(us))
            {
               hots[0]=us;    // if we are a me only
               hot_cnt=1;     // set hots 0 to us, set cnt to 1
               blistIterDone(hIter);
               break;         // and stop looking
            }                 // so we are only hot in the list
            else if (brHot_GetBase(us)==brHot_ACTIVE)
               hots[hot_cnt++]=us;  // add ourselves to current hot list
            if (hot_cnt>=MAX_HOTS)
            {
               Warning(("Too Many Active Hot Regions!!"));
               blistIterDone(hIter);
               return FALSE;
            }
         }
         us=blistIterNext(hIter);
      }
   }
   return TRUE;
}

// @TODO: need to learn about vBrush
// Draw the brush darker if its closest point is far away
int brushSetDrawColor (editBrush *us, r3s_point *p)
{
   int i, lp;
   float val;

   if (us==cur_cursor)        // if we are the cursor, set to white and go
      return brushColors[terrCur];
   switch (brush_color_mode)
   {
   case brFlag_COLOR_NONE:
      return brushColors[terrNorm];
   case brFlag_COLOR_TIME:
      val=(float)abs(blistGetPos()-blistCheck(us));
      break;
   case brFlag_COLOR_DIST: // @J4FIX
      if (cur_cam_vec)        // means we cant look at z since we are Iso View
      {                       // so have to go do the computation based on
         mxs_vector tmp;      // the camera vector
         mx_sub_vec(&tmp,&us->pos,cur_cam_pos);
         val=fabs(mx_dot_vec(&tmp,cur_cam_vec));
      }
      else
      {                          // the z coordinates do indicate depth
         val = 10000.0;          // so just zip through each corner
         for (lp = 0; lp < brush_pt_cnt; lp++)  
            if (p[lp].p.z < val) // and find the closest
               val = p[lp].p.z;
      }
      break;
   }
   if ((val<cur_lo)||(cur_scale==0))  // now actually scale and pick the color
      i=0;                            // underflow, just do zero (max)
   else
      i=(int)((float)NUM_GREY_SHADES*(val-cur_lo)/cur_scale);
   if (i>=NUM_GREY_SHADES) i=NUM_GREY_SHADES-1;
   return greyRange[NUM_GREY_SHADES-i-1];
}

#ifdef PORTAL_CONVERT_USE_DOUBLES
#include <matrixd.h>
#include <mxcvrt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
#ifdef DBG_ON
void mp_mxds_vector(char *prefix, mxds_vector *pt)
{
   char buf[256];
   sprintf(buf,"%s: %.14lf, %.14lf, %.14lf\n",prefix,pt->x,pt->y,pt->z);
   mprintf(buf);
}
#endif

static mxds_vector db_world_pts[MAX_PTS_PER_BRUSH];
void brushTerrFancyDoubleSetupNTransform(editBrush *curBrush)
{
   mxs_vector  obj_single_pts[MAX_PTS_PER_BRUSH];
   mxds_vector o_pt, o_offset, angs;
   mxds_matrix o_angmat;
   int         i;
   
   primalQuantizePos(curBrush,&obj_offset);
   if (brushGetType(curBrush)==brType_TERRAIN)
      brush_primal=curBrush->primal_id;
   else
      brush_primal=PRIMAL_CUBE_IDX;
   primalBr_SetFastUnsafePrimal(brush_primal);
   // need the doubles here...?? would be nice for angles on primal craziness... argh, though
   brush_pt_cnt=primalRawFull(brush_primal,&curBrush->sz,obj_single_pts);
   mx_vec_d_from_s(&o_offset,&obj_offset);
   primalQuantizeAng(&curBrush->ang,&angs);
//   mprintf("Took %x %x %x..\n",curBrush->ang.el[0],curBrush->ang.el[1],curBrush->ang.el[2]);
//   mp_mxds_vector("got  ",&angs);
   mxd_rad2mat(&o_angmat,&angs);

   for (i=0; i<brush_pt_cnt; i++)
   {
      mx_vec_d_from_s(&o_pt,&obj_single_pts[i]);
//      mp_mxds_vector("raw  ",&o_pt);
      mxd_mat_mul_vec(&db_world_pts[i],&o_angmat,&o_pt);
      mxd_addeq_vec(&db_world_pts[i],&o_offset);
      // oh, and here... oh, very very sad
      if (curBrush->grid.grid_enabled)
      {
         mx_vec_s_from_d(&world_pts[i],&db_world_pts[i]);      
         gedit_vertex_snap(&curBrush->grid,&world_pts[i],&world_pts[i]);
         mx_vec_d_from_s(&db_world_pts[i],&world_pts[i]);
      }
//      mp_mxds_vector("real ",&db_world_pts[i]);
   }
}
#endif

// you should setup obj_offset yourself prior to calling this....
void brushGenericPrimalSetup(int primal_id, mxs_vector *scale, mxs_angvec *ang)
{
   brush_primal=primal_id;
   primalBr_SetFastUnsafePrimal(primal_id);
   brush_pt_cnt=primalRawFull(primal_id,scale,obj_pts);

   no_rotations=((ang->tx==0)&&(ang->ty==0)&&(ang->tz==0));

   if (!no_rotations)
      r3_start_object_angles(&obj_offset,ang,R3_DEFANG);
   else if (!fastRender)
      r3_start_object(&obj_offset);  // this is really doing the obj_offset thing, secretly
}

// sets up points for a terrain brush
void brushTerrSetup(editBrush *curBrush)
{
   primalQuantizePos(curBrush,&obj_offset);
   brushGenericPrimalSetup(curBrush->primal_id,&curBrush->sz,&curBrush->ang);
}

// special for "cube" things, such as hotregions, water, so on
void brushCubeSetup(editBrush *curBrush)
{
   obj_offset = curBrush->pos;
   brushGenericPrimalSetup(PRIMAL_CUBE_IDX,&curBrush->sz,&curBrush->ang);
}

// what to scale rendering sizes by for lights
#define lightScale (0.007)  // was 0.025
void brushLightSetup(editBrush *curBrush)
{
   mxs_vector mono_scale_vec;
   mono_scale_vec.x=mono_scale_vec.y=mono_scale_vec.z=brLight_Bright(curBrush)*lightScale;
   obj_offset = curBrush->pos;
   brushGenericPrimalSetup(PRIMAL_LIGHT_IDX,&mono_scale_vec,&curBrush->ang);
}

void brushLineSetup(mxs_vector *p1, mxs_vector *p2)
{
   obj_pts[0]=*p1;
   if (p2)
   {
      obj_pts[1]=*p2;
      brush_pt_cnt=2;
   }
   else
      brush_pt_cnt=1;      
   no_rotations=TRUE;
   obj_offset.x=obj_offset.y=obj_offset.z=0;
   if (!fastRender)
      r3_start_object(&obj_offset);
}

static BOOL pt_on_screen(r3s_point *p)
{
   if (isIso&&!fastRender)    // probably should clip verse the canvas or something
      return TRUE;
   return p->ccodes==0;
}

// hacks for now for the whole line_draw callback thing...
static void line_2d_clip(r3s_point *p1, r3s_point *p2)
{
   gr_fix_line(p1->grp.sx,p1->grp.sy,p2->grp.sx,p2->grp.sy);
}

static void line_2d_unclip(r3s_point *p1, r3s_point *p2)
{
   gr_fix_uline(p1->grp.sx,p1->grp.sy,p2->grp.sx,p2->grp.sy);
}

static void line_3d(r3s_point *p1, r3s_point *p2)
{
   r3_draw_line(p1,p2);
}

// @TODO: add a "show all vGroups" mode, which just does brSelect_Group()!=0
static int _get_color_for_brush(editBrush *curBrush)
{
   BOOL isVb=vBrush_inCurGroup(curBrush);
   int col;

   //  Stuff for AI colouring
   //
   BOOL isAI = FALSE;
   ObjID rootAI;
   IObjectSystem* objSys;
   ITraitManager* traitMan;

   if ((curBrush!=cur_cursor)&&(isActiveHighlight(curBrush->br_id)))
      return brushColors[highlight1];   // someday want an array here of colors

   switch (brushGetType(curBrush))
   {
      case brType_HOTREGION:
         if (curBrush==cur_cursor)                       col=brushColors[areaCur];
         else if (isVb&&cur_cursor)                      col=brushColors[areaMulti];
         else if (brHot_IsMEONLY(curBrush))              col=brushColors[areaMeOnly];
         else if (brHot_GetBase(curBrush)==brHot_ACTIVE) col=brushColors[areaActive];
         else                                            col=brushColors[areaNorm];
         break;
      case brType_TERRAIN:
         if (curBrush==cur_cursor)                       col=brushColors[terrCur];
         else if (isVb&&cur_cursor)                      col=brushColors[terrMulti];
         else                                            col=brushSetDrawColor(curBrush,brush_pts);
         break;
      case brType_OBJECT:
         if (brObjProp_getLightLevel(brObj_ID(curBrush))<0.0)
         {
            if (curBrush==cur_cursor)                       col=brushColors[objCur];
            else if (isVb&&cur_cursor)                      col=brushColors[objMulti];
            else
            {
                if (config_is_defined ("hens_changes"))
                {
                    //	If object is an AI, make it a different colour
                    //
                    objSys = AppGetObj (IObjectSystem);
                    rootAI = IObjectSystem_GetObjectNamed(objSys, "AIs");
                    SafeRelease (objSys);
                    
                    if (rootAI != OBJ_NULL)
                    {
                        traitMan = AppGetObj (ITraitManager);
                        isAI = ITraitManager_ObjHasDonor(traitMan, brObj_ID(curBrush), rootAI);
                        SafeRelease (traitMan);
                        if (isAI)
                        {
                            col=brushColors[roomCur];	//	This happens to be purple
                            break;
                        }
                    }
                }
                col=brushColors[objNorm];
            }
            break;
         }  // otherwise fall though and treat it like a light....
      case brType_LIGHT:
         if (curBrush==cur_cursor)                       col=brushColors[lightCur];
         else if (isVb&&cur_cursor)                      col=brushColors[lightMulti];
         else                                            col=brushColors[lightNorm];
         break;
      case brType_FLOW:
         if (curBrush==cur_cursor)                       col=brushColors[flowCur];
         else if (isVb&&cur_cursor)                      col=brushColors[flowMulti];
         else                                            col=brushColors[flowNorm];
         break;
      case brType_ROOM:
         if (curBrush==cur_cursor)                       col=brushColors[roomCur];
         else if (isVb&&cur_cursor)                      col=brushColors[roomMulti];
         else                                            col=brushColors[roomNorm];
         break;
      default:
         col=Rand()&0xff;
         break;
   }
   return col;
}

#define POINT_SIZE 5

// should have a "get line draw" call

// actually draw the terrain brush, after setup has been called
void brushPrimalRender(editBrush *curBrush)
{
   void (*line_draw)(r3s_point *p1, r3s_point *p2);
   int edge_cnt=primFastUnsafe_EdgeCnt();
   r3s_point *p=brush_pts;
   int i, col;

   if (r3d_ccodes_and)
      return;

   col=_get_color_for_brush(curBrush);
   gr_set_fcolor(col);  // kind of super annoying to have to do both of these everywhere
   r3_set_color(col);   // does this set fcolor, if not, set that here too
   
   if (r3d_ccodes_or)  // @OPTIMIZE: inline all of this, or assemblize it?
      if (fastRender)
         line_draw=line_2d_clip;
      else
         line_draw=line_3d;
   else
      line_draw=line_2d_unclip;

   for (i=0; i<edge_cnt; i++)     // actually draw the brush
   {
//#define LOCAL_HACK
#ifdef LOCAL_HACK
      if (brSelect_Flag(curBrush)&brSelect_EDGE)
         if (i!=brSelect_Edge(curBrush))
            continue;
         else
            mprintf("Use %d and %d\n",primFastUnsafe_EdgePt(i,0),primFastUnsafe_EdgePt(i,1));
#endif
      if ((p[primFastUnsafe_EdgePt(i,0)].ccodes!=CLIP_ALL)&&
          (p[primFastUnsafe_EdgePt(i,1)].ccodes!=CLIP_ALL))
         (*line_draw)(&p[primFastUnsafe_EdgePt(i,0)],&p[primFastUnsafe_EdgePt(i,1)]);
   }

   // only HotRegions and Terrain can have any complex selections on them
   if ((brushGetType(curBrush)!=brType_HOTREGION)&&
       (brushGetType(curBrush)!=brType_TERRAIN))
      return;

   if (curBrush==cur_cursor)
   {
      r3_set_color(brushColors[terrFace]); gr_set_fcolor(brushColors[terrFace]);
      if (brSelect_Flag(curBrush)&brSelect_COMPLEX)
      {  // all complex brush (edge or points here)... test for now to be safe
         if (brSelect_Flag(curBrush)&brSelect_EDGE)
            (*line_draw)(&p[primFastUnsafe_EdgePt(brSelect_Edge(curBrush),0)],
                         &p[primFastUnsafe_EdgePt(brSelect_Edge(curBrush),1)]);
         else     // point case - for now, we know we are a cube
         {
            int pid=brSelect_Point(curBrush);
            if (pt_on_screen(&p[pid]))
               for (i=0; i<POINT_SIZE*POINT_SIZE; i++)
               {
                  grs_point tmp=p[pid].grp;
                  tmp.sx+=fix_make(((i%POINT_SIZE)-((POINT_SIZE-1)/2)),0);
                  tmp.sy+=fix_make(((i/POINT_SIZE)%POINT_SIZE)-((POINT_SIZE-1)/2),0);
                  gr_draw_point(brushColors[terrFace],(g2s_point *)&tmp);
               }
         }
      }  // in simple case, we either just draw, or draw as us
      else if (curBrush->cur_face!=-1)
      {  // have to show which the current face is
         int cur_edge_loop=0, edge_idx;
         primFastUnsafe_FaceEdgeSetup(curBrush->cur_face);
         while ((edge_idx=primFastUnsafe_FaceEdge(cur_edge_loop++))!=-1)
            (*line_draw)(&p[primFastUnsafe_EdgePt(edge_idx,0)],
                         &p[primFastUnsafe_EdgePt(edge_idx,1)]);
      }
   }
}

// dorky render a line code
// @OPTIMIZE: inline all of this, or assemblize it?
static BOOL brushLineRender(int color)
{
   r3s_point *p=brush_pts;

   if (r3d_ccodes_and)
      return FALSE;

   gr_set_fcolor(color);  // kind of super annoying to have to do both of these everywhere

   if (brush_pt_cnt==1)
      gr_point(p[0].grp.sx,p[0].grp.sy);
   else
      if (r3d_ccodes_or)     
         if (fastRender)
            line_2d_clip(&p[0],&p[1]);
         else
         {
            r3_set_color(color);
            line_3d(&p[0],&p[1]);
         }
      else
         line_2d_unclip(&p[0],&p[1]);
   return TRUE;
}

// starts a block, moves world points (world_pts) to view space (brush_pts)
void _brushWorldToView(void)
{
   r3d_ccodes_or=r3d_ccodes_and=0;
   if (!fastRender)
   {
      r3_start_block();      
      r3_transform_block(brush_pt_cnt,brush_pts,world_pts);
      if (isIso)
         r3d_ccodes_or|=CLIP_ALL; // for now, since we dont clip code iso right
   }
   else
      fastrenderTransformBlock(brush_pt_cnt,brush_pts,world_pts);
}

// moves the presetup points (obj_pts) to world space (world_pts)
// Unrolled to try and speed it up...
void _brushTransformToWorld(editBrush *br)
{
   int i;

   if (no_rotations&&fastRender)
      if (br->grid.grid_enabled)
         for (i=0; i<brush_pt_cnt; i++)
         {
            mx_add_vec(&world_pts[i],&obj_pts[i],&obj_offset);
            gedit_vertex_snap(&br->grid,&world_pts[i],&world_pts[i]);
         }
      else
         for (i=0; i<brush_pt_cnt; i++)         
            mx_add_vec(&world_pts[i],&obj_pts[i],&obj_offset);         
   else
   {
      if (br->grid.grid_enabled)
         for (i=0; i<brush_pt_cnt; i++)
         {
            r3_transform_o2w(&world_pts[i],&obj_pts[i]);
            gedit_vertex_snap(&br->grid,&world_pts[i],&world_pts[i]);
         }
      else
         for (i=0; i<brush_pt_cnt; i++)
            r3_transform_o2w(&world_pts[i],&obj_pts[i]);
      r3_end_object();
   }
}

// really just ends the block, for now
void _brushCleanandFinish(void)
{
   if (!fastRender)   
      r3_end_block();
}

///////////
// click distance code

#define INTERVAL_EPSILON (0.01)

// is val between unordered points e1 and e2
BOOL check_interval(float val, float e1, float e2)
{  // make sure we are in the interval... im sure this can be less dumb
   // @OPTIMIZE: it seems like you could just do some tests and muls and check sign
   if (e2>e1)  
    { if ((e1>val)||(e2<val)) return FALSE; }
   else        
    { if ((e1<val)||(e2>val)) return FALSE; }

#ifdef WITH_INTERVAL      
    { if ((e1>val+INTERVAL_EPSILON)||(e2<val-INTERVAL_EPSILON)) return FALSE; }
   else        // it seems like you could just do some tests and muls and check sign
    { if ((e1<val-INTERVAL_EPSILON)||(e2>val+INTERVAL_EPSILON)) return FALSE; }
#endif
   
   return TRUE;
}

// @OPTIMIZE: do a less braindead algorthim, and a less braindead calling convention
// for now, just use cramers rule, solve the system of equations
// then, check either coordinate to make sure we are "in" the line
// then compute distance squared from x0,y0 and x,y
float solve_2d_point_to_line(float x0, float y0, float x1, float y1, float x2, float y2)
{
   static float coeff[6]={0,-1,0,0,-1,0};
   float slope, invslope, x, y, detA;

   if (fabs(x2-x1)<INTERVAL_EPSILON)
      if (check_interval(y0,y1,y2))
         return (x0-x1)*(x0-x1);
      else
         return DISTANCE_INF;
   if (fabs(y2-y1)<INTERVAL_EPSILON)
      if (check_interval(x0,x1,x2))
         return (y0-y1)*(y0-y1);
      else
         return DISTANCE_INF;   
   slope=(y2-y1)/(x2-x1);  // rise over run, woo woo-woo
   invslope=-1.0/slope;
   coeff[0]=slope;    /* -1 */ coeff[2]=slope*x1-y1;     /*    m x - y =     m x1 - y1 */
   coeff[3]=invslope; /* -1 */ coeff[5]=invslope*x0-y0;  /*-(1/m)x - y = -(1/m)x0 - y0 */
   detA=-coeff[0]+coeff[3];  // since i know coeff[1] and [4] are always -1
   x=-coeff[2]+coeff[5];
   x/=detA;

   if (!check_interval(x,x1,x2))
      return DISTANCE_INF;         

   y=coeff[0]*coeff[5]-coeff[3]*coeff[2];
   y/=detA;

   return (x-x0)*(x-x0)+(y-y0)*(y-y0);
}

// get a float from a fix
#define get_float(fixv) (((float)fixv)/65536.0)

// so you can see what the function thought was the best fit
static int best_pt, best_edge;

//#define DBG_CLOSE_PTS
#ifdef DBG_CLOSE_PTS
#define cp_mprint(x) mprintf x
#else
#define cp_mprint(x)
#endif

// write some cool trivial reject based on largest axis plus center?

// check for nearest point
float findClosestPt(editBrush *curBrush, int x, int y)
{
   r3s_point *p=&brush_pts[0];
   float best_dist_sqr=DISTANCE_INF, dist;   // dist is for this computation
   int i, num_edge=primFastUnsafe_EdgeCnt();

   if (r3d_ccodes_and) return best_dist_sqr;
   
   best_pt=best_edge=-1;
   for (i=0; i<brush_pt_cnt; i++)   // first, we go through all the points
      if (pt_on_screen(&p[i]))
      {
         float dx=get_float(p[i].grp.sx)-x, dy=get_float(p[i].grp.sy)-y;
         dist=dx*dx+dy*dy;
         if (dist<best_dist_sqr)
         {
            best_dist_sqr=dist;
            best_pt=i;
            cp_mprint(("New best %d for brush %x (dist %g)\n",i,curBrush,dist));
         }
         else
            cp_mprint(("         %d further for brush %x\n",i,curBrush));
      }
      else cp_mprint(("Discard pt %d for brush %x\n",i,curBrush));

   // points are more interesting, so lie about the distance
   best_dist_sqr/=4;

   // next, the edges - for now i ignore lights, a more satisfying solution would be good, eh?
//   if (brushGetType(curBrush)!=brType_LIGHT)
      for (i=0; i<num_edge; i++)
      {
         int p1=primFastUnsafe_EdgePt(i,0), p2=primFastUnsafe_EdgePt(i,1);
         if (pt_on_screen(&p[p1])&&pt_on_screen(&p[p2]))
         {
            dist=solve_2d_point_to_line((float)x,(float)y,
                                        get_float(p[p1].grp.sx), get_float(p[p1].grp.sy),
                                        get_float(p[p2].grp.sx), get_float(p[p2].grp.sy));
            if (dist<best_dist_sqr)
            {
               best_dist_sqr=dist;
               best_pt=-1;
               best_edge=i;
               cp_mprint(("New edge %d (%d %d) for brush %x (dist %g)\n",i,p1,p2,curBrush,dist));
            }
            else
               cp_mprint(("    edge %d (%d %d) still for brush %x\n",i,p1,p2,curBrush));
            
         }
         else cp_mprint(("Discard pair %d %d for brush %x\n",p1,p2,curBrush));
      }
   
   return best_dist_sqr;
}

#pragma disable_message(202)
void brushGrowExtents(editBrush *curBrush, mxs_vector *bounds)
{
   int i,j;
   for (i=0; i<brush_pt_cnt; i++)
      for (j=0; j<3; j++)
      {
         if (world_pts[i].el[j]<bounds[0].el[j])
            bounds[0].el[j]=world_pts[i].el[j];
         if (world_pts[i].el[j]>bounds[1].el[j])
            bounds[1].el[j]=world_pts[i].el[j];
      }
}
#pragma enable_message(202)

// Basically, first index eliminates an number, and the 
// second index gives the other number
const int index_lookup[3][3] = 
{ { -1,  2,  1},
  {  2, -1,  0},
  {  1,  0, -1} };

// Ya down with OBB yeah you know me
//
// From "Fast Overlap Test for OBBs", by ???, courtesy of Doug
bool brushOBBIntersect(editBrush *b1, editBrush *b2)
{
   mxs_vector T;           // Translation vector between brushes
   mxs_vector L;           // Sparating Axis
   mxs_real T_L;           // Length of projection of T onto L
   mxs_angvec ang;         // Rotation between brushes
   mxs_matrix M, M_abs;    // Rotation matrix between brushes, and 
                           //  absolute value version
   mxs_matrix M_1, M_2;    // Rotation matrices for each brush
   mxs_real   sum;         // Accumulation of summation elements
   mxs_vector A[3], B[3];  // Box-aligned unit vectors
   mxs_vector u_temp[3];   // XYZ unit vectors 
   int i, j, k;

   // Calculate T vector and M matrix
   mx_sub_vec(&T, &b1->pos, &b2->pos); 
   for (i=0; i<3; ++i)
      ang.el[i] = b1->ang.el[i] - b2->ang.el[i];
   mx_ang2mat(&M, &ang);

   // Create unit vectors
   for (i=0; i<3; ++i)
      mx_unit_vec(&(u_temp[i]), i);

   // Rotate for OBB-aligned unit vectors
   mx_ang2mat(&M_1, &(b1->ang));
   for (i=0; i<3; ++i) 
      mx_mat_mul_vec(&(A[i]), &M_1, &(u_temp[i]));
   mx_ang2mat(&M_2, &(b2->ang));
   for (i=0; i<3; ++i) 
      mx_mat_mul_vec(&(B[i]), &M_2, &(u_temp[i]));

   // Calculate absolute valued M matrix
   for (i=0; i<9; ++i) 
      M_abs.el[i] = (M.el[i] > 0) ? M.el[i] : -M.el[i];

   // Use first brush's normals for separating axis generation
   for (i=0; i<3; ++i) {

      // Calculate distance between projected centers
      T_L = mx_dot_vec(&T, &(A[i]));

      // Calculate length of maximum radius of brush, projected
      sum = b1->sz.el[i];
      for (j=0; j<3; ++j)
         sum += b2->sz.el[j] * M_abs.vec[i].el[j];

      if (sum < T_L)
         return FALSE;
   }

   // Use second brush's normals for separating axis generation
   for (i=0; i<3; ++i) {
   
      // Calculate distance between projected centers
      T_L = mx_dot_vec(&T, &(B[i]));

      // Calculate length of maximum radius of brush, projected
      sum = b2->sz.el[i];
      for (j=0; j<3; ++j)
         sum += b1->sz.el[j] * M_abs.vec[j].el[i];

      if (sum < T_L)
         return FALSE;
   }

   // Use edge-pairs for separating axis generation
   for (i=0; i<3; ++i) {
      for (j=0; j<3; ++j) {
         
         // @OPTIMIZE: Somehow this reduces to 2 mults and a sub
         // Calculate distance between projected centers
         mx_cross_vec(&L, &(A[i]), &(B[j]));
         T_L = fabs(mx_dot_vec(&T, &L));

         // Calculate length of maximum radius of brush, projected
         sum = 0;
         for (k=0; k<3; k++) {
            if (k == i)  
               continue;
            sum += b1->sz.el[k] * M_abs.vec[index_lookup[i][k]].el[j];
         }

         for (k=0; k<3; k++) {
            if (k == j)
               continue;
            sum += b2->sz.el[k] * M_abs.vec[i].el[index_lookup[j][k]];
         }
      
         // If radius is smaller than the distance from center to center,
         //  then they must not intersect
         if (sum < T_L)
            return FALSE;
      }
   }

   // All separating axis tests fell through, must intersect
   return TRUE;
}


bool pointOBBIntersect(editBrush *point, editBrush *brush)
{
   mxs_vector u, norm;     // Pre-rotated and rotated normals
   mxs_matrix R;           // Rotation matrix of brush
   mxs_vector pt;          // Point on each plane of brush
   mxs_vector vec;         // Vector from point on plane to point
   int i;

   // Find rotation matrix for brush orientation
   mx_ang2mat(&R, &(brush->ang));

   // Iterate through each face of the brush
   for (i=0; i<6; ++i) {

      // Generate unit vector
      mx_unit_vec(&u, i%3);
      if (i >= 3)
         mx_scaleeq_vec(&u, -1);

      // Rotate to brush orientation
      mx_mat_mul_vec(&norm, &R, &u);

      // Find point on plane of face 
      mx_scale_add_vec(&pt, &(brush->pos), &norm, brush->sz.el[i%3]);

      // Check if given point is in front of face (outside brush)
      mx_sub_vec(&vec, &(point->pos), &pt);
      if (mx_dot_vec(&vec, &norm) > 0.0)
         return FALSE;
   }

   return TRUE;
}

BOOL brrendTypeFilterActive(void)
{
   return (editbr_filter&brFlag_FILTERMASK)!=brFlag_FILTERMASK;
}

BOOL brrendCheckTypeFilter(editBrush *curBrush)
{
   int filter_type;
   if (brushGetType(curBrush)==brType_OBJECT)
      filter_type=brObjProp_getFilterType(brObj_ID(curBrush));
   else
      filter_type=brFlag_GetFilterType(brushGetType(curBrush));
   return (filter_type&editbr_filter)!=0;
}

// Now allows rotation and does proper OBB intersection checking
bool applyFilters(editBrush *curBrush)
{
   int i;

   if (curBrush==NULL)       // we are too stupid
      return FALSE;
   if (curBrush==cur_cursor) // hey, its us, draw
      return TRUE;

   if (!brrendCheckTypeFilter(curBrush))
      return FALSE;
   if (renderHilightOnly&&!isActiveHighlight(curBrush->br_id))
      return FALSE;
   if (brushGetType(curBrush)==brType_HOTREGION) 
      return TRUE;           // for now, we always draw hotregions regardless of time/space
      
   // temporal filters first
   if (editbr_filter_time_hi||editbr_filter_time_lo)
   {
      int tm=blistCheck(curBrush);
      if (editbr_filter_time_lo && (tm<editbr_filter_time_lo))
         return FALSE;
      if (editbr_filter_time_hi && (tm>editbr_filter_time_hi))
         return FALSE;
   }

   if ((editbr_filter_size>0.01)&&(brushGetType(curBrush)==brType_TERRAIN)) 
      if ((curBrush->sz.x<editbr_filter_size)||
          (curBrush->sz.y<editbr_filter_size)||
          (curBrush->sz.z<editbr_filter_size))
         return FALSE;

   // and now hot region filters, with real OBB checking
   if (hot_cnt==0) return TRUE;
   for (i=0; i<hot_cnt; i++)
   {
      if (brushGetType(curBrush) == brType_LIGHT)
      {
         if (pointOBBIntersect(curBrush, hots[i]))
            return TRUE;
      }
      else if (brushOBBIntersect(curBrush, hots[i]))
         return TRUE;
   }

   return FALSE;
}

// dispatches to setup draw and clean based on brush type
bool brushDraw(editBrush *curBrush)
{
   if (!applyFilters(curBrush))
      return FALSE;
   switch (brushGetType(curBrush))
   {
   case brType_TERRAIN:
      brushTerrSetup(curBrush);
      break;
   case brType_HOTREGION:
   case brType_OBJECT:
   case brType_FLOW:
   case brType_ROOM:
      brushCubeSetup(curBrush);
      break;
   case brType_LIGHT:
      brushLightSetup(curBrush);
      break;
   default:
      return FALSE;
   }
   _brushTransformToWorld(curBrush);
   _brushWorldToView();
   brushPrimalRender(curBrush);
   _brushCleanandFinish();
   return TRUE;
}

// dispatches based on brush type, 
float brushClickCheck(editBrush *curBrush, int x, int y)
{
   float best_dist=DISTANCE_INF;
   if (!applyFilters(curBrush))
      return best_dist;
   switch (brushGetType(curBrush))
   {
   case brType_TERRAIN:
      brushTerrSetup(curBrush);
      break;
   case brType_HOTREGION:
   case brType_OBJECT:
   case brType_FLOW:      
   case brType_ROOM:
      brushCubeSetup(curBrush);
      break;
   case brType_LIGHT:
      brushLightSetup(curBrush);
      break;
   default:
      return INT_MAX;
   }
   _brushTransformToWorld(curBrush);
   _brushWorldToView();
   best_dist=findClosestPt(curBrush,x,y);
   _brushCleanandFinish();
   return best_dist;
}

static editBrush dummy_br;  // all zeros, so no grid enabled
BOOL brushLineDraw(mxs_vector *p1, mxs_vector *p2, int color)
{
   BOOL rv;
   // apply point filter here
   brushLineSetup(p1,p2);
   _brushTransformToWorld(&dummy_br);
   _brushWorldToView();
   rv=brushLineRender(color);
   _brushCleanandFinish();
   return rv; // returns FALSE if triv reject
}

float brushLineCheck(mxs_vector *p1, mxs_vector *p2, int x, int y)
{
   float best_dist=DISTANCE_INF;   
   // apply point filter here
   brushLineSetup(p1,p2);
   _brushTransformToWorld(&dummy_br);
   _brushWorldToView();
   best_dist=findClosestPt(&dummy_br,x,y);   
   _brushCleanandFinish();
   return best_dist;
}

BOOL brushSelectFancy(editBrush *curBrush, int x, int y)
{
   if (brushGetType(curBrush)!=brType_TERRAIN)
      return FALSE;
   if (brushClickCheck(curBrush,x,y)==INT_MAX)
   {
      Warning(("We were selected fancy, but no love\n"));
      return FALSE;
   }
   if (best_pt!=-1)
   {
      brSelect_Flag(curBrush)&=~brSelect_EDGE;
      curBrush->point=best_pt;
   }
   else if (best_edge!=-1)
   {
      brSelect_Flag(curBrush)|=brSelect_EDGE;
      curBrush->edge=best_edge;
   }
   else
   {
      Warning(("SelectFancy: Somehow no best point or edge\n"));
      return FALSE;
   }
   brSelect_Flag(curBrush)|=brSelect_COMPLEX;
   return TRUE;
}

BOOL brushCheckExtents(editBrush *curBrush, mxs_vector *bounds)
{
   if (!applyFilters(curBrush))
      return FALSE;
   switch (brushGetType(curBrush))
   {
   case brType_HOTREGION:
      if ((!brHot_IsMEONLY(curBrush))&&(brHot_GetBase(curBrush)==brHot_INACTIVE))
         return FALSE;
   case brType_OBJECT:
   case brType_FLOW:
   case brType_ROOM:
      brushCubeSetup(curBrush);
      break;
   case brType_TERRAIN:
      brushTerrSetup(curBrush);
      break;
   case brType_LIGHT:
      brushLightSetup(curBrush);
      break;
   default:
      return FALSE;
   }
   _brushTransformToWorld(curBrush);
   brushGrowExtents(curBrush,bounds);
   return TRUE;
}

// transform a brush to world space by itself
mxs_vector *brushBuildSolo(editBrush *us)
{
   r3_start_frame();
   brushTerrSetup(us);
   _brushTransformToWorld(us);
   r3_end_frame();
   return world_pts;
}

/// argh, what to do for fancy primals
static mxs_vector cube_src_uv[6][2] =
 { {  { 0, 1, 0 }, { 0, 0,-1 } },
   {  {-1, 0, 0 }, { 0, 0,-1 } },
   {  { 0,-1, 0 }, { 0, 0,-1 } },
   {  { 1, 0, 0 }, { 0, 0,-1 } },
   {  { 1, 0, 0 }, { 0, 1, 0 } },
   {  { 1, 0, 0 }, { 0,-1, 0 } } };

static mxs_vector wedge_src_uv[5][2] = 
{
   {{0,-1,1}, {-1,0,0}},
   {{1,0,0}, {0,1,0}},
   {{1,0,0}, {0,0,-1}},
   {{0,0,1}, {0,1,0}},
   {{0,-1,0,}, {0,0,-1}},
//   {{0,0,1}, {0,1,-1}},
//   {{0,-1,1}, {0,0,-1}},
};

// externed in gedit.c, of course
bool get_uv_align(editBrush *br, int face, mxs_vector *u, mxs_vector *v)
{
   //   if (br->primal_id!=1) return FALSE;  // check for cube somehow???
   r3_start_frame();
   fastRender=FALSE;
   brushTerrSetup(br);   // this is actually totally broken - if we are fastRenderer/norotations we lose
   if (config_is_defined("old_wedge_align"))
   {
      r3_rotate_o2w(u, &cube_src_uv[face][0]);
      r3_rotate_o2w(v, &cube_src_uv[face][1]);
   }
   else
   {
      switch (br->primal_id)
      {
      case PRIMAL_WEDGE_IDX:
         if (face < 5)
         {
            r3_rotate_o2w(u, &wedge_src_uv[face][0]);
            r3_rotate_o2w(v, &wedge_src_uv[face][1]);
         }
         break;

      default:
         if (face < 6)
         {
            r3_rotate_o2w(u, &cube_src_uv[face][0]);
            r3_rotate_o2w(v, &cube_src_uv[face][1]);
         }
         break;
      }
   }

   r3_end_object();
   r3_end_frame();
   return TRUE;
}

// for now we brutally cheat and assume old hotregion contexts and
//   brushSetupDraw data is good
// in real life, i suppose we should not do that, oh well
void brushRunOnActive(void (*cback)(editBrush *me))
{
   editBrush *us, *tmp;
   int hIter;
   brushSetupDraw(brFlag_COLOR_NONE,NULL);
   r3_start_frame();
   us=blistIterStart(&hIter);
   while (us)
   {
      tmp=us;
      us=blistIterNext(hIter);
      if (applyFilters(tmp))   // if we is in the scene
         (*cback)(tmp);        //   rock us
   }
   r3_end_frame();
}

// maybe really should have its own local context?
void brushConvertStart(void)
{
   brushSetupDraw(brFlag_COLOR_NONE,NULL);
   r3_start_frame();
}

void brushConvertEnd(void)
{
   r3_end_frame();
}

BOOL brushConvertTest(editBrush *br)
{
   return applyFilters(br);
}

// why arent these in a header file?  hello? sean? hello?
extern void *PortalMakeEdge(BspVertex *a, BspVertex *b);
extern void *PortalMakePolyhedron(void);
extern void *PortalMakePolygon(void *ph);
extern void *PortalAddPolygonEdge(void *poly, void *edge, void *old_edge);

bool passive_hotregions=FALSE;

// are we part of the deal (tm)
static BOOL validHotRegion(editBrush *br)
{
   int i;
   if (passive_hotregions) return FALSE;
   if (hot_cnt==0) return FALSE;
   if (brushGetType(br)!=brType_HOTREGION)
      return FALSE;
   for (i=0; i<hot_cnt; i++)
      if (hots[i]==br)
         return TRUE;
   return FALSE;
}

// i hate this
// convert from internal format to newer CSG format
void *brushConvertPortal(editBrush *eb)
{
   int edge_cnt, face_cnt, i;          // temps for edge and face cnt, loop var
   BspVertex pts[MAX_PRIMAL_PTS];      // should we malloc this stuff, as opposed to putting it on the stack?
   void *first_usr[MAX_PRIMAL_EDGES];  // keep track of first user in poly of this edge, since CSG database is stupid 
   void *poly_edge[MAX_PRIMAL_EDGES];  // get the PolyEdge data from the CSG database
   void *ph;

   // do type testing here... active hotregion, other wackiness
   if (!((brushGetType(eb)==brType_TERRAIN)||validHotRegion(eb)))
      return NULL;

   if ((eb->sz.x<=0)||(eb->sz.y<=0)||(eb->sz.z<=0)) return NULL;
   if (!applyFilters(eb)) return NULL;

#ifdef PORTAL_CONVERT_USE_DOUBLES
   brushTerrFancyDoubleSetupNTransform(eb);
   for (i=0; i<brush_pt_cnt; i++)
    { pts[i].x=db_world_pts[i].x; pts[i].y=db_world_pts[i].y; pts[i].z=db_world_pts[i].z; }
#else   
   brushTerrSetup(eb);
   _brushTransformToWorld(eb);
   for (i=0; i<brush_pt_cnt; i++)
    { pts[i].x=world_pts[i].x; pts[i].y=world_pts[i].y; pts[i].z=world_pts[i].z; }
#endif

   edge_cnt=primFastUnsafe_EdgeCnt();
   face_cnt=primFastUnsafe_FaceCnt();
   
   memset(first_usr,0,sizeof(int)*edge_cnt);  // first_usr's all start at 0
   for (i=0; i<edge_cnt; i++)   // go setup all the inital edges
      poly_edge[i]=PortalMakeEdge(&pts[primFastUnsafe_EdgePt(i,0)],
                                  &pts[primFastUnsafe_EdgePt(i,1)]);
         
   ph = PortalMakePolyhedron();
   for (i=0; i<face_cnt; i++)
   {  // generate the polyhedron
      int cur_edge_loop=0, edge_idx;
      void *face = PortalMakePolygon(ph);
      primFastUnsafe_FaceEdgeSetup(i);
      while ((edge_idx=primFastUnsafe_FaceEdge(cur_edge_loop++))!=-1)
      {
         void *new_edge_user=PortalAddPolygonEdge(face, poly_edge[edge_idx], first_usr[edge_idx]);
         if (first_usr[edge_idx]==0)
            first_usr[edge_idx]=new_edge_user;
      }
   }
   
   return ph;
}
