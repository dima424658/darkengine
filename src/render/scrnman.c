/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


/*
 * $Source: r:/t2repos/thief2/src/render/scrnman.c,v $
 * $Revision: 1.65 $
 * $Author: toml $
 * $Date: 2000/02/19 12:35:36 $
 *
 * Screen Manager
 *
 */
#include <string.h>
#include <scrnman.h>
#include <lg.h>
#include <2d.h>
#include <gameinfo.h>
#include <cursors.h>
#include <curdat.h>
#include <lgd3d.h>
#include <r3d.h>

#include <appagg.h>
#include <comtools.h>
#include <gshelapi.h>
#include <dispapi.h>
#include <rand.h>

#include <config.h>
#include <cfgdbg.h>

#include <palmgr.h>

// Include this last
#include <initguid.h>
#include <scrnguid.h>





//zb SS2 patch only!
#include <lgSS2P.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 




BOOL g_table_fog;
BOOL g_table_fog_toggle = TRUE;
BOOL g_zbuffer;
BOOL g_zbuffer_toggle = TRUE;
EXTERN BOOL g_lgd3d;
BOOL g_null_raster = FALSE;

static ScrnMode _cur_res = -1;

static ScrnCallBack _curback = NULL;
static int _data = 0;
static grs_canvas *_off_screen = NULL;
static grs_canvas *_draw_canv = NULL; 
static bool _in_frame = FALSE;

ScrnMode ScrnFindModeFlags(short w, short h, ubyte depth,ulong flags)
{
   int i;
   i=gr_find_mode_flags(w,h,depth,flags|GRM_IS_SUPPORTED);
   if (i!=-1)
      return i;

   return SCR_NOMODE;
}


#ifdef DO_GAME_SHELL
static int _in_shell = 0;

void _start_shell(void)
{                    
   if (_in_shell==0) {     
      IGameShell_BeginFrame (pGameShell);
   }
   _in_shell++;
}

void _end_shell(void)
{
   if (_in_shell==1) {
      IGameShell_EndFrame (pGameShell);
   }
   _in_shell--;
}
#else
#define _start_shell()
#define _end_shell()
#endif

#define GET_KIND(p, a, b, c) COMCall3(p, GetKind2, a, b, c)
#define SET_KIND(p, a, b, c) COMCall3(p, SetKind2, a, b, c)
#define SET_FLIP(p, a) COMCall1(p, SetFlip, a)

static BOOL try_flip = FALSE;

int ScrnSetDisplay(int kind, int flags, GUID *DD)
{
   int dispKind, dispFlags;
   int changed;
   GUID *curDD;
   IDisplayDevice *pDispDev = AppGetObj(IDisplayDevice);
   BOOL different_dd = FALSE;
   EXTERN BOOL g_no_dx6;
   BOOL last_flip = try_flip;

   GET_KIND(pDispDev, (eDisplayDeviceKind *)&dispKind, &dispFlags, &curDD);

   if (DD && IsEqualGUID(DD,&SCRNMAN_PRESERVE_DDRAW))
      DD = curDD; 

   different_dd = DD != curDD; 

   // If neither pointer is null, actually compare the guids
   if (different_dd && DD && curDD)
      different_dd = !IsEqualGUID(DD,curDD);

   if (g_no_dx6) {
      kind = kDispDebug;
      DD = NULL;
      flags &= kDispStrictMonitors;
   }


   try_flip = flags & kDispAttemptFlippable;

   // Always attempt flippable! set flipping for real after setting mode!
   flags |= kDispAttemptFlippable;

   if (!different_dd && kind == dispKind)
      // add attempt3d if is was there before...
      flags |= (kDispAttempt3D & dispFlags);

   if (different_dd || kind != dispKind || flags != dispFlags) 
   {
      ConfigSpew("set_display_spew", ("Changing display to %d flags 0x%X\n", kind, flags)); 
      ScrnUnsetRes();
      gr_close();
      SET_KIND(pDispDev, kind, flags, DD);
      gr_init();
      grd_mode = -1;
      changed = TRUE;
   } else
      changed = last_flip != try_flip;

   SafeRelease(pDispDev);


   // return old diplay kind
   return changed;
}


