/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmap.cpp,v 1.30 1999/08/05 17:49:07 Justin Exp $

// alarm icon
#include <2d.h>
#include <math.h>

#include <mprintf.h>
#include <resapi.h>
#include <playrobj.h>
#include <rooms.h>
#include <roomsys.h>
#include <filevar.h>
#include <scrnmode.h>
#include <questapi.h>
#include <dbfile.h>
#include <simtime.h>
#include <traitman.h>

#include <kbcook.h>
#include <keydefs.h>

#include <objpos.h>
#include <iobjsys.h>
#include <propbase.h>

#include <drkpanl.h>
#include <prcniter.h>

#include <shkmap.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkplayr.h>
#include <shkplprp.h>
#include <shkmulti.h>
#include <shkgame.h>
#include <shktrcst.h>
#include <shkprop.h>
#include <shkparam.h>

#include <netman.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

#include <dbmem.h>

// spans the whole width of the screen
static Rect full_rect = {{LMFD_X, LMFD_Y}, {RMFD_X + RMFD_W, RMFD_Y + RMFD_H}}; 
//static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};
static Rect close_rect = {{606,6},{606 + 20, 6 + 21}};

static Rect mini_rect = {{482,268},{482+ 142, 268 + 22}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static LGadButton mini_button;
static DrawElement mini_elem;
static IRes *mini_handles[2];
static grs_bitmap *mini_bitmaps[4];

static IRes *gMapHnd;
static IRes *gMiniMapHnd;
static IRes *gAutoMapHnd;
static IRes *gMiniAutoMapHnd;
static IRes *gMapNone;
static IRes *gMapBack;
static IRes *gOnHnd;
static IRes *gOffHnd;
static IRes *gBrackets;

static ObjID gSelectedObj = OBJ_NULL;
static ObjID gMouseOverObj = OBJ_NULL;

static int map_obj_pos(ObjID o);
static int map_cursor_pos(Point mpt);

bool ShockHasMap();

#define MAX_ROOMS 64

// Here's the type of my global 
struct sExplored
{
   char explored[MAX_ROOMS];
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gExploredDesc = 
{
   kMissionVar,         // Where do I get saved?
   "EXPLORED",          // Tag file tag
   "Rooms Explored",     // friendly name
   FILEVAR_TYPE(sExplored),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "shock",              // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sExplored,&gExploredDesc> gExplored; 

grs_canvas gMapCanvas;
grs_bitmap *gMapBitmap;

//--------------------------------------------------------------------------------------
void ShockMapInit(int which)
{
   int i;
   gMapNone= LoadPCX("nomap");
   gMapBack = LoadPCX("mapback");
   gOnHnd = LoadPCX("minion");
   gOffHnd = LoadPCX("minioff");
   gBrackets = LoadPCX("bracket");
   //gPlayerHnd = LoadPCX("plrpip");

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   mini_handles[0] = LoadPCX("minimap0"); 
   mini_handles[1] = LoadPCX("minimap1"); 
   mini_bitmaps[0] = (grs_bitmap *) mini_handles[0]->Lock();
   mini_bitmaps[1] = (grs_bitmap *) mini_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      mini_bitmaps[i] = mini_bitmaps[0];
   }

   short w,h;
   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);
   w = RectWidth(&full_rect);
   h = RectHeight(&full_rect);
   use_rect.ul.x = (smode.w - w) / 2;
   use_rect.ul.y = full_rect.ul.y;
   use_rect.lr.x = use_rect.ul.x + w;
   use_rect.lr.y = use_rect.ul.y + h;
   ShockOverlaySetRect(which,use_rect);

   gMapBitmap = gr_alloc_bitmap(BMT_FLAT16, 0, w, h); // BMF_TRANS
   gr_make_canvas(gMapBitmap, &gMapCanvas);
}

//--------------------------------------------------------------------------------------
void ShockMapTerm(void)
{
   SafeFreeHnd(&gMapBack);
   SafeFreeHnd(&gMapNone);
   SafeFreeHnd(&gOnHnd);
   SafeFreeHnd(&gOffHnd);
   SafeFreeHnd(&gBrackets);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   mini_handles[0]->Unlock();
   mini_handles[1]->Unlock();
   SafeFreeHnd(&mini_handles[0]);
   SafeFreeHnd(&mini_handles[1]);

   gr_free(gMapBitmap);
}

//--------------------------------------------------------------------------------------
static HRESULT FindReferenceMarker(int maploc, ObjID *pObj, Point *pLoc)
{
   sPropertyObjIter iter;
   ObjID obj;
   sMapRef *pMap;
   int count = 0;
   int use_frame;
   BOOL found = FALSE;

   use_frame = maploc;
   while (!found)
   {
      gPropMapRef->IterStart(&iter);
      while (gPropMapRef->IterNext(&iter,&obj))
      {
         // for now, assume our first one is our match
         gPropMapRef->Get(obj, &pMap);
         if (pMap->m_frame == use_frame)
         {
            *pObj = obj;
            pLoc->x = pMap->m_x;
            pLoc->y = pMap->m_y;
            found = TRUE;
            break;
         }
      }
      gPropMapRef->IterStop(&iter);
      if (use_frame == -1)
      {
         if (!found)
            return(E_FAIL);
      }
      else 
         use_frame = -1; // try the generic scale markers on our next pass
   }

   return(S_OK);
}
//--------------------------------------------------------------------------------------
static HRESULT FindScaleMarkers(ObjID *pObj1, Point *pLoc1, ObjID *pObj2, Point *pLoc2)
{
   sPropertyObjIter iter;
   ObjID obj;
   sMapRef *pMap;
   int count = 0;
   BOOL found = FALSE;

   gPropMapRef->IterStart(&iter);
   while (gPropMapRef->IterNext(&iter,&obj))
   {
      // for now, assume our first one is our match
      gPropMapRef->Get(obj, &pMap);

      // -1 is special scale marker
      if (pMap->m_frame == -1)
      {
         if (count == 0)
         {
            *pObj1 = obj;
            pLoc1->x = pMap->m_x;
            pLoc1->y = pMap->m_y;
         }
         else
         {
            *pObj2 = obj;
            pLoc2->x = pMap->m_x;
            pLoc2->y = pMap->m_y;
         }
         count++;
         if (count == 2)
         {
            //mprintf("found markers with frame %d\n",use_frame);
            found = TRUE;
            break;
         }
      }
   }
   gPropMapRef->IterStop(&iter);

   if (count < 2)
   {
      Warning(("FindScaleMarkers: only found %d markers!\n",count));
      return(S_FALSE);
   }

   return(S_OK);
}
//--------------------------------------------------------------------------------------
static float gScaleX = 0;
static float gScaleY = 0;
static HRESULT GetMapScale(float *pScaleX, float *pScaleY)
{
   ObjPos *p1, *p2;
   ObjID mark1, mark2;
   mxs_vector v1, v2;
   mxs_vector delta;
   Point loc1, loc2;

   if (gScaleX == 0)
   {
      if (FindScaleMarkers(&mark1, &loc1, &mark2, &loc2) != S_OK)
         return(E_FAIL);

      p1 = ObjPosGet(mark1);
      p2 = ObjPosGet(mark2);
      mx_copy_vec(&v1, &p1->loc.vec);
      mx_copy_vec(&v2, &p2->loc.vec);

      // scale is ratio of map units to world units
      mx_sub_vec(&delta, &v2, &v1); // v2 - v1

      sMapParams *params = GetMapParams();
      if (params->m_rotatehack)
      {
         gScaleX = fabs(float(loc2.x) - float(loc1.x)) / fabs(delta.x);
         gScaleY = fabs(float(loc2.y) - float(loc1.y)) / fabs(delta.y);
      }
      else
      {
         gScaleX = fabs(float(loc2.x) - float(loc1.x)) / fabs(delta.y);
         gScaleY = fabs(float(loc2.y) - float(loc1.y)) / fabs(delta.x);
      }
   }

   *pScaleX = gScaleX;
   *pScaleY = gScaleY;
   return(S_OK);
}


//--------------------------------------------------------------------------------------
static HRESULT FindObjectInMapCoords(ObjID o, Point *pt, int *pMaploc)
{
   ObjPos *pos, *markerpos;
   ObjID mark;
   mxs_vector d;
   float scale_x, scale_y;
   Point loc;
   Point map;
   HRESULT retval;
   sMapParams *params = GetMapParams();

   int maploc = map_obj_pos(o); 
   if (pMaploc != NULL)
      *pMaploc = maploc;

   // find our reference markers and establish translation factor
   retval = GetMapScale(&scale_x,&scale_y);
   if (retval != S_OK)
      return(retval);

   retval = FindReferenceMarker(maploc, &mark, &loc);
   if (retval != S_OK)
      return(retval);

   // convert object position into map coords using reference markers
   pos = ObjPosGet(o);
   markerpos = ObjPosGet(mark);

   mx_sub_vec(&d, &pos->loc.vec, &markerpos->loc.vec);
  
   // perform dimensional conversion
   if (params->m_rotatehack)
   {
      // we are on a mutant level that is rotated 90 degrees
      // which, ironically, is more like the world coords
      map.x = loc.x + (d.x * scale_y * -1);
      map.y = loc.y + (d.y * scale_x);
   }
   else
   {
      // normal case
      map.x = loc.x + (d.y * scale_x);
      map.y = loc.y + (d.x * scale_y);
   }

   pt->x = map.x;
   pt->y = map.y;

   return(retval);
}
//--------------------------------------------------------------------------------------
#define SELECT_SIZE 8
static void DrawObjectInMapCoords(Point dp, ObjID o, IRes *hnd, BOOL rotate, BOOL automap)
{
   Point mapcoord;
   Point drawpt;
   ObjPos *pos;
   sMapParams *params = GetMapParams();
   int maploc;

   pos = ObjPosGet(o);
   if (FindObjectInMapCoords(o, &mapcoord, &maploc) != S_OK)
      return;

   // if we aren't automapping and we haven't been to the relevant room, then bail.
   //  unless we are in a network game, and it is another player, cause that is just better
   AutoAppIPtr(NetManager);
   if (!(pNetManager->Networking() && IsAPlayer(o)))
      if (!automap && !gExplored.explored[maploc])
         return;

   // finally, draw the handle
   drawpt.x = dp.x + mapcoord.x;
   drawpt.y = dp.y + mapcoord.y;

   if (rotate)
   {
      fixang theta = 0x8000 - pos->fac.tz;
      if (params->m_rotatehack)
         theta = theta + 0x4000;
      DrawByHandleCenterRotate(hnd, drawpt, theta);
   }
   else
      DrawByHandleCenter(hnd, drawpt);

   // bracket on selected obj
   // note we only do this in fullscreen mode
   if (ShockOverlayCheck(kOverlayMap))
   {
      if (gSelectedObj == o)
         DrawByHandleCenter(gBrackets,drawpt);

      // blinking bracket on potential selection
      Point mpos;
      int dx,dy;
      Rect r = ShockOverlayGetRect(kOverlayMap);
      mouse_get_xy(&mpos.x,&mpos.y);
      dx = abs(mpos.x - r.ul.x - drawpt.x);
      dy = abs(mpos.y - r.ul.y - drawpt.y);
      if ((dx <= SELECT_SIZE) && (dy <= SELECT_SIZE))
      {
         if (o != PlayerObject())
         {
            gMouseOverObj = o;
            if (GetSimTime() & (1 << 9))
               DrawByHandleCenter(gBrackets,drawpt);
         }
      }
   }

   // draw in associated label
   //mprintf("minimap check is %d\n",ShockOverlayCheck(kOverlayMiniMap));
   if (gPropMapText->IsRelevant(o) && ShockOverlayCheck(kOverlayMap))
   {
      const char *pStr;
      char temp[255];
      int w,h;
      gPropMapText->Get(o,&pStr);
      strcpy(temp,pStr); // this is stupid, but gr_font_foo doesn't take constr strings

      w = gr_font_string_width(gShockFont,temp);
      h = gr_font_string_height(gShockFont,temp);
      gr_font_string(gShockFont,temp,drawpt.x - (w / 2), drawpt.y + 10);

      // hey, we are theoretically editing text
      // so draw a blinking cursor
      if ((o == gSelectedObj) && (GetSimTime() & (1 << 9)))
      {
         gr_set_fcolor(gShockTextColor);
         gr_vline(drawpt.x + (w / 2) + 5, drawpt.y + 10 , drawpt.y + 10 + h);
      }
   }
}
//--------------------------------------------------------------------------------------
#define MAP_X 10
#define MAP_Y 8


// we would really get a big speedup by drawing this to a temp offscreen
// buffer -- X
void ShockMapDrawContents(Point dp, IRes *back, BOOL automap)
{
   int maps = 0;
   char buf[64]; 
   char temp[64];
   Rect dr;
   Point drawpt;

   // clear our scale factor cache
   gScaleX = 0;
   gScaleY = 0;

   if (!ShockHasMap())
      DrawByHandle(gMapNone,dp);
   else
   {
      DrawByHandle(back,dp);

      int maploc = map_obj_pos(PlayerObject()); 

      // draw in the "we've been here" decals
      char lvl[255];
      ShockGetLevelName(lvl,sizeof(lvl));
      sprintf(buf,"%s/p001xa",lvl); 
      cRectArray tmpRects; 
      FetchUIRects(buf, tmpRects);

      int i;
      for (i=0; i < tmpRects.Size(); i++)
      {
         if (gExplored.explored[i])
         {
            dr = tmpRects[i]; 

            sprintf(buf,"p001x%03d",i); 
            sprintf(temp,"intrface\\%s",lvl);

            IRes *decal = LoadPCX(buf,temp);
            drawpt.x = dr.ul.x + dp.x;
            drawpt.y = dr.ul.y + dp.y;
            DrawByHandle(decal, drawpt);
            SafeFreeHnd(&decal);
         }
      }

      // draw the current location decal
      sprintf(buf,"%s/p001ra",lvl); 
      FetchUIRects(buf, tmpRects);
   
      if (maploc >= 0 && maploc < tmpRects.Size())
      {
         dr = tmpRects[maploc]; 

         // Get the image
         sprintf(buf,"p001r%03d",maploc); 
         sprintf(temp,"intrface\\%s",lvl);

         IRes *decal = LoadPCX(buf,temp);
         drawpt.x = dr.ul.x + dp.x;
         drawpt.y = dr.ul.y + dp.y;
         DrawByHandle(decal, drawpt);
         SafeFreeHnd(&decal);

         if (!gExplored.explored[maploc])
            ShockMapSetExplored(maploc, 1);
      }

      gMouseOverObj = OBJ_NULL;

      // iterate over all map-relevant objs and draw them 
      cConcreteIter iter(gPropMapObjIcon);
      ObjID obj;
      IRes *icon;
      Label *name;
      BOOL rotate = FALSE;

      iter.Start();   
      while (iter.Next(&obj))
      {
         // how brutal is it to load these each frame?
         gPropMapObjIcon->Get(obj,&name);
         gPropMapObjRotate->Get(obj,&rotate);
         icon = LoadPCX(name->text);
         DrawObjectInMapCoords(dp, obj, icon, rotate, automap);
         SafeFreeHnd(&icon);
      }
      iter.Stop();
   }
}

//--------------------------------------------------------------------------------------
#define ONOFF_X 487
#define ONOFF_Y 268
#define MAPTEXT_X    166
#define MAPTEXT_Y    268
#define MAPTEXT_W    280

void ShockMapDrawCore(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMap);
   Point drawpt;
   BOOL automap;
   
   drawpt.x = 0;
   drawpt.y = 0;
   DrawByHandle(gMapBack,drawpt);

   drawpt.x = MAP_X;
   drawpt.y = MAP_Y;
   AutoAppIPtr(ShockPlayer);
   automap = pShockPlayer->HasTrait(PlayerObject(),kTraitAutomap);
   if (automap)
      ShockMapDrawContents(drawpt, gAutoMapHnd, automap);
   else
      ShockMapDrawContents(drawpt, gMapHnd, automap);

   // draw mouseover text
   Point mpt;
   mouse_get_xy(&mpt.x,&mpt.y);
   // put it in the decal coord frame
   mpt.x = mpt.x - drawpt.x - r.ul.x;
   mpt.y = mpt.y - drawpt.y - r.ul.y;

   int maploc = map_cursor_pos(mpt); 
   //mprintf("maploc is %d\n",maploc);
   if (gExplored.explored[maploc] || automap)
   {
      char tagname[255];
      char text[255];
      char lvl[255];
      ShockGetLevelName(lvl,sizeof(lvl));
      sprintf(tagname,"MapText_%s_",lvl);
      ShockStringFetch(text,sizeof(text),tagname,"maptext",maploc);
      if (strlen(text) > 0)
      {
         short w;
         w = gr_font_string_height(gShockFont,text);
         gr_font_string(gShockFont,text,MAPTEXT_X + ((MAPTEXT_W - w) / 2), MAPTEXT_Y);
      }
   }
}

