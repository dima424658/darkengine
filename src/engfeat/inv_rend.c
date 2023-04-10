/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/inv_rend.c,v 1.47 2000/02/19 13:15:07 toml Exp $
// code to draw the current inventory object
// more generally, code to support the issue of rendering models to various canvases
// caching how they look, and so on

// massively in progress, particularly:
//   do we want to allow hand draw models as well for things which look goofy
//   really need to mip map all this
//   do we want a way to have a "current" one which rotates somehow

#include <g2.h>
#include <font.h>
#include <r3d.h>
#include <md.h>
#include <mdutil.h>
#include <matrix.h>
#include <mlight.h>
#include <string.h>
#include <lgd3d.h>
#include <scrnman.h>

#include <mprintf.h>
#include <appagg.h>
#include <cfgdbg.h>

#include <wrobj.h>
#include <objmodel.h>
#include <objshape.h>

#include <property.h>
#include <invrndpr.h>
#include <propbase.h>

#include <scrnman.h>
#include <storeapi.h>
#include <resapi.h>
#include <imgrstyp.h>
#include <palrstyp.h>
#include <palmgr.h>

#include <config.h>

#include <mnumprop.h>
#include <inv_rend.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// internal flag bits, public ones are in inv_rend.h
#define INVREND_BITMAP           (1<<8)
#define INVREND_MODEL            (1<<9)
#define INVREND_CACHEVALID       (1<<10)     // currently unsupported
#define INVREND_IN_QUEUE         (1<<11)
#define INVREND_FREE_ON_RENDER   (1<<12)

#define INVREND_HARDWARE (INVREND_HARDWARE_QUEUE|INVREND_HARDWARE_IMMEDIATE)
#define INVREND_TYPE (INVREND_BITMAP|INVREND_MODEL)

// so we can flag having no model info
#define INV_NO_MODEL (-1)


// internal structure, we dont expose details of this
struct _invRendState {
   struct sInvRendView;
   ObjID obj_id;            // obj_id for us
   int   model_idx;         // what model idx i have
   int   bitmappalnum;      //palette location, if a bitmap resource
   IRes *pBMRes;            // Bitmap resource, if not a model
   short flags;             // any flags
   grs_canvas *back_buffer; // app managed back buffer; if NULL, render to vis_buffer
   grs_canvas vis_buffer;   // visible buffer; sub canvas of screen canvas specified by draw_rect.
   Rect draw_rect;
   sInvRenderType rendtype;
};

// the override how i draw properties
static IInvRenderProperty* gpInvRendProp = NULL;

// The path for finding bitmaps
#define BITMAP_PATH "obj\\"
static ISearchPath *gpInvRendBitmapPath = NULL;

static float gRenderZoom = 4.0;

/////////////////
// actual renderer

EXTERN BOOL g_lgd3d;
EXTERN BOOL g_null_raster;
int g_inv_bcolor = -1;


static mls_multi_light lights[] =
{
#ifdef RGB_LIGHTING //zb :
    // primary, ~30 degrees clockwise from x axis, 100+ feet away
    {  { 85.0, 50.0, 20.0 }, { 0.0 }, { 100.0, 100.0, 100.0 },  },
        // secondary, behind object but not opposite
    {  { -50.0, -85.0, -10.0 }, { 0.0 }, { 30.0, 30.0, 30.0 },  },
#else //zb

    // primary, ~30 degrees clockwise from x axis, 100+ feet away
    {  { 85.0, 50.0, 20.0 }, { 0.0 }, 100.0,  },
    // secondary, behind object but not opposite
    {  { -50.0, -85.0, -10.0 }, { 0.0 }, 30.0,  },
#endif //zb
};

//zb:
static void start_ir_stride(void)
{
#ifdef RGB_LIGHTING
   if( g_lgd3d )
   {
      r3d_glob.cur_stride = sizeof(g2s_point); // for
      mdd_rgb_lighting = TRUE;
      mld_multi_rgb  = TRUE;
      return;
   }
   mld_multi_rgb  = FALSE;
#endif
   mdd_rgb_lighting = FALSE;
   r3d_glob.cur_stride = sizeof(r3s_point);
}