int scrn_mode_table[] =
{
   GRM_320x200x8,
   GRM_320x400x8,
   GRM_640x400x8,
   GRM_640x480x8,
   GRM_1024x768x8,
};

static BOOL g_page_flip = FALSE;

static void make_draw_canvas(void)
{
   IDisplayDevice* pDisp = AppGetObj(IDisplayDevice);
   int disp_kind, disp_flags;

   GET_KIND(pDisp, &disp_kind, &disp_flags, NULL);

   // This would happen in gr_set_screen(), but we may not have called it.
   gr_set_canvas(grd_back_canvas);

   if (_off_screen != NULL)
   {
      gr_free_canvas(_off_screen);
      _off_screen = NULL;
   }

   if (!g_page_flip && (disp_kind == kDispFullScreen))
   {    // FullScreen, not page flipping...
      _off_screen = gr_alloc_canvas(grd_bm.type,grd_bm.w,grd_bm.h); // Was always BMT_FLAT8
      _draw_canv = _off_screen; 
   }
   else // windowed or page flipping, no backing canvas
   {
      _off_screen = NULL; 
      _draw_canv = grd_visible_canvas; 
   }
   if (_draw_canv->ytab == NULL)
      gr_alloc_ytab(_draw_canv);

   SafeRelease(pDisp); 
}

void ScrnLockDrawCanvas(void)
{
   IDisplayDevice* pDisp;

   if (_off_screen!=NULL)
      return;

   pDisp = AppGetObj(IDisplayDevice);
   IDisplayDevice_Lock(pDisp);
   if (g_lgd3d && (!g_null_raster))
      r3_use_g2();
   SafeRelease(pDisp);
}

void ScrnUnlockDrawCanvas(void)
{
   IDisplayDevice* pDisp;

   if (_off_screen!=NULL)
      return;

   pDisp = AppGetObj(IDisplayDevice);
   IDisplayDevice_Unlock(pDisp);
   if (g_lgd3d && (!g_null_raster))
      if (!pDisp->lpVtbl->IsLocked(pDisp))
         r3_use_lgd3d();
   SafeRelease(pDisp);
}

static BOOL doMiscSetResStuff(int new_mode, ulong flags)
{
   grs_mode_info* info;
   info = gr_mode_info_from_mode(new_mode); 

   if (grd_screen == NULL)
   {
      grs_screen* screen = gr_alloc_screen(info->w,info->h);
      gr_set_screen(screen);
   }
   make_draw_canvas();

   // ScrnInit3d() has to happen before palmgr_recompute_all().
   if (ScrnInit3d()==FALSE) {
      Warning(("ScrnInit3d failed!\n"));
      ScrnUnsetRes();
      return FALSE;
   }

   palmgr_recompute_all();

   if (flags & kScrnPreservePal) 
   {
      ScrnUsePalette();
      ScrnClear(); 
   }
   else 
      ScrnBlacken();

   gr_set_canvas(_draw_canv);

   _cur_res = new_mode;
   
   return TRUE;
}

