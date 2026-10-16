/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/cmdterm.c,v 1.28 2000/02/19 13:28:19 toml Exp $

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <kb.h>
#include <kbcook.h>
#include <keydefs.h>
#include <2d.h>
#include <event.h>
#include <gadbox.h>
#include <gadtext.h>
#include <gcompose.h>
#include <guistyle.h>
#include <hotkey.h>
#include <appagg.h>
#include <gshelapi.h>
#include <resapi.h>
#include <fonrstyp.h>

#include <cmdterm.h>
#include <command.h>
#include <gen_bind.h>
#include <scrnman.h>
#include <status.h>
#include <simstate.h>

#include <config.h>
#include <platform_services.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


enum StateBits
{
   kStatePaused = 1 << 0,
   kStateInUse  = 1 << 1,
   kStateVisible = 1 << 2,
};


struct _CmdTerm
{
   LGadRoot root;
   LGadTextBox titlebox;
   LGadTextBox outputbox;
   LGadTextBox textbox;
   LGadBox scrollbar;
   char titlebuf[128];
   char outputbuf[32768];
   char cmdbuf[256];
   int visiblelines;
   int scroll;
   int scrollbarwidth;
   int scrolldragoffset;
   Region* mousehandlerregions[4];
   int mousehandlers[4];
   int mousehandlercount;
   BOOL scrolldragging;
   guiStyle style;
   IRes* fontres;
   LGadRoot* parentroot;
   Rect bounds;
   grs_canvas* previousguicanvas;
   BOOL canvasredirected;
   int previousgameshellflags;
   BOOL popupcursoractive;
   ulong flags;
   ulong statebits;
} CmdTerm;

#define CMDTERM_MAX_LINES 4096
#define CMDTERM_LINE_LENGTH 256

static char CmdTermLines[CMDTERM_MAX_LINES][CMDTERM_LINE_LENGTH];
static int CmdTermLineCount = 0;
static int CmdTermNextLine = 0;
static char CmdTermCaptureLine[CMDTERM_LINE_LENGTH];
static int CmdTermCaptureLength = 0;
static volatile long CmdTermCommandCaptureDepth = 0;
static volatile long CmdTermDiagnosticCapture = FALSE;
static BOOL CmdTermOutputHookInstalled = FALSE;
static void (*CmdTermPreviousMonoHook)(char*,int) = NULL;
static sPlatformMutex CmdTermModelLock;
static BOOL CmdTermModelLockInitialized = FALSE;
static volatile long CmdTermPendingFontDelta = 0;
static volatile long CmdTermFontChangeDeferred = FALSE;

// Use the stock antialiased interface faces now that their 16-bit blitter can
// blend coverage without depending on renderer-owned lighting-table state.
// Ctrl+wheel retains smaller bitmap faces and offers larger high-DPI faces.
static const char* CmdTermEditorFontNames[] =
{
   "smalfont", "textfont", "fontaa20", "fontaa29", "fontaa36"
};
static const char* CmdTermPopupFontNames[] =
{
   "smalfont", "textfont", "fontaa20", "fontaa29", "fontaa36"
};
static int CmdTermFontIndices[2] = { 0, 2 };

#define CMDTERM_EDITOR_FONT_COUNT \
   (sizeof(CmdTermEditorFontNames) / sizeof(CmdTermEditorFontNames[0]))
#define CMDTERM_POPUP_FONT_COUNT \
   (sizeof(CmdTermPopupFontNames) / sizeof(CmdTermPopupFontNames[0]))

static void cmdterm_activate_focus(BOOL position_cursor);
static BOOL cmdterm_apply_pending_font_change(void);
static void cmdterm_deferred_font_change(void* unused);

static void cmdterm_set_popup_cursor(BOOL active)
{
   IGameShell* shell;
   int flags;

   if (!(CmdTerm.flags & kCmdTermHideUnfocused) ||
       active == CmdTerm.popupcursoractive)
      return;

   shell = AppGetObj(IGameShell);
   if (!shell)
      return;

   if (active)
   {
      IGameShell_GetFlags(shell, &flags);
      CmdTerm.previousgameshellflags = flags;
      CmdTerm.popupcursoractive = TRUE;
      flags |= kShowNativeCursor;
   }
   else
   {
      flags = CmdTerm.previousgameshellflags;
      CmdTerm.popupcursoractive = FALSE;
   }
   IGameShell_SetFlags(shell, flags);
   SafeRelease(shell);
}

static void cmdterm_model_init(void)
{
   if (!CmdTermModelLockInitialized)
   {
      CmdTermModelLockInitialized = PlatformMutexInit(&CmdTermModelLock);
   }
}

static void cmdterm_model_lock(void)
{
   cmdterm_model_init();
   PlatformMutexLock(&CmdTermModelLock);
}

static void cmdterm_model_unlock(void)
{
   PlatformMutexUnlock(&CmdTermModelLock);
}

static BOOL cmdterm_is_scrollable(void)
{
   return (CmdTerm.flags & kCmdTermScrollable) != 0;
}

static int cmdterm_max_scroll(void)
{
   int max_scroll;

   cmdterm_model_lock();
   max_scroll = CmdTermLineCount - CmdTerm.visiblelines;
   cmdterm_model_unlock();
   return max_scroll > 0 ? max_scroll : 0;
}

static void cmdterm_set_scroll(int scroll)
{
   int max_scroll;

   cmdterm_model_lock();
   max_scroll = CmdTermLineCount - CmdTerm.visiblelines;
   if (max_scroll < 0)
      max_scroll = 0;

   if (scroll < 0)
      scroll = 0;
   if (scroll > max_scroll)
      scroll = max_scroll;
   CmdTerm.scroll = scroll;
   cmdterm_model_unlock();
}

static void cmdterm_scroll_by(int lines)
{
   cmdterm_set_scroll(CmdTerm.scroll + lines);
}

static void cmdterm_scrollbar_geometry(int height, int* thumb_top,
                                       int* thumb_height)
{
   int max_scroll = cmdterm_max_scroll();
   int height_inside = height - 2;
   int size;
   int travel;

   if (height_inside < 1)
      height_inside = 1;

   if (CmdTermLineCount <= 0 || CmdTermLineCount <= CmdTerm.visiblelines)
      size = height_inside;
   else
      size = (int)(((long)height_inside * CmdTerm.visiblelines) /
                   CmdTermLineCount);

   if (size < 8)
      size = 8;
   if (size > height_inside)
      size = height_inside;

   travel = height_inside - size;
   *thumb_height = size;
   if (max_scroll > 0 && travel > 0)
      *thumb_top = 1 +
         (int)(((long)travel * (max_scroll - CmdTerm.scroll)) / max_scroll);
   else
      *thumb_top = 1;
}