//zb:
static void end_ir_stride(void)
{
   r3d_glob.cur_stride = sizeof(r3s_point);
   mld_multi_rgb = FALSE;
}

#define NUM_LIGHTS (sizeof(lights)/sizeof(lights[0]))

// if there is a back buffer, render there, else render to vis_buffer
static void render_single_obj(invRendState *us, BOOL bClearZBuffer)
{
   int clr_color=g_inv_bcolor;
   mxs_real zoom = gRenderZoom;

#ifdef DEEPC
#ifndef SHIP
   // Objects with 32-bit textures would kill us in software rendering.
   if (!g_lgd3d)
      return;
#endif
#endif

   if (us->back_buffer)
      gr_push_canvas(us->back_buffer);
   else
      gr_push_canvas(&us->vis_buffer);

   if ((g_lgd3d) && (!g_null_raster))
   {
      if ((us->flags & INVREND_HARDWARE)&&(us->flags & INVREND_MODEL))
      {
         // clear the zbuffer for HW
         lgd3d_set_offsets(us->draw_rect.ul.x, us->draw_rect.ul.y);
         if( bClearZBuffer )
            lgd3d_clear_z_rect(0, 0, grd_bm.w, grd_bm.h);
      }
   }

   if ((us->flags & INVREND_CLEAR) && (clr_color == -1))
      clr_color = 0;

   if (clr_color!=-1)
      gr_clear(clr_color);

   if (us->flags&INVREND_MODEL)
   {
      // first, pick camera location correctly
      mxs_vector cam_base={0,0,0};
      mxs_vector cam_loc, obj_loc={0,0,0};
      mxs_angvec cam_ang;
      mxs_matrix cam_mat;
      mds_model  *m;
      mls_multi_light tmplights[NUM_LIGHTS];

      // check that it's actually an md model
      // MAHK 7/30: this is basically here to catch mesh models
      if(objmodelGetModelType(us->model_idx)!=OM_TYPE_MD)
      {
         ConfigSpew("InvrendSpew",("render_single_obj: unable to render non-md inventory item\n"));
         gr_pop_canvas();
         return;
      }


      cam_base.el[0]=us->cam_dist*zoom;
      if (us->flags&INVREND_DISTANT)
         cam_base.el[0]*=2.00;

      mx_ang2mat(&cam_mat,&us->off_ang);
      mx_mat_mul_vec(&cam_loc,&cam_mat,&cam_base);
      cam_ang=us->off_ang;
      cam_ang.el[2]+=0x8000;   // does this work?
      cam_ang.el[1]=-cam_ang.el[1];

      // orbit the lights with the camera
      {
         int i;
         for (i = 0; i < NUM_LIGHTS; i++)
         {
#ifdef RGB_LIGHTING
            tmplights[i].bright.x = us->lighting*lights[i].bright.x;
            tmplights[i].bright.y = us->lighting*lights[i].bright.y;
            tmplights[i].bright.z = us->lighting*lights[i].bright.z;
#else
            tmplights[i].bright.x = us->lighting*lights[i].bright.x;
            tmplights[i].bright.y = us->lighting*lights[i].bright.y;
            tmplights[i].bright.z = us->lighting*lights[i].bright.z;
#endif
            mx_mat_mul_vec(&tmplights[i].loc,&cam_mat,&lights[i].loc);
         }
      }

      // now start up the 3d
      r3_start_frame();
      r3_set_zoom(zoom);

      r3_set_view_angles(&cam_loc,&cam_ang,R3_DEFANG);

      // @TODO: THIS NEEDS TO BECOME A CALL TO RENDOBJ, OR DOES IT?
      r3_start_object(&obj_loc);

      // @TODO: would like to try to do the DIMMED control
      md_set_render_light(TRUE);
//     md_set_render_light(FALSE);

      // render the darn thing
      if ((m=(mds_model *)objmodelGetModel(us->model_idx))!=NULL)
      {  // get real joint parms, colors?
         extern mds_parm *getRendParms(ObjID obj, mds_model *m);
         mds_parm *parms=NULL;  // this should all be in a function call!

         if (m->parms)
            parms=getRendParms(us->obj_id,m);
         md_mat_colors_set(m);
         mld_multi_hilight = 0;
         mld_multi_ambient = 0.1;
         mld_multi_ambient_only = FALSE;
         mld_multi_unlit = FALSE;
         ml_multi_set_lights_for_object(NUM_LIGHTS,tmplights,&obj_loc,2.0);
         objmodelSetupMdTextures(us->obj_id,m,us->model_idx);
// wsf: why was md_fancy_render_model being called twice?
//         md_fancy_render_model(m,parms);

		 start_ir_stride();//zb
         md_fancy_render_model(m,parms);
		 end_ir_stride(); //zb
      }

      r3_end_object();      // and close down the frame
      r3_end_frame();
   }
   else if (us->flags&INVREND_BITMAP)
   {
      grs_bitmap *bm = IRes_Lock(us->pBMRes);
      bm->align = us->bitmappalnum;
      gr_bitmap(bm,0,0);
      IRes_Unlock(us->pBMRes);
   }
   else
      Warning(("No modelNum for inv obj %d\n",us->obj_id));

   gr_pop_canvas();
}

