/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <2d.h>

#include <appagg.h>
#include <resapi.h>

#include <schema.h>
#include <playrobj.h>
#include <mprintf.h>
#include <simtime.h>
#include <filevar.h>

#include <netmsg.h>

#include <panltool.h>
#include <questapi.h>

#include <dpcpda.h>
#include <dpcpdcst.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpcplayr.h>
#include <dpcplprp.h>
#include <dpcbooko.h>
#include <dpcemail.h>
#include <dpcmfddm.h>
#include <dpcnet.h>
#include <dpciftul.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

static IRes *gNavDownHnds[4];
static IRes *gBackHnd;
static IRes *gCheckBox;
static IRes *gCheckMark;

// hm, do we need to save these out?  Maybe a questbit?
static int gPDAFilter = 0;
static int gPDAOffset = 0;
static int gPDADrawn = 0;

#define NUM_VISIBLE_ENTRIES   9

int gPDAItems[NUM_VISIBLE_ENTRIES];

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static Rect main_rect = {{13,32},{152,263}};
static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

typedef enum ePDAButton { kPDAButtonLeft, kPDAButtonRight, kPDAButtonUp, kPDAButtonDown, 
   kPDAButtonEmail, kPDAButtonLogs, kPDAButtonNotes, kPDAButtonHelp};
#define NUM_PDA_BUTTONS 8
static Rect pda_rects[NUM_PDA_BUTTONS] = 
{
    {{13,11},   {13  + 18, 11  + 17}}, // left
    {{137,11},  {137 + 18, 11  + 17}}, // right
    {{157,152}, {157 + 18, 152 + 17}}, // up
    {{157,247}, {157 + 18, 247 + 17}}, // down
    {{12,268},  {12  + 40, 268 + 17}}, // email
    {{53,268},  {53  + 40, 268 + 17}}, // logs
    {{94,268},  {94  + 40, 268 + 17}}, // media
    {{135,268}, {135 + 40, 268 + 17}}, // help
};

static LGadButtonList pda_blist;
static LGadButtonListDesc pda_blistdesc;
static DrawElement pda_blist_elems[NUM_PDA_BUTTONS];
static IRes *pda_handles[NUM_PDA_BUTTONS][2];
static grs_bitmap *pda_bitmaps[NUM_PDA_BUTTONS][4];

static char *button_names[NUM_PDA_BUTTONS] = { "left", "right", "up", "down", "pemail", "plogs", "pnotes", "pvideo"};
static char *pdatype_names[]               = { "emailname", "logname", "Note_%d_", "helpname", "reportname",                                };
static char *filter_names[]                = { "level%02d", "level%02d", "notes", "infocomp", "research",};
static char *title_text[]                  = { "", "", "", "", "", };

#define PDALIST_DY 24
#define NOTES_DY   36

static int type_dy[] = { PDALIST_DY, PDALIST_DY, NOTES_DY, PDALIST_DY, PDALIST_DY };

#define NOTES_ENTRIES 6

static int type_vis[] = { NUM_VISIBLE_ENTRIES, NUM_VISIBLE_ENTRIES, NOTES_ENTRIES, NUM_VISIBLE_ENTRIES, NUM_VISIBLE_ENTRIES };

#define PDAF_NONE       0x00
#define PDAF_LEVELTEXT  0x01
#define PDAF_LOGDATA    0x02
#define PDAF_RESEARCH   0x04   
#define PDAF_NOTES      0x08
#define PDAF_INFOCOMP   0x10
#define PDAF_UNREAD     0x20

#define PDAF_STD        (PDAF_LEVELTEXT|PDAF_LOGDATA)

uint filter_flags[] = { PDAF_STD, PDAF_STD|PDAF_UNREAD, PDAF_LEVELTEXT|PDAF_NOTES, PDAF_INFOCOMP, PDAF_RESEARCH};

struct sTimeData
{
   tSimTime time;
   int id;
   int level;
};