// set a new screen mode.  Preserve the previous
// palette or not
// TRUE for success.
BOOL ScrnSetRes(ScrnMode mode,ulong flags)
{
   IDisplayDevice *pDispDev = AppGetObj(IDisplayDevice);
   int new_mode;
   static int last_mode = -1;
   int retval = FALSE;

   #ifdef DBG_ON
   if (_in_frame) {
      Warning(("ScrnSetRes: Trying to set res inside frame\n"));
   }
   #endif 

   new_mode = ScrnModeToGrMode(mode);

   ConfigSpew("set_res_spew",
      ("setting screen mode %i. last_mode %i, grd_mode %i\n", new_mode, last_mode, grd_mode));

   // note that grd_mode is the mode we _would_ get if we set last_mode
   // we really want to know if we'd end up with the same mode, 
   // so we compare against both!
   // This will break horribly if we ever gr_set_mode() elsewhere...

   if (grd_mode != -1) // make sure we haven't closed display device...
      if ((last_mode == new_mode) || (grd_mode == new_mode)) {

         if (try_flip == g_page_flip)
            goto ScrnSetResMisc;
            // we're all set!

         // OK, try to set page flipping correctly on the fly!
         g_page_flip = SET_FLIP(pDispDev, try_flip);
         if (g_page_flip == try_flip)
            // Success! Now we just need to make the draw canvas, etc.
            goto ScrnSetResMisc;

         // Couldn't do it, fall through to old way...
      }

   ScrnUnsetRes();

   ConfigSpew("set_res_spew",
      ("calling gr_set_mode with mode %i, flags %i\n", new_mode, flags&0xff));
   if (gr_set_mode(new_mode,flags & 0xFF) == -1) // failure
      goto ScrnSetResDone;

   // we want to save last_mode, not grd_mode, so we can compare later
   last_mode = new_mode;  

   // g_page_flip needs to be set correctly before we try to make the draw canvas.
   if (try_flip)
   {
      g_page_flip = SET_FLIP(pDispDev, TRUE);
      if (!g_page_flip)
         // fail!
         goto ScrnSetResDone;
   }
   else
   {
      g_page_flip = SET_FLIP(pDispDev, FALSE);
      AssertMsg(g_page_flip==FALSE, "Kevin can't code for shit.");
   }


ScrnSetResMisc:
   retval = doMiscSetResStuff(new_mode, flags);

ScrnSetResDone:
   SafeRelease(pDispDev);
   return retval;
}

void ScrnUnsetRes(void)
{
   ConfigSpew("set_res_spew", ("unsetting res...\n"));
   if (grd_screen != NULL)
   {
      gr_free_screen(grd_screen);
      grd_screen = NULL;
   }

   if (_off_screen != NULL)
   {
      gr_free_canvas(_off_screen);   
      _off_screen = NULL;
   }
   _draw_canv = NULL; 

   _cur_res = -1;
}


// returns -1 if not yet set
ScrnMode ScrnGetRes(void)
{
   return _cur_res;
}


EXTERN BOOL g_null_raster;
EXTERN BOOL portal_multitexture;
EXTERN void portal_setup_light_masks(void);

int g_lgd3d_device_index;
bool g_multitexture;


//
// Init/Term 3d accelleration
//
#ifdef USE_HARDWARE3D

