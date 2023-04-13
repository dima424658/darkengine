/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/bugterm.c,v 1.9 2000/02/19 12:37:35 toml Exp $

#ifdef PLAYTEST

#include <string.h>
#include <stdio.h>
#include <stdlib.h>     // _MAX_PATH
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <time.h>

#include <kbcook.h>
#include <keydefs.h>
#include <gadtext.h>

#include <command.h>
#include <bugterm.h>
#include <status.h>
#include <simstate.h>
#include <version.h>
#include <wrtype.h>
#include <objpos.h>
#include <playrobj.h>
#include <dbfile.h>
#include <hotkey.h>
#include <gen_bind.h>

#include <config.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

enum StateBits
{
   kStatePaused = 1 << 0,
   kStateInUse  = 1 << 1,
};

struct _bugterm
{
   LGadRoot root;
   LGadTextBox textbox;
   LGadTextBox titlebox;
   char cmdbuf[1900];
   char titlebuf[256];
   ulong flags;
   ulong statebits;
} bugterm;

void bugterm_setup_cmds(void);
void bugterm_focus(void);

////////////////////
// initialization/cleanup

/////////////////////
// commands/setup

FILE *FindFreeFile(char *buff,char *prefix,char *suffix)
{
   char fname[_MAX_PATH];
   int ps;
   FILE *fp;
   int num=0;
   
   ps = strlen(prefix);
   strcpy(fname,prefix);
   strcat(fname,"000.");
   strcat(fname,suffix);
   
   /* Look for files like uwpic000.gif */
   while ( (fp = fopen(fname,"r")) != NULL)
   {  /* Check next slot */
      fclose(fp);                       // good idea to, like, close the opened file
      ++num;
      fname[ps+2] = '0'+(num%10);
      fname[ps+1] = '0'+((num/10)%10);
      fname[ps] = '0'+((num/100)%10);
   }
   
   if ((fp = fopen(fname,"w"))==NULL)
      return NULL;
   if (buff != NULL)
      strcpy(buff,fname);
   return fp;
}

void WriteBugInfo(FILE *fp, char *bugtext)
{
   char *bugUser;
   char bugBuffer[2048];
   char dataBuffer[128];
   char bugDate[20];
   char gamestring[32];
   time_t ltime;
   struct tm *pCurrentTime;
   Position *player_pos;
   ObjID player;
   char curfile[128];
   int i;

   // fixup bugterm to not have any illegal symbols like commas
   for (i=0; i < strlen(bugtext); i++)
   {
      switch (bugtext[i])
      {
      case ',':
         bugtext[i] = ' ';
         break;
      }
   }

   time( &ltime );
   pCurrentTime = localtime( &ltime );

   bugUser = getenv( "USER" );
   if ( bugUser == NULL )
      bugUser = "Unnamed";
   sprintf( bugDate, "%d/%d/%d", pCurrentTime->tm_mon+1, pCurrentTime->tm_mday, pCurrentTime->tm_year );

   config_get_raw("game",gamestring,sizeof(gamestring));

   player = PlayerObject();
   if (player) {
      player_pos = ObjPosGet(player);
      dbCurrentFile(curfile, sizeof(curfile));
      sprintf(dataBuffer,"(%.2f %.2f %.2f)", player_pos->loc.vec.x,
               player_pos->loc.vec.y, player_pos->loc.vec.z);
   }
   else
      strcpy(dataBuffer,"");

   // Format the output string
   // ReporterName	Version	Category	Status	Description	GameState	DateReported
   sprintf(bugBuffer, "%s,%s,,NEW,,,%s,%s %s,,,,,,,,,,,,,,,,,,,,,,,%s", 
      bugUser, VersionString(), bugDate, bugtext,  dataBuffer, curfile );
   //gamestring,
   fputs(bugBuffer,fp);
   
   fputs("\n",fp);
}
///////////////////
// used by the actual client

// returns correct cursor position
static int move_cursor_into_command(char *cmd, int max_len)
{
   if (cmd)
   {
      char *space=strchr(cmd,' ');
      int len=strlen(cmd);
      if (len==0)
         return 0;
      else if ((space==NULL)&&(len<max_len))
      {
         strcpy(cmd+len," ");
         space=cmd+len;
      }
      while (*space==' ') space++;
      return space-cmd;
   }
   return 0;
}

////////////////
// callback for textbox

