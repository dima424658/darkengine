/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <2d.h>

#include <resapi.h>
#include <schbase.h>
#include <schema.h>

#include <objsys.h>
#include <osysbase.h>
#include <playrobj.h>
#include <dpcplayr.h>
#include <dpcpdcst.h>
#include <dpcplprp.h>

#include <dpcemail.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpcmfddm.h>
#include <dpciftul.h>
#include <dpcgame.h>

#include <mprintf.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

#define NUM_MODES 4

static IRes *gBackHnd[NUM_MODES];
static IRes *gPortraitHnd;
static IRes *gIconHnd;
static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{375,250},{375 + 32, 250 + 32}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

#define FACE_X 15
#define FACE_Y 13
#define ICON_X 83
#define ICON_Y 13
#define TEXT_X 15
#define TEXT_Y 105
#define TEXT_W 136
#define TEXT_H 175
#define EMAIL_SEPARATOR 4

static char gEmailText[2048];
static char gEmailHeader[512];
static int gEmailOffset = 0;
static int gEmailMode = 0;
static int gEmailAudioHnd = -1; //  = SFX_NO_HND;

static char *mode_roots[] = {"Email","Log", "ResRep", "Help"};
static char *short_mode_roots[] = {"Em","Log", "Rep", "Hlp" };

typedef enum eEmailButton { kEmailButtonUp, kEmailButtonPageUp, kEmailButtonPageDown,kEmailButtonDown, kEmailButtonReturn };

#define NUM_EMAIL_BUTTONS 5
static Rect email_rects[NUM_EMAIL_BUTTONS] = 
{
   {{159,154},{159 + 18, 154 + 17}},   // lineup
   {{159,174},{159 + 18, 174 + 26}},   // pageup
   {{159,203},{159 + 18, 203 + 26}},   // pagedown
   {{159,232},{159 + 18, 232 + 17}},   // linedown
   {{159,252},{159 + 18, 252 + 34}},   // return
};

static LGadButtonList email_blist;
static LGadButtonListDesc email_blistdesc;
static DrawElement email_blist_elems[NUM_EMAIL_BUTTONS];
static IRes *email_handles[NUM_EMAIL_BUTTONS][2];
static grs_bitmap *email_bitmaps[NUM_EMAIL_BUTTONS][4];
static char *button_names[NUM_EMAIL_BUTTONS] = { "up","pgup","pgdn","down","return"};

