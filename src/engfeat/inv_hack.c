/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/inv_hack.c,v 1.24 2000/02/19 13:15:05 toml Exp $
// silly inv (mostly for frob testing for now)

#include <stdlib.h>

#include <g2.h>
#include <config.h>

#include <mprintf.h>

#include <appagg.h>
#include <objtype.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <objpos.h>
#include <rendprop.h>
#include <command.h>
#include <inv_hack.h>
#include <inv_rend.h>
#include <physapi.h>
#include <objedit.h>
#include <prjctile.h>
#include <playrobj.h>
#include <frobctrl.h>

#include <guistyle.h>
#include <font.h>

// for silly inv properties for now
#include <propface.h>
#include <property.h>
#include <propbase.h>

///////////////////////////
// actual implementation of system

static IIntProperty   *pInvHotkey;
static IIntProperty   *pInvCount;
static IBoolProperty  *pInvHeld;
static ILabelProperty *pInvLimbModel;

#define MAX_INV_OBJS 20
static ObjID inv_list[MAX_INV_OBJS];
static int   inv_ptr=-1;

#ifndef SHIP
static BOOL yoyo_inv=FALSE;
#endif

// since our loops end at end, lets just make that the not found sentinel
#define SLOT_NOT_FOUND MAX_INV_OBJS

// for setting inv rend state
#define BASE_INV_FLAGS INVREND_ROTATE

///////////////////////////
// UI silliness for now

int  g_inv_bcolor=0;
int  g_inv_timeout=5000; // ms the inv obj will remain up without auto-downing
BOOL g_inv_up=FALSE;     // is the inv obj currently up
static int cur_inv_up_time=0;
static invRendState *cur_inv_rend=NULL;
static BOOL draw_inv_txt=FALSE;

// hack control of where inventory draws
static float hpbar_draw_parms[4]={1.0,1.0,0.18,0.20};
static float inv_draw_parms[4]={1.0,1.0,0.18,0.20};

#define GetDrawX(a) (grd_canvas->bm.w*a[0])
#define GetDrawY(a) (grd_canvas->bm.h*a[1])
#define GetDrawW(a) (grd_canvas->bm.w*a[2])
#define GetDrawH(a) (grd_canvas->bm.h*a[3])

#define INV_DRAW_X  (GetDrawX(inv_draw_parms))
#define INV_DRAW_Y  (GetDrawY(inv_draw_parms))
#define INV_DRAW_W  (GetDrawW(inv_draw_parms))
#define INV_DRAW_H  (GetDrawH(inv_draw_parms))

#define HPBAR_DRAW_X  (GetDrawX(hpbar_draw_parms))
#define HPBAR_DRAW_Y  (GetDrawY(hpbar_draw_parms))
#define HPBAR_DRAW_W  (GetDrawW(hpbar_draw_parms))
#define HPBAR_DRAW_H  (GetDrawH(hpbar_draw_parms))

static grs_canvas inv_md_canvas;

static void get_cnv_xy(float *which, int *xp, int *yp)
{
   int x=GetDrawX(which),y=GetDrawY(which);   // if X < mid-screen, assume we want to be
   if (x>grd_canvas->bm.w/2) x-=GetDrawW(which)+1;
   if (y>grd_canvas->bm.h/2) y-=GetDrawH(which)+1;
   *xp=x; *yp=y;
}

static grs_canvas *get_model_draw_canvas(void)
{
   int x,y;
   static BOOL inited = FALSE;

   if (inited)
      gr_close_canvas(&inv_md_canvas);

   get_cnv_xy(inv_draw_parms,&x,&y);
   gr_init_sub_canvas(grd_canvas, &inv_md_canvas, x, y, INV_DRAW_W, INV_DRAW_H);
   inited = TRUE;
   return &inv_md_canvas;
}

static void inv_show_model(void)
{
   if (cur_inv_rend)
   {
      invRendUpdateState(cur_inv_rend,0,inv_list[inv_ptr],get_model_draw_canvas());
      invRendDrawUpdate(cur_inv_rend);
   }
}

