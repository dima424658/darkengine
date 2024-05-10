/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/cmdterm.c,v 1.28 2000/02/19 13:28:19 toml Exp $

#include <string.h>
#include <stdio.h>

#include <kbcook.h>
#include <keydefs.h>
#include <gadtext.h>
#include <hotkey.h>

#include <cmdterm.h>
#include <command.h>
#include <gen_bind.h>
#include <status.h>
#include <simstate.h>

#include <config.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


enum StateBits
{
   kStatePaused = 1 << 0,
   kStateInUse  = 1 << 1,
};


struct _CmdTerm
{
   LGadRoot root;
   LGadTextBox textbox;
   char cmdbuf[256];
   ulong flags;
   ulong statebits;
} CmdTerm;

void cmdterm_setup_cmds(void);
void cmdterm_focus(char* arg);

///////////////////////////
// stupid command history system

#define MAX_HISTORY_CNT 256

#define HISTORY_NO_LAST (-1)

#define HISTORY_FILE    "loc_hist.hst"
#define PREHISTORY_FILE "pre_hist.hst"

static char *cmd_history_text[MAX_HISTORY_CNT];
static int   cmd_history_idx=0, cmd_history_last_found=0;
static BOOL  cmd_try_command=FALSE;

#define history_string(idx) (cmd_history_text[idx])

//////////////////
// history idx manager
static int _history_get_cur_idx(void)
{
   int cur_idx=cmd_history_last_found;
   if (cur_idx==HISTORY_NO_LAST) cur_idx=cmd_history_idx;
   return cur_idx;
}

static void _history_set_idx(int idx)
{
   cmd_history_last_found=idx;
}

static int _history_get_idx(int offset)
{
   int idx=_history_get_cur_idx();
   idx+=MAX_HISTORY_CNT+offset;
   idx%=MAX_HISTORY_CNT;
   return idx;
}

// clear the history variables
static void _hist_clear(void)
{
   cmd_history_idx=cmd_history_last_found=0;
   cmd_try_command=FALSE;
}

/////////////////////
// file i/o of history list

// write out a history text file
static void _history_write_file(char *fname)
{
   FILE *fp;   // well, files are evil, but oh well
   int i;

   if ((fp=fopen(fname,"w"))!=NULL)
   {
      for (i=0; i<MAX_HISTORY_CNT; i++)
      {
         int idx=(cmd_history_idx+i)%MAX_HISTORY_CNT;
         if (cmd_history_text[idx])
         {
            fputs(cmd_history_text[idx],fp);
            fputs("\n",fp);
         }
         else
            fputs("NULL\n",fp);
      }
      fclose(fp);
   }
   else
      Warning(("Can't save out command history file\n"));
}

// read in a history text file
static void _history_read_file(char *fname, BOOL clean)
{
   FILE *fp;   // well, files are evil, but oh well
   char buf[256];
   int i=0, idx, max_i=-1;

   if ((fp=fopen(fname,"r"))!=NULL)
   {
      while ((!feof(fp))&&(i<MAX_HISTORY_CNT))
      {
         if (clean) idx=i;
         else       idx=(cmd_history_idx+i)%MAX_HISTORY_CNT;
         if (fgets(buf,256,fp))
         {
            if (strlen(buf)>0)
               if (buf[strlen(buf)-1]==0x0a) buf[strlen(buf)-1]='\0';
            if (cmd_history_text[idx]!=NULL)
               Free(cmd_history_text[idx]);
            if ((strncmp(buf,"NULL",4)==0)||(buf[0]=='\0'))
               cmd_history_text[idx]=NULL;
            else
            {
               cmd_history_text[idx]=(char *)Malloc(strlen(buf)+2);
               strcpy(cmd_history_text[idx],buf);
               if (i>max_i) max_i=i;
            }
            i++;
         }
      }
      fclose(fp);
   }   
   if (clean) cmd_history_idx=max_i+1;
   else       cmd_history_idx=(cmd_history_idx+max_i+1)%MAX_HISTORY_CNT;
}

