/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrep.cpp,v 1.42 2000/02/19 13:26:06 toml Exp $

#include <objtype.h>
#include <appsfx.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <linkman.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>
#include <playrobj.h>
#include <questapi.h>
#include <drkdiff.h>

#include <shkinv.h>
#include <shkcharg.h>
#include <shkrep.h>
#include <shkutils.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkprop.h>
#include <shklinks.h>
#include <shkmfddm.h>
#include <shkobjst.h>
#include <shkhplug.h>
#include <shkiftul.h>
#include <shkplayr.h>
#include <shktrcst.h>
#include <shkparam.h>

#include <resapi.h>
#include <res.h>
#include <editor.h>

#include <objsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <objpos.h>
#include <frobctrl.h>
#include <physapi.h>
#include <rand.h>
#include <schbase.h>
#include <schema.h>
#include <gamestr.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

ObjID g_repID = OBJ_NULL;  // ugh, what a hack
//int repMusicSchema;
int gRepSelection = -1; // which actual object
int gRepFocus = 0; // for simple scroll
int gRepFocusMax; // max valid focus
static IRes *gHndRep = NULL;
static IRes *gHndBroken = NULL;
static IRes *gHndSelect = NULL;

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

//static Rect buy_rect = {{156,212},{156 + 17, 212 + 73}};
//static Rect up_rect = {{156,152},{156 + 17, 152 + 17}};
//static Rect down_rect = {{156,183},{156 + 17, 183 + 17}};

#define NUM_REPLICATOR_BOXES  4

#define BOX_BUTTON(x,y) {{(x),(y)},{(x)+142,(y)+62}}

static Rect obj_rects[NUM_REPLICATOR_BOXES] = {
   BOX_BUTTON(10,8),
   BOX_BUTTON(10,74),
   BOX_BUTTON(10,141),
   BOX_BUTTON(10,206),
};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

/*
static LGadButton buy_button;
static DrawElement buy_elem;
static IRes *buy_handles[2];
static grs_bitmap *buy_bitmaps[4];
*/

/*
static LGadButton up_button;
static DrawElement up_elem;
static IRes *up_handles[2];
static grs_bitmap *up_bitmaps[4];

static LGadButton down_button;
static DrawElement down_elem;
static IRes *down_handles[2];
static grs_bitmap *down_bitmaps[4];
*/

static int gRepAudioHnd = -1; //  = SFX_NO_HND;

//--------------------------------------------------------------------------------------
static void DorkBitmaps(grs_bitmap **bitmaps, IRes **handles)
{
   int i;
   for (i=0; i < 2; i++)
   {
      bitmaps[i] = (grs_bitmap *)handles[i]->Lock();
   }
   for (i = 2; i < 4; i++)
   {
      bitmaps[i] = bitmaps[0];
   }
}