static void cmdterm_restore_canvas(void)
{
   if (CmdTerm.canvasredirected)
   {
      DefaultGUIcanvas = CmdTerm.previousguicanvas;
      CmdTerm.previousguicanvas = NULL;
      CmdTerm.canvasredirected = FALSE;
   }
}

static void cmdterm_rebuild_output(void)
{
   int oldest;
   int max_scroll;
   int first;
   int shown;
   int i;
   int used = 0;

   cmdterm_model_lock();
   CmdTerm.outputbuf[0] = '\0';
   if (CmdTerm.visiblelines <= 0 || CmdTermLineCount <= 0)
   {
      cmdterm_model_unlock();
      return;
   }

   max_scroll = CmdTermLineCount - CmdTerm.visiblelines;
   if (max_scroll < 0)
      max_scroll = 0;
   if (CmdTerm.scroll > max_scroll)
      CmdTerm.scroll = max_scroll;
   if (CmdTerm.scroll < 0)
      CmdTerm.scroll = 0;

   shown = CmdTermLineCount < CmdTerm.visiblelines
         ? CmdTermLineCount : CmdTerm.visiblelines;
   first = CmdTermLineCount - shown - CmdTerm.scroll;
   if (first < 0)
      first = 0;
   oldest = (CmdTermNextLine - CmdTermLineCount + CMDTERM_MAX_LINES)
          % CMDTERM_MAX_LINES;

   for (i = 0; i < shown && first + i < CmdTermLineCount; ++i)
   {
      const char* line = CmdTermLines[(oldest + first + i)
                                      % CMDTERM_MAX_LINES];
      int length = (int)strlen(line);
      if (used + length + 2 >= (int)sizeof(CmdTerm.outputbuf))
         break;
      if (used)
         CmdTerm.outputbuf[used++] = '\n';
      memcpy(CmdTerm.outputbuf + used, line, length);
      used += length;
      CmdTerm.outputbuf[used] = '\0';
   }
   cmdterm_model_unlock();
}

static void cmdterm_add_line_locked(const char* text, int length)
{
   int max_scroll;

   if (length >= CMDTERM_LINE_LENGTH)
      length = CMDTERM_LINE_LENGTH - 1;
   memcpy(CmdTermLines[CmdTermNextLine], text, length);
   CmdTermLines[CmdTermNextLine][length] = '\0';
   CmdTermNextLine = (CmdTermNextLine + 1) % CMDTERM_MAX_LINES;
   if (CmdTermLineCount < CMDTERM_MAX_LINES)
      ++CmdTermLineCount;

   // Keep the same historical lines on screen while the user is browsing.
   // When already following the newest output, continue to follow it.
   if (CmdTerm.scroll > 0)
      ++CmdTerm.scroll;
   max_scroll = CmdTermLineCount - CmdTerm.visiblelines;
   if (max_scroll < 0)
      max_scroll = 0;
   if (CmdTerm.scroll > max_scroll)
      CmdTerm.scroll = max_scroll;
}

void cmdterm_print(const char* text)
{
   const char* start;

   if (!text)
      return;

   cmdterm_model_lock();
   start = text;
   while (*start)
   {
      const char* end = start;
      while (*end && *end != '\r' && *end != '\n')
         ++end;

      if (end == start)
         cmdterm_add_line_locked("", 0);
      else
      {
         while (end - start >= CMDTERM_LINE_LENGTH)
         {
            cmdterm_add_line_locked(start, CMDTERM_LINE_LENGTH - 1);
            start += CMDTERM_LINE_LENGTH - 1;
         }
         cmdterm_add_line_locked(start, (int)(end - start));
      }

      if (!*end)
         break;
      start = end + 1;
      if (*end == '\r' && *start == '\n')
         ++start;
   }
   cmdterm_model_unlock();
}

static void cmdterm_capture_flush_locked(void)
{
   if (CmdTermCaptureLength > 0)
   {
      cmdterm_add_line_locked(CmdTermCaptureLine, CmdTermCaptureLength);
      CmdTermCaptureLength = 0;
   }
}

static void cmdterm_capture_stream(const char* text, int length)
{
   int i;

   if (!text || length <= 0)
      return;

   cmdterm_model_lock();
   for (i = 0; i < length; ++i)
   {
      char c = text[i];
      if (c == '\r')
         continue;
      if (c == '\n')
      {
         cmdterm_capture_flush_locked();
         if (CmdTermCaptureLength == 0 && i > 0 && text[i - 1] == '\n')
            cmdterm_add_line_locked("", 0);
         continue;
      }
      if (CmdTermCaptureLength == CMDTERM_LINE_LENGTH - 1)
         cmdterm_capture_flush_locked();
      CmdTermCaptureLine[CmdTermCaptureLength++] = c;
   }
   cmdterm_model_unlock();
}

static void cmdterm_mono_output(char* text, int length)
{
   if (CmdTermPreviousMonoHook)
      CmdTermPreviousMonoHook(text, length);
   if (PlatformAtomicLoad(&CmdTermCommandCaptureDepth) > 0 ||
       PlatformAtomicLoad(&CmdTermDiagnosticCapture))
      cmdterm_capture_stream(text, length);
}

static void cmdterm_install_output_hook(void)
{
   cmdterm_model_init();
   if (!CmdTermOutputHookInstalled)
   {
      CmdTermPreviousMonoHook = mono_spc_func;
      mono_spc_func = cmdterm_mono_output;
      CmdTermOutputHookInstalled = TRUE;
   }
}

static void cmdterm_remove_output_hook(void)
{
   if (CmdTermOutputHookInstalled)
   {
      if (mono_spc_func == cmdterm_mono_output)
         mono_spc_func = CmdTermPreviousMonoHook;
      CmdTermPreviousMonoHook = NULL;
      CmdTermOutputHookInstalled = FALSE;
   }
}

static void cmdterm_begin_command_capture(void)
{
   PlatformAtomicIncrement(&CmdTermCommandCaptureDepth);
   // Installing mono_spc_func makes legacy mprintf calls format their output
   // even when no monochrome screen or log is active. Limit that behavior to
   // an executing console command unless diagnostics were explicitly enabled.
   cmdterm_install_output_hook();
}

