/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendloop.c,v 1.95 2000/02/18 17:25:53 MAT Exp $

#include <lg.h>
#include <loopapi.h>

// Messages
#include <dispbase.h>
#include <loopmsg.h>

// Loops
#include <rendloop.h>
#include <simloop.h>
#include <scrnloop.h>
#include <objloop.h>
#include <wrloop.h>

// Stuff I need to do my shit
#include <font.h>
#include <g2.h>
#include <r3d.h>
#include <camera.h>
#include <wr.h>
#include <portal.h>
#include <animlit.h>
#include <weather.h>
#include <corona.h>
#include <guistyle.h>
#include <timer.h>
#include <simtime.h>
#include <playrobj.h>
#include <rendobj.h>
#include <lgsprntf.h>
#include <objlight.h>
#include <config.h>
#include <simstate.h>
#include <simflags.h>
#include <lgd3d.h>
#include <mm.h>
#include <mprintf.h>
#include <dbfile.h>

#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>

#include <inv_rend.h>
#include <media.h>
#include <objmodel.h>
#include <objpos.h>
#include <scrnman.h>
#include <starhack.h>
#include <rndflash.h>
#include <meshtex.h>

#include <netman.h>
#include <inetstat.h>

#include <scrnovls.h>
#include <scrnstr.h>
#include <render.h>
#include <rendprop.h>
#include <missrend.h>

#include <dbasemsg.h>
#include <vismsg.h>
#include <textarch.h>
#include <h2ocolor.h>
#include <pgrpprop.h>
#include <pgroup.h>
#include <plyrmode.h>

#include <skyrend.h>

// For spiffy remote camera stuff:
#include <camovl.h>

#ifdef PLAYTEST
#include <mprintf.h>
#endif // PLAYTEST

// This should usually be commented out.
//#define CTIMER_ON
#include <ctimer.h>

#include <dbmem.h>

int player_light;
BOOL g_render_on_begin_frame = TRUE;

/////////////////////////////////////////////////////////////
// RENDER LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_FACTORY RenderLoopFactory
#define MY_GUID   LOOPID_Render

// My context data
typedef void Context;

// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
} StateRecord;

//////////////////////////////
// Draw some stupid stats on the screen
//
// @TODO: move this out to its own file.  (own client?)

#ifdef PLAYTEST
#ifdef NEW_NETWORK_ENABLED
INetStats *g_pNetStats = NULL;
INetManager *g_pNetMan = NULL;
#endif

bool show_stats      = FALSE;
bool show_full_stats = TRUE;
bool time_stats      = FALSE;
bool tmgr_stats      = FALSE;

#define FRAMEHISTORY_COUNT (64)
static int dtlist[FRAMEHISTORY_COUNT];
static int dtlist_ptr=0, dtsum=0;
int portal_info_volume = 10;

// clear bkgnd when not rendering...
static int hack_string(char *s, int x, int y)
{
   short w,h;
   extern BOOL g_null_raster;
   extern BOOL lgd3d_punt_d3d;

   if (g_null_raster||lgd3d_punt_d3d) {
      long fc_save = gr_get_fcolor();
      gr_string_size(s, &w, &h);
      gr_set_fcolor(0);
      gr_rect(x,y,x+w,y+h);
      gr_set_fcolor(fc_save);
   } else
      h = 9;
   gr_string(s, x, y);
   return h;
}

static void superimpose_stats (int dt, int overhead, int blit_overhead)
{
   char *p;                     // info string
   int x = 2;                   // location to draw
   int y = 2;
   char text[256];
   char curfile[128];
   float rate;
   Position *player_pos;
   ObjID player;

   dtsum-=dtlist[dtlist_ptr];
   dtlist[dtlist_ptr]=dt+overhead;
   dtsum+=dt+overhead;
   dtlist_ptr=(dtlist_ptr+1)%FRAMEHISTORY_COUNT;

   rate = 1000.0 / dt;

   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG)); // StyleColorWhite?

   if (show_full_stats)
      while ((p = portal_scene_info (portal_info_volume)) != 0)
      {
         y += hack_string ( p, x, y);
      }
