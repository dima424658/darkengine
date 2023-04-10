/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhrm.cpp,v 1.55 2000/02/19 13:25:26 toml Exp $

#include <2d.h>
#include <appagg.h>
#include <mprintf.h>
#include <math.h>

#include <resapi.h>
#include <appsfx.h>
#include <playrobj.h>
#include <gamestr.h>
#include <rand.h>
#include <schema.h>
#include <scrptapi.h>
#include <simtime.h>
#include <cfgdbg.h>
#include <contain.h>

#include <shkhrm.h>
#include <shkscrm.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shktrcst.h>
#include <shktrait.h>
#include <shkimcst.h>
#include <shkprop.h>
#include <shkinv.h>
#include <shkobjst.h>
#include <gunapi.h>
#include <gunprop.h>
//#include <shkgun.h>
#include <shkmfddm.h>
#include <shkiftul.h>
#include <shkparam.h>
#include <shkpsapi.h>
#include <shktrcst.h>
#include <shkincst.h>
#include <shkplprp.h>
#include <shkpsipr.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

#define NUM_HRM_MODES   3

static IRes *gBackHnd[3];
static IRes *gNodeHnd[4];
static IRes *gModeHnd[4][3];
static IRes *gBarHnd[2];

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

#define DOIT_X 157
#define DOIT_Y 232

static Rect doit_rect = {{157,232},{157 + 19, 232 + 55}};
static LGadButton doit_button;
static DrawElement doit_elem;
static IRes *doit_handles[2];
static grs_bitmap *doit_bitmaps[4];

static IRes *reset_handle;

static int gHRMMode = 0;

static uint gHRMFlags = 0;

#define MAX_JARGON_LINES   12
static char gJargonLines[MAX_JARGON_LINES][255];
static char gJargonText[1024];
static int gJargonNum;
static int gJargonTeletype;
static tSimTime gJargonTime;

#define MATRIX_X  5
#define MATRIX_Y  4

typedef enum
{
   kHRMModeNormal,
   kHRMModeLose,
   kHRMModeWin,
   kHRMModeUnpaid,
   kHRMModeUnwinnable,
} eHRMGameMode;

eHRMGameMode gHRMGameMode;

typedef enum
{
   kHRMStateFree = 0,
   kHRMStateLit = 1,
   kHRMStateDead = 2,
   kHRMStateMine = 3,
   kHRMStateEmpty = 4,
} eHRMState;

static char *nodenames[] = { "", "hrmon","hrmburn","hrmmine" };
static char *postfixes[] = { "H","R","M" };
static char *modenames[] = { "lose","win","pay","fail" };
static char *backnames[] = { "hack", "repair", "modify", };
static char *barnames[] = { "hrmbarh", "hrmbarv", };

static eHRMState gHRMState[MATRIX_X][MATRIX_Y];

static int gHRMBoards[NUM_HRM_MODES][MATRIX_Y][MATRIX_X] =
{
   // hack
   {
      {4, 4, 0, 0, 0,},
      {0, 0, 0, 4, 0,},
      {0, 4, 0, 0, 0,},
      {0, 0, 0, 4, 4,},
   },

   // repair
   {
      {4, 0, 0, 0, 4,},
      {4, 0, 0, 0, 4,},
      {4, 0, 0, 0, 4,},
      {4, 0, 0, 0, 4,},
   },

   // modify
   {
      {4, 0, 0, 0, 0,},
      {4, 0, 4, 0, 4,},
      {4, 0, 4, 0, 4,},
      {0, 0, 0, 0, 4,},
   },
};

// okay, I admit this is getting hideous
BOOL gHRMPsi = FALSE;
int gHRMBonus = 0;

static ITechInfoProperty *hrm_props[NUM_HRM_MODES];

typedef enum 
{
   kHRMFlagNone = 0x0,
   kHRMFlagUseINT = 0x1,
} eHRMFlags;

typedef enum 
{
   kHRMResultNone = 0x0,
   kHRMResultWin = 0x1,
   kHRMResultLoss = 0x2,
} eHRMResult;

#define TEXT_X 15
#define TEXT_Y 12   // goal
#define TEXT_Y2 180   // jargon
#define TEXT_W 137
#define JARGON_H 100
#define TELETYPE_SPEED 10