//--------------------------------------------------------------------------------------
static void ReleaseHandles(IRes **handles)
{
   handles[0]->Unlock();
   handles[1]->Unlock();
   SafeFreeHnd(&handles[0]);
   SafeFreeHnd(&handles[1]);
}
//--------------------------------------------------------------------------------------
void ShockRepInit(int which)
{
   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   DorkBitmaps(close_bitmaps, close_handles);

   /*
   buy_handles[0] = LoadPCX("check0"); 
   buy_handles[1] = LoadPCX("check1"); 
   DorkBitmaps(buy_bitmaps, buy_handles);
   */

   /*
   up_handles[0] = LoadPCX("up0"); 
   up_handles[1] = LoadPCX("up1"); 
   DorkBitmaps(up_bitmaps, up_handles);

   down_handles[0] = LoadPCX("down0"); 
   down_handles[1] = LoadPCX("down1"); 
   DorkBitmaps(down_bitmaps, down_handles);
   */

   gHndRep = LoadPCX("replic");
   gHndBroken = LoadPCX("breplic");
   gHndSelect = LoadPCX("repsel");
   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockRepTerm(void)
{
   SafeFreeHnd(&gHndRep);
   SafeFreeHnd(&gHndBroken);
   SafeFreeHnd(&gHndSelect);
   ReleaseHandles(close_handles);
   //ReleaseHandles(buy_handles);
   //ReleaseHandles(up_handles);
   //ReleaseHandles(down_handles);
  
}

//--------------------------------------------------------------------------------------
// Get projectile from link
ObjID GetRepMarker(ObjID objID)
{
   ObjID marker = OBJ_NULL;
   AutoAppIPtr(LinkManager);
   // @TODO: make this a inheritance query on a trait
   ILinkQuery *pQuery = pLinkManager->Query(objID, LINKOBJ_WILDCARD, g_pReplicatorLinks->GetID());

   if (!pQuery->Done())
   {
      sLink link;
      pQuery->Link(&link);
      marker = link.dest;
   }
   SafeRelease(pQuery);
   return marker;
}
//--------------------------------------------------------------------------------------
static void SchemaDoneCallback(int /*hSchema*/, ObjID /*schemaID*/, void* /*pData*/)
{
   gRepAudioHnd = -1;
}
//--------------------------------------------------------------------------------------

static void FabricateItem(char *name, ObjID srcobj)
{
   ObjID oid,arch,dest;
   ObjPos *pos, *temp, pos2;
   IObjectSystem *objsys;
   int maxobj, currobj;

   objsys = AppGetObj(IObjectSystem);
   maxobj = objsys->MaxObjID();
   currobj = objsys->ActiveObjects();
   if (currobj > maxobj * 9 / 10)
   {
      char temp[255];
      ShockStringFetch(temp,sizeof(temp),"TooManyObjs","misc");
      ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
      return;
   }

   arch = objsys->GetObjectNamed(name);
   oid = objsys->BeginCreate(arch,kObjectConcrete);
   pos = ObjPosGet(srcobj);

   // get our target location by following the link
   dest = GetRepMarker(srcobj);
   temp = ObjPosGet(dest);
   memcpy(&pos2,temp,sizeof(ObjPos));
   // a minor deviation so they don't stack
   pos2.loc.vec.x += (((float)(Rand() % 100)) / 200.0F) - 0.25F;
   pos2.loc.vec.y += (((float)(Rand() % 100)) / 200.0F) - 0.25F;

   // move it
   ObjPosCopyUpdate(oid,&pos2);
   objsys->EndCreate(oid);
   PhysSetModLocation(oid,&pos2.loc.vec);
   SafeRelease(objsys);    
   //PhysRegisterSphere(oid,1,0,0.5F);
   SFX_Play_Raw(SFX_STATIC, NULL, "zot4");

   // Set up callback which resets our global upon play completion.
   sSchemaCallParams schParams;
   memset (&schParams, 0, sizeof (schParams));;
   schParams.flags = SCH_SET_CALLBACK;
   schParams.sourceID = OBJ_NULL;
   schParams.callback = SchemaDoneCallback;
   //   schParams.pData = ;         // user data for callback

   // Halt if an email is already playing.
   if (gRepAudioHnd != -1)
   {
      SchemaPlayHalt(gRepAudioHnd);
      gRepAudioHnd = -1;
   }
   gRepAudioHnd = SchemaPlay((Label *)"replicator2e", &schParams); //NULL);
   //SFX_Play_Raw(SFX_STATIC, NULL, "replicator2e");
}
//--------------------------------------------------------------------------------------
static int RepFindSlot(Point pt)
{
   int retval = -1;
   int i;

   // find any obj boxes that it hits
   for (i=0; i < NUM_REPLICATOR_BOXES; i++)
   {
      if (RectTestPt(&obj_rects[i],pt))
         retval = i;
   }
   return(retval);
}
//--------------------------------------------------------------------------------------
bool ShockReplicate(int which)
{
   int cost,total;
   char temp[64];

   if (which == -1)
      return(FALSE);

   if (g_repID != OBJ_NULL)
   {
      cost = RepContentsGetCost(g_repID,which);  
      // cost == 0 means no item there
      total = ShockInvNaniteTotal();
      if ((cost != 0) && (cost <= total)) 
      {
         if (ShockInvPayNanites(cost))
         {
            RepContentsGetObj(g_repID,which,temp);
            FabricateItem(temp,g_repID);
            return(TRUE);
         }
      }
      else
      {
         SchemaPlay((Label *)"repfail",NULL);

         char temp[255];
         ShockStringFetch(temp,sizeof(temp),"RepFail","misc");
         ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
      }
   }
   return(FALSE);
}

//--------------------------------------------------------------------------------------
bool ShockRepHandleMouse(Point pt)
{
   Rect r;
   r = ShockOverlayGetRect(kOverlayRep);
   // get the mouse location and make it rect-relative

   eObjState state;
   state = ObjGetObjState(g_repID);

   if (state == kObjStateBroken)
      return(TRUE);

   int slot;
   slot = RepFindSlot(pt);
   if (slot != -1)
   {
      gRepSelection = slot + gRepFocus;
      ShockReplicate(gRepSelection);
   }
   return(TRUE);
}

//--------------------------------------------------------------------------------------
#define TEXT_OFFSET  37 // 68
void ShockRepDraw(void)
{
   cStr str; 
   Rect r,drawrect; 
   int i;
   IObjectSystem *objsys;
   ObjID arch;
   char temp[64];
   int dx,dy,w,h,textwidth;
   eObjState state;

   AutoAppIPtr(GameStrings);

   r = ShockOverlayGetRect(kOverlayRep);

   objsys = AppGetObj(IObjectSystem);

   if (g_repID == OBJ_NULL)
      return;

   // draw the replicator background
   state = ObjGetObjState(g_repID);

   if (state == kObjStateBroken)
   {
      DrawByHandle(gHndBroken, r.ul);
      return;
   }

   DrawByHandle(gHndRep, r.ul);

   gr_set_fcolor(gShockTextColor);

   // draw in obj icons for the available loot
   for (i=gRepFocus; i < gRepFocus + 4; i++)
   {
      RepContentsGetObj(g_repID, i, temp);
      if (strlen(temp) > 0)
      {
         // get the archetype of the given object
         arch = objsys->GetObjectNamed(temp);
         // draw the bitmap
         drawrect = obj_rects[i - gRepFocus];
         RectMove(&drawrect, r.ul);
         ShockInvObjDraw(arch, drawrect.ul.x, drawrect.ul.y); // r.ul.x + obj_rects[i].ul.x, r.ul.y + obj_rects[i].ul.y);
         // and the cost
         sprintf(temp,"%03d",RepContentsGetCost(g_repID,i));
         dx = drawrect.ul.x + 9;
         dy = drawrect.lr.y - 11;
         gr_font_string(gShockFont,temp,dx,dy);
         
         //str = pGameStrings->FetchObjString(arch,PROP_OBJNAME_NAME);
         //strcpy(temp,str);  // need a copy since we are wrapping
         ObjGetObjShortNameSubst(arch,temp,sizeof(temp));
         textwidth = RectWidth(&obj_rects[i - gRepFocus]) - TEXT_OFFSET;
         gr_font_string_wrap(gShockFont,temp,textwidth);
         h = gr_font_string_height(gShockFont,temp);
         w = gr_font_string_width(gShockFont, temp);
         dx = drawrect.ul.x + TEXT_OFFSET;
         dy = drawrect.ul.y + 4;
         gr_font_string(gShockFont,temp,dx,dy);
      }
      if (i == gRepSelection)
         DrawByHandle(gHndSelect, drawrect.ul);
   }

   // draw in the player's total nanite quantity
   sprintf(temp,"%04d",ShockInvNaniteTotal());
   gr_font_string(gShockFont,temp,r.ul.x + 104, r.ul.y + 274);

   // draw in the close button
   LGadDrawBox(VB(&close_button),NULL);
   // and the buy button
   //LGadDrawBox(VB(&buy_button),NULL);

   // draw the up and down buttons (only if appropriate)
   //if (gRepFocus > 0)
      //LGadDrawBox(VB(&up_button),NULL);
   //if (gRepFocus < gRepFocusMax)
      //LGadDrawBox(VB(&down_button),NULL);
}
//--------------------------------------------------------------------------------------
// Property Definitions
//--------------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////
// Replicator contents (replicator-side)
/////////////////////////////////////////////////////////////

// the all new improved property system...
// data ops
class cRepContentsDataOps: public cClassDataOps<sRepContents>
{
};

// storage class
class cRepContentsStore: public cHashPropertyStore<cRepContentsDataOps>
{
};

// property implementation class
class cRepContentsProperty: public cSpecificProperty<IRepContentsProperty, &IID_IRepContentsProperty, sRepContents*, cRepContentsStore>
{
   typedef cSpecificProperty<IRepContentsProperty, &IID_IRepContentsProperty, sRepContents*, cRepContentsStore> cParent; 

public:
   cRepContentsProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sRepContents); 

};