static void cmdterm_end_command_capture(void)
{
   if (PlatformAtomicLoad(&CmdTermCommandCaptureDepth) > 0)
      PlatformAtomicDecrement(&CmdTermCommandCaptureDepth);
   cmdterm_model_lock();
   cmdterm_capture_flush_locked();
   cmdterm_model_unlock();
   if (PlatformAtomicLoad(&CmdTermCommandCaptureDepth) == 0 &&
       !PlatformAtomicLoad(&CmdTermDiagnosticCapture))
      cmdterm_remove_output_hook();
}

void cmdterm_capture_status(const char* text)
{
   if (PlatformAtomicLoad(&CmdTermCommandCaptureDepth) > 0 &&
       text && *text)
      cmdterm_print(text);
}

void cmdterm_redraw(void)
{
   grs_canvas canvas;

   if (!(CmdTerm.statebits & kStateInUse))
      return;

   if (cmdterm_apply_pending_font_change())
      return;

   if (!cmdterm_is_scrollable())
   {
      LGadUpdateTextBox(&CmdTerm.textbox);
      LGadDrawBox(VB(&CmdTerm.textbox),NULL);
      return;
   }

   if (!(CmdTerm.statebits & kStateVisible))
      return;

   cmdterm_rebuild_output();
   LGadUpdateTextBox(&CmdTerm.outputbox);
   LGadUpdateTextBox(&CmdTerm.textbox);
   gr_init_sub_canvas(ScrnGetDrawCanvas(), &canvas,
                      CmdTerm.bounds.ul.x, CmdTerm.bounds.ul.y,
                      RectWidth(&CmdTerm.bounds), RectHeight(&CmdTerm.bounds));
   LGadDrawBoxCanvas(VB(&CmdTerm.root), NULL, &canvas, CmdTerm.bounds.ul);
   gr_close_sub_canvas(&canvas);
}

static void cmdterm_draw(void)
{
   cmdterm_redraw();
   ScrnForceUpdate();
}

static void cmdterm_scroll_and_draw(int lines)
{
   cmdterm_scroll_by(lines);
   cmdterm_rebuild_output();
   cmdterm_draw();
}

static BOOL cmdterm_control_down(void)
{
   return kb_state(KBC_LCTRL) == KBS_DOWN ||
          kb_state(KBC_RCTRL) == KBS_DOWN;
}

static BOOL cmdterm_handle_wheel(short wheel)
{
   if (!wheel)
      return FALSE;

   if (cmdterm_control_down())
   {
      PlatformAtomicAdd(&CmdTermPendingFontDelta, wheel);
      if (PlatformAtomicCompareExchange(&CmdTermFontChangeDeferred,
                                        TRUE, FALSE) == FALSE)
      {
         if (uiDefer(cmdterm_deferred_font_change, NULL) != OK)
            PlatformAtomicExchange(&CmdTermFontChangeDeferred, FALSE);
      }
      return TRUE;
   }

   cmdterm_scroll_and_draw(3 * wheel);
   return TRUE;
}

static void cmdterm_scrollbar_draw(void* data, LGadBox* box)
{
   int width = grd_canvas->bm.w;
   int height = grd_canvas->bm.h;
   int thumb_top;
   int thumb_height;

   (void)data;
   (void)box;

   cmdterm_scrollbar_geometry(height, &thumb_top, &thumb_height);

   gr_set_fcolor(guiStyleGetColor(&CmdTerm.style, StyleColorBG));
   gr_rect(0, 0, width - 1, height - 1);
   gr_set_fcolor(guiStyleGetColor(&CmdTerm.style, StyleColorBorder));
   gr_box(0, 0, width - 1, height - 1);

   if (width > 4 && thumb_height > 0)
   {
      gr_set_fcolor(guiStyleGetColor(&CmdTerm.style,
         CmdTerm.scrolldragging ? StyleColorHilite : StyleColorFG));
      gr_rect(2, thumb_top, width - 3, thumb_top + thumb_height - 1);
   }
}

static bool cmdterm_scrollbar_mouse(short x, short y, short action,
                                    short wheel, LGadBox* box)
{
   int thumb_top;
   int thumb_height;
   int local_y;
   int height = RectHeight(LGadBoxRect(box));

   (void)x;

   cmdterm_scrollbar_geometry(height, &thumb_top, &thumb_height);
   local_y = y - LGadBoxRegion(box)->abs_y;

   if (wheel)
      return cmdterm_handle_wheel(wheel);

   if (action & MOUSE_LDOWN)
   {
      int max_scroll = cmdterm_max_scroll();
      int travel = height - 2 - thumb_height;
      int new_top;

      if (max_scroll > 0)
      {
         CmdTerm.scrolldragging = TRUE;
         if (local_y >= thumb_top && local_y < thumb_top + thumb_height)
            CmdTerm.scrolldragoffset = local_y - thumb_top;
         else
            CmdTerm.scrolldragoffset = thumb_height / 2;

         uiGrabFocus(LGadBoxRegion(box), UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE);
         uiSetMouseMotionPolling(TRUE);

         new_top = local_y - CmdTerm.scrolldragoffset;
         if (new_top < 1)
            new_top = 1;
         if (new_top > 1 + travel)
            new_top = 1 + travel;
         if (travel > 0)
            cmdterm_set_scroll(max_scroll -
               (int)(((long)(new_top - 1) * max_scroll + travel / 2) /
                     travel));
         cmdterm_rebuild_output();
         cmdterm_draw();
      }
      return TRUE;
   }

   if ((action & MOUSE_LUP) && CmdTerm.scrolldragging)
   {
      CmdTerm.scrolldragging = FALSE;
      uiReleaseFocus(LGadBoxRegion(box), UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE);
      uiSetMouseMotionPolling(TRUE);
      LGadDrawBox(box, NULL);
      return TRUE;
   }

   return FALSE;
}