////////////////
// cache/canvas controller
static void _invInitCamera(invRendState *us)
{
   if (us->flags&INVREND_MODEL)
   {
      mds_model *m;
      us->off_ang.el[0]=us->off_ang.el[2]=0;
      us->off_ang.el[1]=0xf000;
      us->cam_dist=3.0;  // hopefully this gets fixed below
      if ((m=(mds_model *)objmodelGetModel(us->model_idx))!=NULL)
         us->cam_dist=m->radius+0.7;
   }  // set scale factor here, i guess
}

static void _invGetModelIdx(invRendState *us)
{
   int new_idx;

   us->model_idx=INV_NO_MODEL;    // so we have a value if nothing works
     us->bitmappalnum=0;          // so if we aren't a bitmap, this is set
                                  // correctly

   switch (us->rendtype.type)
   {

      case kInvRenderModel:
         if ((new_idx=objmodelLoadModel(us->rendtype.resource.text))!=MIDX_INVALID)
         {
            us->model_idx=new_idx;
            objmodelIncRef(us->model_idx);
            us->flags|=INVREND_MODEL;
         }
         break;

      case kInvRenderBitmap:
      {
         IResMan *pResMan = AppGetObj(IResMan);
         IRes *new_pRes = IResMan_Bind(pResMan,
                                       us->rendtype.resource.text,
                                       RESTYPE_IMAGE,
                                       gpInvRendBitmapPath,
                                       NULL,
                                       0);
         if (new_pRes)
         {
            IRes *pPallRes = IResMan_Retype(pResMan,
                                       new_pRes,
                                       RESTYPE_PALETTE,
                                       0);


            us->pBMRes=new_pRes; //ok, so we send this resource over to "us"
                                 //presumably it gets released later.  
            us->flags|=INVREND_BITMAP;



            if (pPallRes)
               {
                 void *pPall = IRes_Lock(pPallRes);
                 if (us->bitmappalnum==0)
                   {
                     us->bitmappalnum = palmgr_alloc_pal(pPall);
                   }
                 IRes_Unlock(pPallRes);
                 IRes_Drop(pPallRes);
                 SafeRelease(pPallRes);
               }
         }
         SafeRelease(pResMan);
         break;
      }

      case kInvRenderDefault:
         if (ObjGetModelNumber(us->obj_id,&us->model_idx))
            us->flags|=INVREND_MODEL;  // just use the default world model for us
         break;
   }
   if ((us->model_idx==INV_NO_MODEL) && (!us->pBMRes))
      Warning(("No Inventory Model IDX for %d",us->obj_id));
}

static void _invFreeModelIdx(invRendState *us)
{

   switch (us->rendtype.type)
   {
      case kInvRenderModel:
         if (us->model_idx!=INV_NO_MODEL)
            objmodelDecRef(us->model_idx);
         break;
      case kInvRenderBitmap:
	 if (us->bitmappalnum!=0)
	   {
	     palmgr_release_slot(us->bitmappalnum);
	     us->bitmappalnum=0;
	   }
         if (us->pBMRes)
            SafeRelease(us->pBMRes);
         break;
   }
}

