/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/metasnd.cpp,v 1.5 2000/02/19 12:37:11 toml Exp $

#include <guistyle.h>
#include <metasnd.h>
#include <config.h>
#include <timer.h>

#include <mprintf.h>

#include <appsfx.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////
// state

static BOOL inGameMode=FALSE;    // from gamemode.c - what a hack
static int  screen_type=-1;      // which type of panel is this
static int  vol_offset=0;        // do we want to hack volumes
static BOOL metaGameSound=TRUE;  // if we want to be able to turn it off

//#define TEST_MSND
#ifdef TEST_MSND
#define msnd_mprintf(x) mprintf x
#else
#define msnd_mprintf(x)
#endif

///////////////////
// guistyle

#define kMetaSndTick     (0)
#define kMetaSndSelect   (1)
#define kMetaSndAttend   (2)
#define kMetaSndAmbient1 (3)
#define kMetaSndAmbient2 (4)
#define kMetaSndNumTypes (5)

static char  metaSndNames[kMetaSndPanelCount][kMetaSndNumTypes][16];
static char *screens[kMetaSndPanelCount]={"def","main","saveload","options","loadout","book","game"};
static char *posts[kMetaSndNumTypes]={"tick","select","attend","ambient1","ambient2"};
static int   metaSndAmbHandle=SFX_NO_HND;

#define metaGetNamed(scr,snd) (metaSndNames[scr][kMetaSnd##snd])
#define metaGetNum(scr,num)   (metaSndNames[scr][num])

// load the stuff
void metaSndLoadGUINames(char *prefix)
{
   char buf[64];
   for (int i=0; i<kMetaSndPanelCount; i++)
      for (int j=0; j<kMetaSndNumTypes; j++)
      {
         sprintf(buf,"metasnd_%s_%s_%s",prefix,screens[i],posts[j]);
         if (config_is_defined(buf))
         {
            config_get_raw(buf,metaSndNames[i][j],16 /* sizeof(metaSndNames[i][j]) */);
         }
         else
            strcpy(metaSndNames[i][j],"");
      }
}

//////////////////////
// this is our actual callback

// do a frame
void metaSndDoFrame(void)
{
#ifdef AUTO_PAUSE   
   int tm=tm_get_millisec_unrecorded();
   while (tm_get_millisec_unrecorded()<tm+1)
      ;  // we are kinda trying to wait for asynch for now
#endif   
   SFX_Frame(NULL,NULL);  // lets repump
}

bool metaSndPlayGUICallback(StyleSound sound)
{  // will be StyleSoundTick, Select, Attention
   if (screen_type!=-1)
      if (metaSndNames[screen_type][sound][0]!='\0')
         if (metaGameSound)
         {
            SFX_Play_Raw(SFX_STATIC,NULL,metaGetNum(screen_type,sound));
            msnd_mprintf(("Argh, %d on screen %d\n",sound,screen_type));
            if (sound!=0)
               metaSndDoFrame();
         }
   return TRUE;
}

/////////////////////
// controlled plays for mode transistions
static void _metasnd_end_callback(int hSound, void *user_data)
{
   if (hSound==metaSndAmbHandle)
   {
      metaSndAmbHandle=SFX_NO_HND;
      msnd_mprintf(("Killing our amb in callback\n"));
   }
}

void metaSndEnterPanel(int panel_type)
{
   if (inGameMode&&panel_type!=kMetaSndPanelOptions&&panel_type!=kMetaSndPanelBook) 
      panel_type=kMetaSndPanelInGame; // for now - ok, this is getting idiotic
   
   // in real life we should, if inGameMode, do nothing here, i think
   BOOL no_change=(metaSndAmbHandle!=SFX_NO_HND);
   msnd_mprintf(("meta Enter %d (%d) [%d]\n",panel_type,inGameMode,screen_type));
   if (panel_type!=kMetaSndPanelLast)
   {
      if ((screen_type!=panel_type)&&(no_change))
         if (screen_type==-1)
            no_change=FALSE;
         else if (stricmp(metaGetNamed(screen_type,Ambient1),metaGetNamed(panel_type,Ambient1))!=0)
            no_change=FALSE;
      screen_type=panel_type;
   }
   if (no_change)
    { msnd_mprintf(("Same Amb\n")); return; }
   if (metaGetNamed(screen_type,Ambient1)[0]!='\0')
   {
      sfx_parm loop_parm={SFX_DEF_PARM,SFXFLG_LOOP,0,0};
      loop_parm.end_callback=_metasnd_end_callback;
      if (metaSndAmbHandle!=SFX_NO_HND)
      {
         msnd_mprintf(("Already Playing\n"));
         SFX_Kill_Hnd(metaSndAmbHandle);
      }
      if (metaGameSound)
         metaSndAmbHandle=SFX_Play_Raw(SFX_STATIC,&loop_parm,metaGetNamed(screen_type,Ambient1));
      msnd_mprintf(("handle %d for [%s]\n",metaSndAmbHandle,metaGetNamed(screen_type,Ambient1)));
   }
   else if (metaSndAmbHandle!=SFX_NO_HND)
   {  // we are supposed to play nothing!
      msnd_mprintf(("Nothing to play, killing existing\n"));
      SFX_Kill_Hnd(metaSndAmbHandle);
   }
   else
      msnd_mprintf(("Nothing to play, nothing currently playing\n"));
}

void metaSndExitPanel(BOOL kill_snds)
{
   if (kill_snds&&(metaSndAmbHandle!=SFX_NO_HND))
   {
      SFX_Kill_Hnd(metaSndAmbHandle);
      metaSndAmbHandle=SFX_NO_HND;
      screen_type=-1;
      msnd_mprintf(("killed..."));
   }
   else if (kill_snds)
      screen_type=-1;
   msnd_mprintf(("meta Exit\n"));
}

///////////
// find out what mode

void metaSndEnterGame(void)
{
   inGameMode=TRUE;
   metaSndExitPanel(TRUE);
   msnd_mprintf(("msGameOn\n"));
}

void metaSndExitGame(void)
{
   inGameMode=FALSE;
   msnd_mprintf(("msGameOff\n"));
}

///////////
// init/term

void metaSndInit(char *snd_prefix)
{
   metaSndLoadGUINames(snd_prefix);
   if (config_is_defined("no_metagame_sound"))
      metaGameSound=FALSE;
}

void metaSndTerm(void)
{
}