static bool cmdterm_scrollbar_motion(short x, short y, LGadBox* box)
{
   int thumb_top;
   int thumb_height;
   int travel;
   int max_scroll;
   int height = RectHeight(LGadBoxRect(box));

   (void)x;

   if (!CmdTerm.scrolldragging)
      return FALSE;

   cmdterm_scrollbar_geometry(height, &thumb_top, &thumb_height);
   travel = height - 2 - thumb_height;
   max_scroll = cmdterm_max_scroll();
   thumb_top = y - LGadBoxRegion(box)->abs_y - CmdTerm.scrolldragoffset;

   if (thumb_top < 1)
      thumb_top = 1;
   if (thumb_top > 1 + travel)
      thumb_top = 1 + travel;

   if (travel > 0 && max_scroll > 0)
      cmdterm_set_scroll(max_scroll -
         (int)(((long)(thumb_top - 1) * max_scroll + travel / 2) / travel));
   else
      cmdterm_set_scroll(0);

   cmdterm_rebuild_output();
   cmdterm_draw();
   return TRUE;
}

static BOOL cmdterm_mouse_handler(uiEvent* event, Region* region, void* state)
{
   uiMouseEvent* mouse = (uiMouseEvent*)event;

   (void)region;
   (void)state;

   if (event->type != UI_EVENT_MOUSE || !mouse->wheel ||
       !(CmdTerm.statebits & kStateVisible))
      return FALSE;

   return cmdterm_handle_wheel(mouse->wheel);
}

static void cmdterm_install_mouse_handler(Region* region)
{
   int handler;

   if (!region || CmdTerm.mousehandlercount >= 4)
      return;
   if (uiInstallRegionHandler(region, UI_EVENT_MOUSE, cmdterm_mouse_handler,
                              NULL, &handler) == OK)
   {
      CmdTerm.mousehandlerregions[CmdTerm.mousehandlercount] = region;
      CmdTerm.mousehandlers[CmdTerm.mousehandlercount] = handler;
      ++CmdTerm.mousehandlercount;
   }
}

static BOOL cmdterm_set_clipboard_text(const char* text)
{
   return PlatformSetClipboardText(text);
}

static BOOL cmdterm_copy_output(BOOL all)
{
   int oldest;
   int first;
   int shown;
   int i;
   size_t capacity;
   size_t used = 0;
   char* text;
   BOOL result;

   cmdterm_model_lock();
   shown = all ? CmdTermLineCount :
      (CmdTermLineCount < CmdTerm.visiblelines
         ? CmdTermLineCount : CmdTerm.visiblelines);
   first = all ? 0 : CmdTermLineCount - shown - CmdTerm.scroll;
   if (first < 0)
      first = 0;
   oldest = (CmdTermNextLine - CmdTermLineCount + CMDTERM_MAX_LINES)
          % CMDTERM_MAX_LINES;
   capacity = (size_t)shown * (CMDTERM_LINE_LENGTH + 2) + 1;
   text = (char*)Malloc(capacity);
   if (!text)
   {
      cmdterm_model_unlock();
      return FALSE;
   }

   for (i = 0; i < shown && first + i < CmdTermLineCount; ++i)
   {
      const char* line = CmdTermLines[(oldest + first + i)
                                      % CMDTERM_MAX_LINES];
      size_t length = strlen(line);
      if (used)
      {
         text[used++] = '\r';
         text[used++] = '\n';
      }
      memcpy(text + used, line, length);
      used += length;
   }
   text[used] = '\0';
   cmdterm_model_unlock();

   result = cmdterm_set_clipboard_text(text);
   Free(text);
   return result;
}

static BOOL cmdterm_copy_command(void)
{
   return cmdterm_set_clipboard_text(CmdTerm.cmdbuf);
}

static BOOL cmdterm_paste_command(LGadTextBox* box)
{
   char clipboard[sizeof(CmdTerm.cmdbuf)];
   const char* source;
   char insertion[sizeof(CmdTerm.cmdbuf)];
   int insertion_length = 0;
   int cursor = LGadTextBoxCursor(box);
   int current_length = (int)strlen(CmdTerm.cmdbuf);
   int available = sizeof(CmdTerm.cmdbuf) - current_length - 1;
   int i;

   if (available <= 0 ||
       !PlatformGetClipboardText(clipboard, sizeof(clipboard)))
      return FALSE;

   source = clipboard;
   while (*source && insertion_length < available)
   {
      char c = *source++;
      if (c == '\r')
         continue;
      if (c == '\n' || c == '\t')
         c = ' ';
      insertion[insertion_length++] = c;
   }
   if (!insertion_length)
      return FALSE;

   memmove(CmdTerm.cmdbuf + cursor + insertion_length,
           CmdTerm.cmdbuf + cursor, current_length - cursor + 1);
   for (i = 0; i < insertion_length; ++i)
      CmdTerm.cmdbuf[cursor + i] = insertion[i];
   LGadUpdateTextBox(box);
   LGadTextBoxSetCursor(box, cursor + insertion_length);
   LGadTextBoxClrFlag(box, TEXTBOX_EDIT_BRANDNEW);
   return TRUE;
}

static BOOL cmdterm_argument_is(const char* argument, const char* value)
{
   size_t length;

   if (!argument)
      return FALSE;
   while (*argument && isspace((unsigned char)*argument))
      ++argument;
   length = strlen(value);
   if (_strnicmp(argument, value, length))
      return FALSE;
   argument += length;
   while (*argument && isspace((unsigned char)*argument))
      ++argument;
   return *argument == '\0';
}

static void cmdterm_console_copy(char* argument)
{
   while (argument && *argument && isspace((unsigned char)*argument))
      ++argument;

   if (cmdterm_argument_is(argument, "all"))
   {
      cmdterm_print(cmdterm_copy_output(TRUE)
         ? "Copied all console output." : "Unable to copy console output.");
   }
   else if (cmdterm_argument_is(argument, "command"))
   {
      cmdterm_print(cmdterm_copy_command()
         ? "Copied the command line." : "Unable to copy the command line.");
   }
   else if (!argument || !*argument || cmdterm_argument_is(argument, "visible"))
      cmdterm_print(cmdterm_copy_output(FALSE)
         ? "Copied visible console output." : "Unable to copy console output.");
   else
      cmdterm_print("Usage: console_copy [visible/all/command]");
}

