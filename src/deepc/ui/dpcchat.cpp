/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deal with multiplayer chat
//
// This file serves as the primary chat interface between Deep Cover and the
// Net Manager. It deals with both sides of the equation: accepting user
// chat text which is then sent to the other players; and displaying
// chat messages from those other players.
//
// This is structured in a Cish style, because that seems to be the norm.
// One could argue that this whole thing really should just be an object...
//
// This module also deals with getting a text message to put into the
// monolog for all players; that's bundled in here mainly because it's
// more or less identical code...
//

#include <netman.h>
#include <netmsg.h>
#include <mprintf.h>
#include <lg.h>
#include <appagg.h>

#include <headmove.h>
#include <kbcook.h>
#include <keydefs.h>

// Is this still needed?
extern "C" {
#include <gadbase.h>
#include <gadbox.h>
#include <gadtext.h>
}

#include <playrobj.h>

#include <gen_bind.h>

#include <questapi.h>

#include <command.h>
#include <dpcovrly.h>
#include <dpciface.h>
#include <dpcutils.h>
#include <dpccntxt.h>
#include <dpcchat.h>
#include <dpcgame.h>
#include <dpcnet.h>

// Must be last:
#include <dbmem.h>

// The time that a chat message will show before timing out, assuming
// it doesn't scroll off:
#define CHAT_TIMEOUT 10000

//////////
//
// NETWORK CODE
//

static void handleSpewMsg(const char *pText)
{
   mprintf("NET_SPEW: %s\n", pText);
}

static cNetMsg *g_pSpewMsg = NULL;

static sNetMsgDesc sSpewDesc =
{
   kNMF_Broadcast,
   "Spew",
   "Monolog Spew",
   "net_spew_spew",
   handleSpewMsg,
   {{kNMPT_String, kNMPF_None, "Message"},
    {kNMPT_End}}
};

//////////

static void ShowChatMsg(const char *pText, int fromPlayer)
{
   // In case we're in earth/station, make sure that we can display text.
   DPCOverlayChange(kOverlayText, kOverlayModeOn);

   AutoAppIPtr(NetManager);
   const char *playerName;
   char temp[255];
   // Note that fromPlayer is a player *number*, not ObjID:
   if (fromPlayer == pNetManager->MyPlayerNum()) {
      if (!DPCStringFetch(temp, sizeof(temp), "YouSay", "network"))
      {
         temp[0] = '\0';
      }
      playerName = temp;
   } else {
      playerName = pNetManager->GetPlayerNameByNum(fromPlayer);
   }
   char *msgBuf = (char *) malloc(strlen(pText) + MAX_PLAYER_NAME_LEN + 3);
   sprintf(msgBuf, "%s: %s", playerName, pText);
   //int color[3] = {0, 255, 0};
   DPCOverlayAddColoredText(msgBuf,
                              CHAT_TIMEOUT, 
                              DPCPlayerNumColor(fromPlayer));
   free(msgBuf);
}

// The network message descriptor
static sNetMsgDesc DPCChatDesc =
{
   kNMF_MetagameBroadcast | kNMF_AlwaysBroadcast,
   "DPCChat",
   "DPC Chat",
   NULL,
   ShowChatMsg,
   // One parameter, a string:
   {{kNMPT_String, kNMPF_None, "Message"},
    {kNMPT_Int, kNMPF_None, "Sender Num"},
    {kNMPT_End}},
};

// The network message object itself:
static cNetMsg *g_pChatMsg = NULL;

static void DPCChatSend(const char *text)
{
   AutoAppIPtr(NetManager);
   if (!pNetManager->IsNetworkGame())
      // Don't let players talk to themselves
      return;

   g_pChatMsg->Send(OBJ_NULL, text, pNetManager->MyPlayerNum());
   ShowChatMsg(text, pNetManager->MyPlayerNum());
}

//////////
//
// Local variables
//

#define MAX_CHAT_MSG_LEN 255

// Are we currently in the middle of accepting chat input?
BOOL g_bGettingChat;
// What was the hotkey context before we went into chat?
unsigned long g_OldContext;
// Are we actually getting spewage to put into the monolog?
BOOL g_bGettingSpew;

// The full description of the state of the chat message currently
// being composed. (Inspired by the CmdTerm.)
struct _ChatState {
   LGadRoot root;
   LGadTextBox textbox;
   char msgbuf[MAX_CHAT_MSG_LEN];
} g_ChatState;