static short speckeys[] = 
{
   KB_FLAG_DOWN|KEY_ENTER,
   KB_FLAG_DOWN|KEY_ESC,
   KB_FLAG_DOWN|KB_FLAG_CTRL|'g',
   KB_FLAG_DOWN|KB_FLAG_CTRL|'n',
   KB_FLAG_DOWN|KB_FLAG_CTRL|'p',
   KB_FLAG_DOWN|KEY_TAB,
   0
};

void ReportBug(char *buf)
{
   FILE *fp;
   char bugroot[255],bugdir[255];

   config_get_raw("bugrep_dir",bugdir,sizeof(bugdir));
   //sprintf(bugroot,"%s\\bug",bugdir);
   sprintf(bugroot,"%s\\bugs.txt",bugdir);
   fp = fopen(bugroot,"at"); //FindFreeFile(NULL,bugroot,"txt");
   if (fp != NULL)
   {
      WriteBugInfo(fp,buf);
      fclose(fp);
   }
}
#pragma off(unreferenced)
bool bugterm_textbox_cb(LGadTextBox* box, LGadTextBoxEvent event, int evdata, void* data)
{
   char* text = LGadTextBoxText(box);
   BOOL update = FALSE, clear = FALSE, new_base=FALSE;
   
   if (event == TEXTBOX_SPECKEY)
   {
      char *tmp=NULL;
      unsigned long context;

      switch(evdata)
      {
      case KB_FLAG_DOWN|KEY_ENTER:
         Status("");  // dont want to do this as part of clear since the 

         // Unpause before command executes, so the command is free to change state
         if (bugterm.statebits & kStatePaused)
         {
            SimStateUnpause();
            bugterm.statebits &= ~kStatePaused;
         }
      
         g_pInputBinder->lpVtbl->GetContext (g_pInputBinder, &context);
         if (context == HK_COMMAND_MODE && HotkeyContext == HK_GAME_MODE) {
            g_pInputBinder->lpVtbl->SetContext (g_pInputBinder, HK_GAME_MODE, TRUE);
         }

         if (strlen(bugterm.cmdbuf) > 0)
         {
            ReportBug(bugterm.cmdbuf);
            Status("Bug reported.");
         }
         *text = '\0';

         update = TRUE;
         clear = TRUE;
         break;
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'g':
      case KB_FLAG_DOWN|KEY_ESC:
         g_pInputBinder->lpVtbl->GetContext (g_pInputBinder, &context);
         if (context == HK_COMMAND_MODE && HotkeyContext == HK_GAME_MODE) {
            g_pInputBinder->lpVtbl->SetContext (g_pInputBinder, HK_GAME_MODE,TRUE);
         }
         if (text&&((*text)!='\0'))
            Status("");
         *text = '\0';
         update = TRUE;
         clear = TRUE;
         break;
      }
      if (tmp!=NULL)
      {
         strcpy(text,tmp);
         update = TRUE;
      }
   }
   if (update)
   {
      LGadUpdateTextBox(box);
      if (clear)
      {
         LGadUnfocusTextBox(box);
         if (bugterm.flags & kBugTermHideUnfocused)
            region_set_invisible(LGadBoxRegion(&bugterm.root),TRUE);
         if (bugterm.statebits & kStatePaused)
         {
            SimStateUnpause();
            bugterm.statebits &= ~kStatePaused;
         }
      }
      else if (!(bugterm.statebits & kStatePaused))
      {
         SimStatePause();
         bugterm.statebits |= kStatePaused;
      }
   }
   return update;
}
#pragma on(unreferenced)

#define TITLE_H 15