#ifdef NEW_NETWORK_ENABLED
   if (INetMan_Networking(g_pNetMan))
   {
      ulong f_sent, f_rcv, t_sent, t_rcv, m_sent=0, m_rcv=0;
      float seconds;
      char *f_culprit, *t_culprit;
      INetStats_FrameStats(g_pNetStats, &f_sent, &f_rcv, &f_culprit);
      INetStats_BandwidthStats(g_pNetStats, &seconds, &t_sent, &t_rcv, &t_culprit);
      INetStats_MaxStats(g_pNetStats, &m_sent, &m_rcv);
      sprintf(text, "net sent %4d rcv %4d", f_sent, f_rcv);
      y += hack_string(text, x, y);
      sprintf(text, "  %3.1fs %4d/%4d max %4d/%4d", seconds, t_sent, t_rcv, m_sent, m_rcv);
      y += hack_string(text, x, y);
      if (strnicmp(t_culprit,"None",4)!=0)
      {
         sprintf(text, "  culprit %s", t_culprit);
         y += hack_string(text, x, y);
      }
   }
#endif

   sprintf (text, "ms: %d; fps: %.1f", dt, rate);
   y += hack_string ( text, x, y);
   lg_sprintf (text, "sim ms: %d blit ms: %d", overhead-blit_overhead, blit_overhead);
   y += hack_string ( text, x, y);
   if (dtsum)
   {
      sprintf (text, "actual fps: %.1f avg:%.1f",
               1000.0 / (dt+overhead), 1000.0 / (dtsum/(float)FRAMEHISTORY_COUNT) );
      y += hack_string (text, x, y);
   }

   player = PlayerObject();
   if (player) {
      player_pos = ObjPosGet(player);
      dbCurrentFile(curfile, sizeof(curfile));
      sprintf (text, "at %5.3g %5.3g %5.3g (%s)", player_pos->loc.vec.x,
               player_pos->loc.vec.y, player_pos->loc.vec.z, curfile);
      y += hack_string(text, x, y);
   }

   guiStyleCleanupFont(NULL,StyleFontNormal);
}

// Hack for getting rendering time into histogram profiler
extern ulong frame_time;

static void do_stats(ulong time, ulong overhead, ulong blit_overhead)
{
   frame_time = time;

   if (show_stats)
      superimpose_stats (time, overhead, blit_overhead);
   // @TODO: fix time stats
   if (tmgr_stats) {
      mono_setxy(0, 0);
      if (g_tmgr)
         g_tmgr->stats();
   }
   if (time_stats)
      mprintf("r:%d\n",time);
}

#else // !PLAYTEST
#define do_stats(time, overhead, blit_overhead)
#endif

////////////////////////////////////////
//
// Frame handler
//

#ifdef PLAYTEST
#define FRAME_TIMINGS
#endif

#ifdef FRAME_TIMINGS
// for measuring overhead time between frames
static ulong start_frame_time=0, end_frame_time=0;  // when did we see the startFrame and endFrame messages
static ulong start_rend_time=0, end_rend_time=0, end_last_rend_time=0;  // when did we start render, end render
                                                    // and what was end render last frame
#define FrameTimeSet(var,val) var=val
#else
#define FrameTimeSet(var,val)
#endif

float game_zoom_factor = 1.0;

// externed for inv_rend.c, Xemu 7/22/98
int rendloop_counter=0;
static void start_frame(void)
{
   FrameTimeSet(start_frame_time,tm_get_millisec_unrecorded());
   porthw_start_frame(++rendloop_counter);
}

extern BOOL g_lgd3d;
extern void rendobj_finish_object_rendering(void);
extern BOOL g_zbuffer;

static void Flash(float alpha, ulong flash_rgb)
{
   if (alpha <= 0.0)
      return;

   if (g_zbuffer) {
      lgd3d_set_zwrite(FALSE);
      lgd3d_set_zcompare(FALSE);
   }

   lgd3d_disable_palette();
   lgd3d_set_blend(TRUE);
   lgd3d_set_alpha(alpha);
   lgd3d_clear(flash_rgb);
   lgd3d_set_alpha(1.0);
   lgd3d_set_blend(FALSE);
   lgd3d_enable_palette();
   if (g_zbuffer) {
      lgd3d_set_zwrite(TRUE);
   }
   // tell r3d that we've changed driver state:
   r3d_do_setup = TRUE;
}

BOOL rendloop_clear;