BOOL ScrnInit3d(void)
{

   if( g_lgd3d )
   {
      int                  num_devices = lgd3d_enumerate_devices();
      lgd3ds_device_info*  info;
      ulong                set_flags;


      if (num_devices <= 0) {
         Warning(("No direct 3d devices available."));
         return FALSE;
      }
      
      if( (g_lgd3d_device_index > num_devices) || (g_lgd3d_device_index < 0) )
         g_lgd3d_device_index = num_devices-1;
      
      info = lgd3d_get_device_info(g_lgd3d_device_index);

      info->request_flags  = LG3D_CF_DITHER | LG3D_SHADER_MODULATE_ALPHA;
      info->required_flags = LG3D_SHADER_MULTITEX_COLOR;

      if( info->caps & LG3D_CF_SP_MULTI_TEX )
         info->request_flags |= LG3D_CF_SP_2LEV_TEX;
      
      if( g_zbuffer_toggle ) // that's a joke, right?
         info->required_flags |= LG3D_CF_Z_BUFFER;
      
#ifndef SHIP
      {
         int spew = 0;
         config_get_int("lgd3d_spew", &spew);
         
         if( spew )
            info->request_flags |= LG3D_SPEW;
      }
      
      if (config_is_defined("fail_lgd3d_init"))
         return FALSE;

      if( config_is_defined("lg3d_cache_alpha") )
         info->request_flags |= LG3D_CF_RND_ALPHA_LAST;

      if( config_is_defined("force_two_passes") )
         info->request_flags &= ~LG3D_CF_SP_2LEV_TEX;

#endif
      
      if( FAILED( lgd3d_init( info, &set_flags ) ) )
         return FALSE;
      
      // make sure our lightmap method is confirmed.
      if( !(set_flags & LG3D_SHADER_MULTITEX_COLOR) )
      {
         lgd3d_shutdown();
         // set error message?
         return FALSE;
      }
      
      lgd3d_set_chromakey(255, 0, 255);
      
      portal_multitexture = g_multitexture = (set_flags & LG3D_CF_SP_MULTI_TEX) != 0;      
      g_zbuffer = (set_flags & LG3D_CF_Z_BUFFER) != 0;

      if( !g_zbuffer && g_zbuffer_toggle ) 
      {
         Warning(("Couldn't create zbuffer.\n"));
      }

      if( !g_null_raster )
         r3_use_lgd3d();
   }
   
#ifdef RGB_LIGHTING
   portal_setup_light_masks();
#endif
   return TRUE;
}

#else # !USE_HARDWARE3D
BOOL ScrnInit3d(void)
{
   if (g_lgd3d)
   {
      int num_devices = lgd3d_enumerate_devices();
      lgd3ds_device_info *info;

      if (num_devices <= 0) {
         Warning(("No direct 3d devices available."));
         return FALSE;
      }

      if ((g_lgd3d_device_index > num_devices)||(g_lgd3d_device_index < 0))
         g_lgd3d_device_index = num_devices-1;

      info = lgd3d_get_device_info(g_lgd3d_device_index);

      // see if we can single pass multitexture
      if (info->flags & LGD3DF_CAN_DO_SINGLE_PASS_MT)
         info->flags |= LGD3DF_MULTI_TEXTURING;
      else
         info->flags &= ~LGD3DF_MULTI_TEXTURING;

      // check our lightmap method.
      info->flags |= LGD3DF_MULTITEXTURE_COLOR;

      // try to do zbuffering if the user turned it on
      if (g_zbuffer_toggle)
         info->flags |= LGD3DF_ZBUFFER;
      else
         info->flags &= ~LGD3DF_ZBUFFER;

      if (g_table_fog_toggle)
        info->flags |= LGD3DF_TABLE_FOG;
      else
        info->flags &= ~LGD3DF_TABLE_FOG;

#ifndef SHIP
      {
         int spew = 0;
         config_get_int("lgd3d_spew", &spew);

         if (spew)
            info->flags |= LGD3DF_SPEW;
         else
            info->flags &= ~LGD3DF_SPEW;

      }

      if (config_is_defined("fail_lgd3d_init"))
         return FALSE;

      if( config_is_defined("force_two_passes") )
         info->flags &= ~LGD3DF_MULTI_TEXTURING;
#endif

      info->flags |= LGD3DF_DITHER | LGD3DF_MODULATEALPHA; //zb :






      //zb SS2 patch only!
      b_SS2_UseSageTexManager = (config_is_defined("safe_texture_manager"));

      



      // note that flags may be changed during this call, so we check afterwards
      if (lgd3d_init(info) == FALSE)
         return FALSE;

      // make sure our lightmap method is confirmed.
      if (!(info->flags & LGD3DF_MULTITEXTURE_COLOR))
      {
         lgd3d_shutdown();
         // set error message?
         return FALSE;
      }

      lgd3d_set_chromakey(255, 0, 255);

      portal_multitexture = g_multitexture = (info->flags & LGD3DF_MULTI_TEXTURING) != 0;
      g_zbuffer = (info->flags & LGD3DF_ZBUFFER) != 0;

      if (!g_zbuffer && g_zbuffer_toggle) 
      {
         lgd3d_shutdown();
         Warning(("Couldn't create zbuffer.\n"));
         return FALSE; 
      }

      g_table_fog = (info->flags & LGD3DF_TABLE_FOG) != 0;

      if (!g_null_raster)
         r3_use_lgd3d();
   }
#ifdef RGB_LIGHTING
   portal_setup_light_masks();
#endif
   return TRUE;

}
#endif


