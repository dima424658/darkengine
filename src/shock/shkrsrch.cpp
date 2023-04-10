/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrsrch.cpp,v 1.30 2000/02/19 13:26:12 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>
#include <lgd3d.h>

#include <resapi.h>

#include <objsys.h>
#include <osysbase.h>
#include <mprintf.h>
#include <inv_rend.h>
#include <gamestr.h>
#include <traitman.h>
#include <scrnman.h>
#include <questapi.h>
#include <linkman.h>
#include <relation.h>
#include <lnkquery.h>
#include <linkbase.h>
#include <playrobj.h>
#include <simtime.h>
#include <contain.h>
#include <scrptapi.h>
#include <schema.h>
#include <combprop.h>
#include <netmsg.h>
#include <netman.h>

#include <shkprop.h>
#include <shkrsrch.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkrsprp.h>
#include <shkplayr.h>
#include <shkplprp.h>
#include <shkplcst.h>
#include <shkinv.h>
#include <shkmfddm.h>
#include <shkpda.h>
#include <shkiftul.h>
#include <shkparam.h>
#include <shkobjst.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

// fwd declarations
float ResearchComplete(ObjID o);
ObjID FindResearchedObject();
void doSetResearchText(ObjID o);
void SetResearchText(ObjID o);
int GetResearchQB(ObjID o, char *suffix);
void SetResearchQB(ObjID o, int t, char *suffix);
BOOL ResearchQBFilter(const char *pName, int oldValue, int newValue);

static tSimTime gResearchClearTime = 0;
static tSimTime gLastNetworkedTime = 0;

void ShockResearchInvRend(bool up);
void BreakResearchLink(ObjID o);

static IRes *gBackHnd = NULL;
static IRes *gHndProgress = NULL;
static char gResearchText[1024];

static Rect text_rect = {{15,153},{138,261}};

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static Rect suspend_rect = {{157,152},{157 + 18, 152 + 134}};
static LGadButton suspend_button;
static DrawElement suspend_elem;
static IRes *suspend_handles[2];
static grs_bitmap *suspend_bitmaps[4];

static Rect report_rect = {{13, 243},{13 + 142, 243 + 21}};
static LGadButton report_button;
static DrawElement report_elem;
static IRes *report_handles[2];
static grs_bitmap *report_bitmaps[4];

extern "C" BOOL g_lgd3d;

//////////
//
// NETWORKING CODE
//

//
// Tell other players which research reports you've done.
//
// We specifically don't try to set the ResearchReport property for
// other players, because that risks screwing up due to raceway
// conditions.
//

static cNetMsg *g_pResearchResultsMsg = NULL;

static void handleResearchResults(ObjID arch, int newbits)
{
   int reportbits = 0;
   g_ResearchReportProperty->Get(PlayerObject(), &reportbits);
   reportbits = reportbits | newbits;
   g_ResearchReportProperty->Set(PlayerObject(), reportbits);

   ObjID resobj = FindResearchedObject();
   
   if (resobj != OBJ_NULL) {
      AutoAppIPtr(TraitManager);
      ObjID myArch = pTraitManager->GetArchetype(resobj);
      if (myArch == arch) {
         // Yep, we're researching it too, so update our screen as needed.
         // This code is pretty closely linked to the stuff in ResearchLoop;
         // we probably ought to subroutinize it, but I'm not going to
         // hassle with that right now...

         // give notification
         char temp[1024];
         ShockStringFetch(temp,sizeof(temp),"ResearchDone", "misc");
         ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
         SchemaPlay((Label *)"bb06",NULL);         

         // register what time we got the report at
         int i;
         for (i=0; i < 32; i++)
            if (newbits & (1 << i))
               ShockPDASetTime(0,4,i); // 4 is code for research, should be an enum but I suck

         // the object's script should automatically update it's 
         // objstate property upon listening to this QB change
         SetResearchQB(resobj,1,"ResearchState");

         // update text
         SetResearchText(resobj);

         // remove the researching link
         BreakResearchLink(resobj);

         AutoAppIPtr(ScriptMan);
         sScrMsg msg(resobj,"Research"); 
         pScriptMan->SendMessage(&msg); 
      }
   }
}

