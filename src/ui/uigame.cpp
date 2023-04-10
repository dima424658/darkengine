/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uigame.cpp,v 1.16 1999/05/27 12:56:05 mahk Exp $
#include <uigame.h>
#include <appagg.h>

#include <guistyle.h>
#include <dev2d.h>
#include <string.h>
#include <ctype.h>
#include <gcompose.h>
#include <config.h>
#include <cfgdbg.h>
#include <gadbase.h>
#include <uiapp.h>
#include <scrnman.h>
#include <metasnd.h>

#include <resapi.h>
#include <fonrstyp.h>

// Must be last header 
#include <dbmem.h>

extern guiStyle gameStyle;   // at the bottom
static IRes* gGUIFont = NULL; 

guiStyle* uiGameStyle()
{
   return &gameStyle; 
}

static char* strip_trailing(char* arg)
{
   char* s = arg + strlen(arg) - 1; 
   for (; s >= arg && isspace(*s); s--)
      *s = '\0'; 

   return arg; 
}

static BOOL cfg_get_lang(const char* var, char* buf, int buflen)
{
   char lang[64]; 
   if (config_get_raw("language",lang,sizeof(lang)))
   {
      char langvar[256];
      sprintf(langvar,"%s_%s",lang,var); 
      if (config_get_raw(langvar,buf,buflen))
         return TRUE; 
   }
   return config_get_raw(var,buf,buflen); 
}

void GameInitGUI(void)
{
   AutoAppIPtr(ResMan); 
   char buf[256] = "default\\font";
   cfg_get_lang("gui_font",buf,sizeof(buf)); 
   strip_trailing(buf);
   gGUIFont = pResMan->Bind(buf,RESTYPE_FONT,NULL); 
   AssertMsg(gGUIFont,"Cannot find GUI font"); 
   LGadSetDefaultFont(gGUIFont); 
   metaSndInit("game");
}

void GameTermGUI(void)
{
   LGadSetDefaultFont(NULL);
   SafeRelease(gGUIFont);
   metaSndTerm();
}

void GameCreateGUI(void)
{
   uiGameStyleSetup();
}

void GameDestroyGUI(void)
{
   uiGameStyleCleanup();
}

static guiStyleMethods *oldMeth=NULL, ourMeth;

void SetMetaSndGUI(void)
{
   if (oldMeth==NULL)
   {
      oldMeth=guiStyleMethodsGet();
      ourMeth=*oldMeth;
      ourMeth.playsound_func=metaSndPlayGUICallback;
      guiStyleMethodsSet(&ourMeth);
   }
   else
      Warning(("Mismatched Set/Restore metasnd GUIs\n"));
}

void RestoreMetaSndGUI(void)
{
   if (oldMeth)
   {
      if (guiStyleMethodsGet()==&ourMeth)   // lets not reset it unless
         guiStyleMethodsSet(oldMeth);       //    it is currently ours
      oldMeth=NULL;
   }
   else
      Warning(("Mismatched Set/Restore metasnd GUIs\n"));
}

////////////////////////////////////////////////////////////
// The game style
//

#define SQR(x) ((x)*(x))

guiStyle gameStyle;     // this is the current usable style

guiStyle masterGameStyle = 
{
   0, // palette
   {  // colors
      uiRGB(255,255,255), // fg
      uiRGB( 10, 10, 10), // bg
      uiRGB( 40,200,200), // text
      uiRGB(255,  0,255), // hilite
      uiRGB(255,255,255), // bright
      uiRGB( 96, 96, 96), // dim
      uiRGB(255,255,255), // fg2
      uiRGB( 64, 64, 64), // bg2
      uiRGB( 40,200,200), // border
      uiRGB(255,255,255), // white
      uiRGB(  5,  5,  5), // black
      1, // xor
      1, // light bevel
      0, // dark bevel
   },
}; 

static char* color_cfg_vars[] =
{
   "color_fg",
   "color_bg",
   "color_text",
   "color_hilite",
   "color_bright",
   "color_dim",
   "color_fg2",
   "color_bg2",
   "color_border",
   "color_white",
   "color_black",
   "color_xor",
   "color_bevel_light",
   "color_bevel_dark",
};

#define NUM_COLOR_VARS (sizeof(color_cfg_vars)/sizeof(color_cfg_vars[0]))
   

void uiGameLoadStyleColors(const char* prefix, guiStyle* style)
{
   int i;
   for (i = 0; i < NUM_COLOR_VARS; i++)                    
   {
      char buf[256];
      int cnt = 3;
      int rgb[3];
      strcpy(buf,prefix);
      strcat(buf,color_cfg_vars[i]);
      if (config_get_value(buf,CONFIG_INT_TYPE,rgb,&cnt))
         if (cnt == 3)
         {
            style->colors[i] = uiRGB(rgb[0],rgb[1],rgb[2]);
         }
   }
}

static const char* font_cfg_vars[] = 
{
   "font_normal",
   "font_title", 
};

#define NUM_FONT_VARS (sizeof(font_cfg_vars)/sizeof(font_cfg_vars[0]))

static void load_style_fonts(const char* prefix,guiStyle* style,const char* relpath)
{
   AutoAppIPtr(ResMan); 
   for (int i = 0; i < NUM_FONT_VARS; i++)                    
   {
      char var[256];
      sprintf(var,"%s%s",prefix,font_cfg_vars[i]); 
      char font[256]; 
      if (cfg_get_lang(var,font,sizeof(font)))
      {
         ConfigSpew("font_load_spew",("Loading %s for %s\n",font,var)); 
         SafeRelease(style->fonts[i]); 
         style->fonts[i] = pResMan->Bind(strip_trailing(font),RESTYPE_FONT,NULL,relpath);
      } 
      if (!style->fonts[i] && gGUIFont)
      {
         ConfigSpew("font_load_spew",("Using default for %s\n",var)); 
         gGUIFont->AddRef(); 
         style->fonts[i] = gGUIFont; 
      }
   }
}

static void unload_style_fonts(guiStyle* style)
{
   for (int i = 0; i < NUM_FONT_VARS; i++)
      SafeRelease(style->fonts[i]); 
}

void uiGameLoadStyle(const char* prefix, guiStyle* style, const char* relpath)
{
   uiGameLoadStyleColors(prefix,style); 
   load_style_fonts(prefix,style,relpath);
   style->sounds[0]=0; style->sounds[1]=1; style->sounds[2]=2; // im a moron
}

void uiGameUnloadStyle(guiStyle* style)
{
   unload_style_fonts(style); 
}

void uiGameProtectStyle(const guiStyle* style)
{
   for (int i = 0; i < StyleNumColors; i++)
   {
      int color = guiScreenColor(style->colors[i]);
      gr_contrast_protect_range(color,color,TRUE); 
   }
}


void uiGameStyleSetup(void)
{
   uiGameStyleCleanup();  // idempotentize

   guiStyle style = masterGameStyle;
   uiGameLoadStyle("game_",&style); 
   guiCompileStyleColors(&gameStyle,&style);
   uiGameProtectStyle(&gameStyle);

   SetCurrentStyle(&gameStyle);
}


void uiGameStyleCleanup(void)
{
   uiGameUnloadStyle(&gameStyle);
}