IRepContentsProperty *g_RepContentsProperty;
IRepContentsProperty *g_RepContentsHackedProperty;

static sFieldDesc RepContentsFields[] = 
{
   {"Obj 1 Name", kFieldTypeString, FieldLocation(sRepContents, m_objs[0]),},
   {"Obj 1 Cost", kFieldTypeInt, FieldLocation(sRepContents, m_costs[0]),},
   {"Obj 2 Name", kFieldTypeString, FieldLocation(sRepContents, m_objs[1]),},
   {"Obj 2 Cost", kFieldTypeInt, FieldLocation(sRepContents, m_costs[1]),},
   {"Obj 3 Name", kFieldTypeString, FieldLocation(sRepContents, m_objs[2]),},
   {"Obj 3 Cost", kFieldTypeInt, FieldLocation(sRepContents, m_costs[2]),},
   {"Obj 4 Name", kFieldTypeString, FieldLocation(sRepContents, m_objs[3]),},
   {"Obj 4 Cost", kFieldTypeInt, FieldLocation(sRepContents, m_costs[3]),},
   {"Obj 5 Name", kFieldTypeString, FieldLocation(sRepContents, m_objs[4]),},
   {"Obj 5 Cost", kFieldTypeInt, FieldLocation(sRepContents, m_costs[4]),},
   {"Obj 6 Name", kFieldTypeString, FieldLocation(sRepContents, m_objs[5]),},
   {"Obj 6 Cost", kFieldTypeInt, FieldLocation(sRepContents, m_costs[5]),},
};