//--------------------------------------------------------------------------------------

void ShockMapDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMap);
   gr_push_canvas(&gMapCanvas);
   ShockMapDrawCore();
   gr_pop_canvas();

   gr_bitmap(gMapBitmap, r.ul.x, r.ul.y);

   LGadDrawBox(VB(&close_button),NULL);
   LGadDrawBox(VB(&mini_button),NULL);

   Point drawpt;
   drawpt.x = ONOFF_X + r.ul.x;
   drawpt.y = ONOFF_Y + r.ul.y;
   if (ShockOverlayCheck(kOverlayMiniMap))
      DrawByHandle(gOnHnd, drawpt);
   else
      DrawByHandle(gOffHnd, drawpt);
}

//--------------------------------------------------------------------------------------
static bool rawkey_handler_func(uiEvent* ev, Region* r, void* data)
{
   ushort keycode;
   bool result;

   if (gSelectedObj == OBJ_NULL)
      return(FALSE);
   
   kbs_event kbe;
   kbe.code = ((uiRawKeyEvent*)ev)->scancode;
   kbe.state = ((uiRawKeyEvent*)ev)->action;

   kb_cook(kbe,&keycode,&result);

   int kc;
   if (keycode&KB_FLAG_DOWN)
   {
      kc=keycode&~KB_FLAG_DOWN;
      {
         char text[255];
         const char *oldtext;
         AutoAppIPtr(ObjectSystem);

         if (gPropMapText->Get(gSelectedObj, &oldtext))
            strcpy(text,oldtext);
         else
            strcpy(text,"");
         int l = strlen(text);
         switch (kc)
         {
         case KEY_BS:
            if (l >= 1)
            {
               text[l-1] = '\0';
               gPropMapText->Set(gSelectedObj, text);
            }
            break;
         case KEY_DEL:
         case KEY_PAD_DEL:
            {
               AutoAppIPtr(TraitManager);
               ObjID markerarch = pObjectSystem->GetObjectNamed("Nav Marker");
               if (pTraitManager->ObjHasDonor(gSelectedObj,markerarch))
               {
                  pObjectSystem->Destroy(gSelectedObj);
                  gSelectedObj = OBJ_NULL;
               }
            }
            break;
         case KEY_ENTER:
            gSelectedObj = OBJ_NULL;
            break;
         default:
            if (isprint(kc))
            {
               if (l < 254)
               {
                  text[l] = kc;
                  text[l+1] = '\0';
               }
            }
            gPropMapText->Set(gSelectedObj, text);
            break;
         }
         return(TRUE);
      }
   }
   return(FALSE);
}
//--------------------------------------------------------------------------------------
bool ShockMapHandleMouse(Point pos) 
{
   if (gMouseOverObj != PlayerObject())
      gSelectedObj = gMouseOverObj;
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool mini_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);
   uiDefer(DeferOverlayToggle,(void *)kOverlayMiniMap);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      uiDefer(DeferOverlayClose,(void *)kOverlayMap);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static int key_handler_id;
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMap);

   // set up the continue button
   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   // set up the minimap button
   mini_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   mini_elem.draw_data = mini_bitmaps;
   mini_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&mini_button, LGadCurrentRoot(), mini_rect.ul.x + r.ul.x, mini_rect.ul.y + r.ul.y,
      RectWidth(&mini_rect), RectHeight(&mini_rect), &mini_elem, mini_cb, 0);

   // grab keyboard focus
   uiInstallRegionHandler(LGadBoxRegion(VB(&close_button)),UI_EVENT_KBD_RAW,rawkey_handler_func,NULL,&key_handler_id);
   uiGrabFocus(LGadBoxRegion(VB(&close_button)), UI_EVENT_KBD_RAW);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   uiReleaseFocus(LGadBoxRegion(VB(&close_button)), UI_EVENT_KBD_RAW);
   uiRemoveRegionHandler(LGadBoxRegion(VB(&close_button)),key_handler_id);

   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&mini_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockMapStateChange(int which)
{
   gSelectedObj = OBJ_NULL;
   if (ShockOverlayCheck(which))
   {
      char buf[255];
      // just got put up
      BuildInterfaceButtons();

      char lvl[255];
      ShockGetLevelName(lvl,sizeof(lvl));
      sprintf(buf,"intrface\\%s",lvl);
      gMapHnd = LoadPCX("page001",buf);
      gAutoMapHnd = LoadPCX("page001a",buf);
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
      SafeFreeHnd(&gMapHnd);
      SafeFreeHnd(&gAutoMapHnd);
   }
}
//--------------------------------------------------------------------------------------
static int map_obj_pos(ObjID o)
{
   cRoom* objroom = g_pRooms->GetObjRoom(o); 
   if (objroom) 
   {
      ObjID room = objroom->GetObjID(); 

      int loc;
      if (room != OBJ_NULL && g_MapLocProperty->Get(room,&loc))
      {
         return(loc);
      }
   }

   return -1;
}
//--------------------------------------------------------------------------------------
// mpt is in decal coordinate frame, not screen coords
static int map_cursor_pos(Point mpt)
{
   // look through all the decals
   char lvl[255];
   char buf[255];
   ShockGetLevelName(lvl,sizeof(lvl));
   sprintf(buf,"%s/p001xa",lvl); 
   cRectArray tmpRects; 
   FetchUIRects(buf, tmpRects);

   int i;
   for (i=0; i < tmpRects.Size(); i++)
   {
      //dr = tmpRects[i]; 
      if (RectTestPt(&tmpRects[i],mpt))
      {
         return(i);
      }
   }
   return -1;
}
//--------------------------------------------------------------------------------------
void ShockMapSetExplored(int loc, char val)
{
   gExplored.explored[loc] = val;
}
//--------------------------------------------------------------------------------------
#define MM_X   (639 - 128)
#define MM_Y   0