static sNetMsgDesc sResearchResultsDesc =
{
   kNMF_Broadcast,
   "ResResult",
   "Research Results",
   NULL,
   handleResearchResults,
   {{kNMPT_AbstractObjID, kNMPF_None, "Arch"},
    {kNMPT_Int, kNMPF_None, "Flag Bits"},
    {kNMPT_End}}
};

//
// Kick the other players to update their research text.
//
// When one player updates his research text, he needs to tell the others
// to do so as well, if they're researching the same thing. The others
// ignore this if they aren't researching that kind of object.
//
// The others should have the necessary info to display the right text;
// that's all networked at the QB level. They just need to know to redo
// their display text.
//

static cNetMsg *g_pResearchTextMsg = NULL;

static void handleResearchText(ObjID arch)
{
   ObjID resobj = FindResearchedObject();
   
   if (resobj != OBJ_NULL) {
      AutoAppIPtr(TraitManager);
      ObjID myArch = pTraitManager->GetArchetype(resobj);
      if (myArch == arch) {
         // Yep, we're researching it too
         doSetResearchText(resobj);
      }
   }
}

static sNetMsgDesc sResearchTextDesc =
{
   kNMF_Broadcast,
   "ResText",
   "Research Text",
   NULL,
   handleResearchText,
   {{kNMPT_AbstractObjID, kNMPF_None, "Archetype"},
    {kNMPT_End}}
};

static void ShockResearchBroadcastText(ObjID obj)
{
   if (obj != OBJ_NULL) {
      AutoAppIPtr(TraitManager);
      ObjID arch = pTraitManager->GetArchetype(obj);
      g_pResearchTextMsg->Send(OBJ_NULL, arch);
   }
}

// These are separate from the overlay init, because they have to happen
// only once, at app init/term:
void ShockResearchNetInit()
{
   g_pResearchResultsMsg = new cNetMsg(&sResearchResultsDesc);
   g_pResearchTextMsg = new cNetMsg(&sResearchTextDesc);
   AutoAppIPtr(QuestData);
   pQuestData->Filter(ResearchQBFilter, NULL);
}

void ShockResearchNetTerm()
{
   delete g_pResearchResultsMsg;
   delete g_pResearchTextMsg;
}

//--------------------------------------------------------------------------------------
static invRendState *cur_inv_rend=NULL;
Rect *get_model_draw_rect(void);
void inv_show_model(ObjID o);

//--------------------------------------------------------------------------------------
void ShockResearchInit(int which)
{
   int i;

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   suspend_handles[0] = LoadPCX("sus0"); 
   suspend_handles[1] = LoadPCX("sus1"); 
   suspend_bitmaps[0] = (grs_bitmap *) suspend_handles[0]->Lock();
   suspend_bitmaps[1] = (grs_bitmap *) suspend_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      suspend_bitmaps[i] = suspend_bitmaps[0];
   }

   report_handles[0] = LoadPCX("report0"); 
   report_handles[1] = LoadPCX("report1"); 
   report_bitmaps[0] = (grs_bitmap *) report_handles[0]->Lock();
   report_bitmaps[1] = (grs_bitmap *) report_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      report_bitmaps[i] = report_bitmaps[0];
   }

   gBackHnd = LoadPCX("research");
   gHndProgress = LoadPCX("resprog");

   SetLeftMFDRect(which, full_rect);

   ObjID o;
   o = FindResearchedObject();
   SetResearchText(o);
}

//--------------------------------------------------------------------------------------
void ShockResearchTerm(void)
{
   SafeFreeHnd(&gBackHnd);
   SafeFreeHnd(&gHndProgress);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   suspend_handles[0]->Unlock();
   suspend_handles[1]->Unlock();
   SafeFreeHnd(&suspend_handles[0]);
   SafeFreeHnd(&suspend_handles[1]);

   report_handles[0]->Unlock();
   report_handles[1]->Unlock();
   SafeFreeHnd(&report_handles[0]);
   SafeFreeHnd(&report_handles[1]);
}