static sStructDesc RepContentsStructDesc = 
   StructDescBuild(sRepContents, kStructFlagNone, RepContentsFields);

//--------------------------------------------------------------------------------------
IRepContentsProperty *CreateRepContentsProperty(sPropertyDesc *desc, ePropertyImpl impl)
{
   StructDescRegister(&RepContentsStructDesc);
   return new cRepContentsProperty(desc);
}

static sPropertyDesc RepContentsDesc = 
{
   PROP_REPCONTENTS, 0,
   NULL, 0, 0,
   {"GameSys", "Rep. Contents"},
};

static sPropertyDesc RepContentsHackedDesc = 
{
   PROP_REPCONTENTSHACK, 0,
   NULL, 0, 0,
   {"GameSys", "Rep. Hack Contents"},
};

//--------------------------------------------------------------------------------------
void RepContentsPropertyInit()
{
   g_RepContentsProperty = CreateRepContentsProperty(&RepContentsDesc, kPropertyImplSparse);
   g_RepContentsHackedProperty = CreateRepContentsProperty(&RepContentsHackedDesc, kPropertyImplSparse);
}

//--------------------------------------------------------------------------------------
BOOL RepContentsGet(ObjID objID, sRepContents **ppRepContents)
{
   eObjState st;

   st = ObjGetObjState(objID);
   if (st == kObjStateHacked)
      return g_RepContentsHackedProperty->Get(objID, ppRepContents);
   else
      return g_RepContentsProperty->Get(objID, ppRepContents);
}

//--------------------------------------------------------------------------------------
bool RepContentsGetObj(ObjID objID, int slot, char *name)
{
   sRepContents *pRepContents;
   eObjState st;

   st = ObjGetObjState(objID);
   if (st == kObjStateHacked)
   {
      if (g_RepContentsHackedProperty->Get(objID, &pRepContents))
      {
         strcpy(name,pRepContents->m_objs[slot]);
         return(TRUE);
      }
   }
   else
   {
      if (g_RepContentsProperty->Get(objID, &pRepContents))
      {
         strcpy(name,pRepContents->m_objs[slot]);
         return(TRUE);
      }
   }
   return(FALSE);
}

//--------------------------------------------------------------------------------------
int RepContentsGetCost(ObjID objID, int slot)
{
   sRepContents *pRepContents;
   eObjState st;
   int retval = 0;
   st = ObjGetObjState(objID);

   if (st == kObjStateHacked)
   {
      if (g_RepContentsHackedProperty->Get(objID, &pRepContents))
      {
         retval=pRepContents->m_costs[slot];
      }
   }
   else
   {
      if (g_RepContentsProperty->Get(objID, &pRepContents))
      {
         retval=pRepContents->m_costs[slot];
      }
   }
   AutoAppIPtr(ShockPlayer);
   if (pShockPlayer->HasTrait(PlayerObject(),kTraitReplicator))
      retval = retval * 8 / 10;

   // now multiply the cost by difficulty param
   AutoAppIPtr(QuestData);
   sDiffParams* params = GetDiffParams();
   int diff = pQuestData->Get(DIFF_QVAR); 
   retval = float(retval) * params->m_repcost[diff];

   return(retval);
}