//--------------------------------------------------------------------------------------
void JargonClear()
{
   int i;
   for (i=0; i < MAX_JARGON_LINES; i++)
   {
      strcpy(gJargonLines[i],"");
   }
   strcpy(gJargonText,"");
   gJargonNum = 0;
   gJargonTime = 0;
}
//--------------------------------------------------------------------------------------
void JargonAdd(char *text)
{
   BOOL reorg = FALSE;
   int i,h;

   h = gr_font_string_height(gShockFont, gJargonText);
   while ((gJargonNum == MAX_JARGON_LINES) || (h > JARGON_H))
   {
      //ConfigSpew("hrm_spew",("removing old line\n"));
      reorg = TRUE;
      // copy everything down by one
      for (i=0; i < MAX_JARGON_LINES - 1; i++)
         strcpy(gJargonLines[i],gJargonLines[i+1]);

      // reset pointer back one
      gJargonNum--;

      strcpy(gJargonText,"");
      for (i=0; i < gJargonNum; i++)
      {
         strcat(gJargonText,gJargonLines[i]);
      }
      gr_font_string_wrap(gShockFont, gJargonText, TEXT_W);
      h = gr_font_string_height(gShockFont, gJargonText);
   }

   tSimTime simtime = GetSimTime();
   if (gJargonTime == 0)
   {
      gJargonTime = simtime - (strlen(gJargonText) * TELETYPE_SPEED);
      //ConfigSpew("hrm_spew",("resetting jargontime to %d\n",gJargonTime));
   }

   strcpy(gJargonLines[gJargonNum],text);
   gJargonNum++;
   ConfigSpew("hrm_spew",("adding text: %s",text));

   // reassemble main text
   gr_font_string_unwrap(gJargonText);
   strcat(gJargonText,gJargonLines[gJargonNum-1]);
   gr_font_string_wrap(gShockFont, gJargonText, TEXT_W);
}
//--------------------------------------------------------------------------------------
static void ResetBoard(void)
{
   int x,y;
   for (x = 0; x < MATRIX_X; x++)
   {
      for (y = 0; y < MATRIX_Y; y++)
      {
         gHRMState[x][y] = (eHRMState)gHRMBoards[gHRMMode][y][x]; // yes, this is reversed!
      }
   }
}
//--------------------------------------------------------------------------------------
// actually resolve the effect!

static void ShockHRMTriggerEffect(eHRMResult result)
{
   uint flags = kHRMFlagNone;

   ObjID o = ShockOverlayGetObj();
   ObjID plr = PlayerObject();

   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(ScriptMan);

   char temp[255];
   char name[255];
   sprintf(name,"%sResult",backnames[gHRMMode]);
   ShockStringFetch(temp,sizeof(temp),name,"hrm",result);
   if (strlen(temp) > 0)
      ShockOverlayAddText(temp,DEFAULT_MSG_TIME);

   // no effect, then just reset the board
   // maybe this should just lock out input and
   // force the player to reset the board?
   if (result == kHRMResultNone)
   {
      //ResetBoard();
      gHRMGameMode = kHRMModeUnwinnable;
      // feedback
      //SchemaPlay((Label *)"login",NULL);
      return;
   }

   switch (gHRMMode)
   {
   case 0: // hack
      if (result == kHRMResultWin)
      {
         // send a "done hacking" message to the object
         sScrMsg msg(o,"HackSuccess"); 
         // We want to run this script locally, even if we're a client:
         msg.flags |= kSMF_MsgSendToProxy;
         pScriptMan->SendMessage(&msg); 
         SchemaPlay((Label *)"hack_success",NULL);
      }
      else
      {
         ObjSetObjState(o,kObjStateBroken);
         SchemaPlay((Label *)"hack_critical",NULL);

         // break it
         sScrMsg msg(o,"HackCritfail"); 
         msg.flags |= kSMF_MsgPostToOwner;
         pScriptMan->SendMessage(&msg); 

      }
      break;
   case 1: // repair
      if (result == kHRMResultWin)
      {
         ObjSetObjState(o,kObjStateNormal);

         // improve it's condition as well, if it is a weapon
         if (ObjHasGunState(o))
         {
            float cond = GunGetCondition(o);
            cond = cond + 10;
            if (cond > 100) cond = 100;
            GunSetCondition(o,cond);
         }

         SchemaPlay((Label *)"hack_success",NULL);
      }
      else
      {
         // destroy it!!

         // okay, is this an equipped item?  If so, we need to unequip it
         AutoAppIPtr(ContainSys);
         eContainType ctype = pContainSys->IsHeld(PlayerObject(),o);
         if (ctype != ECONTAIN_NULL)
         {
            pShockPlayer->Equip(PlayerObject(),(ePlayerEquip)(ctype - SHOCKCONTAIN_PDOLLBASE), OBJ_NULL, FALSE);
         }

         //ObjSetObjState(o,kObjStateDestroyed);
         AutoAppIPtr(ObjectSystem);
         pObjectSystem->Destroy(o);


         SchemaPlay((Label *)"hack_critical",NULL);
      }
      break;
   case 2: // modify
      if (result == kHRMResultWin)
      {
         int modlevel = GunGetModification(o);
         if (modlevel < 2)
         {
            GunSetModification(o, modlevel + 1);
            SchemaPlay((Label *)"hack_success",NULL);
         }
      }
      else 
      {
         // break it
         ObjSetObjState(o,kObjStateBroken);
         ShockInvRefresh();
         SchemaPlay((Label *)"hack_critical",NULL);
      }
      break;
   }

   ShockInvRefresh();

   if (result == kHRMResultWin)
      gHRMGameMode = kHRMModeWin;
   else
      gHRMGameMode = kHRMModeLose;
}
//--------------------------------------------------------------------------------------
static eTechSkills FindSkill()
{
   eTechSkills useskill;
   switch(gHRMMode)
   {
   case 0: useskill = kTechHacking; break;
   case 1: useskill = kTechRepair; break;
   case 2: useskill = kTechModify; break;
   default:
      Warning(("FindSkill: invalid HRM mode %d!\n",gHRMMode));
      useskill = kTechHacking;
      break;
   }
   return(useskill);
}
//--------------------------------------------------------------------------------------
#define BASE_COST 1
static int FindCost()
{
   sTechInfo *ti;
   float retval;
   float factor;
   ObjID o = ShockOverlayGetObj();

   if (!hrm_props[gHRMMode]->Get(o,&ti))
      factor = 1.0;
   else
      factor = ti->m_cost;

   retval = BASE_COST * factor;

   AutoAppIPtr(ShockPlayer);
   if ((gHRMMode == 2) && pShockPlayer->HasTrait(PlayerObject(),kTraitTinker))
      retval = retval / 2;

   /*
   // psi only costs half as much
   // this removed 6/17/99 as retuning -- Xemu
   if (gHRMPsi)
      retval = retval / 2;
   */

   if (retval < 1)
      retval = 1;

   return(retval);
}
//--------------------------------------------------------------------------------------
static bool PayNanites()
{
   int cost;
   ObjID plr = PlayerObject();
   AutoAppIPtr(ShockPlayer);

   BOOL pay = TRUE;

   if (pay)
   {
      cost = FindCost();
      if (!gHRMPsi)
      {
         if (ShockInvNaniteTotal() < cost)
         {
            SchemaPlay((Label *)"login",NULL);

            char temp[255];
            ShockStringFetch(temp,sizeof(temp),"NoNoNanites","misc");
            ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
            return(FALSE);
         }
   
         ShockInvPayNanites(cost);
      }
      else
      {
         AutoAppIPtr(PlayerPsi);
         int psipoints = pPlayerPsi->GetPoints();

         if (psipoints < cost)
         {
            char temp[255];
            ShockStringFetch(temp,sizeof(temp),"NoNoPsi","misc");
            ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
            return(FALSE);
         }

         pPlayerPsi->SetPoints(psipoints - cost);
      }
   }

   return(TRUE);
}
//--------------------------------------------------------------------------------------
int FindSoftLevel()
{
   ObjID soft;
   AutoAppIPtr(ShockPlayer);

   switch(FindSkill())
   {
   case kTechHacking: soft = pShockPlayer->GetEquip(PlayerObject(), kEquipHack); break;
   case kTechRepair: soft = pShockPlayer->GetEquip(PlayerObject(), kEquipRepair); break;
   case kTechModify: soft = pShockPlayer->GetEquip(PlayerObject(), kEquipModify); break;
   }
   
   int softlevel = 0;
   if (soft != OBJ_NULL)
      gPropSoftwareLevel->Get(soft, &softlevel);
   return(softlevel);
}