static void cmdterm_console_diagnostics(char* argument)
{
   while (argument && *argument && isspace((unsigned char)*argument))
      ++argument;

   if (!argument || !*argument)
   {
      cmdterm_print(PlatformAtomicLoad(&CmdTermDiagnosticCapture)
         ? "Diagnostic output capture is on."
         : "Diagnostic output capture is off.");
      return;
   }
   if (cmdterm_argument_is(argument, "on") ||
       cmdterm_argument_is(argument, "1") ||
       cmdterm_argument_is(argument, "true"))
   {
      PlatformAtomicExchange(&CmdTermDiagnosticCapture, TRUE);
      cmdterm_install_output_hook();
   }
   else if (cmdterm_argument_is(argument, "off") ||
            cmdterm_argument_is(argument, "0") ||
            cmdterm_argument_is(argument, "false"))
   {
      PlatformAtomicExchange(&CmdTermDiagnosticCapture, FALSE);
      if (PlatformAtomicLoad(&CmdTermCommandCaptureDepth) == 0)
         cmdterm_remove_output_hook();
   }
   else
   {
      cmdterm_print("Usage: console_diagnostics [on/off]");
      return;
   }

   cmdterm_print(PlatformAtomicLoad(&CmdTermDiagnosticCapture)
      ? "Diagnostic output capture enabled."
      : "Diagnostic output capture disabled.");
}

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
   int len=(int)strlen(cmd);
      if (len==0)
         return 0;
      else if ((space==NULL)&&(len<max_len))
      {
         strcpy(cmd+len," ");
         space=cmd+len;
      }
      while (*space==' ') space++;
   return (int)(space-cmd);
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
   KB_FLAG_DOWN|KB_FLAG_CTRL|'v',
   KB_FLAG_DOWN|KB_FLAG_CTRL|'c',
   KB_FLAG_DOWN|KB_FLAG_CTRL|KB_FLAG_SHIFT|'c',
   KB_FLAG_DOWN|KEY_TAB,
   KB_FLAG_DOWN|KEY_PGUP,
   KB_FLAG_DOWN|KEY_PAD_PGUP,
   KB_FLAG_DOWN|KEY_PGDN,
   KB_FLAG_DOWN|KEY_PAD_PGDN,
   KB_FLAG_DOWN|KEY_HOME,
   KB_FLAG_DOWN|KEY_PAD_HOME,
   KB_FLAG_DOWN|KEY_END,
   KB_FLAG_DOWN|KEY_PAD_END,
   0
};

static char last_prefix[sizeof(CmdTerm.cmdbuf)]=" ";

bool cmdterm_textbox_cb(LGadTextBox* box, LGadTextBoxEvent event, int evdata, void* data)
{
   char  help_buf[256], *help_ptr=NULL;
   char* text = LGadTextBoxText(box);
   BOOL update = FALSE, clear = FALSE, new_base=FALSE;
   int cursor_pos;

   if (event == TEXTBOX_BUTTON && (evdata & BUTTONGADG_LCLICK))
   {
      // TextGadg grabs keyboard focus itself, but gameplay also needs the
      // command input context and pause state that the hotkey establishes.
      cmdterm_activate_focus(FALSE);
      return TRUE;
   }
   
   if (event == TEXTBOX_SPECKEY)
   {
      const char *tmp=NULL, *ret;
      ulong context;

      switch(evdata)
      {
      case KB_FLAG_DOWN|KEY_ENTER:
         if (!cmdterm_is_scrollable())
         {
            Status("");

            if (CmdTerm.statebits & kStatePaused)
            {
               SimStateUnpause();
               CmdTerm.statebits &= ~kStatePaused;
            }

            IInputBinder_GetContext (g_pInputBinder, &context);
            if (context == HK_COMMAND_MODE && HotkeyContext == HK_GAME_MODE)
               IInputBinder_SetContext (g_pInputBinder, HK_GAME_MODE, TRUE);

            cmdterm_begin_command_capture();
            ret = IInputBinder_ProcessCmd (g_pInputBinder, text);
            cmdterm_end_command_capture();
            if (ret != NULL)
            {
               Status(ret);
               cmdterm_print(ret);
            }
            else
               history_add(text);
            *text = '\0';
            update = TRUE;
            clear = TRUE;
            break;
         }

         if (text && *text)
         {
            char entered[sizeof(CmdTerm.cmdbuf) + 3];
            cmdterm_set_scroll(0);
            _snprintf(entered, sizeof(entered) - 1, "> %s", text);
            entered[sizeof(entered) - 1] = '\0';
            cmdterm_print(entered);
            Status("");

            // Temporarily unpause while executing so commands may safely
            // change modes or simulation state.
            if (CmdTerm.statebits & kStatePaused)
            {
               SimStateUnpause();
               CmdTerm.statebits &= ~kStatePaused;
            }

            IInputBinder_GetContext (g_pInputBinder, &context);
            if (context == HK_COMMAND_MODE && HotkeyContext == HK_GAME_MODE)
               IInputBinder_SetContext (g_pInputBinder, HK_GAME_MODE, TRUE);

            cmdterm_begin_command_capture();
            ret = IInputBinder_ProcessCmd (g_pInputBinder, text);
            cmdterm_end_command_capture();
            if (ret != NULL)
            {
               Status(ret);
               cmdterm_print(ret);
            }
            else
               history_add(text);

            if ((CmdTerm.statebits & kStateInUse) &&
                !(CmdTerm.flags & kCmdTermNoPause))
            {
               SimStatePause();
               CmdTerm.statebits |= kStatePaused;
               if (HotkeyContext == HK_GAME_MODE)
                  IInputBinder_SetContext (g_pInputBinder,
                                           HK_COMMAND_MODE, TRUE);
            }
         }
         *text = '\0';
         update = TRUE;
         break;
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'v':
         update = cmdterm_paste_command(box);
         break;
      case KB_FLAG_DOWN|KB_FLAG_CTRL|KB_FLAG_SHIFT|'c':
         cmdterm_copy_output(TRUE);
         break;
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'c':
         if (text && *text)
            cmdterm_copy_command();
         else
            cmdterm_copy_output(FALSE);
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
      {
         char* history = history_get(-1);
         cursor_pos=move_cursor_into_command(history,sizeof(CmdTerm.cmdbuf));
         tmp=history;
         LGadTextBoxSetCursor(box,cursor_pos);   // cp was strlen(tmp)
         break;
      }
      case KB_FLAG_DOWN|KB_FLAG_CTRL|'n':
      {
         char* history = history_get(+1);
         cursor_pos=move_cursor_into_command(history,sizeof(CmdTerm.cmdbuf));
         tmp=history;
         LGadTextBoxSetCursor(box,cursor_pos);
         break;
      }
      case KB_FLAG_DOWN|KEY_PGUP:
      case KB_FLAG_DOWN|KEY_PAD_PGUP:
         if (cmdterm_is_scrollable())
         {
            cmdterm_scroll_by(CmdTerm.visiblelines);
            cmdterm_rebuild_output();
            update = TRUE;
         }
         break;
      case KB_FLAG_DOWN|KEY_PGDN:
      case KB_FLAG_DOWN|KEY_PAD_PGDN:
         if (cmdterm_is_scrollable())
         {
            cmdterm_scroll_by(-CmdTerm.visiblelines);
            cmdterm_rebuild_output();
            update = TRUE;
         }
         break;
      case KB_FLAG_DOWN|KEY_HOME:
      case KB_FLAG_DOWN|KEY_PAD_HOME:
         if (cmdterm_is_scrollable())
         {
            cmdterm_set_scroll(cmdterm_max_scroll());
            cmdterm_rebuild_output();
            update = TRUE;
         }
         break;
      case KB_FLAG_DOWN|KEY_END:
      case KB_FLAG_DOWN|KEY_PAD_END:
         if (cmdterm_is_scrollable())
         {
            cmdterm_set_scroll(0);
            cmdterm_rebuild_output();
            update = TRUE;
         }
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
         if (cmdterm_is_scrollable())
         {
            if (CmdTerm.flags & kCmdTermHideUnfocused)
               CmdTerm.statebits &= ~kStateVisible;
            if (CmdTerm.scrolldragging)
            {
               uiReleaseFocus(LGadBoxRegion(&CmdTerm.scrollbar),
                              UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE);
               CmdTerm.scrolldragging = FALSE;
            }
         }
         LGadUnfocusTextBox(box);
         PlatformSetTextInputActive(FALSE);
         if (CmdTerm.flags & kCmdTermHideUnfocused)
            region_set_invisible(LGadBoxRegion(&CmdTerm.root),TRUE);
         if (CmdTerm.statebits & kStatePaused)
         {
            SimStateUnpause();
            CmdTerm.statebits &= ~kStatePaused;
         }
         if (cmdterm_is_scrollable())
            cmdterm_restore_canvas();
         cmdterm_set_popup_cursor(FALSE);
      }
      else if (!(CmdTerm.flags & kCmdTermNoPause) &&
               !(CmdTerm.statebits & kStatePaused))
      {
         SimStatePause();
         CmdTerm.statebits |= kStatePaused;
      }

      if (!clear && cmdterm_is_scrollable())
         cmdterm_draw();
   }
   return update;
}