#define INNERMAP_W 614
#define INNERMAP_H 260

// this is all kind of overkill at 1:1
#define MINIBIG_W 128
#define MINIBIG_H 128

#define MINI_W 128
#define MINI_H 128
static Rect minimap_rect = {{MM_X,MM_Y}, {MM_X + MINI_W, MM_Y + MINI_H}};

static IRes *gMinimapHnd;

void ShockMiniMapInit(int which)
{
   gMinimapHnd = LoadPCX("minimap");

   sScrnMode smode;
   Rect use_rect;
   ScrnModeGet(&smode);
   use_rect.ul.x = smode.w - (640 - minimap_rect.ul.x);
   use_rect.ul.y = minimap_rect.ul.y;
   use_rect.lr.x = use_rect.ul.x + RectWidth(&minimap_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&minimap_rect);
   
   ShockOverlaySetRect(which, use_rect);
   ShockOverlaySetFlags(which, kOverlayFlagBufferTranslucent);
}
//--------------------------------------------------------------------------------------
void ShockMiniMapTerm(void)
{
   SafeFreeHnd(&gMinimapHnd);
}
//--------------------------------------------------------------------------------------
static int gMapLoc = -1;
void ShockMiniMapDraw(void)
{
   Rect r = {{0,0},{MINI_W,MINI_H}}; // our actual final drawing rect size
   Rect r2 = {{0,0},{MINIBIG_W, MINIBIG_H}}; // our pre-scaled rect
   Point drawpt;
   int mx, my;
   int offset_x, offset_y, bw, bh;
   char buf[64];
   cRectArray tmpRects; 
   int maploc;
   char lvl[255];
   ShockGetLevelName(lvl,sizeof(lvl));

   AutoAppIPtr(ShockPlayer);

   // don't do anything except in freelook mode
   if (shock_mouse)
      return;

   if (!ShockHasMap())
      return;

   maploc = map_obj_pos(PlayerObject()); 
   gMapLoc = maploc;
   sprintf(buf,"%s/p001ra",lvl); 
   FetchUIRects(buf, tmpRects);

   if (maploc >= 0 && maploc < tmpRects.Size())
   {
      /*
      mx = tmpRects[maploc].ul.x;
      my = tmpRects[maploc].ul.y;
      */
      Point mappt;
      FindObjectInMapCoords(PlayerObject(), &mappt, NULL);
      mx = mappt.x;
      my = mappt.y;

      // so we want to pretend to have a bigger cliprect
      // then gr_scale_bitmap ourselves down to draw our actual self

      grs_bitmap *bmp, *newbitmap;
      grs_canvas newcanv;

      bmp = (grs_bitmap *) gMinimapHnd->Lock();
      newbitmap = gr_alloc_bitmap(bmp->type,bmp->flags, MINIBIG_W, MINIBIG_H);
      newbitmap->align = bmp->align;
      gr_make_canvas(newbitmap, &newcanv);
      gr_push_canvas(&newcanv);

      grs_clip saveClip = grd_gc.clip;                 // Clip it good!

      gr_safe_set_cliprect(r2.ul.x, r2.ul.y, r2.lr.x, r2.lr.y) ;
      /*
      offset_x = ((RectWidth(&r2) - RectWidth(&tmpRects[maploc])) / 2);
      offset_y = ((RectHeight(&r2) - RectHeight(&tmpRects[maploc])) / 2);
      */
      offset_x = RectWidth(&r2) / 2;
      offset_y = RectHeight(&r2) / 2;

      drawpt.x = r2.ul.x - mx + offset_x; 
      drawpt.y = r2.ul.y - my + offset_y; 
      bw = INNERMAP_W;
      bh = INNERMAP_H; 
      // make sure we always draw a full portion
      if (drawpt.x > r2.ul.x)
         drawpt.x = r2.ul.x;
      if (drawpt.y > r2.ul.y)
         drawpt.y = r2.ul.y;
      if (drawpt.x < r2.lr.x - bw)
         drawpt.x = r2.lr.x - bw;
      if (drawpt.y < r2.lr.y - bh)
         drawpt.y = r2.lr.y - bh;
      if (pShockPlayer->HasTrait(PlayerObject(),kTraitAutomap))
         ShockMapDrawContents(drawpt, gMiniAutoMapHnd, TRUE);
      else
         ShockMapDrawContents(drawpt, gMiniMapHnd, FALSE);

      // restore the old cliprect
      grd_gc.clip = saveClip;
      gr_pop_canvas();

      // now draw out our scaled bitmap
      gr_scale_bitmap(newbitmap, r.ul.x, r.ul.y, RectWidth(&r), RectHeight(&r));

      gr_free(newbitmap);
      gMinimapHnd->Unlock();
   }
}
//--------------------------------------------------------------------------------------
void ShockMiniMapStateChange(int which)
{
   gMapLoc = -1;
   if (ShockOverlayCheck(which))
   {
      char buf[256];

      // just got put up
      char lvl[255];
      ShockGetLevelName(lvl,sizeof(lvl));
      sprintf(buf,"intrface\\%s",lvl);
      gMiniMapHnd = LoadPCX("page001",buf);
      gMiniAutoMapHnd = LoadPCX("page001a",buf);
   }
   else
   {
      // just got taken down
      SafeFreeHnd(&gMiniMapHnd);
      SafeFreeHnd(&gMiniAutoMapHnd);
   }
}
//--------------------------------------------------------------------------------------
bool ShockMiniMapCheckTransp(Point pt)
{
   // always return true since it is effectively transparent
   return(TRUE);
}
//--------------------------------------------------------------------------------------
bool ShockHasMap()
{
   /*
   AutoAppIPtr(QuestData);
   char qbname[255];
   char lvl[255];
   ShockGetLevelName(lvl,sizeof(lvl));

   sprintf(qbname,"Map%s",lvl);
   if (pQuestData->Get(qbname))
      return(TRUE);
   else
      return(FALSE);
   */
   return(TRUE);
}
//--------------------------------------------------------------------------------------
IRes *ShockMiniMapBitmap(void)
{
   return(gMinimapHnd);
}
//--------------------------------------------------------------------------------------
bool ShockMiniMapUpdate(void)
{
   // do we need to be refreshed?
   /*
   int newmap = map_player_pos();
   if (newmap != gMapLoc)
      return(TRUE);
   return(FALSE);
   */
   return(TRUE);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayMap = { 
   ShockMapDraw, // draw
   ShockMapInit, // init
   ShockMapTerm, // term
   ShockMapHandleMouse, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "subpanel_op", // upschema
   "subpanel_cl", // downschema
   ShockMapStateChange, // state
   NULL, // transparency
   0, // distance
   TRUE, // needmouse
   0, // alpha
};

sOverlayFunc OverlayMiniMap = { 
   ShockMiniMapDraw, // draw
   ShockMiniMapInit, // init
   ShockMiniMapTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   ShockMiniMapBitmap, // bitMiniMap
   "", // upschema
   "", // downschema
   ShockMiniMapStateChange, // state
   NULL, // transparency
   0, // distance
   FALSE, // needmouse
   200, // alpha
   ShockMiniMapUpdate, // update
};