static void composite(float *alpha, uchar *rgb, float under_alpha, const uchar *under_rgb)
{
   // composite two non-premultiplied alpha & rgb pairs
   //
   //     out = C*(1-a0) + a0*rgb0
   //     out' = (C*(1-a0) + a0*rgb0)*(1-a1) + a1*rgb1
   // extract out an alpha form:
   //     out' = C*(1-a') + a' * rgb'
   //     a'      = 1-(1-a0)*(1-a1)
   //     a'*rgb' = a0*rgb0*(1-a1) + a1*rgb1
   //     a'*rgb' = (a0-1)*(1-a1)*rgb0 + (1-a1)*rgb0 + a1*rgb1
   //     a'*rgb' = -(1-a0)*(1-a1)*rgb0 + rgb0 + -a1*rgb0 + a1*rgb1
   //     a'*rgb' = a'*rgb0 + a1*(rgb1-rgb0)
   //     rgb' = rgb0 + (rgb1-rgb0)*a1/a'

   float new_alpha;
   float scale;

   new_alpha = 1-(1-*alpha)*(1-under_alpha);
   scale = *alpha / new_alpha;

   rgb[0] = under_rgb[0] + ((int) rgb[0] - (int) under_rgb[0]) * scale;
   rgb[1] = under_rgb[1] + ((int) rgb[1] - (int) under_rgb[1]) * scale;
   rgb[2] = under_rgb[2] + ((int) rgb[2] - (int) under_rgb[2]) * scale;
   *alpha = new_alpha;
}


// from portal.c:
extern BOOL (*pSkyRenderCallback)(void);

// This is so we can draw the new sky, if specified.
// return FALSE if new-style sky is NOT to be rendered, and old one should be used.
static BOOL RenderNewSky(void)
{
   if (SkyRendererUseEnhanced())
   {
      SkyRendererRender();
      return TRUE;
   }

   return FALSE;
}


void cam_render_scene(Position *pos, double zoom)
{
   float alpha;
   uchar rgb[3];
   extern BOOL lgd3d_z_normal;

   CTIMER_START(kCTimerRendTotal);

   if (rendloop_clear)
      if (g_lgd3d) {
         if (g_zbuffer) {
            lgd3d_set_zwrite(FALSE);
            lgd3d_set_zcompare(FALSE);
            lgd3d_clear(254);
         } else {
            lgd3d_set_zwrite(TRUE);
            lgd3d_clear(254);
         }
      }
      else
         gr_clear(254);

   lgd3d_z_normal = !g_zbuffer;

   // before we render anything, determine our screen flash,
   // so we'll have a fighting chance of doing it right in software

   // returns description of effect in obvious hardware overdraw way
   alpha = compute_flashbomb_effects(rgb);
   // @TODO: in software, set up a CLUT or something

   if ((GetSkyMode() == kSkyModeStars) && !SkyRendererUseEnhanced())
      starfield_startframe(pos, zoom);

   rendobj_init_frame();
   pSkyRenderCallback = RenderNewSky;

   portal_render_scene(pos, zoom);
   rendobj_finish_object_rendering();

   // actually, this should come before all translucent rendering
   if ((GetSkyMode() == kSkyModeStars) && !SkyRendererUseEnhanced())
      starfield_endframe(pos, zoom);

   // check if we're under water and alpha blend the whole scene if we are
   if (g_lgd3d) {
      int cell = CellFromPos(pos);
      if ((cell >=0) && (WR_CELL(cell)->medium == MEDIA_WATER))
      {
         int bank = GetWaterBankForFlow(WR_CELL(cell)->motion_index);
         const sRGBA* color = GetWaterColorForBank(bank);

         Assert_(color != NULL);

         if (alpha == 0.0)
         {
            alpha = color->alpha;

            rgb[0] = color->rgb[0];
            rgb[1] = color->rgb[1];
            rgb[2] = color->rgb[2];
         }
         else
         {
            composite(&alpha, rgb, color->alpha, color->rgb);
         }
      }

      {
         float fade_alpha;
         uchar color[3];
         if (PlayerModeGetFade(&color[0], &color[1], &color[2], &fade_alpha))
         {
            composite(&fade_alpha, color, alpha, rgb);
            alpha = fade_alpha;
            rgb[0] = color[0];
            rgb[1] = color[1];
            rgb[2] = color[2];
         }
      }
      if (alpha != 0.0)
         Flash(alpha, (rgb[0]<<16)|(rgb[1]<<8)|rgb[2]);
   }
   CTIMER_RETURN(kCTimerRendTotal);
}

static void pre_draw_callback(void)
{
   lgd3d_start_frame(rendloop_counter);
}

static int nOldMode = -1;