// horrible hack to attempt to for now allow a way to set good model flags
BOOL invHackSetActive(BOOL active)
{
   int use_flags=active?INVREND_ROTATE:INVREND_DISTANT;
   if (cur_inv_rend)
      invRendUpdateState(cur_inv_rend,use_flags|INVREND_SET|BASE_INV_FLAGS,OBJ_NULL,NULL);
   else
      return FALSE;
   return TRUE;
}

static void inv_show_string(void)
{
   int x,y,cnt;
   char buf[10];
   if (!draw_inv_txt) return;
   get_cnv_xy(inv_draw_parms,&x,&y);
   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG));
   gr_string((char *)ObjEditName(inv_list[inv_ptr]),x+3,y+INV_DRAW_H-12-2);
   if (PROPERTY_GET(pInvCount,inv_list[inv_ptr],&cnt))
      gr_string(itoa(cnt,buf,10),x+3,y+INV_DRAW_H-12-12-2);
   guiStyleCleanupFont(NULL,StyleFontNormal);
}

#define HPBAR_HACK
#ifdef HPBAR_HACK
#include <objhp.h>
#include <scrnovls.h>
//EXTERN int master_overlay;
static int g_hpbar_color;
static void inv_draw_hp_bar_hack(void)    // BOOL redraw)
{
   int x,y,max_hp,hp,last_hp=0xffffffff;
   if (hpbar_draw_parms[0]<0.001) return; // no draw if parms 0
   get_cnv_xy(hpbar_draw_parms,&x,&y);
   if (ObjGetHitPoints(PlayerObject(),&hp))
//   if ((hp!=last_hp)||(redraw))
      if (ObjGetMaxHitPoints(PlayerObject(),&max_hp))
      {
         float zing=1.0;
         gr_set_fcolor(0);
         gr_rect(HPBAR_DRAW_X,HPBAR_DRAW_Y,HPBAR_DRAW_X+HPBAR_DRAW_W,HPBAR_DRAW_Y+HPBAR_DRAW_H);
         gr_set_fcolor(g_hpbar_color);
         if (max_hp<1) max_hp=1;
         if (hp>max_hp) max_hp=hp;
         zing=hp*HPBAR_DRAW_W/max_hp;
         gr_rect(HPBAR_DRAW_X,HPBAR_DRAW_Y,HPBAR_DRAW_X+zing,HPBAR_DRAW_Y+HPBAR_DRAW_H);
         last_hp=hp;
      }   
}
#endif

void inv_draw(void)
{
   inv_show_model();
   inv_show_string();
}

void inv_lower(void)
{
   g_inv_up=FALSE;
   if (cur_inv_rend)
   {
      invRendFreeState(cur_inv_rend);
      cur_inv_rend=NULL;
   }
}

void inv_raise(void)
{
   if (!cur_inv_rend)
      cur_inv_rend=invRendBuildState(BASE_INV_FLAGS,inv_list[inv_ptr],get_model_draw_canvas());
   cur_inv_up_time=0;
   g_inv_up=TRUE;
}



void inv_update_frame(int ms)
{
   if (g_inv_up)
   {
#ifdef DBG_ON
      if ((inv_ptr==-1)||(inv_list[inv_ptr]==OBJ_NULL))
      {
         Warning(("InvUp but bad data (ptr %d list %x)\n",inv_ptr,inv_ptr>=0?inv_list[inv_ptr]:666));
         inv_lower();
      }
      else
#endif
      {
         cur_inv_up_time+=ms;
         if ((g_inv_timeout!=0)&&(cur_inv_up_time>g_inv_timeout))
            inv_lower();
         if (g_inv_up)  // if we are still up
            inv_draw(); // draw
      }
   }
#ifdef HPBAR_HACK
   // now, the hp bar, somehow... should be master_overlay
   if ((g_inv_up)||(ScreenOverlayCurrentlyUp(0)))
      inv_draw_hp_bar_hack();
#endif
}

////////////////////////
// low level utility/helpers