///////////////////
// actually add this command to the history ring
static void history_add(char *txt)
{
   int history_idx,previous_idx;
   char *ptr;

   // set this regardless, for now, unless we blow it off
   config_set_string("last_command",txt);

   // first, are we the same as either the history val
#define NO_MULTI_HISTORY
#ifdef NO_MULTI_HISTORY
   history_idx=_history_get_cur_idx(); // history command this may have come from
   if (history_string(history_idx))
      if (stricmp(history_string(history_idx),txt)==0)  // same
         return;
#endif
   previous_idx=(cmd_history_idx+MAX_HISTORY_CNT-1)%MAX_HISTORY_CNT;
   if (previous_idx!=history_idx)      // previous command
      if (history_string(previous_idx))
         if (stricmp(history_string(previous_idx),txt)==0)  // same
            return;
   ptr=(char *)Malloc(strlen(txt)+2);
   if (cmd_history_text[cmd_history_idx])
      Free(cmd_history_text[cmd_history_idx]);
   strcpy(ptr,txt);
   cmd_history_text[cmd_history_idx]=ptr;
   cmd_history_idx=(cmd_history_idx+1)%MAX_HISTORY_CNT;
}

////////////////////
// initialization/cleanup

// start up history system
void history_start(void)
{  // look for a history.cfg file
   _hist_clear();
   _history_read_file(HISTORY_FILE,TRUE);
   _history_read_file(PREHISTORY_FILE,FALSE);
}

// free all history memory
void history_free_all(void)
{
   int i;
   _history_write_file(HISTORY_FILE);
   for (i=0; i<MAX_HISTORY_CNT; i++)
      if (cmd_history_text[i])
      {
         Free(cmd_history_text[i]);
         cmd_history_text[i]=NULL;
      }
   _hist_clear();
}

/////////////////////
// commands/setup

static void history_new_cmdterm(void)
{
   cmd_history_last_found=HISTORY_NO_LAST;
   cmd_try_command=FALSE;
}

///////////////////
// used by the actual client

// find in the history ring
static char *history_find(char *base)
{
   int i;
   
   for (i=1; i<MAX_HISTORY_CNT; i++)
   {  // relativize it into right count space
      int idx=_history_get_idx(-i);
      if (idx==cmd_history_idx)
      {
         cmd_history_last_found=HISTORY_NO_LAST;  // ok, no history context now
         break;                                   // wrapped around, so switch to commands
      }
      if (cmd_history_text[idx])
         if (strnicmp(cmd_history_text[idx],base,strlen(base))==0)
         {
            _history_set_idx(idx);
            return cmd_history_text[idx];
         }
   }
   return NULL;
}

static char *history_get(int offset)
{
   int idx, base=_history_get_idx(0);

   do {
      idx=_history_get_idx(offset);
      _history_set_idx(idx);
   } while ((idx!=base)&&(cmd_history_text[idx]==NULL));
   return cmd_history_text[idx];
}

#ifdef DBG_ON
static void history_show(void)
{
   int i;
   for (i=0; i<MAX_HISTORY_CNT; i++)
      if (cmd_history_text[i])
         mprintf("%d> %s\n",i,cmd_history_text[i]);
}
#endif

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

static char last_prefix[sizeof(CmdTerm.cmdbuf)]=" ";