void CreateBugTerminal(LGadRoot* root, Rect* bounds, ulong flags)
{
   if (bugterm.statebits & kStateInUse)
      DestroyBugTerminal();
   memset(&bugterm,0,sizeof(bugterm));
   LGadSetupSubRoot(&bugterm.root,root,bounds->ul.x,bounds->ul.y,(short)RectWidth(bounds),(short)RectHeight(bounds));

   // make a text gadg on the bottom row.
   {   
      LGadTextBoxDesc tdesc;

      memset(&tdesc,0,sizeof(tdesc));
      tdesc.bounds.lr = MakePoint(RectWidth(bounds),RectHeight(bounds));
      tdesc.bounds.ul = MakePoint(0,TITLE_H);
      
      tdesc.editbuf = bugterm.cmdbuf;
      tdesc.editbuflen = sizeof(bugterm.cmdbuf);
      bugterm.cmdbuf[0] = '\0';
      tdesc.flags = TEXTBOX_BORDER_FLAG|TEXTBOX_FOCUS_FLAG;
      tdesc.cb = bugterm_textbox_cb;

      LGadCreateTextBoxDesc(&bugterm.textbox,&bugterm.root,&tdesc);
      LGadTextBoxSetSpecialKeys(&bugterm.textbox,speckeys);
      LGadBoxSetFlags(&bugterm.textbox,
         LGadBoxFlags(&bugterm.textbox)|BOXFLAG_FLIP);
   }
   {
      LGadTextBoxDesc tdesc;

      memset(&tdesc,0,sizeof(tdesc));
      tdesc.bounds.lr = MakePoint(RectWidth(bounds),TITLE_H);
      tdesc.bounds.ul = MakePoint(0,0);
      
      tdesc.editbuf = bugterm.titlebuf;
      tdesc.editbuflen = sizeof(bugterm.titlebuf);
      sprintf(bugterm.titlebuf, "Enter bug report:");
      //tdesc.flags = TEXTBOX_BORDER_FLAG|TEXTBOX_FOCUS_FLAG;
      //tdesc.cb = bugterm_textbox_cb;

      LGadCreateTextBoxDesc(&bugterm.titlebox,&bugterm.root,&tdesc);
      LGadBoxSetFlags(&bugterm.titlebox,
         LGadBoxFlags(&bugterm.titlebox)|BOXFLAG_FLIP);
      LGadTextBoxClrFlag(&bugterm.titlebox,TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW);
   }

   bugterm.statebits = kStateInUse;
   bugterm.flags = flags;
   if (flags & kBugTermBeginFocused)
   {
      bugterm_focus();
   }
   else if (flags & kBugTermHideUnfocused)
   {
      region_set_invisible(LGadBoxRegion(&bugterm.root),TRUE);
   }
   LGadDrawBox(VB(&bugterm.root),NULL);

   bugterm_setup_cmds();
}

void DestroyBugTerminal(void)
{
   bugterm.statebits &= ~kStateInUse;
   LGadDestroyTextBox(&bugterm.textbox);
   LGadDestroyTextBox(&bugterm.titlebox);
   LGadDestroyRoot(&bugterm.root);
   
}

#if 0
void RedrawBugTerminal(void)
{
   LGadDrawBox(VB(&bugterm.root),NULL);
}
#endif

void bugterm_focus()
{
   char* buf = &bugterm.cmdbuf[0];
   int bufsiz = sizeof(bugterm.cmdbuf);
   int cursor_pos;
   unsigned long context;

   if (!(bugterm.statebits & kStateInUse))
      return;

   strcpy(buf,"");

   // make the region visible, if necessary
   region_set_invisible(LGadBoxRegion(&bugterm.root),FALSE);
   cursor_pos=move_cursor_into_command(buf,bufsiz);

   LGadTextBoxSetCursor(&bugterm.textbox,cursor_pos);
   LGadUpdateTextBox(&bugterm.textbox);
   LGadFocusTextBox(&bugterm.textbox);
   LGadTextBoxClrFlag(&bugterm.textbox,TEXTBOX_EDIT_BRANDNEW);
   LGadDrawBox(VB(&bugterm.textbox),NULL);
   LGadDrawBox(VB(&bugterm.titlebox),NULL);

   g_pInputBinder->lpVtbl->GetContext (g_pInputBinder, &context);
   if (context == HK_GAME_MODE)
      g_pInputBinder->lpVtbl->SetContext (g_pInputBinder, HK_COMMAND_MODE, TRUE);

   // pause the sim state
   SimStatePause();
   bugterm.statebits |= kStatePaused;
}

static void new_bugterm()
{
   //history_new_bugterm();
   bugterm_focus();
}

void report_bugtext(char *text)
{
   ReportBug(text);
}

static Command commands[] = 
{
   // should this be ifndef ship'ed ?
   { "report_bug", FUNC_VOID, new_bugterm, "signal a bug in game",HK_GAME_MODE},
   { "report_bug_text", FUNC_STRING, report_bugtext, "signal a bug in game",HK_GAME_MODE},
};

void bugterm_setup_cmds(void)
{
   static bool setup = FALSE;
   if (!setup)
   {
     COMMANDS(commands,HK_ALL);
     setup = TRUE;
   }
}

#endif