static void invrend_init_state(invRendState* us, int flags, const Rect *r, grs_canvas *draw_cnv)
{
   grs_canvas *screen_canvas = ScrnGetDrawCanvas();

   us->flags=flags&INVREND_USERFLAGS;

   if (r!=NULL)
      us->draw_rect = *r;
   else {
      us->draw_rect.ul.x = 0;
      us->draw_rect.ul.y = 0;
      us->draw_rect.lr.x = screen_canvas->bm.w;
      us->draw_rect.lr.y = screen_canvas->bm.h;
   }

   gr_init_sub_canvas(screen_canvas, &us->vis_buffer,
      us->draw_rect.ul.x, us->draw_rect.ul.y,
      RectWidth(&us->draw_rect), RectHeight(&us->draw_rect));

   _invGetModelIdx(us);
   _invInitCamera(us);

   us->back_buffer=draw_cnv;

   us->lighting = 1.0;
}

// need to look up inv_rend property here!
invRendState *invRendBuildState(int flags, ObjID o_id, const Rect *r, grs_canvas *draw_cnv)
{
   invRendState *us=(invRendState *)Malloc(sizeof(invRendState));
   sInvRenderType defprop = { kInvRenderDefault };
   sInvRenderType* rendprop = &defprop;

   PROPERTY_GET(gpInvRendProp,o_id,&rendprop);
   us->rendtype = *rendprop;
   us->obj_id = o_id;

   invrend_init_state(us, flags, r, draw_cnv);
   return us;
}


// need to look up inv_rend property here!
invRendState *invRendBuildStateFromType(int flags, sInvRenderType* type, const Rect *r, grs_canvas *draw_cnv)
{
   invRendState *us=(invRendState *)Malloc(sizeof(invRendState));

   us->obj_id = OBJ_NULL;
   us->rendtype = *type;

   invrend_init_state(us, flags, r, draw_cnv);
   return us;
}


void invRendFreeState(invRendState *us)
{
   if (us->flags & INVREND_IN_QUEUE) {
      us->flags |= INVREND_FREE_ON_RENDER;
      return;
   }
   gr_close_canvas(&us->vis_buffer);
   _invFreeModelIdx(us);
   Free(us);
}


// set canvas
void invRendSetCanvas(invRendState* us, grs_canvas* canv)
{
   if (canv)
      us->back_buffer=canv;
}

// set rect
void invRendSetRect(invRendState* us, const Rect *r)
{
   if (r!=NULL) {
      us->draw_rect = *r;
      gr_close_canvas(&us->vis_buffer);
      gr_init_sub_canvas(ScrnGetDrawCanvas(), &us->vis_buffer, r->ul.x, r->ul.y, RectWidth(r), RectHeight(r));
   }
}

void invRendSetType(invRendState* us, const sInvRenderType* type)
{
   // if same, just exit
   if (us->rendtype.type == type->type)  // slight overuse of "type"
   {
      if (type->type == kInvRenderDefault
          || strncmp(us->rendtype.resource.text,
                     type->resource.text,
                     sizeof(us->rendtype.resource.text)) == 0)
      return;
   }
   _invFreeModelIdx(us);
   us->rendtype = *type;
   _invGetModelIdx(us);
}

// this is the big mess, having to deal with changing whether im now hilight, if my canvas changes, so on
void invRendUpdateState(invRendState *us, int flags, ObjID o_id, const Rect *r, grs_canvas *draw_cnv)
{
   if ((o_id!=us->obj_id)&&(o_id!=OBJ_NULL))
   {
      _invFreeModelIdx(us);
      us->obj_id=o_id;
      us->rendtype.type = kInvRenderDefault;
      _invGetModelIdx(us);
      _invInitCamera(us);
   }
   invRendSetRect(us,r);
   invRendSetCanvas(us,draw_cnv);
   if (flags&INVREND_SET)
   {
#ifdef SUPPORT_CACHE
      if ((flags&INVREND_CACHEPICTURE)!=(us->flags&INVREND_CACHEPICTURE))
         Warning(("Hey! cant update state and change from double buffered to not in inv_rend\n"));
      us->flags&=~INVREND_CACHEVALID;
#endif
      us->flags=(us->flags&(~INVREND_USERFLAGS))|(flags&INVREND_USERFLAGS);
   }
}