static int inv_get_free(void)
{
   int i=0;
   while ( i<MAX_INV_OBJS && inv_list[i]!=OBJ_NULL )
      i++;
   return i;
}

static int inv_find_obj(ObjID o)
{
   int i=0;
   while ( i<MAX_INV_OBJS && inv_list[i]!=o)
      i++;
   return i;
}

// moves inv_ptr to next occupied slot in direction... dir must be + or - 1
// sets to -1 if no slots in use in that dir
static int inv_cycle_occupied(int dir)
{
   int i=inv_ptr;
   if ((i==-1)&&(dir<0))  // reverse cycle from front to end
      i=MAX_INV_OBJS;
   do {
      i+=dir;
   } while ( i>=0 && i<MAX_INV_OBJS && inv_list[i]==OBJ_NULL);
   if (i>=MAX_INV_OBJS) i=-1;
   return i;
}

static void set_inv_ptr(int n_pos, BOOL do_gui)
{
   inv_ptr=n_pos;
   if (n_pos!=-1)
   {
      if (do_gui)
         inv_raise();
      FrobSetCurrent(kFrobLocInv,kFrobLevSelect,inv_list[n_pos]);
   }
   else
   {
      if (do_gui)
         inv_lower();
      FrobSetCurrent(kFrobLocInv,kFrobLevSelect,OBJ_NULL);
   }
}

////////////////////
// raw move into/out of inv slots code

// @TODO: when these are per player/owner, maintain inv_ptr smarter, eh?
// takes obj and adds to inventory list - focuses inventory on new obj
BOOL invAddObj(ObjID o)
{
   int n_pos=inv_get_free();

   if (n_pos==SLOT_NOT_FOUND) return FALSE;
   // should we magically support count here?


   inv_list[n_pos]=o;       // add to list
   set_inv_ptr(n_pos,TRUE); // @API?: for now, auto-focus on last added object
   return TRUE;
}

// takes obj, removes from inv list
BOOL invRemObj(ObjID o)
{
   int pos=inv_find_obj(o);
   
   if (pos==SLOT_NOT_FOUND) return FALSE;
   // should we magically support count here?
   

   inv_list[pos]=OBJ_NULL;       // remove from list
   if (inv_ptr==pos)
      set_inv_ptr(-1,TRUE);
   return TRUE;
}

///////////////////
// interface to world for moving objs to and from inv

// take obj from world and put into inventory
// @TODO: use owner to determine whose link set when we are links not array
BOOL invTakeObjFromWorld(ObjID o, ObjID owner)
{
   if (owner!=PlayerObject())
   {
      Warning(("TakeObj %d %d:We dont yet support non-player Inv\n",o,owner));
      return FALSE;
   }
   
   // store object into inventory
   if (!invAddObj(o)) return FALSE;   // didnt fit, so punt
   
   ObjSetHasRefs(o,FALSE);   // punt refs
   // kill pos here
   PhysDeregisterModel(o);
   return TRUE;
}

// take object in inventory (o) and put it back in world at src
BOOL invPutBackInWorld(ObjID o, ObjID src)
{  // add refs and a position
   if (src!=PlayerObject())
   {
      Warning(("PutBack %d %d:We dont yet support non-player Inv\n",o,src));
      return FALSE;
   }
   if (invRemObj(o))
   {
      ObjSetHasRefs(o,TRUE);
      if (src)      // put it in the right place, eh?
      {  // should we add physics here?
         // yes! (cc)
         PhysRegisterSphereDefault(o);
         ObjPosCopyUpdate(o,ObjPosGet(src));
      }
      return TRUE;      
   }
   else
      return FALSE;
}

// take object in inventory (o) and launch it from source (src is NECESSARY)
BOOL invThrowBackToWorld(ObjID o, ObjID src)
{
   if (src==OBJ_NULL) return FALSE;  // need a source
#ifdef SHIP
   if (invRemObj(o))
#else
   if (yoyo_inv || invRemObj(o))     // if yoyo, dont rem from inv, as brutal hack for test
#endif      
   {
      ObjSetHasRefs(o,TRUE);
      launchProjectileDefault(src,o,12.0);
      return TRUE;
   }
   else
      return FALSE;
}