//--------------------------------------------------------------------------------------
void DPCEmailInit(int )
{
   int i,n;
   char temp[255];

   close_handles[0] = LoadPCX("butup"); 
   close_handles[1] = LoadPCX("butdown"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   for (i=0; i < NUM_MODES; i++)
      gBackHnd[i] = LoadPCX(mode_roots[i]);

   SetLeftMFDRect(kOverlayEmail, full_rect);

   for (n=0; n < NUM_EMAIL_BUTTONS; n++)
   {
      for (i=0; i < 2; i++)
      {
         sprintf(temp,"%s%d",button_names[n],i);
         email_handles[n][i] = LoadPCX(temp); 
         email_bitmaps[n][i] = (grs_bitmap *) email_handles[n][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         email_bitmaps[n][i] = email_bitmaps[n][0];
      }
   }

}

//--------------------------------------------------------------------------------------
void DPCEmailTerm(void)
{
   int i,n;
   for (i=0; i < NUM_MODES; i++)
      SafeFreeHnd(&gBackHnd[i]);

   if (gPortraitHnd != NULL)
      SafeFreeHnd(&gPortraitHnd);
   if (gIconHnd != NULL)
      SafeFreeHnd(&gIconHnd);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   for (n=0; n < NUM_EMAIL_BUTTONS; n++)
   {
      email_handles[n][0]->Unlock();
      email_handles[n][1]->Unlock();
      SafeFreeHnd(&email_handles[n][0]);
      SafeFreeHnd(&email_handles[n][1]);
   }
}

//--------------------------------------------------------------------------------------
void DPCEmailDraw(unsigned long inDeltaTicks)
{
   Rect r = DPCOverlayGetRect(kOverlayEmail);
   Point drawpt;
   int dx,dy,h;
   grs_clip saveclip;

      // background
   DrawByHandle(gBackHnd[gEmailMode],r.ul);

   // art area up top
   drawpt = r.ul;
   drawpt.x += FACE_X;
   drawpt.y += FACE_Y;
   DrawByHandle(gPortraitHnd,drawpt);

   drawpt.x = r.ul.x + ICON_X;
   drawpt.y = r.ul.y + ICON_Y;
   DrawByHandle(gIconHnd, drawpt);

   // main text
   dx = TEXT_X + r.ul.x;
   dy = TEXT_Y + r.ul.y - gEmailOffset;
   memcpy(&saveclip,&grd_clip,sizeof(grs_clip)); // gr_save_cliprect(&saveclip);
   gr_set_cliprect(TEXT_X + r.ul.x, TEXT_Y + r.ul.y, TEXT_X + TEXT_W + r.ul.x, TEXT_Y + TEXT_H + r.ul.y);
   gr_set_fcolor(gDPCTextColor);

   gr_font_string(gDPCFont,gEmailHeader,dx,dy);
   h = gr_font_string_height(gDPCFont,gEmailHeader);

   gr_font_string(gDPCFont,gEmailText,dx,dy+h+EMAIL_SEPARATOR);
   memcpy(&grd_clip,&saveclip,sizeof(grs_clip)); //gr_restore_cliprect(&saveclip);

   // @TODO:  Bodisafa 10/30/1999
   // Don't draw the other interface buttons for the Microsoft demo.
   // scroll and return buttons
   //// LGadDrawBox(VB(&email_blist),NULL);

   // close button
   LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
bool DPCEmailHandleMouse(Point pt)
{
   return(FALSE);
}
//--------------------------------------------------------------------------------------
static void SchemaDoneCallback(int /*hSchema*/, ObjID /*schemaID*/, void* /*pData*/)
{
   gEmailAudioHnd = -1;
}

/* Was...
static void SoundDoneCallback(int iHandle, void *pIgnoreUserData)
{
   gEmailAudioHnd = -1; // SFX_NO_HND;
}
*/

//--------------------------------------------------------------------------------------
void DPCEmailPlayAudio(char *artname)
{
   //mprintf("artname is %s\n",artname);

   // bail instantly if not really trying to play
   if (strlen(artname) == 0)
      return;

   /*
   sfx_parm parm;

   // kill previous sample
   if (gEmailAudioHnd != SFX_NO_HND)
   {
      SFX_Kill_Hnd(gEmailAudioHnd);
      gEmailAudioHnd = SFX_NO_HND;
   }

   // set up parms
   memset(&parm,0,sizeof(sfx_parm));
   parm.gain = -1;
   parm.end_callback = &SoundDoneCallback;
   parm.num_loops = 1;
   parm.flag = SFXFLG_NOCACHE|SFXFLG_STREAM;

   // play it!
   gEmailAudioHnd = SFX_Play_Raw(SFX_STATIC, &parm, artname);
   */

   // Set up callback which resets our global upon play completion.
   sSchemaCallParams schParams;
   memset (&schParams, 0, sizeof (schParams));;
   schParams.flags = SCH_SET_CALLBACK;
   schParams.sourceID = OBJ_NULL;
   schParams.callback = SchemaDoneCallback;
   //   schParams.pData = ;         // user data for callback

   // Halt if an email is already playing.
   if (gEmailAudioHnd != -1)
   {
      SchemaPlayHalt(gEmailAudioHnd);
      gEmailAudioHnd = -1;
   }

   // Play the email.
   Label l;
   strncpy(l.text,artname,sizeof(l));
   gEmailAudioHnd = SchemaPlay(&l, &schParams); //NULL);
}
//--------------------------------------------------------------------------------------
void DPCEmailDisplay(int mode, int level, int which, BOOL show_mfd)
{
   char artname[256];
   char findname[64];
   char levelname[64];

   gEmailOffset = 0;
   gEmailMode = mode;

   if (show_mfd)
   {
      if (gPortraitHnd != NULL)
         SafeFreeHnd(&gPortraitHnd);
      if (gIconHnd != NULL)
         SafeFreeHnd(&gIconHnd);

      // okay, this is getting retarded
      if (mode == 2)
         sprintf(levelname,"research",level+1);
      else if (mode == 3)
         sprintf(levelname,"infocomp", level + 1);
      else
         sprintf(levelname,"level%02d",level+1);
         
      sprintf(findname,"%sPortrait",mode_roots[gEmailMode]);
      DPCStringFetch(artname,sizeof(artname),findname,levelname,which+1);
      gPortraitHnd = LoadPCX(artname, "book\\");

      sprintf(findname,"%sIcon",mode_roots[gEmailMode]);
      DPCStringFetch(artname,sizeof(artname),findname,levelname,which+1);
      gIconHnd = LoadPCX(artname, "book\\");

      strcpy(gEmailText,"");
      sprintf(findname,"%sText",mode_roots[gEmailMode]);
      DPCStringFetch(gEmailText,sizeof(gEmailText),findname,levelname,which+1);
      gr_font_string_wrap(gDPCFont,gEmailText,TEXT_W);

      strcpy(gEmailHeader,"");
      sprintf(findname,"%sName",mode_roots[gEmailMode]);
      DPCStringFetch(gEmailHeader,sizeof(gEmailHeader),findname,levelname,which+1);
      gr_font_string_wrap(gDPCFont,gEmailHeader,TEXT_W);

      DPCOverlayChange(kOverlayEmail, kOverlayModeOn);
   }

   //sprintf(artname,"..\\logs\\%s%02d%02d",short_mode_roots[gEmailMode],level+1,which+1);
   // don't try and play audio on reports notes or help text
   if (gEmailMode <= 1)
   {
      sprintf(artname,"%s%02d%02d",short_mode_roots[gEmailMode],level+1,which+1);
      DPCEmailPlayAudio(artname);
   }
}

void DeferredMouseMode(void *);

//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
// @TODO:  Bodisafa 10/30/1999
//  Hack for Microsoft Demo
#if 0
      SchemaPlay((Label *)"subpanel_cl",NULL);
      uiDefer(DeferOverlayClose,(void *)kOverlayEmail);
#else
      uiDefer(DeferredMouseMode, NULL);
#endif // 0
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
#define SCROLL_LINE     12
#define SCROLL_PAGE     (TEXT_H - (SCROLL_LINE * 2))
#define SCROLL_BUFFER   40
static bool email_cb(ushort action, int button, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   int scrollamt;
   int h,max_offset;

   switch(button)
   {
   case kEmailButtonDown:
      scrollamt = SCROLL_LINE;
      // fallthrough
   case kEmailButtonPageDown:
      if (button == kEmailButtonPageDown)
         scrollamt = SCROLL_PAGE;

      SchemaPlay((Label *)"bscroll",NULL);

      h = gr_font_string_height(gDPCFont, gEmailText) + gr_font_string_height(gDPCFont, gEmailHeader) + EMAIL_SEPARATOR;
      if (h > TEXT_H)
      {
         gEmailOffset += scrollamt;
         max_offset = h - TEXT_H + SCROLL_BUFFER;
         if (gEmailOffset > max_offset)
            gEmailOffset = max_offset;
      }
      break;
   case kEmailButtonUp:
      scrollamt = SCROLL_LINE;
      // fallthrough
   case kEmailButtonPageUp:
      if (button == kEmailButtonPageUp)
         scrollamt = SCROLL_PAGE;
      SchemaPlay((Label *)"bscroll",NULL);

      gEmailOffset -= scrollamt;
      if (gEmailOffset < 0)
         gEmailOffset = 0;
      break;

   case kEmailButtonReturn:
      // close
      uiDefer(DeferOverlayClose,(void *)kOverlayEmail);
      // open PDA
      uiDefer(DeferOverlayOpen,(void *)kOverlayPDA);
      break;
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   int n;
   Rect r= DPCOverlayGetRect(kOverlayEmail);
   static Rect use_rects[NUM_EMAIL_BUTTONS];

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   for (n=0; n < NUM_EMAIL_BUTTONS; n++)
   {
      DrawElement *elem = &email_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = email_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required

      use_rects[n].ul.x = email_rects[n].ul.x + r.ul.x;
      use_rects[n].ul.y = email_rects[n].ul.y + r.ul.y;
      use_rects[n].lr.x = email_rects[n].lr.x + r.ul.x;
      use_rects[n].lr.y = email_rects[n].lr.y + r.ul.y;
   }
   
   email_blistdesc.num_buttons = NUM_EMAIL_BUTTONS;
   email_blistdesc.button_rects = use_rects;
   email_blistdesc.button_elems = email_blist_elems;
   email_blistdesc.cb = email_cb;

   LGadCreateButtonListDesc(&email_blist, LGadCurrentRoot(), &email_blistdesc);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&email_blist),FALSE);
}
//--------------------------------------------------------------------------------------
void DPCEmailStateChange(int which)
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
bool DPCEmailCheckTransp(Point pt)
{
   return(FALSE);
}
//--------------------------------------------------------------------------------------
// needs a better return type!
int DPCFindLogData(ObjID obj, int usetype, int *level, int *which)
{
   ObjID player = PlayerObject();
   int i;
   sLogData *ldp;
   
   // okay, first check whether it actually has any log properties on it
   *which = *level = -1;
   for (i=0; i < NUM_PDA_LEVELS; i++)
   {
      if (g_LogProperties[i]->IsRelevant(obj))
      {
         *level = i;
         break;
      }
   }
   if (*level == -1)
      return(S_FALSE);
   // fetch the data
   g_LogProperties[*level]->Get(obj,&ldp);

   // figure out the kind of log object
   if (ldp->m_data[usetype] == 0)
      return(S_FALSE);

   // figure out which actual log bit is set
   for (i=0; i < MAX_LOG_DATA; i++)
   {
      if (ldp->m_data[usetype] & (1 << i))
      {
         *which = i;
         break;
      }
   }
   return(S_OK);
}

//--------------------------------------------------------------------------------------
void DPCEmailStop(void)
{
   if (gEmailAudioHnd != -1)
   {
      SchemaPlayHalt(gEmailAudioHnd);
      gEmailAudioHnd = -1;
   }
}
//--------------------------------------------------------------------------------------

sOverlayFunc OverlayEmail = 
{ 
   DPCEmailDraw,           // draw
   DPCEmailInit,           // init
   DPCEmailTerm,           // term
   DPCEmailHandleMouse,    // mouse
   NULL,                   // dclick (really use)
   NULL,                   // dragdrop
   NULL,                   // key
   NULL,                   // bitmap
   "",                     // upschema
   "",                     // downschema
   DPCEmailStateChange,    // state
   DPCEmailCheckTransp,    // transparency
   0,                      // distance
   TRUE,                   // needmouse
};