#define QUEUE_SIZE 25
static invRendState *objQueue[QUEUE_SIZE];
static int numObjs=0;

///////////////////
// draw/update control
// Note: we assume draw canvas is locked when this is called.
void invRendDrawUpdate(invRendState *us)
{
   if (g_lgd3d) {
      switch (us->flags & (INVREND_HARDWARE|INVREND_TYPE))
      {
      case INVREND_HARDWARE_QUEUE|INVREND_MODEL:
         // queue object for hardware accelerated rendering...
         if (numObjs >= QUEUE_SIZE) {
            Warning(("invRendDrawUpdate(): too many objects to queue.\n"));
         } else if (us->back_buffer != NULL) {
            Warning(("Can't hardware accelerate object rendered off screen.\n"));
         } else {
            objQueue[numObjs++] = us;
            us->flags |= INVREND_IN_QUEUE;
         }
		   break;
      case INVREND_HARDWARE_IMMEDIATE|INVREND_MODEL:
         if (us->back_buffer != NULL) {
            Warning(("Can't hardware accelerate object rendered off screen.\n"));
            render_single_obj(us, TRUE);
         } else {
            extern int rendloop_counter;
            BOOL bOverlaysWereOn; //zb

            // @TODO really we want a ScrnBreakLock / ScrnRestoreLock here...
            ScrnUnlockDrawCanvas();

            lgd3d_start_frame(rendloop_counter);
            bOverlaysWereOn = lgd3d_overlays_master_switch( FALSE ); //zb

            render_single_obj(us, TRUE);
            lgd3d_set_offsets(0,0);
            lgd3d_end_frame();

            if( bOverlaysWereOn ) //zb
                lgd3d_overlays_master_switch( TRUE ); //zb

            ScrnLockDrawCanvas();
         }
         break;
      default:
         render_single_obj(us, TRUE);
         break;
      }
   } else
      render_single_obj(us, TRUE);

   if (us->flags&INVREND_ROTATE)
      us->off_ang.el[2]+=0x0100;
   //   us->off_ang.el[1]+=0x100;  // if you want wack-a-tronic implementation
}

void invRendSetView(invRendState* us, const sInvRendView* view)
{
   *(sInvRendView*)us = *view;
}

// Flush objects queued for hardware rendering
// @TODO: these are a frame behind similar overlay state
void invRendFlushQueue(void)
{
   int            i;
   grs_canvas*    p_screen_canvas;

   if (numObjs == 0)
      return;
   
   // Clear the whole z-buffer
   p_screen_canvas = ScrnGetDrawCanvas();

   lgd3d_set_offsets(0,0);
   lgd3d_clear_z_rect(0, 0, p_screen_canvas->bm.w, p_screen_canvas->bm.h);


   for (i=0; i<numObjs; i++) 
   {
      invRendState *us=objQueue[i];
   
      render_single_obj( us, FALSE );

      us->flags &= ~INVREND_IN_QUEUE;
      if (us->flags & INVREND_FREE_ON_RENDER)
         invRendFreeState(us);
   }
   lgd3d_set_offsets(0,0);
   numObjs = 0;
}

void invRendGetView(const invRendState* us, sInvRendView * view)
{
   *view = *(sInvRendView*)us;
}

////////////////
// build our properties here


void invRendInit(void)
{
   IResMan *pResMan = AppGetObj(IResMan);

   gpInvRendProp = CreateInvRenderProp();
   gpInvRendBitmapPath = IResMan_NewSearchPath(pResMan, BITMAP_PATH);

   config_get_float("inv_model_zoom",&gRenderZoom);

   SafeRelease(pResMan);
}

void invRendFreeQueue(void)
{
   int i;

   // Flush queue and free states as appropriate...
   for (i=0; i<numObjs; i++) {
      invRendState *us = objQueue[i];
      us->flags &= ~INVREND_IN_QUEUE;
      if (us->flags &INVREND_FREE_ON_RENDER)
         invRendFreeState(us);
   }
   numObjs = 0;
}

void invRendTerm(void)
{
   invRendFreeQueue();
   SafeRelease(gpInvRendProp);
   SafeRelease(gpInvRendBitmapPath);
}