static const float fRemoteZoom = 0.4;
static void do_frame(sLoopFrameInfo* info)
{
   Position campos;              // position of camera for renderer
   mxs_vector pos;               // position of camera
   mxs_angvec ang;               // angle of camera
   float fZoom;
   ObjID playobj = PlayerObject();
   Camera* cam = PlayerCamera();
   extern BOOL g_zbuffer;
   EXTERN void do_set_gamma(void);
   do_set_gamma();

   fZoom = cam->zoom;
   if (CameraIsRemote(cam))
      fZoom = fRemoteZoom;

//   mprintf("a) us %x vis %x\n",grd_canvas->bm.bits,grd_visible_canvas->bm.bits);

   FrameTimeSet(start_rend_time,tm_get_millisec_unrecorded());
   rendobj_seen=0;   // @TODO: should put this off somewhere safe

   // Update dynamic lighting
   if (player_light)
   {
      ObjPos* pos = ObjPosGet(playobj);
#ifdef RGB_LIGHTING
      portal_set_normalized_color(255,255,255);
#endif
      add_dynamic_light(&pos->loc, player_light, 0.0);
   }

   // clear the array which tells us which cells were visible in the
   // previous frame
   portal_unmark_visible_cells();

   // move raindrops & such (actual rendering is through a Portal
   // callback)
   WeatherFrame(GetSimFrameTime());

   // render
   CameraGetLocation(cam, &pos, &ang);
   MakePositionFromVectors(&campos, &pos, &ang);

   CoronaFrame(&campos);

   // wsf : call Init all the time, assuming it uses internal bool to keep track.
   // This is so we can support runtime new sky option.
   // Also, before SetSkyMode is called, store previous setting so that we can restore
   // it if we need to.
   if (!SkyRendererUseEnhanced())
   {
      starfield_init();
      if (nOldMode >= 0) // restore.
         SetSkyMode(nOldMode);
   }
   else // force to non-star sky.
   {
      starfield_term();
      if (nOldMode < 0)
         nOldMode = GetSkyMode();
      SetSkyMode(kSkyModeTextures);
   }

   cam_render_scene(&campos,fZoom*game_zoom_factor);

   if (g_lgd3d) {
      CameraOverlayOnFrame();
      if (CameraIsRemote(cam))
         CameraOverlayRender(FALSE);

      // render objects queued last frame
      invRendFlushQueue();

      lgd3d_end_frame();
   }

   // these 3 times are: how long rendering this frame took (render time)
   //                    how long it took to get to this frame from last
   //                      frame ending (total non-render time)
   //                    how long it took to get to our BeginLoop message
   //                      this frame from our EndLoop message last frame
   //                      (total blit time approximate)

   FrameTimeSet(end_rend_time,tm_get_millisec_unrecorded());   // compute time delta
}

static void end_frame()
{
   FrameTimeSet(end_frame_time,tm_get_millisec_unrecorded());
}

EXTERN void set_mm_sort(BOOL sort);
EXTERN ulong g_MeshRenderFlags;

static void set_pipelines()
{
   set_mm_sort(!g_lgd3d || !g_zbuffer);
   if (g_lgd3d)
      g_MeshRenderFlags |= MMF_INDEXED;
   else
      g_MeshRenderFlags &= ~MMF_INDEXED;
}

static void setup_from_config_vars()
{
   if (!config_get_int("fogging", &portal_fog_on))
      portal_fog_on = 0;
}


////////////////////////////////////////
// Database message handler
//