//--------------------------------------------------------------------------------------
static int HRMSkill()
{
   int skillval, softlevel;

   AutoAppIPtr(ShockPlayer);
   if (gHRMPsi)
   {
      skillval = pShockPlayer->GetStat(kStatPsi);
      if (PsiOverloaded(PlayerObject(),kPsiCyberHack))
         skillval = skillval + 2;
      skillval = (skillval + 1) / 2; // the +1 for rounding up
   }
   else
      skillval = pShockPlayer->GetTechSkill(FindSkill());
//   if (skillval >= MAX_SKILL_VAL)
//      skillval = MAX_SKILL_VAL - 1;
   if (pShockPlayer->HasImplant(PlayerObject(), kImplantTech) && !gHRMPsi)
      skillval += 1;

   if (!gHRMPsi)
   {
      softlevel = FindSoftLevel();
      skillval = skillval + (softlevel);
   }
   skillval = skillval + gHRMBonus;
   return(skillval);
}

//--------------------------------------------------------------------------------------
static int HRMStat()
{
   AutoAppIPtr(ShockPlayer);
   int statval;
   if (gHRMPsi)
   {
      statval = pShockPlayer->GetStat(kStatPsi);
      if (PsiOverloaded(PlayerObject(),kPsiCyberHack))
         statval = statval + 2;
      statval = (statval + 1) / 2;
   }
   else
      statval = pShockPlayer->GetStat(kStatCyber);
   return(statval);
}

   /*
   if (!hrm_props[gHRMMode]->Get(o,&ti))
      return;

   *succeed = ti->m_success;
   *critfail = ti->m_critfail;

   AutoAppIPtr(ShockPlayer);

   // relevant skill
   skillval = HRMSkill();
   statval = HRMStat();
   */