#pragma off(unreferenced)
bool cmdterm_textbox_cb(LGadTextBox* box, LGadTextBoxEvent event, int evdata, void* data)
{
   char  help_buf[256], *help_ptr=NULL;
   char* text = LGadTextBoxText(box);
   BOOL update = FALSE, clear = FALSE, new_base=FALSE;
   int cursor_pos;
   
   if (event == TEXTBOX_SPECKEY)
   {
      char *tmp=NULL, *ret;
      unsigned long context;

      switch(evdata)
      {
      case KB_FLAG_DOWN|KEY_ENTER:
         Status("");  // dont want to do this as part of clear since the 

         // Unpause before command executes, so the command is free to change state
         if (CmdTerm.statebits & kStatePaused)
         {
            SimStateUnpause();
            CmdTerm.statebits &= ~kStatePaused;
         }

         IInputBinder_GetContext (g_pInputBinder, &context);
         if (context == HK_COMMAND_MODE && HotkeyContext == HK_GAME_MODE) {
            IInputBinder_SetContext (g_pInputBinder, HK_GAME_MODE, TRUE);
         }
         if ((ret = IInputBinder_ProcessCmd (g_pInputBinder, text)) != NULL)  // command might do status' itself
            Status(ret);
         else
            history_add(text);       // only add commands which succeed
         *text = '\0';
         update = TRUE;
         clear = TRUE;
         break;
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'g':
      case KB_FLAG_DOWN|KEY_ESC:
         IInputBinder_GetContext (g_pInputBinder, &context);
         if (context == HK_COMMAND_MODE && HotkeyContext == HK_GAME_MODE) {
            IInputBinder_SetContext (g_pInputBinder, HK_GAME_MODE,TRUE);
         }
         if (text&&((*text)!='\0'))
            Status("");
         *text = '\0';
         update = TRUE;
         clear = TRUE;
         break;
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'p':
         tmp=history_get(-1);
         cursor_pos=move_cursor_into_command(tmp,sizeof(CmdTerm.cmdbuf));
         LGadTextBoxSetCursor(box,cursor_pos);   // cp was strlen(tmp)
         break;
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'n':
         tmp=history_get(+1);
         cursor_pos=move_cursor_into_command(tmp,sizeof(CmdTerm.cmdbuf));
         LGadTextBoxSetCursor(box,cursor_pos);
         break;
      case KB_FLAG_DOWN|KEY_TAB:
         text[LGadTextBoxCursor(box)] = '\0';
         if (strnicmp(last_prefix,text,strlen(last_prefix)))
         {  // if last string start not the same, the reset 
            new_base=TRUE;
            cmd_history_last_found=HISTORY_NO_LAST;
            strcpy(last_prefix,text);
         }
         if (!cmd_try_command)
         {
            if ((tmp=history_find(text))==NULL)
            {
               cmd_try_command=TRUE;
               tmp=command_find(text,TRUE);
            }
         }
         else
         {
            if ((tmp=command_find(text,new_base))==NULL)
            {
               cmd_try_command=FALSE;
               tmp=history_find(text);
            }
         }
         if (tmp==NULL)
            Status("No Completion");
         else if (cmd_try_command)
            if (cur_command_help_str(help_buf))
               Status(help_buf);
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
         if (CmdTerm.flags & kCmdTermHideUnfocused)
            region_set_invisible(LGadBoxRegion(&CmdTerm.root),TRUE);
         if (CmdTerm.statebits & kStatePaused)
         {
            SimStateUnpause();
            CmdTerm.statebits &= ~kStatePaused;
         }
      }
      else if (!(CmdTerm.statebits & kStatePaused))
      {
         SimStatePause();
         CmdTerm.statebits |= kStatePaused;
      }
   }
   return update;
}
#pragma on(unreferenced)