static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;
   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         WeatherReset();
         ClearTextureArchetypeMapping();
         MeshTexReset();
         break;

      case kDatabasePostLoad:
         if (msg->subtype & kObjPartAbstract)
             TextureArchetypesPostLoad();
         if (msg->subtype & kDBMap)
            BuildTextureArchetypes();
         break;

      case kDatabaseDefault:
         TextureArchetypesPostLoad();
         break;

      case kDatabaseSave:
         if (msg->subtype & kObjPartConcrete)
            WeatherSave(data.save);
         break;

      case kDatabaseLoad:
         if (msg->subtype & kObjPartConcrete)
            WeatherLoad(data.load);
         break;
   }
}

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata;

   switch(msg)
   {
      case kMsgExitMode:
         if (!SkyRendererUseEnhanced())
            starfield_term();
         SkyRendererTerm();
         CameraOverlayTerm();
         nOldMode = -1;
      case kMsgSuspendMode:
         portal_pre_draw_callback = NULL;
         invRendFreeQueue();
         ScreenOverlaysFree();
         ParticleGroupExitMode();
         break;

      case kMsgEnterMode:
         CameraOverlayInit();
         SkyRendererInit();
      case kMsgResumeMode:
         if (g_lgd3d)
            portal_pre_draw_callback = pre_draw_callback;
#ifdef PLAYTEST
         show_full_stats=(!config_is_defined("show_quick_stats"));
#endif
         ScreenOverlaysInit();
         set_pipelines();

         MissionPalSet();
         ParticleGroupEnterMode();
         WaterColorSetupTextures();
         starfield_enter_mode();
         set_pipelines();
         setup_from_config_vars();
         break;

      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimRender))
         {

            if (g_lgd3d && !g_render_on_begin_frame)
               do_frame(info.frame);

            ScrnLockDrawCanvas();

            if (!g_lgd3d)
               do_frame(info.frame);
#ifdef FRAME_TIMINGS
            do_stats(end_rend_time-start_rend_time,
                     start_rend_time-end_last_rend_time,
                     start_frame_time-end_frame_time);     // draw stats
            end_last_rend_time = end_rend_time;
#endif
            ScreenOverlaysUpdate();
            ScrnUnlockDrawCanvas();
         }
         break;

      case kMsgBeginFrame:
         start_frame();
         if (g_lgd3d && g_render_on_begin_frame)
            if (SimStateCheckFlags(kSimRender))
               do_frame(info.frame);
         // this has to come before sim and after render
         reset_dynamic_lighting();
         break;

      case kMsgEndFrame:
         end_frame();
         break;

      case kMsgEnd:
         Free(state);
         break;

      case kMsgAppTerm:
         ParticleGroupCleanup();
         break;

   }
   return result;
}

////////////////////////////////////////////////////////////
//
// Loop client factory function.
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;

   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//

sLoopClientDesc RenderLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Render Client",                    // NAME
   kPriorityNormal,                    // PRIORITY
   kMsgEnd | kMsgsMode | kMsgsFrame | kMsgsAppOuter,   // INTERESTS

   kLCF_Callback,
   _CreateClient,

   NO_LC_DATA,

   {
      {kConstrainAfter, &LOOPID_SimFinish, kMsgsFrame},
      {kConstrainAfter,&LOOPID_ScrnMan,   kMsgsFrame|kMsgsMode},
      {kNullConstraint} // terminator
   }
};

////////////////////////////////////////
//
// RENDERER BASE MODE CLIENT
//

#pragma off(unreferenced)
static eLoopMessageResult LGAPI BaseMode(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata;

   switch(msg)
   {
      case kMsgAppInit:
#ifdef NEW_NETWORK_ENABLED
#ifdef PLAYTEST
         g_pNetMan = AppGetObj(INetManager);
         g_pNetStats = AppGetObj(INetStats);
#endif
#endif
         RendererInit();
         TextureArchetypesInit();
         WeatherInit();
         SkyRendererAppInit();
         MeshTexInit();
         CoronaInit();
         break;

      case kMsgAppTerm:
         RendererTerm();
         TextureArchetypesTerm();
         MeshTexTerm();
         CoronaTerm();
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         RendererDBMessage(info.dispatch->subtype,
                           (IUnknown*)info.dispatch->data);
         break;

      case kMsgVisual:
         if (info.dispatch->subtype == kPaletteChange)
            RendererPalChange();
         break;

      case kMsgEnd:
         Free(state);
         break;

      case kMsgEnterMode:
         break;

      case kMsgExitMode:
         break;
   }
   return result;
}

////////////////////////////////////////////////////////////
//
// Loop client factory function.
//

#pragma off(unreferenced)
static ILoopClient* LGAPI CreateBaseClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;

   return CreateSimpleLoopClient(BaseMode,state,pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//

sLoopClientDesc RenderBaseLoopClientDesc =
{
   &LOOPID_RenderBase,                 // GUID
   "Render Base Mode Client",                    // NAME
   kPriorityNormal,                    // PRIORITY
   kMsgEnd | kMsgsAppOuter | kMsgVisual | kMsgDatabase,   // INTERESTS

   kLCF_Callback,
   CreateBaseClient,

   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase|kMsgAppInit },  // for texture archetypes, so we can register our objpos listener
      {kConstrainAfter, &LOOPID_Wr, kMsgDatabase},
      {kNullConstraint} // terminator
   }
};