//--------------------------------------------------------------------------------------
/*
static bool up_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   SchemaPlay((Label *)"bscroll",NULL);

   gRepFocus = gRepFocus - 1;
   if (gRepFocus < 0)
      gRepFocus = 0;
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool down_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   SchemaPlay((Label *)"bscroll",NULL);

   // don't scroll down into illegal turf
   if (gRepFocus + 1 > gRepFocusMax)
      return(TRUE);

   gRepFocus = gRepFocus + 1;
   return(TRUE);
}
*/
//--------------------------------------------------------------------------------------
/*
static bool buy_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);
   if (ShockReplicate(gRepSelection))
      uiDefer(DeferOverlayClose,(void *)kOverlayRep);
   return(TRUE);
}
*/
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);
   uiDefer(DeferOverlayClose,(void *)kOverlayRep);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void SetupElem(DrawElement *dep, grs_bitmap **bmps)
{
   dep->draw_type = DRAWTYPE_BITMAPOFFSET;
   dep->draw_data = bmps;
   dep->draw_data2 = (void *)4; // should be 2 but hackery required
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayRep);

   // set up the continue button
   SetupElem(&close_elem, close_bitmaps);
   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   /*
   SetupElem(&buy_elem, buy_bitmaps);
   LGadCreateButtonArgs(&buy_button, LGadCurrentRoot(), buy_rect.ul.x + r.ul.x, buy_rect.ul.y + r.ul.y,
      RectWidth(&buy_rect), RectHeight(&buy_rect), &buy_elem, buy_cb, 0);
   */

   /*
   SetupElem(&up_elem, up_bitmaps);
   LGadCreateButtonArgs(&up_button, LGadCurrentRoot(), up_rect.ul.x + r.ul.x, up_rect.ul.y + r.ul.y,
      RectWidth(&up_rect), RectHeight(&up_rect), &up_elem, up_cb, 0);

   SetupElem(&down_elem, down_bitmaps);
   LGadCreateButtonArgs(&down_button, LGadCurrentRoot(), down_rect.ul.x + r.ul.x, down_rect.ul.y + r.ul.y, 
      RectWidth(&down_rect), RectHeight(&down_rect), &down_elem, down_cb, 0);
   */
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   //LGadDestroyBox(VB(&buy_button),FALSE);
   //LGadDestroyBox(VB(&down_button),FALSE);
   //LGadDestroyBox(VB(&up_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockRepStateChange(int which)
{
   /*
   extern bool gNoMouseToggle;
   gNoMouseToggle = ShockOverlayCheck(which);
   */
   if (ShockOverlayCheck(which))
   {
      //repMusicSchema = SchemaPlay((const Label*)"RepMusic", NULL);
      BuildInterfaceButtons();

      ObjID o;
      eObjState state;
      o = ShockOverlayGetObj();
      state = ObjGetObjState(g_repID);

      if (state == kObjStateBroken)
      {
         ShockHRMPlugSetMode(1, g_repID);
         ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
      }
      else if (gPropHackDiff->IsRelevant(g_repID) && !(state == kObjStateHacked))
      {
         ShockHRMPlugSetMode(0, g_repID);
         ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
      }
   }
   else
   {
      //SchemaPlayHalt(repMusicSchema);
      DestroyInterfaceButtons();
      ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOff);
   }
}
//--------------------------------------------------------------------------------------
void ShockRepSetup(ObjID rep)
{
   int cost,i;

   g_repID = rep;
   ShockOverlayChange(kOverlayRep,kOverlayModeOn);
   ShockOverlaySetObj(kOverlayRep, rep);
   for (i = 0; i < NUM_REPLICATOR_CONTENTS - NUM_REPLICATOR_BOXES; i++)
   {
      cost = RepContentsGetCost(rep,i + NUM_REPLICATOR_BOXES);  
      if (cost == 0)
         break;
   }
   gRepFocusMax = i;
   
   eObjState state;
   state = ObjGetObjState(rep);

   if (state != kObjStateBroken)
   {
      SchemaPlay((Label *)"replicator1e",NULL);
   }
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayRep = { 
   ShockRepDraw, // draw
   ShockRepInit, // init
   ShockRepTerm, // term
   ShockRepHandleMouse, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "subpanel_op", // upschema
   "subpanel_cl", // downschema
   ShockRepStateChange, // state
   NULL, // transparency
   TRUE, // distance
   TRUE, // needmouse
};