// The keys that are considered "special" for the chat input box, and which
// will cause the callback to be invoked.
static short g_SpecKeys[] = 
{
   KB_FLAG_DOWN|KEY_ENTER,
   KB_FLAG_DOWN|KEY_ESC,
   KB_FLAG_DOWN|KB_FLAG_CTRL|'g',
   0
};

//
// Callback for the chat input box.
// This will be called whenever a "special" character is entered into the
// box. The special characters are currently Return (send the message),
// Esc (shut down the box without sending), and CTL-g (same as Esc for
// Emacs geeks).
// The code is liberally adapted from cmdterm.
//
#pragma off(unreferenced)
bool chatTextboxCallback(LGadTextBox* box, 
                         LGadTextBoxEvent event, 
                         int evdata, 
                         void* data)
{
   // The actual text currently in the chat input box:
   char* text = LGadTextBoxText(box);
   // Set to TRUE iff we change the text for the input box, and need to
   // tell it to update itself:
   BOOL update = FALSE;
   // Set to TRUE iff we should hide the input box for the time being:
   BOOL clear = FALSE;
   
   if (event == TEXTBOX_SPECKEY)
   {
      switch(evdata)
      {
      case KB_FLAG_DOWN|KEY_ENTER:
      {
#ifdef PLAYTEST
         if (g_bGettingSpew)
         {
            g_pSpewMsg->Send(OBJ_NULL, text);
            handleSpewMsg(text);
         }
         else
         {
            DPCChatSend(text);
         }
#else
         DPCChatSend(text);
#endif
         *text = '\0';
         update = TRUE;
         clear = TRUE;
         break;
      }
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'g':
      case KB_FLAG_DOWN|KEY_ESC:
         // Leave the text as it is; they can try again later:
         clear = TRUE;
         break;
      }
   }
   if (update)
   {
      LGadUpdateTextBox(box);
   }
   if (clear)
   {
      LGadUnfocusTextBox(box);
      region_set_invisible(LGadBoxRegion(&g_ChatState.root),TRUE);
      g_pInputBinder->SetContext(g_OldContext, TRUE);
      if (DPC_mouse) {
         g_pInputBinder->SetValidEvents (ALL_EVENTS & (~UI_EVENT_MOUSE_MOVE)); //  & (~UI_EVENT_MOUSE));
      } else {
         g_pInputBinder->SetValidEvents (ALL_EVENTS); //  & (~UI_EVENT_MOUSE));
      }
      g_bGettingChat = FALSE;
#ifdef PLAYTEST
      g_bGettingSpew = FALSE;
#endif
      BOOL slim_mode;
      AutoAppIPtr(QuestData);
      slim_mode = pQuestData->Get("HideInterface");
      if (slim_mode)
      {
         // Okay, we're in earth/station. Pop the miniframe down again.
         DPCOverlayChange(kOverlayMiniFrame, kOverlayModeOff);
      }
   }
   return update;
}
#pragma on(unreferenced)

//
// Do init-time setup of the chat input box.
//
void initChatInputBox(int x, int y, int width, int height)
{
   // Create the chat input textbox gadget
   memset(&g_ChatState,0,sizeof(g_ChatState));
   LGadRoot *pRoot = LGadCurrentRoot();
   LGadSetupSubRoot(&g_ChatState.root, pRoot, x, y, width, height);

   LGadTextBoxDesc tdesc;
   memset(&tdesc,0,sizeof(tdesc));
   tdesc.bounds.lr = MakePoint(width,height);
   tdesc.bounds.ul = MakePoint(x, y);
   tdesc.editbuf = g_ChatState.msgbuf;
   tdesc.editbuflen = MAX_CHAT_MSG_LEN;
   g_ChatState.msgbuf[0] = '\0';
   tdesc.flags = TEXTBOX_FOCUS_FLAG;
   tdesc.cb = chatTextboxCallback;

   LGadCreateTextBoxDesc(&g_ChatState.textbox, &g_ChatState.root, &tdesc);
   LGadTextBoxSetSpecialKeys(&g_ChatState.textbox, g_SpecKeys);
}

//
// Does what is necessary to draw the chat input area. Should be called
// each frame.
//
void DPCChatDrawInput()
{
   // Note that this line determines the color of the chat input text:
   static int chatColor[3] = {0, 255, 255};
   int *color;
#ifdef PLAYTEST
   static int spewColor[3] = {255, 0, 0};
   if (g_bGettingSpew)
      color = spewColor;
   else
      color = chatColor;
#else
   color = chatColor;
#endif

   DPCInterfaceDrawString(g_ChatState.msgbuf, 
                            FindColor(color),
                            LGadTextBoxCursor(&g_ChatState.textbox));
   // LGadDrawBox(VB(&g_ChatState.root), NULL);
}