// Here's the type of my global 
#define NUM_PDA_TYPES   5
struct sLogTimes
{
   tSimTime m_times[NUM_PDA_LEVELS][NUM_PDA_TYPES][MAX_LOG_DATA];
   int m_level;
   BOOL m_read[NUM_PDA_LEVELS][NUM_PDA_TYPES][MAX_LOG_DATA];
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gLogTimesDesc = 
{
   kCampaignVar,            // Where do I get saved?
   "LOGTIMES",              // Tag file tag
   "PDA Times",             // friendly name
   FILEVAR_TYPE(sLogTimes), // Type (for editing)
   { 1, 0},                 // version
   { 1, 0},                 // last valid version 
   "deepc",                 // optional: what game am I in NULL means all 
}; 

// The actual global variable
cFileVar<sLogTimes,&gLogTimesDesc> gLogTimes; 


//--------------------------------------------------------------------------------------
void DPCPDAInit(int )
{
    int n,i;
    char temp[255];

    gBackHnd   = LoadPCX("pda");
    gCheckBox  = LoadPCX("notebox");
    gCheckMark = LoadPCX("notechek");

    AssertMsg(gBackHnd,   "DPCPDAInit:  pda art missing");
    AssertMsg(gCheckBox,  "DPCPDAInit:  notebox art missing");
    AssertMsg(gCheckMark, "DPCPDAInit:  notechek art missing");

    close_handles[0] = LoadPCX("CloseOff"); 
    close_handles[1] = LoadPCX("CloseOn"); 
    close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
    close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
    for (i = 2; i < 4; i++)
    {
      close_bitmaps[i] = close_bitmaps[0];
    }
    
    for (i=0; i < 4; i++)
    {
      char bname[32];
      sprintf(bname,"%s1",button_names[i + 4]);
      gNavDownHnds[i] = LoadPCX(bname);
    }
    
    for (n=0; n < NUM_PDA_BUTTONS; n++)
    {
      for (i=0; i < 2; i++)
      {
         sprintf(temp,"%s%d",button_names[n],i);
         pda_handles[n][i] = LoadPCX(temp); 
         AssertMsg(pda_handles[n][i], "DPCPDAInit:  PDA Button art missing");
         pda_bitmaps[n][i] = (grs_bitmap *) pda_handles[n][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         pda_bitmaps[n][i] = pda_bitmaps[n][0];
      }
    }
    
    SetLeftMFDRect(kOverlayPDA, full_rect);
}

//--------------------------------------------------------------------------------------
void DPCPDATerm(void)
{
   int n;
   SafeFreeHnd(&gBackHnd);
   SafeFreeHnd(&gCheckBox);
   SafeFreeHnd(&gCheckMark);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   for (n = 0; n < NUM_PDA_BUTTONS; n++)
   {
      pda_handles[n][0]->Unlock();
      pda_handles[n][1]->Unlock();
      SafeFreeHnd(&pda_handles[n][0]);
      SafeFreeHnd(&pda_handles[n][1]);
   }
   for (n = 0; n < 4; n++)
      SafeFreeHnd(&gNavDownHnds[n]);
}

//--------------------------------------------------------------------------------------
#define LEVEL_X   35
#define LEVEL_Y   13
#define LEVEL_W   95

#define LEVELNAME_X  35
#define LEVELNAME_Y  23
#define LEVELNAME_W  95

#define PDALIST_X 22
#define PDALIST_Y 36
#define PDALIST_W 130
#define CHECK_W 16

static int time_compare(const void *p, const void *q)
{
   sTimeData a = * (sTimeData *) p;
   sTimeData b = * (sTimeData *) q;

   if (b.time > a.time)
      return(-1);
   return(b.time < a.time);
}

void DPCPDADraw(unsigned long inDeltaTicks)
{
   char lnum[64];
   char temp[255];
   int w,i;
   Rect r;
   int dx,dy;
   int minlog,maxlog;
   sLogData *ldp;
   ObjID player;
   int itembits;
   BOOL hasprop;

   r= DPCOverlayGetRect(kOverlayPDA);
   player = PlayerObject();

   DrawByHandle(gBackHnd,r.ul);

   gr_set_fcolor(gDPCTextColor);

   // text for current level
   DPCStringFetch(lnum,sizeof(lnum),"levelbase","misc",gPDAFilter);
   sprintf(temp,lnum,gLogTimes.m_level + 1);
   w = gr_font_string_width(gDPCFont,temp);
   dx = LEVEL_X + ((LEVEL_W - w) / 2) + r.ul.x;
   dy = LEVEL_Y + r.ul.y;
   gr_font_string(gDPCFont, temp, dx, dy);

   if (filter_flags[gPDAFilter] & PDAF_LEVELTEXT)
   {
      DPCStringFetch(temp,sizeof(temp),"level","misc",gLogTimes.m_level);
      w = gr_font_string_width(gDPCFont,temp);
      dx = LEVELNAME_X + ((LEVELNAME_W - w) / 2) + r.ul.x;
      dy = LEVELNAME_Y + r.ul.y;
      gr_font_string(gDPCFont, temp, dx, dy);
   }

   // current list   
   minlog = 0;
   maxlog = MAX_LOG_DATA; // gPDAOffset + NUM_VISIBLE_ENTRIES;
   gPDADrawn = 0;
   // clear drawn items table
   for (i=0; i < type_vis[gPDAFilter]; i++)
      gPDAItems[i] = -1;

   hasprop = FALSE;
   int uselevel = 0;
   itembits = 0;
   if (filter_flags[gPDAFilter] & PDAF_LOGDATA)
   {
      if (g_LogProperties[gLogTimes.m_level]->Get(player, &ldp))
      {
         hasprop = TRUE;
         uselevel = gLogTimes.m_level;
         itembits = ldp->m_data[gPDAFilter];
      }
   }
   else if (filter_flags[gPDAFilter] & PDAF_RESEARCH)
   {
      if (g_ResearchReportProperty->Get(player, &itembits))
      {
         hasprop = TRUE;
         uselevel = 0;
      }
   }
   else if (filter_flags[gPDAFilter] & PDAF_NOTES)
   {
      char qbname[255];
      // fill out itembits based on what notes for
      // this level we want to show.
      itembits = 0;
      hasprop = TRUE;
      AutoAppIPtr(QuestData);
      for (i = 0; i < MAX_LOG_DATA; i++)
      {
         sprintf(qbname,"Note_%d_%d",gLogTimes.m_level+1,i+1);
         if (pQuestData->Get(qbname) != 0)
            itembits = itembits | (1 << i);
      }
      uselevel = 0;
   }
   else if (filter_flags[gPDAFilter] & PDAF_INFOCOMP)
   {
      if (g_HelpTextProperty->Get(player, &itembits))
      {
         hasprop = TRUE;
         uselevel = 0;
      }
   }

   sTimeData timelist[MAX_LOG_DATA];
   char text[255],note[255];
   sprintf(note,"NoteOrder_%d_",gLogTimes.m_level+1);
   for (i=0; i < MAX_LOG_DATA; i++)
   {
      if (filter_flags[gPDAFilter] & PDAF_NOTES)
      {
         // generate our data from a fake arbitrary ordering
         DPCStringFetch(text,sizeof(text),note,"notes",i+1);
         if (strlen(text) == 0)
            timelist[i].time = 0;
         else
            timelist[i].time = atoi(text);
      }
      else
      {
         // assemble the time sorted list from raw data
         timelist[i].time = gLogTimes.m_times[uselevel][gPDAFilter][i];
      }
      timelist[i].id = i;
   }
   qsort(timelist,MAX_LOG_DATA, sizeof(timelist[0]), time_compare);

   if (hasprop)
   {
      // draw all the actual items, filtering for scrolling and for whether we have it.
      int iter;
      for (iter=minlog; iter < maxlog; iter++)
      {
         // okay, we want to draw the iter-th thing in the list
         // lets look through our time-sorted list and figure out what that is.
         i = timelist[iter].id;

         if (itembits & (1 << i))
         {
            if (gPDADrawn < gPDAOffset)
            {
               gPDADrawn++;
               continue;
            }
            if (gPDADrawn - gPDAOffset >= type_vis[gPDAFilter])
               break;

            dx = PDALIST_X + r.ul.x;
            dy = PDALIST_Y + ((gPDADrawn - gPDAOffset) * type_dy[gPDAFilter]) + r.ul.y;
            BOOL drawme = TRUE;
            if (filter_flags[gPDAFilter] & PDAF_NOTES)
            {
               char qbname[255];
               Point p;

               AutoAppIPtr(QuestData);
               dx = dx - 12;
               p.x = dx;
               p.y = dy;
               sprintf(qbname,"Note_%d_%d",gLogTimes.m_level+1,i+1);
               switch (pQuestData->Get(qbname))
               {
               case 1:
                  DrawByHandle(gCheckBox,p);
                  // fallthrough
               case 2:
                  if (pQuestData->Get(qbname) == 2)
                     DrawByHandle(gCheckMark,p);
                  char usename[32];
                  sprintf(usename,pdatype_names[gPDAFilter],gLogTimes.m_level+1);
                  DPCStringFetch(temp,sizeof(temp),usename,filter_names[gPDAFilter],i+1);
                  gr_font_string_wrap(gDPCFont,temp,PDALIST_W - 5);
                  dx += CHECK_W + 2;
                  break;
               default:
                  drawme = FALSE;
                  break;
               }
               
            }
            else
            {
               char levelname[32];
               sprintf(levelname,filter_names[gPDAFilter],gLogTimes.m_level + 1);
               DPCStringFetch(temp,sizeof(temp),pdatype_names[gPDAFilter],levelname,i+1);
            }
            if (drawme)
            {
               BOOL dim_font = FALSE;
               if (filter_flags[gPDAFilter] & PDAF_UNREAD)
               {
                  if (gLogTimes.m_read[uselevel][gPDAFilter][i] == TRUE)
                     dim_font = TRUE;
               }
               if (dim_font)
               {
                  extern grs_font *gDPCFontDimmed;
                  gr_font_string(gDPCFontDimmed, temp, dx, dy);
               }
               else
                  gr_font_string(gDPCFont, temp, dx, dy);

               strcpy(temp,"");
               gPDAItems[gPDADrawn - gPDAOffset] = i;
               gPDADrawn++;
            }
         }
      }
   }

   // close button
   LGadDrawBox(VB(&close_button),NULL);

   // filter & scroll buttons
   LGadDrawBox(VB(&pda_blist),NULL);

   // draw on top of the nav button our "down" state
   // in order to feedback filter state
   if (gPDAFilter < 4)
   {
      Point pt;
      pt.x = pda_rects[gPDAFilter + 4].ul.x + r.ul.x;
      pt.y = pda_rects[gPDAFilter + 4].ul.y + r.ul.y;
      DrawByHandle(gNavDownHnds[gPDAFilter], pt);
   }
}
//--------------------------------------------------------------------------------------
bool DPCPDAHandleMouse(Point mpt)
{
   Rect r = DPCOverlayGetRect(kOverlayPDA);
   ObjID obj;
   int y;

   obj = PlayerObject();

   if (RectTestPt(&main_rect,mpt))
   {
      y = (mpt.y - PDALIST_Y) / type_dy[gPDAFilter];
      if ((y >= 0) && (y < type_vis[gPDAFilter]))
      {
         if (gPDAItems[y] != -1)
         {
            //mprintf("drawing item %d\n",gPDAItems[y]);
            DPCPDAUseLog(gPDAFilter, gLogTimes.m_level, gPDAItems[y],TRUE);
         }
      }
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlayPDA);
   SchemaPlay((Label *)"subpanel_cl",NULL);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool pda_cb(ushort action, int button, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   ObjID player = PlayerObject();
   BOOL accept = FALSE;
   sLogData *ldp;
   int dir;

   switch(button)
   {
   case kPDAButtonRight:
      dir = +1;
      // intentional fallthrough
   case kPDAButtonLeft:
      if (button == kPDAButtonLeft)
         dir = -1;

      while (!accept)
      {
         gLogTimes.m_level += dir;

         if (gLogTimes.m_level >= NUM_PDA_LEVELS)
            gLogTimes.m_level = 0;
         if (gLogTimes.m_level < 0)
            gLogTimes.m_level = NUM_PDA_LEVELS - 1;

         // level 1 is always available
         // this clause also prevents an infinite loop here
         if (gLogTimes.m_level == 1)
            accept = TRUE;
         // allow any level on which we have a log or email or note
         if (g_LogProperties[gLogTimes.m_level]->Get(player, &ldp))
         {
            if ((ldp->m_data[0] != 0) || (ldp->m_data[1] != 0) || (ldp->m_data[2] != 0))
               accept = TRUE;
         }
      }
      gPDAOffset = 0;
      SchemaPlay((Label *)"btabs", NULL);
      break;

   case kPDAButtonUp:
      SchemaPlay((Label *)"bscroll",NULL);
      gPDAOffset -= 1;
      if (gPDAOffset < 0)
         gPDAOffset = 0;
      break;
   case kPDAButtonDown:
      SchemaPlay((Label *)"bscroll",NULL);

      // don't let player scroll to reveal blank lines)
      if (gPDAItems[type_vis[gPDAFilter] - 1] == -1)
         break;

      gPDAOffset += 1;
      // don't scroll off the end of the list
      if (gPDAOffset >= MAX_LOG_DATA - type_vis[gPDAFilter])
         gPDAOffset = MAX_LOG_DATA - type_vis[gPDAFilter] - 1;
      break;
   case kPDAButtonEmail:
   case kPDAButtonLogs:
   case kPDAButtonNotes:
   case kPDAButtonHelp:
      SchemaPlay((Label *)"btabs", NULL);
      gPDAFilter = button - kPDAButtonEmail;
      gPDAOffset = 0;
      break;
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   Rect r= DPCOverlayGetRect(kOverlayPDA);
   static Rect use_rects[NUM_PDA_BUTTONS];
   int n;

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   for (n=0; n < NUM_PDA_BUTTONS; n++)
   {
      DrawElement *elem = &pda_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = pda_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required

      use_rects[n].ul.x = pda_rects[n].ul.x + r.ul.x;
      use_rects[n].ul.y = pda_rects[n].ul.y + r.ul.y;
      use_rects[n].lr.x = pda_rects[n].lr.x + r.ul.x;
      use_rects[n].lr.y = pda_rects[n].lr.y + r.ul.y;
   }
   
   pda_blistdesc.num_buttons = NUM_PDA_BUTTONS;
   pda_blistdesc.button_rects = use_rects;
   pda_blistdesc.button_elems = pda_blist_elems;
   pda_blistdesc.cb = pda_cb;

   LGadCreateButtonListDesc(&pda_blist, LGadCurrentRoot(), &pda_blistdesc);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&pda_blist),FALSE);
}
//--------------------------------------------------------------------------------------
void DPCPDAStateChange(int which)
{
   if (DPCOverlayCheck(which))
   {
      BuildInterfaceButtons();
   }
   else
   {
      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
void DPCPDASetFilter(int which)
{
   gPDAFilter = which;
}
//--------------------------------------------------------------------------------------
// note at what time we recieved a particular piece of data
// this is stored in the persistent filevar of this kind of info
// then we use it to sort the entries down the road.
void DPCPDASetTime(int uselevel, int usetype, int which)
{
   gLogTimes.m_times[uselevel][usetype][which] = GetSimTime();
   gLogTimes.m_read[uselevel][usetype][which] = FALSE;
}
//--------------------------------------------------------------------------------------
// Returns TRUE iff we still need this log (ie, we haven't shown it before).
// Has the side-effect of recording that this log is no longer needed.
BOOL DPCPDANeedLog(int usetype, int uselevel, int usewhich)
{
   sLogData *useldp,plrld;
   ObjID player = PlayerObject();
   
   if ((uselevel == -1) || (usewhich == -1))
   {
      Warning(("DPCPDANeedLog: trying to play invalid log!  probably means missing log property."));
      return(FALSE);
   }

   // set the player's bits
   if (g_LogProperties[uselevel]->Get(player,&useldp))
      memcpy(&plrld,useldp,sizeof(sLogData));
   else
      memset(&plrld,0,sizeof(sLogData));

   // abort out if you already have the data
   if (plrld.m_data[usetype] & (1 << usewhich))
      return(FALSE);

   plrld.m_data[usetype] |= (1 << usewhich); 
   g_LogProperties[uselevel]->Set(player,&plrld);

   // record what time we got it, for sorting later
   DPCPDASetTime(uselevel,usetype,usewhich);

   // if we are changing default page or level, rescroll to top
   if ((gLogTimes.m_level != uselevel) || (gPDAFilter != usetype))
      gPDAOffset = 0; // MAX_LOG_DATA - type_vis[usetype] - 1;

   // also set the default page, level, etc. to this
   gLogTimes.m_level = uselevel;
   gPDAFilter = usetype;
   
   return (TRUE);
}
//--------------------------------------------------------------------------------------
// show_mfd is ignored for media and video, which don't have meaningful audio-only data
static void BroadcastDPCPDA(int usetype,
                              int uselevel, 
                              int usewhich, 
                              BOOL show_mfd);
void DPCPDAUseLog(int usetype, int uselevel, int usewhich, BOOL show_mfd)
{
   if (uselevel != -1)
      gLogTimes.m_level = uselevel;

   gLogTimes.m_read[uselevel][usetype][usewhich] = TRUE;

   gPDAFilter = usetype;
   // set offset based on usewhich?
   switch(usetype)
   {
   case 0:
      DPCEmailDisplay(0, uselevel, usewhich, show_mfd);
      BroadcastDPCPDA(usetype, uselevel, usewhich, show_mfd);
      break;
   case 1:
      DPCEmailDisplay(1, uselevel, usewhich, show_mfd);
      // We no longer bother to network at this level; it all gets
      // handled earlier.
      //BroadcastDPCPDA(usetype, uselevel, usewhich, show_mfd);
      break;
   case 2:
      //DPCBookDisplay(uselevel,usewhich);
      break;
   case 3:
      DPCEmailDisplay(3, 0, usewhich, show_mfd);
      break;
      /*
      char moviename[255];
      sprintf(moviename,"vid%02d_%d.avi",uselevel+1,usewhich+1);
      MoviePanel(moviename);
      */
   case 4:
      DPCEmailDisplay(2, 0, usewhich, show_mfd);
      break;
   }
}
//--------------------------------------------------------------------------------------
bool DPCPDACheckTransp(Point pos)
{
   return(FALSE);
}

//--------------------------------------------------------------------------------------
void DPCPDAPlayUnreadLog()
{
   // don't allow reading logs in "slim" mode
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");
   if (slim_mode)
      return;

   //BOOL m_read[NUM_PDA_LEVELS][NUM_PDA_TYPES][MAX_LOG_DATA];
   int level, i;
   sLogData *ldp;
   int itembits;
   sTimeData timelist[MAX_LOG_DATA*NUM_PDA_LEVELS];
   int count = 0;
   int n;

   // so the first thing we do is iterate over all known
   // logs and assemble a massive sorted list of all unread
   // logs
   for (level=0; level < NUM_PDA_LEVELS; level++)
   {
      if (g_LogProperties[level]->Get(PlayerObject(), &ldp))
      {
         itembits = ldp->m_data[1];
         for (i=0; i < MAX_LOG_DATA; i++)
         {
            if (itembits & (1 << i))
            {
               // we have a match, is it unread?
               if (gLogTimes.m_read[level][1][i] == FALSE)
               {
                  timelist[count].time = gLogTimes.m_times[level][1][i];
                  timelist[count].id = i;
                  timelist[count].level = level;
                  count++;
               }
            }
         }
      }
   }

   if (count > 0)
   {
      // now sort that list and actually play the most recent
      qsort(timelist,count, sizeof(timelist[0]), time_compare);
      // which log to actually play?  To play oldest, make n = 0
      n = count - 1;
      DPCPDAUseLog(1, timelist[n].level, timelist[n].id, TRUE); 
   }
}
//--------------------------------------------------------------------------------------
// NETWORKING CODE
//

static cNetMsg *g_pPDAMsg = NULL;

static void handlePDA(int usetype, int uselevel, int usewhich, BOOL show_mfd)
{
   if (DPCPDANeedLog(usetype, uselevel, usewhich)) {
      gLogTimes.m_level = uselevel;
      gPDAFilter = usetype;
      switch (usetype) {
         case 0:
            // We display email for everyone immediately:
            DPCEmailDisplay(0, uselevel, usewhich, show_mfd);
            break;
         case 1:
         {
            // We currently do *not* display logs for everyone. We
            // just let the higher levels say its been picked up.
            // The important work -- noting that we have it -- is
            // a side-effect of DPCPDANeedLog(), above.
            // We now no longer network logs here...
            //break;
         }
         default:
            Warning(("Unknown type of PDA message!\n"));
      }
   }
}

static sNetMsgDesc sPDADesc =
{
   kNMF_Broadcast,
   "PDAMsg",
   "PDA Message",
   NULL,
   handlePDA,
   {{kNMPT_Byte, kNMPF_None, "Type"},
    {kNMPT_Int, kNMPF_None, "Level"},
    {kNMPT_Int, kNMPF_None, "Which"},
    {kNMPT_BOOL, kNMPF_None, "Show MFD"},
    {kNMPT_End}}
};

// Tell the clients to also display the email
static void BroadcastDPCPDA(int usetype,
                              int uselevel, 
                              int usewhich, 
                              BOOL show_mfd)
{
   g_pPDAMsg->Send(OBJ_NULL, usetype, uselevel, usewhich, show_mfd);
}

// Startup and shutdown, which must happen at app init, *not* game init:
void DPCPDANetInit(void)
{
   g_pPDAMsg = new cNetMsg(&sPDADesc);
}

void DPCPDANetTerm(void)
{
   delete g_pPDAMsg;
}
