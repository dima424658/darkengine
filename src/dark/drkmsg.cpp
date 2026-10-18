/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmsg.cpp,v 1.6 2000/02/19 12:27:23 toml Exp $
#include <drkmsg.h>
#include <str.h>
#include <simtime.h>
#include <resapi.h>
#include <fonrstyp.h>
#include <appagg.h>
#include <config.h>
#include <gshelapi.h>
#include <gcompose.h>
#include <uigame.h>
#include <2d.h>

#include <command.h>
#include <stdio.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// Dark Message Stuff
//

#define FONTDIR "intrface"
#define FONTRES "textfont"

struct sMsgData
{
   cStr text; 
   ulong timeout; 
   int color; 
}; 

static sMsgData gMsg; 
static IRes* gpFontRes; 
static char gFontName[64] = FONTRES;
static IGameShell* gpGameShell; 
static int gTextRGB[3] = { 0xFF, 0xFF, 0xFF }; 
static int gTextMargins[] = { 10, 5 }; // x,y margins in % of screen 
static short gLastTextBottom;

static void init_commands();

//
// Init/Term
//
void DarkMessageInit(void)
{
   AutoAppIPtr(ResMan); 
   // The game canvas is not established yet. Bind the legacy fallback now
   // and choose a density-aware face on the first frame, once the logical to
   // physical presentation scale is available.
   gpFontRes = pResMan->Bind(FONTRES, RESTYPE_FONT, NULL, FONTDIR);
   strncpy(gFontName, FONTRES, sizeof(gFontName));
   gFontName[sizeof(gFontName) - 1] = '\0';
   gpGameShell = AppGetObj(IGameShell); 
   int cnt = 3;
   config_get_value("text_message_rgb",CONFIG_INT_TYPE,gTextRGB,&cnt); 

   cnt = 2; 
   config_get_value("text_message_margins",CONFIG_INT_TYPE,gTextMargins,&cnt); 
   init_commands(); 

}

void DarkMessageTerm(void)
{
   SafeRelease(gpFontRes); 
   strncpy(gFontName, FONTRES, sizeof(gFontName));
   gFontName[sizeof(gFontName) - 1] = '\0';
   SafeRelease(gpGameShell); 
}

static void resolve_message_font(void)
{
   char font[64];
   IRes* readable;

   strncpy(font, FONTRES, sizeof(font));
   font[sizeof(font) - 1] = '\0';
   uiGameGetReadableOverlayFont("text_message_font", font, sizeof(font));
   if (!stricmp(font, gFontName))
      return;

   AutoAppIPtr(ResMan);
   readable = pResMan->Bind(font, RESTYPE_FONT, NULL, FONTDIR);
   if (readable)
   {
      SafeRelease(gpFontRes);
      gpFontRes = readable;
      strncpy(gFontName, font, sizeof(gFontName));
      gFontName[sizeof(gFontName) - 1] = '\0';
   }
}

//
// Display 
//

#define DEFAULT_TIMEOUT 5000

void DarkMessageParams(const char* msg, ulong timeout, int color)
{
#ifdef PLAYTEST
   if (config_is_defined("message_spew"))
      fprintf(stderr, "DarkMessage: %s\n", msg ? msg : "(null)");
#endif
   if (color == kDefaultMsgColor)
      color = uiRGB(gTextRGB[0],gTextRGB[1],gTextRGB[2]);
   ulong time = gpGameShell->GetTimerTick();  
   gMsg.text = msg; 
   gMsg.timeout = time + timeout; 
   gMsg.color = color; 
}

void DarkMessage(const char* msg)
{
   DarkMessageParams(msg,DEFAULT_TIMEOUT,kDefaultMsgColor); 
}

void DarkMessageUpdateFrame(void)
{
   resolve_message_font();
   Rect r = { 0, 0, grd_canvas->bm.w, grd_canvas->bm.h };
   BOOL drawText = gpGameShell->GetTimerTick() < gMsg.timeout;

   // compute margins
   // left margin
   r.ul.x = gTextMargins[0]*RectWidth(&r)/100;
   // right margin
   r.lr.x -= r.ul.x;

   // top margin
   r.ul.y = gTextMargins[1]*RectHeight(&r)/100;

   char* s = NULL;
   grs_font* font = NULL;
   short w = 0;
   short h = 0;
   if (drawText)
   {
      s = (char*)(const char*)gMsg.text;
      font = (grs_font*)gpFontRes->Lock();
      gr_font_string_wrap(font,s,RectWidth(&r));

      gr_font_string_size(font,s,&w,&h);
      r.lr.y = r.ul.y + h;
      if (r.lr.y < gLastTextBottom)
         r.lr.y = gLastTextBottom;
   }
   else
      r.lr.y = gLastTextBottom;

   if (r.lr.y <= r.ul.y)
      return;

   GUIcompose c;
   GUIsetup(&c,&r,(GUIcomposeFlags)(ComposeFlagRead|ComposeFlagClear),GUI_CANV_ANY);

   if (drawText)
   {
      int x = (RectWidth(&r) - w)/2;
      gr_set_font(font); 
      int color = gr_make_screen_fcolor(gMsg.color); 
      gr_set_fcolor(color); 
      gr_string(s,x,0);

      gr_font_string_unwrap(s);
      gpFontRes->Unlock();
   }
   gLastTextBottom = drawText ? r.ul.y + h : 0;

   GUIdone(&c);
}

static Command commands[] = 
{
   { "game_message", FUNC_STRING, DarkMessage, "Display a message on the game screen", HK_GAME_MODE },
}; 

static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}