void ScrnTerm3d(void)
{
   if (g_lgd3d)
   {
      if (!g_null_raster)
         r3_use_g2(); 
      lgd3d_shutdown(); 
   }
}

// background drawer callback function, takes data,
// takes effect next start frame.  Should call it outside
// of a frame.  Only one can be installed at a time
// no chaining.  Maybe later if you ever need it
typedef void (*ScrnCallBack)(int data);

void ScrnInstallBackground(ScrnCallBack cb,int data)
{
   #ifdef DBG_ON
   if (_curback != NULL) {
      Warning(("ScrnInstallBackground: trying to install background over existing one\n"));
   }
   #endif

   _data = data;
   _curback = cb;
}

void ScrnClear(void)
{
   BOOL have_canv = grd_visible_canvas != NULL;
   IDisplayDevice* pDisp = NULL; 

   if (grd_mode < 0 || _cur_res < 0) 
      return; 


   if (have_canv)
   {
      pDisp = AppGetObj(IDisplayDevice); 
      IDisplayDevice_Lock(pDisp); 
      gr_push_canvas(grd_visible_canvas);
      gr_clear(0);
      gr_pop_canvas();
   }

   if (_off_screen != 0)
   {
      gr_push_canvas(_off_screen);
      gr_clear(0);
      gr_pop_canvas();
   }   
   if (have_canv)
   {
      IDisplayDevice_Unlock(pDisp);
      IDisplayDevice_Flush(pDisp); 
      SafeRelease(pDisp); 
   }
}

// clear the screen to black and blacken the palette
// note that this works great, 8 or 16 bit as a prelude
// to loading a new screen
// Note that this works in or out of a frame
void ScrnBlacken(void)
{

   BOOL have_canv = grd_visible_canvas != NULL;
   IDisplayDevice* pDisp = NULL; 

   if (have_canv)
   {
      pDisp = AppGetObj(IDisplayDevice); 
      IDisplayDevice_Lock(pDisp); 
   }
   
   // we don't want to change the palette, because we want to remember
   // what it is.  
   ScrnNewBlackPalette();

   // gr clear the visible canvas, so new palette set
   // does not get garbage
   // Zero is the universal black.  
   if (have_canv)
   {
      gr_push_canvas(grd_visible_canvas);
      gr_clear(0);
      gr_pop_canvas();
   }
   if (_off_screen != 0)
   {
      gr_push_canvas(_off_screen);
      gr_clear(0);
      gr_pop_canvas();
   }   

   if (have_canv)
   {
      IDisplayDevice_Unlock(pDisp);
      IDisplayDevice_Flush(pDisp); 
      SafeRelease(pDisp); 
   }

}

// call the background callback if present
// otherwise just start the frame and go
void ScrnStartFrame(void)
{
   _start_shell();

   ScrnSetOffscreen(); 

   if (_curback != NULL) {
      _curback(_data);
      _curback = NULL;
   }
   _in_frame = TRUE;
}

// compose cursor onto/off of back canvas

void compose_cursor(int cmd)
{
   if (LastCursor != NULL)
   {
      grs_canvas *save_canvas = CursorCanvas;
      CursorCanvas = _draw_canv; 
      LastCursor->func(cmd,NULL,LastCursor,LastCursorPos);
      CursorCanvas = save_canvas;   
   }
}