void CreateCommandTerminal(LGadRoot* root, Rect* bounds, ulong flags)
{
   IResMan* resman;
   const char* font_name;
   int font_profile;
   short font_width;
   short font_height;
   short row_height;
   short title_height;
   short input_top;
   short output_width;

   if (CmdTerm.statebits & kStateInUse)
      DestroyCommandTerminal();
   memset(&CmdTerm,0,sizeof(CmdTerm));
   CmdTerm.flags = flags;
   CmdTerm.parentroot = root;
   CmdTerm.bounds = *bounds;
   if (PlatformAtomicLoad(&CmdTermDiagnosticCapture))
      cmdterm_install_output_hook();

   // Retain the compact profile for other historical game targets. Thief 2
   // and DromEd both select the shared scrollable profile below.
   if (!cmdterm_is_scrollable())
   {
      LGadTextBoxDesc tdesc;

      LGadSetupSubRoot(&CmdTerm.root,root,bounds->ul.x,bounds->ul.y,
                       (short)RectWidth(bounds),(short)RectHeight(bounds));

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

      CmdTerm.statebits = kStateInUse;
      if (flags & kCmdTermBeginFocused)
         cmdterm_focus("");
      else if (flags & kCmdTermHideUnfocused)
         region_set_invisible(LGadBoxRegion(&CmdTerm.root),TRUE);
      LGadDrawBox(VB(&CmdTerm.root),NULL);
      cmdterm_setup_cmds();
      return;
   }

   // Editor and gameplay consoles share the implementation but have different
   // available space. Each keeps its own runtime-adjustable font preference.
   if (GetCurrentStyle())
      memcpy(&CmdTerm.style, GetCurrentStyle(), sizeof(CmdTerm.style));
   // The scrollable console targets Dark's 16-bit compatibility canvas even
   // when it is constructed while the startup screen still reports an 8-bit
   // mode. Store an RGB16 value directly so white cannot be mistaken for a
   // palette index and rendered cyan later. The AA blender masks the spare
   // high bit when the destination is RGB555.
   CmdTerm.style.colors[StyleColorFG] = 0xffff;
   CmdTerm.style.colors[StyleColorText] = 0xffff;
   font_profile = (flags & kCmdTermHideUnfocused) ? 1 : 0;
   font_name = font_profile
      ? CmdTermPopupFontNames[CmdTermFontIndices[font_profile]]
      : CmdTermEditorFontNames[CmdTermFontIndices[font_profile]];
   resman = AppGetObj(IResMan);
   if (resman)
   {
      CmdTerm.fontres = IResMan_Bind(resman, font_name, RESTYPE_FONT,
                                    NULL, "intrface", 0);
      if (!CmdTerm.fontres && strcmp(font_name, "textfont"))
         CmdTerm.fontres = IResMan_Bind(resman, "textfont", RESTYPE_FONT,
                                       NULL, "intrface", 0);
      SafeRelease(resman);
   }
   if (CmdTerm.fontres)
   {
      CmdTerm.style.fonts[StyleFontNormal] = (IDataSource*)CmdTerm.fontres;
      CmdTerm.style.fonts[StyleFontTitle] = (IDataSource*)CmdTerm.fontres;
   }

   LGadSetupSubRoot(&CmdTerm.root,root,bounds->ul.x,bounds->ul.y,(short)RectWidth(bounds),(short)RectHeight(bounds));

   guiStyleSetupFont(&CmdTerm.style,StyleFontNormal);
   gr_string_size("X",&font_width,&font_height);
   guiStyleCleanupFont(&CmdTerm.style,StyleFontNormal);
   row_height = font_height + 2;
   title_height = row_height;
   input_top = (short)(RectHeight(bounds) - row_height);
   CmdTerm.scrollbarwidth = row_height;
   if (CmdTerm.scrollbarwidth < 14)
      CmdTerm.scrollbarwidth = 14;
   if (CmdTerm.scrollbarwidth > 24)
      CmdTerm.scrollbarwidth = 24;
   output_width = (short)(RectWidth(bounds) - CmdTerm.scrollbarwidth);
   CmdTerm.visiblelines = (input_top - title_height) / font_height;
   if (CmdTerm.visiblelines < 1)
      CmdTerm.visiblelines = 1;

   // Use a separate title row so an empty command line is still visibly open.
   {
      LGadTextBoxDesc tdesc;

      memset(&tdesc,0,sizeof(tdesc));
      tdesc.bounds.lr = MakePoint(RectWidth(bounds),title_height);
      tdesc.bounds.ul = MakePoint(0,0);

      tdesc.editbuf = CmdTerm.titlebuf;
      tdesc.editbuflen = sizeof(CmdTerm.titlebuf);
      tdesc.style = &CmdTerm.style;
      if (flags & kCmdTermHideUnfocused)
         strcpy(CmdTerm.titlebuf,
                "Console - wheel scroll; Ctrl+wheel font; Ctrl+V paste; Esc closes");
      else
         strcpy(CmdTerm.titlebuf,
                "Console :/; focus | wheel scroll | Ctrl+wheel font");

      LGadCreateTextBoxDesc(&CmdTerm.titlebox,&CmdTerm.root,&tdesc);
      LGadTextBoxClrFlag(&CmdTerm.titlebox,
         TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW);
      LGadBoxSetFlags(&CmdTerm.titlebox,
         LGadBoxFlags(&CmdTerm.titlebox)|BOXFLAG_FLIP);
   }

   // Scrollable output area between the title and command line.
   {
      LGadTextBoxDesc tdesc;

      memset(&tdesc,0,sizeof(tdesc));
      tdesc.bounds.lr = MakePoint(output_width,input_top);
      tdesc.bounds.ul = MakePoint(0,title_height);

      tdesc.editbuf = CmdTerm.outputbuf;
      tdesc.editbuflen = sizeof(CmdTerm.outputbuf);
      tdesc.flags = TEXTBOX_BORDER_FLAG;
      tdesc.style = &CmdTerm.style;

      LGadCreateTextBoxDesc(&CmdTerm.outputbox,&CmdTerm.root,&tdesc);
      LGadTextBoxClrFlag(&CmdTerm.outputbox,
         TEXTBOX_EDIT_EDITABLE|TEXTBOX_EDIT_BRANDNEW);
      LGadBoxSetFlags(&CmdTerm.outputbox,
         LGadBoxFlags(&CmdTerm.outputbox)|BOXFLAG_FLIP);
   }

   // A conventional proportional scrollbar for the retained output history.
   LGadCreateBox(&CmdTerm.scrollbar, &CmdTerm.root, output_width, title_height,
                 (short)CmdTerm.scrollbarwidth,
                 (short)(input_top - title_height), cmdterm_scrollbar_mouse,
                 NULL, cmdterm_scrollbar_draw, 0);
   LGadBoxSetStyle(&CmdTerm.scrollbar, &CmdTerm.style);
   LGadBoxSetFlags(&CmdTerm.scrollbar,
      LGadBoxFlags(&CmdTerm.scrollbar)|BOXFLAG_FLIP);
   LGadBoxMouseMotion(&CmdTerm.scrollbar, cmdterm_scrollbar_motion);

   // Make the editable command line on the bottom row.
   {   
      LGadTextBoxDesc tdesc;

      memset(&tdesc,0,sizeof(tdesc));
      tdesc.bounds.lr = MakePoint(RectWidth(bounds),RectHeight(bounds));
      tdesc.bounds.ul = MakePoint(0,input_top);
      
      tdesc.editbuf = CmdTerm.cmdbuf;
      tdesc.editbuflen = sizeof(CmdTerm.cmdbuf);
      CmdTerm.cmdbuf[0] = '\0';
      tdesc.flags = TEXTBOX_BORDER_FLAG|TEXTBOX_FOCUS_FLAG;
      tdesc.style = &CmdTerm.style;
      tdesc.cb = cmdterm_textbox_cb;

      LGadCreateTextBoxDesc(&CmdTerm.textbox,&CmdTerm.root,&tdesc);
      LGadTextBoxSetSpecialKeys(&CmdTerm.textbox,speckeys);
      LGadBoxSetFlags(&CmdTerm.textbox,
         LGadBoxFlags(&CmdTerm.textbox)|BOXFLAG_FLIP);
   }

   // Child gadgets normally consume mouse events before their parent sees
   // them. Install wheel handling on every console surface so scrolling and
   // Ctrl+wheel font sizing work wherever the pointer is within the terminal.
   cmdterm_install_mouse_handler(LGadBoxRegion(&CmdTerm.root));
   cmdterm_install_mouse_handler(LGadBoxRegion(&CmdTerm.titlebox));
   cmdterm_install_mouse_handler(LGadBoxRegion(&CmdTerm.outputbox));
   cmdterm_install_mouse_handler(LGadBoxRegion(&CmdTerm.textbox));

   CmdTerm.statebits = kStateInUse;
   if (!CmdTermLineCount)
      cmdterm_print("Console ready. Type help for commands.");
   else
      cmdterm_rebuild_output();
   if (flags & kCmdTermBeginFocused)
   {
      cmdterm_focus("");
   }
   else if (flags & kCmdTermHideUnfocused)
   {
      region_set_invisible(LGadBoxRegion(&CmdTerm.root),TRUE);
   }
   else
   {
      // DromEd keeps the shared console visible in its layout even when the
      // command line does not own keyboard focus.
      CmdTerm.statebits |= kStateVisible;
   }
   LGadDrawBox(VB(&CmdTerm.root),NULL);
   cmdterm_setup_cmds();
}