//--------------------------------------------------------------------------------------
#define TEXT_X 24
#define TEXT_Y 133
#define PROGRESS_X (r.ul.x + 15)
#define PROGRESS_Y (r.ul.y + 267)
#define PROGRESS_W 138
void ShockResearchDraw(void)
{
   Rect r;
   Point drawpt;
   int w,h;
   int dx,dy;
   char temp[255];
   cStr str;
   ObjID resobj;
   int rescolor[3] = {128,128,128};

   AutoAppIPtr(GameStrings);

   r = ShockOverlayGetRect(kOverlayResearch);
   drawpt = r.ul;
   DrawByHandle(gBackHnd,drawpt);

   resobj = FindResearchedObject();

   if (resobj != OBJ_NULL)
      inv_show_model(resobj);

   // should we timeout a "suspend" message?
   tSimTime curtime = GetSimTime();
   if ((gResearchClearTime != 0) && (gResearchClearTime < curtime))
   {
      SetResearchText(resobj);
   }

   gr_set_fcolor(gShockTextColor);

   // draw in the shortname
   eObjState st = ObjGetObjState(resobj);
   if (st != kObjStateUnresearched)
   {
      str = pGameStrings->FetchObjString(resobj,PROP_OBJSHORTNAME_NAME);
      strcpy(temp,str);
   }
   else
      ShockStringFetch(temp,sizeof(temp),"NameUnresearched","research");
   dx = r.ul.x + TEXT_X;
   dy = r.ul.y + TEXT_Y;
   gr_font_string(gShockFont,temp,dx,dy);

   // draw in the text
   h = gr_font_string_height(gShockFont,gResearchText);
   w = gr_font_string_width(gShockFont,gResearchText);
   dx = r.ul.x + text_rect.ul.x; //  + (RectWidth(&text_rect) - w) / 2;
   dy = r.ul.y + text_rect.ul.y; //  + (RectHeight(&text_rect) - h) / 2;
   gr_font_string(gShockFont,gResearchText,dx,dy);

   // fill in the research time bar
   float p;
   int clippix;
   grs_bitmap *bm;
   grs_clip saveClip;

   p = ResearchComplete(resobj);
   clippix = p * PROGRESS_W;
   bm = (grs_bitmap *) gHndProgress->Lock();
   saveClip = grd_gc.clip;                 // Clip it good!
   gr_safe_set_cliprect(PROGRESS_X, PROGRESS_Y,PROGRESS_X + clippix,PROGRESS_Y + bm->h);
   gr_bitmap(bm, PROGRESS_X, PROGRESS_Y);
   gHndProgress->Unlock();
   grd_gc.clip = saveClip;

   // draw in our completion amount
   sprintf(temp,"%.01f %%",p * 100);
   w = gr_font_string_width(gShockFont,temp);
   dx = PROGRESS_X + (PROGRESS_W - w) / 2;
   dy = PROGRESS_Y + 3;
   gr_set_fcolor(gShockTextColor); // FindColor(rescolor));
   gr_font_string(gShockFontMono,temp,dx,dy);

   if ((resobj != OBJ_NULL) && (p != 0) && (p != 1))
      LGadDrawBox(VB(&suspend_button),NULL);

   // reports button
   LGadDrawBox(VB(&report_button), NULL);

   // close button
   LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
// return true if we have used it, and want it to be destroyed
BOOL ShockResearchConsume(ObjID obj)
{
   AutoAppIPtr(TraitManager);
   ObjID needed, arch, resobj;
   resobj = FindResearchedObject();
   arch = pTraitManager->GetArchetype(obj);
   needed = GetResearchQB(resobj, "ResearchNeeded");
   if (arch == needed)
   {
      SetResearchQB(resobj, OBJ_NULL, "ResearchNeeded");
      SetResearchText(resobj);
      return(TRUE);
   }
   else
   {
      char errmsg[255];
      ShockStringFetch(errmsg,sizeof(errmsg),"ChemNotNeeded","misc");
      ShockOverlayAddText(errmsg, DEFAULT_MSG_TIME);
   }
   return(FALSE);
}
//--------------------------------------------------------------------------------------
bool ShockResearchHandleMouse(Point pt)
{
   if (shock_cursor_mode == SCM_DRAGOBJ)
   {
      if (ShockResearchConsume(drag_obj))
      {
         int count;
         // consume one object from the stack
         if (!gStackCountProp->Get(drag_obj,&count) || (count == 1))
         {
            ShockInvDestroyCursorObj();
         }
         else
         {
            AutoAppIPtr(ContainSys);
            pContainSys->StackAdd(drag_obj,-1);
         }
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      uiDefer(DeferOverlayClose,(void *)kOverlayResearch);
      SchemaPlay((Label *)"subpanel_cl",NULL);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BreakResearchLink(ObjID resobj)
{
   IRelation *pResRel;
   LinkID lid;

   AutoAppIPtr(LinkManager);

   pResRel = pLinkManager->GetRelationNamed("Research");

   // snap the researching link
   lid = pResRel->GetSingleLink(PlayerObject(), resobj);
   pResRel->Remove(lid);
}
//--------------------------------------------------------------------------------------
static void CancelResearch(ObjID resobj)
{
   if (resobj == OBJ_NULL)
      return;

   BreakResearchLink(resobj);

   // update the text
   SetResearchText(OBJ_NULL);
}
//--------------------------------------------------------------------------------------
static bool suspend_cb(short action, void* data, LGadBox* vb)
{
   ObjID resobj;
   float p;

   resobj = FindResearchedObject();
   if (resobj == OBJ_NULL)
      return(TRUE);

   p = ResearchComplete(resobj);
   if ((p != 0.0) && (p != 1.0))
   {
      CancelResearch(resobj);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool report_cb(short action, void* data, LGadBox* vb)
{
   ShockOverlayChange(kOverlayPDA, kOverlayModeOn);
   ShockPDASetFilter(4); // 3);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayResearch);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   suspend_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   suspend_elem.draw_data = suspend_bitmaps;
   suspend_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&suspend_button, LGadCurrentRoot(), suspend_rect.ul.x + r.ul.x, suspend_rect.ul.y + r.ul.y,
      RectWidth(&suspend_rect), RectHeight(&suspend_rect), &suspend_elem, suspend_cb, 0);

   report_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   report_elem.draw_data = report_bitmaps;
   report_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&report_button, LGadCurrentRoot(), report_rect.ul.x + r.ul.x, report_rect.ul.y + r.ul.y,
      RectWidth(&report_rect), RectHeight(&report_rect), &report_elem, report_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&suspend_button),FALSE);
   LGadDestroyBox(VB(&report_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockResearchInvRend(bool up)
{
   if (up)
   {
      // just got put up
      if (!cur_inv_rend)
      {
         cur_inv_rend=invRendBuildState(0,FindResearchedObject(),get_model_draw_rect(), NULL);
         invRendUpdateState(cur_inv_rend,INVREND_ROTATE|INVREND_HARDWARE_IMMEDIATE|INVREND_SET,OBJ_NULL,NULL,NULL);
      }
   }
   else
   {
      // just got taken down
      if (cur_inv_rend)
      {
         invRendFreeState(cur_inv_rend);
         cur_inv_rend=NULL;
      }
   }
}
//--------------------------------------------------------------------------------------
void ShockResearchStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      BuildInterfaceButtons();

      ShockResearchInvRend(TRUE);
   }
   else
   {
      DestroyInterfaceButtons();
      ShockResearchInvRend(FALSE);
   }
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
/*
bool ShockResearchCheckTransp(Point pt)
{
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayResearch, gBackHnd);
   return(retval);
}
*/
//--------------------------------------------------------------------------------------
// This may be kicked off remotely, if another player is researching the
// same kinda thing.
static void doSetResearchText(ObjID o)
{
   AutoAppIPtr(GameStrings);
   AutoAppIPtr(ObjectSystem);
   cStr str;
   ObjID needed;
   
   if (o != OBJ_NULL)
      needed = GetResearchQB(o,"ResearchNeeded");

   if (o == OBJ_NULL)
   {
      ShockStringFetch(gResearchText, sizeof(gResearchText), "ResearchReady", "misc");      
   }
   else if (needed != OBJ_NULL)
   {
      char temp[1024];
      cStr str;
      ShockStringFetch(temp,sizeof(temp),"ResearchChemNeeded", "misc");
      //sprintf(gResearchText, temp, pObjectSystem->GetName(needed));
      str = pGameStrings->FetchObjString(needed,PROP_OBJSHORTNAME_NAME);
      sprintf(gResearchText, temp, str);
   }
   else if (GetResearchQB(o,"ResearchState") == 1)
   {
      ShockStringFetch(gResearchText, sizeof(gResearchText), "ResearchDone", "misc");
   }
   else
   {
      str = pGameStrings->FetchObjString(o,PROP_RESEARCHTEXT_NAME);
      strcpy(gResearchText, str);
   }
   gr_font_string_wrap(gShockFont, gResearchText, RectWidth(&text_rect));
}
static void SetResearchText(ObjID o)
{
   doSetResearchText(o);
   ShockResearchBroadcastText(o);
}
//--------------------------------------------------------------------------------------
void ShockResearch(ObjID o) 
{
   // don't bring up if one already up
   //if (ShockOverlayCheck(kOverlayResearch))
   //  return;

   ObjID resobj;
   IRelation *pResRel;
   BOOL allowadd = FALSE;
   int skillval, needed;
   sTechSkills *pTech;

   AutoAppIPtr(LinkManager);
   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(ContainSys);

   // must have research skill
   skillval = pShockPlayer->GetTechSkill(kTechResearch);
   /*
   if (skillval == 0)
   {
      char errmsg[255];
      ShockStringFetch(errmsg, sizeof(errmsg), "ResearchSkillNeeded", "misc");
      ShockOverlayAddText(errmsg, DEFAULT_MSG_TIME);
      return;
   }
   */

   // must have sufficient research skill
   needed = 1;
   if (g_BaseTechProperty->Get(o,&pTech))
      needed = pTech->m_tech[kTechResearch];
   if (skillval < needed)
   {
      char errmsg[255], temp[255];
      ShockStringFetch(errmsg, sizeof(errmsg), "ResearchSkillNeeded", "misc");
      sprintf(temp,errmsg,needed);
      ShockOverlayAddText(temp, DEFAULT_MSG_TIME);
      return;
   }

   // must have research software
   /*
   if (pShockPlayer->GetEquip(PlayerObject(), kEquipResearch) == OBJ_NULL)
   {
      char errmsg[255];
      ShockStringFetch(errmsg, sizeof(errmsg), "ResearchSoftNeeded", "misc");
      ShockOverlayAddText(errmsg, DEFAULT_MSG_TIME);
      return;
   }
   */

   // object must be in inventory
   if ((o != OBJ_NULL) && !pContainSys->Contains(PlayerObject(), o))
      return;

   resobj = FindResearchedObject();
   pResRel = pLinkManager->GetRelationNamed("Research");

   if ((o != OBJ_NULL) && (resobj != o))
   {
      // check for pre-existing research obj?
      if (resobj == OBJ_NULL)
      {
         allowadd = TRUE;
      }
      else
      {
         // alternatively, if we are done researching this item
         // allow a replacement
         if (GetResearchQB(resobj,"ResearchState") == 1)
            allowadd = TRUE;
      }
  
      // establish researching link, if legal
      // set text (reject or accept)
      if (allowadd)
      {
         pResRel->Add(PlayerObject(), o);
         SetResearchText(o);
         ShockResearchInvRend(FALSE);
         ShockResearchInvRend(TRUE);
      }
      else
      {
         // if we are trying to re-research the thing we are already working
         // on (by type, not by ID) then just don't do anything more.
         AutoAppIPtr(TraitManager);
         ObjID resarch = pTraitManager->GetArchetype(resobj);
         ObjID objarch = pTraitManager->GetArchetype(o);
         if (resarch != objarch)
         {
            ShockStringFetch(gResearchText, sizeof(gResearchText), "ResearchBusy", "misc");
            gr_font_string_wrap(gShockFont, gResearchText, RectWidth(&text_rect));
            gResearchClearTime = GetSimTime() + 5000;
         }
      }
   }

   ShockOverlayChange(kOverlayResearch, kOverlayModeOn);
}

//--------------------------------------------------------------------------------------
static ObjID FindResearchedObject()
{
   IRelation *pResRel;
   ILinkQuery *pQuery;
   AutoAppIPtr(LinkManager);

   pResRel = pLinkManager->GetRelationNamed("Research");

   ObjID retval = OBJ_NULL;

   pQuery = pResRel->Query(PlayerObject(), LINKOBJ_WILDCARD);

   if (!pQuery->Done())
   {
      sLink link;
      pQuery->Link(&link);
      retval = link.dest;
   }
   SafeRelease(pQuery);
   return(retval);
}

//--------------------------------------------------------------------------------------
void SetResearchQB(ObjID o, int t, char *suffix)
{
   ObjID arch;
   const char *archname;
   char qbname[255];
   int i;

   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(QuestData);

   arch = pTraitManager->GetArchetype(o);
   if (arch == OBJ_NULL)
      return;

   archname = pObjectSystem->GetName(arch);
   sprintf(qbname,"%s_%s",archname,suffix);
   // remove spaces from name
   for (i=0; i < strlen(qbname); i++)
   {
      if (qbname[i] == ' ')
         qbname[i] = '_';
   }
   if (pQuestData->Exists(qbname))
      pQuestData->Set(qbname,t);
   else
      pQuestData->Create(qbname,t,kQuestDataCampaign);
}
//--------------------------------------------------------------------------------------
int GetResearchQB(ObjID o, char *suffix)
{
   ObjID arch;
   AutoAppIPtr(TraitManager);
   arch = pTraitManager->GetArchetype(o);
   if (arch == OBJ_NULL)
      return(0);
   return(GetResearchQBArch(arch, suffix));
}
//--------------------------------------------------------------------------------------
int GetResearchQBArch(ObjID arch, char *suffix)
{
   const char *archname;
   char qbname[255];
   int retval;
   int i;

   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(QuestData);

   archname = pObjectSystem->GetName(arch);
   sprintf(qbname,"%s_%s",archname,suffix);
   // remove spaces from name
   for (i=0; i < strlen(qbname); i++)
   {
      if (qbname[i] == ' ')
         qbname[i] = '_';
   }
   retval = pQuestData->Get(qbname);
   return(retval);
}
//--------------------------------------------------------------------------------------
// Make sure that research time only goes up, not down. We need to make sure
// that multiple players researching the same thing don't step on each other.
static BOOL ResearchQBFilter(const char *pName, int oldValue, int newValue)
{
   if (strstr(pName, "ResearchTime") != NULL)
   {
      // This is a researchTime, so don't let it go down
      if (newValue >= oldValue)
      {
         return TRUE;
      } else {
         return FALSE;
      }
   } else {
      // Don't interfere with anything else
      return TRUE;
   }
}
//--------------------------------------------------------------------------------------
int GetResearchTotalTime(ObjID o)
{
   //ObjID arch;
   int totaltime;

   AutoAppIPtr(TraitManager);

   totaltime = 0;
   //arch = pTraitManager->GetArchetype(o);
   //if (arch == OBJ_NULL)
   //   return(0);

   // find out the required research time
   if (!gPropResearchTime->Get(o,&totaltime))
      totaltime = 0;
   return(totaltime * 1000);
}
//--------------------------------------------------------------------------------------
float ResearchComplete(ObjID o)
{
   int totaltime,researched;
   float retval;

   totaltime = GetResearchTotalTime(o);
   if (totaltime == 0)
      return(0);

   researched = GetResearchQB(o,"ResearchTime");

   retval = (float)researched / (float)totaltime;
   if (retval > 1.0)
      retval = 1.0;

   return(retval);
}
//--------------------------------------------------------------------------------------
// and now, a bunch of stuff stolen and slightly modified from Dark's inv_hack.c
//--------------------------------------------------------------------------------------
#define OBJAREA_X 15
#define OBJAREA_Y 14
static Rect *get_model_draw_rect(void)
{
   static Rect draw_rect;

   draw_rect = ShockOverlayGetRect(kOverlayResearch);
   draw_rect.ul.x += OBJAREA_X;
   draw_rect.ul.y += OBJAREA_Y;
   draw_rect.lr.x = draw_rect.ul.x + 138;
   draw_rect.lr.y = draw_rect.ul.y + 109;
   return &draw_rect;
}
//--------------------------------------------------------------------------------------
static void inv_show_model(ObjID o)
{
   if (cur_inv_rend)
   {
      invRendUpdateState(cur_inv_rend,0,o,get_model_draw_rect(),NULL);
      invRendDrawUpdate(cur_inv_rend);
   }
}
//--------------------------------------------------------------------------------------
#define SKILL_FACTOR 0.25F
#define RESEARCH_MSG_INTERVAL 1000
void ShockResearchLoop()
{
   tSimTime delta;
   ObjID o;
   int totaltime;
   int curval,oldtime;
   int skillval;
   int i;
   sChemInfo *pci;
   char temp[1024];
   sSkillParams *params = GetSkillParams();
   ObjID soft;
   int softlevel;
   BOOL networkMe = FALSE;

   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(QuestData);
   AutoAppIPtr(ScriptMan);

   delta = GetSimFrameTime();
   o = FindResearchedObject();

   // exit if we aren't researching anything
   if (o == OBJ_NULL)
      return;
   
   // or if what we are researching is already done
   if (GetResearchQB(o,"ResearchState") == 1)
      return;

   // are we blocked on needing a chemical?
   if (GetResearchQB(o,"ResearchNeeded") != OBJ_NULL)
      return;

   // advance the timer!
   totaltime = GetResearchTotalTime(o);
   if (totaltime == 0)
      return;

   oldtime = curval = GetResearchQB(o,"ResearchTime");
   soft = pShockPlayer->GetEquip(PlayerObject(), kEquipResearch);
   if (!gPropSoftwareLevel->Get(soft, &softlevel))
      softlevel = 0;
   skillval = pShockPlayer->GetTechSkill(kTechResearch) + softlevel;
   // extra 10% bonus for every skill level above 1, squared
   delta = (float)delta * (1.0 + (params->m_research * (float)(skillval - 1) * (float)(skillval - 1)));
   curval = curval + delta;

   // We keep a throttle on how often we actually send out the quest
   // data over the network, lest is over-saturate:
   tSimTime now = GetSimTime();
   if (now > (gLastNetworkedTime + RESEARCH_MSG_INTERVAL))
      networkMe = TRUE;

   // need more chemicals?
   if (gPropChemNeeded->Get(o, &pci))
   {
      for (i=0; i < MAX_CHEM_ITEMS; i++)
      {
         int chemtime = pci->m_time[i] * 1000;
         if (chemtime == 0)
            continue;

         if ((oldtime < chemtime) && (curval >= chemtime))
         {
            // trigger chemical needed
            ObjID chemarch; 
            chemarch = pObjectSystem->GetObjectNamed(pci->m_chem[i]);
            if (chemarch != OBJ_NULL)
            {
               SetResearchQB(o,chemarch,"ResearchNeeded");
               SetResearchText(o);
               ShockOverlayAddText(gResearchText,DEFAULT_MSG_TIME * 2);
               // need a sound effect too.
               SchemaPlay((Label *)"bb05",NULL);      
               // major state change, so update the network
               networkMe = TRUE;
            }
         }
      }
   }

   // are we done?
   if (curval > totaltime )
   {
      curval = totaltime;

       // give notification
      ShockStringFetch(temp,sizeof(temp),"ResearchDone", "misc");
      ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
      SchemaPlay((Label *)"bb06",NULL);         

      // set the right PDA bit
      int reportbits, newbits;
      reportbits = newbits = 0;
      g_ResearchReportProperty->Get(PlayerObject(), &reportbits);
      g_ResearchReportProperty->Get(o, &newbits);
      reportbits = reportbits | newbits;
      g_ResearchReportProperty->Set(PlayerObject(), reportbits);

      // Tell the other players that we're done
      AutoAppIPtr(TraitManager);
      ObjID arch = pTraitManager->GetArchetype(o);
      g_pResearchResultsMsg->Send(OBJ_NULL, arch, newbits);

      // register what time we got the report at
      int i;
      for (i=0; i < 32; i++)
         if (newbits & (1 << i))
            ShockPDASetTime(0,4,i); // 4 is code for research, should be an enum but I suck

      // play a sound

      // the object's script should automatically update it's 
      // objstate property upon listening to this QB change
      SetResearchQB(o,1,"ResearchState");

       // update text
      SetResearchText(o);

      // remove the researching link
      BreakResearchLink(o);

      sScrMsg msg(o,"Research"); 
      pScriptMan->SendMessage(&msg); 
   
      // Major state change, so update the network
      networkMe = TRUE;
   }

   // This is pretty crude, but effective: we avoid overloading the network
   // with ResearchTime questdata changes, by suppressing most such messages.
   // We allow them through once a second, or when something significant
   // happens.
   AutoAppIPtr(NetManager);
   if (!networkMe)
      pNetManager->SuspendMessaging();
   SetResearchQB(o,curval,"ResearchTime");
   if (!networkMe)
      pNetManager->ResumeMessaging();
   if (networkMe)
      gLastNetworkedTime = now;
}
//--------------------------------------------------------------------------------------
// object o is being thrown out into the world.
// check if this should terminate research
void ShockResearchCheckObj(ObjID o)
{
   ObjID resobj;
   resobj = FindResearchedObject();
   if (o == resobj)
      CancelResearch(resobj);
}
//--------------------------------------------------------------------------------------