//--------------------------------------------------------------------------------------
int FindChance(sTechInfo *ti)
{
   int chance;
   sHRMParams *params;
   params = GetHRMParams();

   chance = ti->m_success + 
      (HRMSkill() * params->m_skillSuccessBonus) +
      (HRMStat() * params->m_statSuccessBonus);
   if (chance > 85)
      chance = 85;
   return(chance);
}
//--------------------------------------------------------------------------------------
int FindMines(sTechInfo *ti)
{
   int num_mines;
   sHRMParams *params;
   params = GetHRMParams();

   num_mines = ti->m_critfail 
      - (HRMSkill() * params->m_skillCritfailBonus) 
      - (HRMStat() * params->m_statCritfailBonus);
   return(num_mines);
}
static int BoardValue(int x, int y)
{
   if ((x < 0) || (y < 0) || (x >= MATRIX_X) || (y >= MATRIX_Y))
      return(0);

   if (gHRMState[x][y] == kHRMStateLit)
      return(1);

   return(0);
}
//--------------------------------------------------------------------------------------
// board has changed at location X, see whether this causes a win
// or an un-winnable situation
static void EvaluateBoard(Point loc)
{
   int x,y;
   int num_free;
   BOOL win = FALSE;

   x = loc.x;
   y = loc.y;

   // this is kind of retarded, I admit
   if (BoardValue(x-2,y) + BoardValue(x-1,y) + BoardValue(x,y) == 3)
      win = TRUE;
   if (BoardValue(x-1,y) + BoardValue(x,y) + BoardValue(x+1,y) == 3)
      win = TRUE;
   if (BoardValue(x,y) + BoardValue(x+1,y) + BoardValue(x+2,y) == 3)
      win = TRUE;
   if (BoardValue(x,y-2) + BoardValue(x,y-1) + BoardValue(x,y) == 3)
      win = TRUE;
   if (BoardValue(x,y-1) + BoardValue(x,y) + BoardValue(x,y+1) == 3)
      win = TRUE;
   if (BoardValue(x,y) + BoardValue(x,y+1) + BoardValue(x,y+2) == 3)
      win = TRUE;

   if (win)
   {
      // hey, yay, we win
      ShockHRMTriggerEffect(kHRMResultWin);
      
      char temp[255];
      ShockStringFetch(temp,sizeof(temp),"JargonWin","jargon",gHRMMode);
      JargonAdd(temp);
      return;
   }

   // now count open squares
   num_free = 0;
   for (x=0; x < MATRIX_X; x++)
   {
      for (y = 0; y < MATRIX_Y; y++)
      {
         if ((gHRMState[x][y] == kHRMStateFree) || (gHRMState[x][y] == kHRMStateMine))
            num_free++;
      }
   }

   // if none left, game is unwinnable
   if (num_free == 0)
   {
      ShockHRMTriggerEffect(kHRMResultNone);
   }
}
//--------------------------------------------------------------------------------------
static void PlaceMines()
{
   int i, num_mines, num_free, r;
   int x,y,n;
   BOOL placed;
   sTechInfo *ti;
   ObjID o = ShockOverlayGetObj();

   if (!hrm_props[gHRMMode]->Get(o,&ti))
      return;

   // figure out how many mines to place
   num_mines = FindMines(ti);

   // how many blank spaces do we have?
   num_free = 0;
   for (x=0; x < MATRIX_X; x++)
   {
      for (y = 0; y < MATRIX_Y; y++)
      {
         if (gHRMState[x][y] == kHRMStateFree)
            num_free++;
      }
   }
   ConfigSpew("hrm_spew",("%d free spaces on board, %d mines\n",num_free,num_mines));

   for (i=0; (i < num_mines) && (num_free > 0); i++)
   {
      // choose a random location
      // try it
      // if we have tried too many times, bail out
      r = (Rand() % num_free) + 1;
      num_free--;

      // count up until we find the Nth free spot
      // turn it into a mine square
      n = 0;
      placed = FALSE;
      for (x=0; !placed && (x < MATRIX_X); x++)
      {
         for (y=0; !placed && (y < MATRIX_Y); y++)
         {
            if (gHRMState[x][y] == kHRMStateFree)
               n++;
            if (n == r)
            {
               ConfigSpew("hrm_spew",("Placing mine at %d, %d\n",x,y));
               gHRMState[x][y] = kHRMStateMine;
               placed = TRUE;
            }
         }
      }
   }
}
//--------------------------------------------------------------------------------------
static void PlayPiece(Point loc)
{
   eHRMState state;
   BOOL mined;
   float diceval,chance;
   sTechInfo *ti;
   ObjID o;
   char temp[255];
   int r;

   o = ShockOverlayGetObj();

   state = gHRMState[loc.x][loc.y];
   if ((state == kHRMStateLit) || (state == kHRMStateDead) || (state == kHRMStateEmpty))
   {
      // not a valid play, give feedback
      SchemaPlay((Label *)"login",NULL);
      return;
   }

   // are we playing on a "mine" square?
   mined = (state == kHRMStateMine);

   diceval = (Rand() % 10000) / 100.0F;
  
   if (!hrm_props[gHRMMode]->Get(o,&ti))
      return;

   chance = FindChance(ti);

   ConfigSpew("hrm_spew",("diceval %g vs success %g\n",diceval,chance));

   if (diceval <= chance)
   {
      gHRMState[loc.x][loc.y] = kHRMStateLit;
      if (!mined)
      {
         r = Rand() % 5;
         ShockStringFetch(temp,sizeof(temp),"JargonLit","jargon",((r+1) * 10) + gHRMMode);
      }
      else
         ShockStringFetch(temp,sizeof(temp),"JargonMineLit","jargon", gHRMMode);
      JargonAdd(temp);
      EvaluateBoard(loc);
   }
   else
   {
      if (mined)
      {
         ShockStringFetch(temp,sizeof(temp),"JargonMineBurnt","jargon", gHRMMode);
         JargonAdd(temp);
         ShockHRMTriggerEffect(kHRMResultLoss);
      }
      else
      {
         r = Rand() % 5;
         ShockStringFetch(temp,sizeof(temp),"JargonBurnt","jargon",((r+1) * 10) + gHRMMode);
         JargonAdd(temp);

         gHRMState[loc.x][loc.y] = kHRMStateDead;
         EvaluateBoard(loc);
      }
   }
   
}
//--------------------------------------------------------------------------------------
#define BOARD_X   16
#define BOARD_Y   48
#define BOARD_DX  30 
#define BOARD_DY  36
#define MODE_X    13
#define MODE_Y    42