void DestroyCommandTerminal(void)
{
   ulong context;
   int i;

   PlatformSetTextInputActive(FALSE);
   cmdterm_set_popup_cursor(FALSE);
   if (CmdTerm.statebits & kStatePaused)
   {
      SimStateUnpause();
      CmdTerm.statebits &= ~kStatePaused;
   }
   IInputBinder_GetContext (g_pInputBinder, &context);
   if (context == HK_COMMAND_MODE && HotkeyContext == HK_GAME_MODE)
      IInputBinder_SetContext (g_pInputBinder, HK_GAME_MODE, TRUE);
   if (cmdterm_is_scrollable())
      cmdterm_restore_canvas();

   CmdTerm.statebits &= ~kStateInUse;
   if (cmdterm_is_scrollable() && CmdTerm.scrolldragging)
   {
      uiReleaseFocus(LGadBoxRegion(&CmdTerm.scrollbar),
                     UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE);
      CmdTerm.scrolldragging = FALSE;
   }
   for (i = CmdTerm.mousehandlercount - 1; i >= 0; --i)
      uiRemoveRegionHandler(CmdTerm.mousehandlerregions[i],
                            CmdTerm.mousehandlers[i]);
   CmdTerm.mousehandlercount = 0;
   LGadDestroyTextBox(&CmdTerm.textbox);
   if (cmdterm_is_scrollable())
   {
      LGadDestroyBox(&CmdTerm.scrollbar, FALSE);
      LGadDestroyTextBox(&CmdTerm.outputbox);
      LGadDestroyTextBox(&CmdTerm.titlebox);
   }
   LGadDestroyRoot(&CmdTerm.root);
   SafeRelease(CmdTerm.fontres);
   cmdterm_model_lock();
   cmdterm_capture_flush_locked();
   cmdterm_model_unlock();
   cmdterm_remove_output_hook();
   
}