// actually deletes an object in the inventory from the world
BOOL invDelObj(ObjID o)
{
   BOOL rv=invRemObj(o);
   if (rv)
   {
      IObjectSystem* objsys = AppGetObj(IObjectSystem);
      IObjectSystem_Destroy(objsys,o);
      SafeRelease(objsys);
   }
   return rv;
}

Label *invGetLimbModelName(ObjID o)
{
   Label *pName;

   if (PROPERTY_GET(pInvLimbModel,o,&pName))
      return pName;
   return NULL;
}

////////////////////////////
// command implementations
#ifndef SHIP
static void inv_dump_cmd(void)
{
   int i;
   mprintf("Inventory size %d ptr %d\n",MAX_INV_OBJS,inv_ptr);
   for (i=0; i<MAX_INV_OBJS; i++)
      if (inv_list[i]!=OBJ_NULL)
         mprintf("%s>%s\n",i==inv_ptr?"cur":"   ",ObjEditName(inv_list[i]));
      else
         mprintf("%s empty\n",i==inv_ptr?"cur":"   ");
}
#endif

void inv_clear(int destroy)
{
   int i;
   for (i=0; i<MAX_INV_OBJS; i++)
      if (inv_list[i]!=OBJ_NULL)
         if (destroy)
            invDelObj(i);  // remove from inventory, dont put back in world
         else
            inv_list[i]=OBJ_NULL;
   inv_ptr=-1;
}

////////////////////
// commands to be called by inv clients

void invHackCycle(int cycle_dir)
{
   int n_ptr;

   FrobAbort(kFrobLocInv,NULL);
   
   if (cycle_dir==0) n_ptr=0;
   else n_ptr=inv_cycle_occupied(cycle_dir);
   
   if ((n_ptr!=-1)&&(inv_list[n_ptr]!=OBJ_NULL))
      set_inv_ptr(n_ptr,TRUE);
   else
   {
      inv_ptr=n_ptr;
      FrobSetCurrent(kFrobLocInv,kFrobLevSelect,OBJ_NULL);
      inv_lower();
   }
}

void invHackDisplay(int display_action)
{
   BOOL do_raise=(display_action==1)||((display_action==0)&&(!g_inv_up));
   if (do_raise)
    { if (inv_ptr!=-1) inv_raise(); }
   else                inv_lower();
}

////////////////////
// misc internals

void inv_hotkey_use(int hk)
{
   int i, hk_val;
   for (i=0; i<MAX_INV_OBJS; i++)
      if (inv_list[i]!=OBJ_NULL)
         if (PROPERTY_GET(pInvHotkey,inv_list[i],&hk_val))
            if (hk==hk_val)
            {
               set_inv_ptr(i,TRUE);
               return;
            }
}

void inv_hotkey_bind(int hk)
{
   if ((inv_ptr!=-1)&&(inv_list[inv_ptr]!=OBJ_NULL))
   {
      int old_val;
      if (PROPERTY_GET(pInvHotkey,inv_list[inv_ptr],&old_val))
         Warning(("Changing hotkey from %d to %d for %x\n",old_val,hk,inv_list[inv_ptr]));
      PROPERTY_SET(pInvHotkey,inv_list[inv_ptr],hk);
   }
   else
      Warning(("No inv object, cant bind key!"));
}

void inv_set_to_null(void)
{
   inv_ptr=-1;
   FrobAbort(kFrobLocInv,NULL);
   FrobSetCurrent(kFrobLocInv,kFrobLevSelect,OBJ_NULL);
   inv_lower();
}

///////////////////////////
// commands and init

// should this have inv_show and inv_hide as well?

static Command invsys_keys[]=
{
   {"inv_clear",FUNC_INT,inv_clear,"erase/destroy all inv objects"},
#ifndef SHIP
   {"inv_dump",FUNC_VOID,inv_dump_cmd,"mprint all inventory"},
#endif
   {"inv_hotkey",FUNC_INT,inv_hotkey_use,"go to obj with hotkey prop of this"},
   {"inv_hotkey_bind",FUNC_INT,inv_hotkey_bind,"set hotkey bind of cur inv to this"},
};