static Rect board_rect = {{BOARD_X,BOARD_Y},{BOARD_X + (BOARD_DX * MATRIX_X),BOARD_Y + (BOARD_DY * MATRIX_Y)}};

static void DrawBoard()
{
   int x,y;
   Point draw;
   eHRMState state;
   Rect r = ShockOverlayGetRect(kOverlayHRM);

   for (x=0 ; x < MATRIX_X; x++)
   {
      for (y=0; y < MATRIX_Y; y++)
      {
         state = gHRMState[x][y];
         if ((state != kHRMStateEmpty) && (state != kHRMStateFree))
         {
            draw.x = BOARD_X + (x * BOARD_DX) + r.ul.x;
            draw.y = BOARD_Y + (y * BOARD_DY) + r.ul.y;
            DrawByHandle(gNodeHnd[state],draw);
         }
         if (state == kHRMStateLit) 
         {
            // are we right-connected?
            if (x < MATRIX_X - 1)
            {
               if (gHRMState[x+1][y] == kHRMStateLit)
                  DrawByHandle(gBarHnd[0],draw);
            }
            if (y < MATRIX_Y - 1)
            {
               if (gHRMState[x][y+1] == kHRMStateLit)
                  DrawByHandle(gBarHnd[1],draw);
            }
         }
      }
   }
}
//--------------------------------------------------------------------------------------
static ePlayerEquip g_softslots[] = { kEquipHack, kEquipRepair, kEquipModify }; 
static ePlayerEquip FindSoftSlot()
{
   int usemode;
   if ((gHRMMode < 0) || (gHRMMode > 2))
   {
      Warning(("FindSoftSlot: asked for invalid slot %d!\n",gHRMMode));
      usemode = 0;
   }
   else
      usemode = gHRMMode;
   return(g_softslots[usemode]);
}
//--------------------------------------------------------------------------------------
void ShockHRMInit(int )
{
   int i,j;
   char temp[255];

   for (i=1; i < 4; i++)
   {
      sprintf(temp,"%s",nodenames[i]);
      gNodeHnd[i] = LoadPCX(temp);
   }

   for (i=0; i < 2; i++)
   {
      gBarHnd[i] = LoadPCX(barnames[i]);
   }

   for (i=0; i < 4; i++)
   {
      for (j=0; j < 3; j++)
      {
         sprintf(temp,"%s%s",modenames[i],postfixes[j]);
         gModeHnd[i][j] = LoadPCX(temp);
      }
   }

   close_handles[0] = LoadPCX("closeoff"); // LoadPCX(temp); 
   close_handles[1] = LoadPCX("Closeon"); // LoadPCX(temp); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   sprintf(temp,"start0");
   doit_handles[0] = LoadPCX(temp); 
   sprintf(temp,"start1");
   doit_handles[1] = LoadPCX(temp); 
   doit_bitmaps[0] = (grs_bitmap *) doit_handles[0]->Lock();
   doit_bitmaps[1] = (grs_bitmap *) doit_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      doit_bitmaps[i] = doit_bitmaps[0];
   }

   for (i=0; i < 3; i++)
      gBackHnd[i] = LoadPCX(backnames[i]);

   hrm_props[0] = gPropHackDiff;
   hrm_props[1] = gPropRepairDiff;
   hrm_props[2] = gPropModifyDiff;

   reset_handle = LoadPCX("reset0");

   SetLeftMFDRect(kOverlayHRM, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockHRMTerm(void)
{
   int i,j;

   for (i=0; i < 3; i++)
      SafeFreeHnd(&gBackHnd[i]);

   for (i=0; i < 2; i++)
      SafeFreeHnd(&gBarHnd[i]);

   for (i=0; i < 4; i++)
   {
      SafeFreeHnd(&gNodeHnd[i]);
   }

   for (i=0; i < 4; i++)
      for (j=0; j < 3; j++)
         SafeFreeHnd(&gModeHnd[i][j]);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
   doit_handles[0]->Unlock();
   doit_handles[1]->Unlock();
   SafeFreeHnd(&doit_handles[0]);
   SafeFreeHnd(&doit_handles[1]);

   SafeFreeHnd(&reset_handle);
}

//--------------------------------------------------------------------------------------
static Rect help_rects[] = {
   {{13,31},{48,65}},
   {{49,31},{83,65}},
   {{84,31},{118,65}},
   {{119,31},{154,65}},
   {{-1,-1},{-1,-1}}, //{{159,149},{159 + 19, 149 + 140}},
   {{13,271},{154,288}},
   {{13,248},{154,266}},
};

#define NUM_HELP_RECTS (sizeof(help_rects)/sizeof(Rect))

static void DrawHelpText(Point mpt)
{
   //int i;
   char temp[1024];
   cStr str;
   ObjID o,plr;
   int modlevel;
   //BOOL allow;
   Rect r = ShockOverlayGetRect(kOverlayHRM);

   o = ShockOverlayGetObj();
   plr = PlayerObject();
   AutoAppIPtr(GameStrings);
   AutoAppIPtr(ShockPlayer);

   strcpy(temp,"");

   /*
   for (i=0; i < NUM_HELP_RECTS; i++)
   {
      if (RectTestPt(&help_rects[i], mpt))
      {
         allow = TRUE;
         if (allow)
            ShockStringFetch(temp,sizeof(temp),"HRMHelp","hrm",i);
         break;
      }
   }
   */

   // also display the effect of the tech op
   switch (gHRMMode)
   {
   case 0: // hack
      str = pGameStrings->FetchObjString(o, PROP_HACKTEXT_NAME);
      strcpy(temp,str);
      break;
   case 1: // repair
      ShockStringFetch(temp,sizeof(temp),"RepairText","hrm");
      break;
   case 2: // modify
      modlevel = GunGetModification(o);
      // adjust if we've just won
      if (gHRMGameMode == kHRMModeWin) 
         modlevel -= 1;

      switch (modlevel)
      {
      case 0:
         str = pGameStrings->FetchObjString(o, PROP_MODIFY1TEXT_NAME);
         strcpy(temp,str);
         break;
      case 1:
         str = pGameStrings->FetchObjString(o, PROP_MODIFY2TEXT_NAME);
         strcpy(temp,str);
         break;
      case 2:
         ShockStringFetch(temp,sizeof(temp),"ModifyResult3","misc");
         break;
      }
   }
   
   gr_font_string_wrap(gShockFont, temp, TEXT_W);
   gr_set_fcolor(gShockTextColor);
   gr_font_string(gShockFont, temp, TEXT_X + r.ul.x, TEXT_Y + r.ul.y);

   // display jargon
   tSimTime simtime = GetSimTime();

   if (gJargonTime == 0)
   {
      gJargonTeletype = strlen(gJargonText);
   }
   else
   {
      gJargonTeletype = (simtime - gJargonTime) / TELETYPE_SPEED;
      if (gJargonTeletype > strlen(gJargonText))
         gJargonTime = 0;
   }

   //ConfigSpew("hrm_spew",("JargonTeletype = %d, strlen = %d\n",gJargonTeletype,strlen(gJargonText)));
   strcpy(temp,gJargonText);
   temp[gJargonTeletype] = '\0';
   gr_font_string(gShockFont, temp, TEXT_X + r.ul.x, TEXT_Y2 + r.ul.y);

   int chance; 
   sTechInfo *ti;
   if (hrm_props[gHRMMode]->Get(o,&ti))
   {
      chance = FindChance(ti);
      sprintf(temp,"%d%%",100 - chance);
      gr_font_string(gShockFont, temp, 14 + r.ul.x, 49 + r.ul.y);
   }
}
//--------------------------------------------------------------------------------------
#define COST_X 128
#define COST_Y 161
#define COST_W 48
#define NANITE_X 90
#define NANITE_Y 274
#define NAME_X 15
#define NAME_Y 13

void ShockHRMDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayHRM);
   ObjID plr;
   char temp[255]; // ,fmtstr[64];
   ObjID o = ShockOverlayGetObj();
   Point drawpt;

   AutoAppIPtr(ShockPlayer);
   AutoAppIPtr(GameStrings);
   plr = PlayerObject();

   // background
   DrawByHandle(gBackHnd[gHRMMode],r.ul);

   // draw the board state
   DrawBoard();
   if (gHRMGameMode != kHRMModeNormal)
   {
      drawpt.x = MODE_X + r.ul.x;
      drawpt.y = MODE_Y + r.ul.y;
      DrawByHandle(gModeHnd[gHRMGameMode - 1][gHRMMode], drawpt);
   }

   // draw in the main text
   Point mpt;
   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x -= r.ul.x;
   mpt.y -= r.ul.y;
   DrawHelpText(mpt);
 
   /*
   // short name up top
   if (ObjHasObjShortName(o))
   {
      cStr str;
      str = pGameStrings->FetchObjString(o,PROP_OBJSHORTNAME_NAME);
      strcpy(temp,str);
      gr_font_string(gShockFont,temp,r.ul.x + NAME_X, r.ul.y + NAME_Y);
   }

   // and how many nanites we actually have
   if (gHRMPsi)
      strcpy(temp,"0");
   else
      sprintf(temp,"%d",ShockInvNaniteTotal());

   gr_font_string(gShockFont,temp,NANITE_X + r.ul.x, NANITE_Y + r.ul.y);
   */

   // draw in the cost at the bottom
   //ShockStringFetch(fmtstr,sizeof(fmtstr),"Cost","hrm");
   sprintf(temp,"%d",FindCost());
   short w;
   w = gr_font_string_width(gShockFont, temp);
   gr_font_string(gShockFont,temp,COST_X + r.ul.x + ((COST_W - w) / 2),COST_Y + r.ul.y);

   // draw in the reset button
   if ((gHRMGameMode != kHRMModeWin) && (gHRMGameMode != kHRMModeLose))
   {
      //LGadDrawBox(VB(&doit_button), NULL);
      Point pt;
      pt.x = r.ul.x + DOIT_X;
      pt.y = r.ul.y + DOIT_Y;
      if (gHRMGameMode == kHRMModeUnpaid)
         DrawByHandle(doit_handles[0],pt);
      else
         DrawByHandle(reset_handle,pt);
   }

   // close button
   LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