static BOOL cmdterm_apply_pending_font_change(void)
{
   long delta = PlatformAtomicExchange(&CmdTermPendingFontDelta, 0);
   LGadRoot* parent;
   Rect bounds;
   ulong flags;
   int profile;
   int index;
   int font_count;
   int scroll;
   BOOL focused;
   BOOL visible;
   char command[sizeof(CmdTerm.cmdbuf)];

   if (!delta || !(CmdTerm.statebits & kStateInUse) ||
       !cmdterm_is_scrollable())
      return FALSE;

   profile = (CmdTerm.flags & kCmdTermHideUnfocused) ? 1 : 0;
   font_count = profile ? (int)CMDTERM_POPUP_FONT_COUNT
                        : (int)CMDTERM_EDITOR_FONT_COUNT;
   index = CmdTermFontIndices[profile] + delta;
   if (index < 0)
      index = 0;
   if (index >= font_count)
      index = font_count - 1;
   if (index == CmdTermFontIndices[profile])
      return FALSE;

   parent = CmdTerm.parentroot;
   bounds = CmdTerm.bounds;
   flags = CmdTerm.flags;
   scroll = CmdTerm.scroll;
   focused = LGadTextBoxEditing(&CmdTerm.textbox) != 0;
   visible = (CmdTerm.statebits & kStateVisible) != 0;
   strncpy(command, CmdTerm.cmdbuf, sizeof(command));
   command[sizeof(command) - 1] = '\0';

   CmdTermFontIndices[profile] = index;
   DestroyCommandTerminal();
   CreateCommandTerminal(parent, &bounds, flags);
   strncpy(CmdTerm.cmdbuf, command, sizeof(CmdTerm.cmdbuf));
   CmdTerm.cmdbuf[sizeof(CmdTerm.cmdbuf) - 1] = '\0';
   LGadUpdateTextBox(&CmdTerm.textbox);
   cmdterm_set_scroll(scroll);
   cmdterm_rebuild_output();

   if (focused)
      cmdterm_activate_focus(FALSE);
   else if (visible)
   {
      CmdTerm.statebits |= kStateVisible;
      region_set_invisible(LGadBoxRegion(&CmdTerm.root), FALSE);
      cmdterm_draw();
   }
   return TRUE;
}

static void cmdterm_deferred_font_change(void* unused)
{
   (void)unused;
   PlatformAtomicExchange(&CmdTermFontChangeDeferred, FALSE);
   cmdterm_apply_pending_font_change();
}

static void cmdterm_activate_focus(BOOL position_cursor)
{
   char* buf = &CmdTerm.cmdbuf[0];
   int bufsiz = sizeof(CmdTerm.cmdbuf);
   int cursor_pos;
   ulong context;

   if (!(CmdTerm.statebits & kStateInUse))
      return;

   // make the region visible, if necessary
   region_set_invisible(LGadBoxRegion(&CmdTerm.root),FALSE);
   if (cmdterm_is_scrollable())
   {
      CmdTerm.statebits |= kStateVisible;
      if (!CmdTerm.canvasredirected)
      {
         CmdTerm.previousguicanvas = DefaultGUIcanvas;
         DefaultGUIcanvas = ScrnGetDrawCanvas();
         CmdTerm.canvasredirected = TRUE;
      }
   }
   cursor_pos = position_cursor
      ? move_cursor_into_command(buf,bufsiz) : (int)strlen(buf);

   LGadTextBoxSetCursor(&CmdTerm.textbox,cursor_pos);
   LGadUpdateTextBox(&CmdTerm.textbox);
   LGadFocusTextBox(&CmdTerm.textbox);
   PlatformSetTextInputActive(TRUE);
   LGadTextBoxClrFlag(&CmdTerm.textbox,TEXTBOX_EDIT_BRANDNEW);
   if (cmdterm_is_scrollable())
   {
      cmdterm_rebuild_output();
      cmdterm_draw();
   }
   else
      LGadDrawBox(VB(&CmdTerm.textbox),NULL);
   // Gameplay pauses while the console owns input. The editor uses the same
   // view without changing its distinct simulation lifecycle.
   if (!(CmdTerm.flags & kCmdTermNoPause))
      SimStatePause();
   

   IInputBinder_GetContext (g_pInputBinder, &context);
   if (context == HK_GAME_MODE)
      IInputBinder_SetContext (g_pInputBinder, HK_COMMAND_MODE, TRUE);

   cmdterm_set_popup_cursor(TRUE);
   
   if (!(CmdTerm.flags & kCmdTermNoPause))
      CmdTerm.statebits |= kStatePaused;
}

void cmdterm_focus(char* prefix)
{
   char* buf = &CmdTerm.cmdbuf[0];
   int bufsiz = sizeof(CmdTerm.cmdbuf);

   if (!(CmdTerm.statebits & kStateInUse))
      return;

   if (prefix)
      strncpy(buf,prefix,bufsiz);
   else
      strcpy(buf,"");
   buf[bufsiz-1] = '\0';
   cmdterm_activate_focus(TRUE);
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
   { "console_copy", FUNC_STRING, cmdterm_console_copy,
     "copy console_copy [visible/all/command] to the clipboard", HK_ALL },
   { "console_diagnostics", FUNC_STRING, cmdterm_console_diagnostics,
     "capture console_diagnostics [on/off] mprintf diagnostics", HK_ALL },
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