// property desc fun
static sPropertyDesc inv_hotkey_desc = { "InvHotkey", 0, NULL, 0, 0, { "Inventory", "Hotkey" } };
static sPropertyDesc inv_count_desc =  { "InvCount", 0, NULL, 0, 0, { "Inventory", "Count" } };
static sPropertyDesc inv_held_desc =  { "InvHeld", 0, NULL, 0, 0, { "Inventory", "Held" } };
static sPropertyDesc inv_limb_desc =  { "InvLimbModel", 0, NULL, 0, 0, { "Inventory", "Limb Model" } };

void invsysInit(void)
{
   int i, cnt=4;
   for (i=0; i<MAX_INV_OBJS; i++)
      inv_list[i]=OBJ_NULL;
   inv_ptr=-1;
   COMMANDS(invsys_keys,HK_GAME_MODE);
#ifndef SHIP
   yoyo_inv=config_is_defined("yoyo_inv");
#endif
   config_get_int("inv_timeout",&g_inv_timeout);
   config_get_int("inv_bg_color",&g_inv_bcolor);
   config_get_value("inv_draw_parms",CONFIG_FLOAT_TYPE,(config_valptr)inv_draw_parms,&cnt);
   draw_inv_txt=config_is_defined("inv_strings");
   pInvHotkey=CreateIntProperty(&inv_hotkey_desc,kPropertyImplVerySparse);
   pInvCount=CreateIntProperty(&inv_count_desc,kPropertyImplVerySparse);
   pInvHeld=CreateBoolProperty(&inv_held_desc,kPropertyImplVerySparse);
   pInvLimbModel=CreateLabelProperty(&inv_limb_desc,kPropertyImplVerySparse);
#ifdef HPBAR_HACK
   cnt=4; config_get_value("hpbar_draw_parms",CONFIG_FLOAT_TYPE,(config_valptr)hpbar_draw_parms,&cnt);
   config_get_int("hpbar_color",&g_hpbar_color);
#endif
}

void invsysTerm(void)
{
   SafeRelease(pInvHotkey);
   SafeRelease(pInvCount);
   SafeRelease(pInvHeld);
}

// THIS IS HATEFUL
#include <relation.h>
#include <lnkquery.h>
#include <linkbase.h>
#include <linkman.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// HATEFUL!!!
BOOL invBackDoorObjFromWorld(ObjID o, ObjID owner)
{
   int n_pos=inv_get_free();
   if (n_pos==SLOT_NOT_FOUND) return FALSE;

   if (owner!=PlayerObject())
   {
      Warning(("TakeObj %d %d:We dont yet support non-player Inv\n",o,owner));
      return FALSE;
   }

   // should we magically support count here?
   inv_list[n_pos]=o;        // add to list
   ObjSetHasRefs(o,FALSE);   // punt refs
   PhysDeregisterModel(o);   // kill pos here
   return TRUE;
}

static BOOL copy_in=TRUE;

// the horror, for the demo
void PostLoadInitInv(BOOL prep_and_clear)
{
   if (prep_and_clear)
   {
//      inv_lower();
      inv_clear(FALSE);
      copy_in=TRUE;
   }
   else if (copy_in)
   {  // iterate across all of this link
      ILinkManager *pLinkMan=AppGetObj(ILinkManager);
      IRelation *invRel=ILinkManager_GetRelationNamed(pLinkMan,"InitInv");
      ILinkQuery *initial_inv=IRelation_Query(invRel,PlayerObject(),LINKOBJ_WILDCARD);

      for (; !ILinkQuery_Done(initial_inv); ILinkQuery_Next(initial_inv))
      {
         sLink link;
         ILinkQuery_Link(initial_inv,&link);
         invBackDoorObjFromWorld(link.dest,link.source);
      }
   
      SafeRelease(initial_inv);
      SafeRelease(invRel);
      SafeRelease(pLinkMan);

      copy_in=FALSE;
   }
}