bool ShockHRMHandleMouse(Point pt)
{
   float x,y;
   Point loc;
   if (RectTestPt(&board_rect, pt))
   {
      // all these casts are probably excessive, but better safe than sorry
      x = (float)(pt.x - board_rect.ul.x) / (float)BOARD_DX;
      y = (float)(pt.y - board_rect.ul.y) / (float)BOARD_DY;
      // okay, now "round" so that we get the closest hit
      // the zany constant is because the node itself takes up about half the 
      // size, so the remaining intervening space wants to be split in half
      loc.x = floor(x + 0.25);
      loc.y = floor(y + 0.25);
      ConfigSpew("hrm_spew",("piece loc %d, %d\n",loc.x,loc.y));
      if ((loc.x < 0) || (loc.y < 0) || (loc.x >= MATRIX_X) || (loc.y >= MATRIX_Y))
         return(TRUE);

      if (gHRMGameMode == kHRMModeNormal)
      {
         SchemaPlay((Label *)"hacking",NULL);
         PlayPiece(loc);
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
void ShockHRMDisplay(ObjID o, int mode, BOOL use_psi, int bonus)
{
   if (ShockOverlayCheck(kOverlayHRM))
      ShockOverlayChange(kOverlayHRM, kOverlayModeOff);

   gHRMMode = mode;
   gHRMPsi = use_psi;
   gHRMBonus = bonus;
   gHRMGameMode = kHRMModeUnpaid;

   // check for skill zero and terminate
   int skillval;
   ObjID plr;
   AutoAppIPtr(ShockPlayer);
   plr = PlayerObject();

   if (!gHRMPsi)
   {
      skillval = pShockPlayer->GetTechSkill(FindSkill());

      /*
      if (skillval == 0)
      {
         char errmsg[255];
         ShockStringFetch(errmsg, sizeof(errmsg), "notechskill", "hrm", mode);
         ShockOverlayAddText(errmsg, DEFAULT_MSG_TIME);
         return; // dont even open the MFD
      }
      */

      sTechSkills *req;
      int reqval;
      if (g_ReqTechProperty->Get(o,&req))
         reqval = req->m_tech[FindSkill()];
      else
         reqval = 1;
      if (gHRMMode == kTechModify)
      {
         int modlevel = GunGetModification(o);
         // second modification is harder
         if (modlevel == 1)
            reqval = reqval + 2;
      }
      if (skillval < reqval)
      {
         char errmsg[255],temp[255];
         ShockStringFetch(errmsg, sizeof(errmsg), "techminskill", "hrm", mode);
         sprintf(temp,errmsg,reqval);
         ShockOverlayAddText(temp, DEFAULT_MSG_TIME);
         return; // dont even open the MFD
      }

      // don't require softs anymore
      /*
      ObjID softobj;
      softobj = pShockPlayer->GetEquip(plr, FindSoftSlot());
      if (softobj == OBJ_NULL)
      {
         char errmsg[255];
         ShockStringFetch(errmsg, sizeof(errmsg), "notechsoft", "hrm", mode);
         ShockOverlayAddText(errmsg, DEFAULT_MSG_TIME);
         return; // dont even open the MFD
      }
      */
   }

   // if we are modifying but our weapon is already at max modification,
   // just give some feedback text
   if (gHRMMode == 2)
   {
      int modlevel = GunGetModification(o);
      char errmsg[255];
      switch (modlevel)
      {
      case 2:
         ShockStringFetch(errmsg, sizeof(errmsg), "ModifyResult3", "hrm");
         ShockOverlayAddText(errmsg, DEFAULT_MSG_TIME);
         return; // dont even open the MFD

      // these cases handle poking in the right difficulty property
      case 1:
         hrm_props[2] = gPropModify2Diff;
         break;
      case 0:
         hrm_props[2] = gPropModifyDiff;
         break;
      }
   }

   if (gHRMPsi)
      ShockOverlaySetDist(kOverlayHRM, FALSE);
   else
      ShockOverlaySetDist(kOverlayHRM, TRUE);

   // okay, we've qualified so open the darned MFD
   ShockOverlayChange(kOverlayHRM, kOverlayModeOn);
   ShockOverlaySetObj(kOverlayHRM, o);

   // reset the board
   ResetBoard();
   // place the mines
   PlaceMines();

   // set up the initial jargon
   JargonClear();

   char temp[255],temp2[255];
   int r;
   int statval, softval;
   int basediff,chance,mines;
   int bonusval;
   sTechInfo *ti;

   if (!hrm_props[gHRMMode]->Get(o,&ti))
      return;

   r = Rand() % 3;

   sHRMParams *param;
   param = GetHRMParams();

   // base & final difficulty
   ShockStringFetch(temp2,sizeof(temp2),"JargonBaseDiff","jargon", gHRMMode);
   basediff = 100 - ti->m_success;
   sprintf(temp,temp2,basediff);
   JargonAdd(temp);


   if (!gHRMPsi)
   {
      ShockStringFetch(temp2,sizeof(temp2),"JargonSkill","jargon", gHRMMode);
      skillval = pShockPlayer->GetTechSkill(FindSkill());
      bonusval = skillval * param->m_skillSuccessBonus;
      sprintf(temp,temp2,skillval,bonusval);
      JargonAdd(temp);

      ShockStringFetch(temp2,sizeof(temp2),"JargonStat","jargon", gHRMMode);
      statval = pShockPlayer->GetStat(kStatCyber);
      bonusval = statval * param->m_statSuccessBonus;
      sprintf(temp,temp2,statval,bonusval);
      JargonAdd(temp);

      ShockStringFetch(temp2,sizeof(temp2),"JargonSoft","jargon", gHRMMode);
      softval = FindSoftLevel();
      if (softval > 0)
      {
         bonusval = (softval) * param->m_skillSuccessBonus;
         sprintf(temp,temp2,softval,bonusval);
         JargonAdd(temp);
      }

      if (pShockPlayer->HasImplant(PlayerObject(), kImplantTech) && !gHRMPsi)
      {
         ShockStringFetch(temp2,sizeof(temp),"JargonImplant","jargon", gHRMMode);
         bonusval = param->m_skillSuccessBonus;
         sprintf(temp,temp2,bonusval);
         JargonAdd(temp);
      }

      if (gHRMBonus != 0)
      {
         ShockStringFetch(temp2,sizeof(temp2),"JargonBonus","jargon", gHRMMode);
         bonusval = gHRMBonus * param->m_skillSuccessBonus;
         sprintf(temp,temp2,bonusval);
         JargonAdd(temp);
      }
   }
   else
   {
      ShockStringFetch(temp2,sizeof(temp2),"JargonSkill","jargon", gHRMMode);
      skillval = HRMSkill();
      bonusval = skillval * param->m_skillSuccessBonus;
      sprintf(temp,temp2,skillval,bonusval);
      JargonAdd(temp);

      ShockStringFetch(temp2,sizeof(temp2),"JargonStat","jargon", gHRMMode);
      statval = HRMStat(); // pShockPlayer->GetStat(kStatPsi);
      bonusval = statval * param->m_statSuccessBonus;
      sprintf(temp,temp2,statval,bonusval);
      JargonAdd(temp);

      /*
      ShockStringFetch(temp2,sizeof(temp2),"JargonPsiBonus","jargon", gHRMMode);
      sprintf(temp,temp2,gHRMBonus);
      JargonAdd(temp);
      */
   }

   ShockStringFetch(temp2,sizeof(temp2),"JargonFinalDiff","jargon", gHRMMode);
   chance = 100 - FindChance(ti);
   sprintf(temp,temp2,chance);
   JargonAdd(temp);

   // mines
   mines = FindMines(ti);
   if (mines < 0)
      mines = 0;
   if (mines == 1)
      ShockStringFetch(temp2,sizeof(temp2),"JargonMinesOne","jargon", gHRMMode);
   else
      ShockStringFetch(temp2,sizeof(temp2),"JargonMines","jargon", gHRMMode);
   sprintf(temp,temp2,mines);
   JargonAdd(temp);
}
//--------------------------------------------------------------------------------------
static bool reset_cb(short action, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);      

   if ((gHRMGameMode == kHRMModeWin) || (gHRMGameMode == kHRMModeLose))
      return(TRUE);

   if (PayNanites())
   {
      SchemaPlay((Label *)"start_hack",NULL);

      // reset the state of the board
      ResetBoard();
      // place the mines
      PlaceMines();
      gHRMGameMode = kHRMModeNormal;
   }

   return(TRUE);
}
   
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      uiDefer(DeferOverlayClose,(void *)kOverlayHRM);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r = ShockOverlayGetRect(kOverlayHRM);

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   doit_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   doit_elem.draw_data = doit_bitmaps;
   doit_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&doit_button, LGadCurrentRoot(), doit_rect.ul.x + r.ul.x, doit_rect.ul.y + r.ul.y,
      RectWidth(&doit_rect), RectHeight(&doit_rect), &doit_elem, reset_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&doit_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockHRMStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // take down any plug-in that got us here
      ShockOverlayChange(kOverlayHRMPlug, kOverlayModeOff);
      BuildInterfaceButtons();
   }
   else
   {
      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
int ShockFindTechType(ObjID obj)
{
   if (ObjGetObjState(obj) == kObjStateUnresearched)
      return(kTechResearch);

   if ((ObjGetObjState(obj) == kObjStateBroken) && (gPropRepairDiff->IsRelevant(obj)))
      return(kTechRepair);

   if (IsPlayerGun(obj) && gPropModifyDiff->IsRelevant(obj))
      return(kTechModify);

   if (gPropHackDiff->IsRelevant(obj))
      return(kTechHacking);

   return(-1);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayHRM = { 
   ShockHRMDraw, // draw
   ShockHRMInit, // init
   ShockHRMTerm, // term
   ShockHRMHandleMouse, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "", // upschema
   "subpanel_cl", // downschema
   ShockHRMStateChange, // state
   NULL, // transparency
   TRUE, // distance
   TRUE, // needmouse
   0, // alpha
   NULL, // update func
   TRUE, // check contains?
};