//
// Begin getting chat input from the player.
// This code was liberally adapted from cmdterm_focus.
// Currently ignores the prefix; this may want to change.
//
void DPCChatGetInput(char *prefix)
{
   if (g_bGettingChat) {
      // We're already there
      return;
   }

   AutoAppIPtr(NetManager);
   if (!pNetManager->IsNetworkGame())
      // Don't let players talk to themselves; however, do allow this
      // command to run even when not apparently networking, like in
      // earth/station.
      return;

   g_bGettingChat = TRUE;

   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");
   if (slim_mode)
   {
      // Okay, we're in earth/station. That means we need to pop up the
      // miniframe, so the player has something to type into.
      DPCOverlayChange(kOverlayMiniFrame, kOverlayModeOn);
   }

   // Make it visible:
   region_set_invisible(LGadBoxRegion(&g_ChatState.root),FALSE);

   // Set the cursor to the end of the string:
   LGadTextBoxSetCursor(&g_ChatState.textbox,strlen(g_ChatState.msgbuf));
   // Make sure it is using the current text:
   LGadUpdateTextBox(&g_ChatState.textbox);
   // Grab the cursor:
   LGadFocusTextBox(&g_ChatState.textbox);
   // By default, the textbox gadget wipes any initial text;
   // we don't like that:
   LGadTextBoxClrFlag(&g_ChatState.textbox, TEXTBOX_EDIT_BRANDNEW);
   // We actually draw the box for the first time on the next frame;
   // don't do so now, because it screws up the graphics locks.

   // Now change the input bindings to chat mode:
   unsigned long context;
   g_pInputBinder->GetContext (&context);
   if (context != HK_CHAT_MODE) {
      g_pInputBinder->SetContext (HK_CHAT_MODE, TRUE);

      g_OldContext = context;

      //dont let input binding sys process mouse move events
      headmoveSetRelPosX(0);
      headmoveSetRelPosY(0);
      headmoveTouch();
      g_pInputBinder->SetValidEvents (ALL_EVENTS & (~UI_EVENT_MOUSE_MOVE)); //  & (~UI_EVENT_MOUSE));
   }
}

#ifdef PLAYTEST
//
// Get a message to put into the monospew.
//
void DPCChatGetSpew(char *prefix)
{
   g_bGettingSpew = TRUE;
   DPCChatGetInput(prefix);
}
#endif

//
// Returns TRUE iff we are in the middle of accepting a chat line from
// the player.
//
BOOL DPCChatGettingInput()
{
   return g_bGettingChat;
}

//////////
//
// Command interface, to go into chat mode
//

static Command commands[] = 
{
   { "chat",         FUNC_STRING, DPCChatGetInput,  "Get a chat message from the player",    HK_GAME_MODE},
   { "chatsend",     FUNC_STRING, DPCChatSend,      "Send a specific chat message",          HK_GAME_MODE},
#ifdef PLAYTEST
   { "dpc_net_spew", FUNC_STRING, DPCChatGetSpew,   "Get a message to put into the monolog", HK_GAME_MODE},
#endif
};

void chatSetupCmds(void)
{
   static bool setup = FALSE;
   if (!setup)
   {
     COMMANDS(commands,HK_GAME_MODE);
     setup = TRUE;
   }
}

//////////
//
// Initialization and termination
//

//
// Prepare for using chat. The dimensions given should indicate where the
// chat input box goes on the screen.
//
void DPCChatInit(int x, int y, int width, int height)
{
   // Initialize the local variables
   g_bGettingChat = FALSE;
#ifdef PLAYTEST
   g_bGettingSpew = FALSE;
#endif

#ifdef NEW_NETWORK_ENABLED
   initChatInputBox(x, y, width, height);

   chatSetupCmds();
#endif
}

void DPCChatTerm(void)
{
#ifdef NEW_NETWORK_ENABLED
   LGadDestroyTextBox(&g_ChatState.textbox);
   LGadDestroyRoot(&g_ChatState.root);
#endif
}

void DPCChatNetInit()
{
   // Create the message itself
   g_pChatMsg = new cNetMsg(&DPCChatDesc, NULL);
   g_pSpewMsg = new cNetMsg(&sSpewDesc, NULL);
}

void DPCChatNetTerm()
{
   delete g_pChatMsg;
   delete g_pSpewMsg;
}