void CreateCommandTerminal(LGadRoot* root, Rect* bounds, ulong flags)
{
   if (CmdTerm.statebits & kStateInUse)
      DestroyCommandTerminal();
   memset(&CmdTerm,0,sizeof(CmdTerm));
   LGadSetupSubRoot(&CmdTerm.root,root,bounds->ul.x,bounds->ul.y,(short)RectWidth(bounds),(short)RectHeight(bounds));

   // make a text gadg on the bottom row.
   {   
      LGadTextBoxDesc tdesc;

      memset(&tdesc,0,sizeof(tdesc));
      tdesc.bounds.lr = MakePoint(RectWidth(bounds),RectHeight(bounds));
      tdesc.bounds.ul = MakePoint(0,0);
      
      tdesc.editbuf = CmdTerm.cmdbuf;
      tdesc.editbuflen = sizeof(CmdTerm.cmdbuf);
      CmdTerm.cmdbuf[0] = '\0';
      tdesc.flags = TEXTBOX_BORDER_FLAG|TEXTBOX_FOCUS_FLAG;
      tdesc.cb = cmdterm_textbox_cb;

      LGadCreateTextBoxDesc(&CmdTerm.textbox,&CmdTerm.root,&tdesc);
      LGadTextBoxSetSpecialKeys(&CmdTerm.textbox,speckeys);
      LGadBoxSetFlags(&CmdTerm.textbox,
         LGadBoxFlags(&CmdTerm.textbox)|BOXFLAG_FLIP);
   }
   CmdTerm.statebits = kStateInUse;
   CmdTerm.flags = flags;
   if (flags & kCmdTermBeginFocused)
   {
      cmdterm_focus("");
   }
   else if (flags & kCmdTermHideUnfocused)
   {
      region_set_invisible(LGadBoxRegion(&CmdTerm.root),TRUE);
   }
   LGadDrawBox(VB(&CmdTerm.root),NULL);
   cmdterm_setup_cmds();
}

void DestroyCommandTerminal(void)
{
   CmdTerm.statebits &= ~kStateInUse;
   LGadDestroyTextBox(&CmdTerm.textbox);
   LGadDestroyRoot(&CmdTerm.root);
   
}

void cmdterm_focus(char* prefix)
{
   char* buf = &CmdTerm.cmdbuf[0];
   int bufsiz = sizeof(CmdTerm.cmdbuf);
   int cursor_pos;
   unsigned long context;

   if (!(CmdTerm.statebits & kStateInUse))
      return;

   if (prefix)
      strncpy(buf,prefix,bufsiz);
   else
      strcpy(buf,"");
   buf[bufsiz-1] = '\0';
   // make the region visible, if necessary
   region_set_invisible(LGadBoxRegion(&CmdTerm.root),FALSE);
   cursor_pos=move_cursor_into_command(buf,bufsiz);

   LGadTextBoxSetCursor(&CmdTerm.textbox,cursor_pos);
   LGadUpdateTextBox(&CmdTerm.textbox);
   LGadFocusTextBox(&CmdTerm.textbox);
   LGadTextBoxClrFlag(&CmdTerm.textbox,TEXTBOX_EDIT_BRANDNEW);
   LGadDrawBox(VB(&CmdTerm.textbox),NULL);
   // pause the sim state
   SimStatePause();
   

   IInputBinder_GetContext (g_pInputBinder, &context);
   if (context == HK_GAME_MODE)
      IInputBinder_SetContext (g_pInputBinder, HK_COMMAND_MODE, TRUE);
   
   CmdTerm.statebits |= kStatePaused;
}

static void new_cmdterm(char *prefix)
{
   history_new_cmdterm();
   cmdterm_focus(prefix);
}

static void history_cmdterm(int offset)
{
   int idx;
   history_new_cmdterm();
   idx=_history_get_idx(offset);
   if (history_string(idx))
   {
      _history_set_idx(idx);
      cmdterm_focus(history_string(idx));
   }
   else
      cmdterm_focus("");
}

static Command commands[] = 
{
   { "edit_command", FUNC_STRING, new_cmdterm, "edit a command in the command editor: edit_command <cmd>",HK_ALL},
   { "history_cmd", FUNC_INT, history_cmdterm, "edit command from history offset" },
#ifdef DBG_ON   
   { "history_dump", FUNC_VOID, history_show },
#endif   
};

void cmdterm_setup_cmds(void)
{
   static bool setup = FALSE;
   if (!setup)
   {
     COMMANDS(commands,HK_ALL);
     setup = TRUE;
   }
}