// blit or page flip, transparently, woo woo.
void ScrnEndFrame(void)
{
   ScrnForceUpdate();
   
   _in_frame = FALSE;
   _end_shell();
}

// blit or page flip right now dammit
void ScrnForceUpdate(void)
{
   Rect r = { { 0, 0}, };
   r.lr.x = grd_visible_canvas->bm.w;
   r.lr.y = grd_visible_canvas->bm.h;
   ScrnForceUpdateRect(&r);
}

void ScrnForceUpdateRect(Rect *r)
{
   IDisplayDevice *disp;
   //   BOOL frame = _in_frame;
   grs_clip clip;

   if (g_page_flip)
      return; 

   disp = AppGetObj(IDisplayDevice);


   if (_off_screen)
   {
      IDisplayDevice_Lock(disp);

      MouseLock++;
      compose_cursor(CURSOR_DRAW);

      gr_push_canvas(grd_visible_canvas);
      clip = grd_gc.clip;

      gr_safe_set_cliprect(r->ul.x, r->ul.y, r->lr.x, r->lr.y);
      gr_bitmap(&(_off_screen->bm),0,0);

      grd_gc.clip = clip;
      gr_pop_canvas();

      compose_cursor(CURSOR_UNDRAW);
      MouseLock--;

      IDisplayDevice_Unlock(disp);
   
   }


   IDisplayDevice_FlushRect(disp, r->ul.x, r->ul.y, r->lr.x, r->lr.y);

   

   SafeRelease(disp);

}


// set the current canvas to be the visible screen
// the idea is you're not page flipping or blitting
// in fact, if you do an end_frame in this mode, you get
// an error.  Can we do this under gameshell?
// THis is probably bad, since you need it locked

// unless we say you always have to be
// in a frame

void ScrnSetVisible(void)
{
   gr_set_canvas(grd_visible_canvas);
}

// set the current canvas to be the offscreen canvas(default)
void ScrnSetOffscreen(void)
{
   gr_set_canvas(_draw_canv); 
}

grs_canvas *ScrnGetDrawCanvas(void)
{
   return _draw_canv;
}

static void set_palette(void)
{
   palmgr_update_pal_slot(0); 
   grd_ipal = palmgr_get_ipal(0);
}

static void build_palette(uchar * pal,int start,int size)
{
   palmgr_set_pal_slot_passively(0, 256, pal, 0);
}

// load and use this palette
void ScrnNewPalette(uchar * pal,int start,int size)
{
   #ifdef DBG_ON
   if (start < 0 || start>255 || (start+size)>256 || size < 0 ) {
      Warning(("ScrnNewPalette: start is %d and size is %d, illegal\n",start,size));
   }
   #endif

   build_palette(pal,start,size);
   ScrnUsePalette(); 

}

// make the current palette black and use it
void ScrnNewBlackPalette(void)
{
   uchar pal[768]; 
   memset(pal,0,768);
   build_palette(pal,0,256); 
   ScrnUsePalette(); 

}

// load but do not set this palette
void ScrnLoadPalette(uchar *pal,int start,int size)
{
   #ifdef DBG_ON
   if (start < 0 || start>255 || (start+size)>256 || size < 0 ) {
      Warning(("ScrnLoadPalette: start is %d and size is %d, illegal\n",start,size));
   }
   #endif

   build_palette(pal,start,size);
}

// Use the currently loaded palette
void ScrnUsePalette(void)
{
   if (_cur_res > 0)
      set_palette(); 
   else
      grd_pal = palmgr_get_pal(0); 
}

// return pointer to loaded palette.  Beware, don't mess with it.
const uchar *ScrnGetPalette(void)
{
   return palmgr_get_pal(0);
}

////////////////////////////////////////
const uchar* ScrnGetInvPalette(void)
{
   return palmgr_get_ipal(0);
}

int ScrnInvPaletteSize(void)
{
   return 32768;
}



