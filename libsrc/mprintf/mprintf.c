/* Portable implementation of the legacy monochrome-output API. */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <coremutx.h>
#include <mprintf.h>
#include <platform_services.h>

#define MONO_WIDTH 80
#define MONO_HEIGHT 25
#define MONO_FILE "mprintf.log"

bool mono_to_debugger = FALSE;
void (*mono_spc_func)(char *, int) = NULL;

static FILE *g_log;
static char g_last_log[260] = MONO_FILE;
static int g_mode = MONO_OFF;
static int g_x;
static int g_y;
static int g_page;
static int g_previous_page = -1;
static int g_focus;
static int g_window = -1;

static void update_cursor(const char *text, int length)
{
   int i;
   for (i = 0; i < length; ++i)
   {
      if (text[i] == '\n')
      {
         g_x = 0;
         if (g_y < MONO_HEIGHT - 1)
            ++g_y;
      }
      else if (text[i] == '\r')
         g_x = 0;
      else if (++g_x >= MONO_WIDTH)
      {
         g_x = 0;
         if (g_y < MONO_HEIGHT - 1)
            ++g_y;
      }
   }
}

int _mprint(const char *text, int length)
{
   if (!text || length <= 0)
      return 0;
   if (g_log)
   {
      fwrite(text, 1, length, g_log);
      fflush(g_log);
   }
   if (mono_to_debugger)
   {
      char buffer[1024];
      int offset = 0;
      while (offset < length)
      {
         int count = length - offset;
         if (count >= (int)sizeof(buffer))
            count = sizeof(buffer) - 1;
         memcpy(buffer, text + offset, count);
         buffer[count] = '\0';
         PlatformDebugOutput(buffer);
         offset += count;
      }
   }
   if (mono_spc_func)
      mono_spc_func((char *)text, length);
   update_cursor(text, length);
   return length;
}

bool mono_detect(void) { return FALSE; }
bool mono_init(void) { return TRUE; }
bool mono_win_init(bool have_screen) { (void)have_screen; return mono_init(); }

int mono_setmode(int mode)
{
   int previous = g_mode;
   g_mode = mode == MONO_TOG ? (g_mode == MONO_ON ? MONO_OFF : MONO_ON)
                             : (mode == MONO_ON ? MONO_ON : MONO_OFF);
   return previous;
}

int mono_getmode(void) { return g_mode; }

int mprint(const char *text)
{
   int result;
   if (!text)
      return 0;
   CoreThreadLock();
   result = _mprint(text, (int)strlen(text));
   CoreThreadUnlock();
   return result;
}

int mprintf(const char *format, ...)
{
   char buffer[2048];
   int length;
   va_list args;
   if (!format)
      return 0;
   va_start(args, format);
   length = vsnprintf(buffer, sizeof(buffer), format, args);
   va_end(args);
   if (length < 0)
      return length;
   if (length >= (int)sizeof(buffer))
      length = sizeof(buffer) - 1;
   return mprint(buffer);
}

int mget(uchar *value, int x, int y)
{
   (void)value; (void)x; (void)y;
   return -1;
}

int mput(uchar value, int x, int y)
{
   (void)value; (void)x; (void)y;
   return -1;
}

void mono_clear(void) { g_x = g_y = 0; }
void mono_scroll(int lines) { (void)lines; }

int mono_logon(char *filename, int how, int which)
{
   const char *name = (how & MONO_LOG_DEF) || !filename ? MONO_FILE : filename;
   const char *mode = (how & MONO_LOG_CON) ? "ab" : "wb";
   (void)which;
   mono_logoff();
   strncpy(g_last_log, name, sizeof(g_last_log) - 1);
   g_last_log[sizeof(g_last_log) - 1] = '\0';
   g_log = fopen(g_last_log, mode);
   return g_log ? 0 : -1;
}

void mono_logoff(void)
{
   if (g_log)
   {
      fclose(g_log);
      g_log = NULL;
   }
}

bool mono_logdel(bool kill_last, bool always)
{
   const char *name = kill_last ? g_last_log : MONO_FILE;
   if (g_log && !always)
      return FALSE;
   mono_logoff();
   return remove(name) == 0;
}

void mono_setattr(uchar attribute) { (void)attribute; }

bool mono_setxy(int x, int y)
{
   if (x < 0 || x >= MONO_WIDTH || y < 0 || y >= MONO_HEIGHT)
      return FALSE;
   g_x = x;
   g_y = y;
   return TRUE;
}

void mono_getxy(int *x, int *y)
{
   if (x) *x = g_x;
   if (y) *y = g_y;
}

bool mono_setpage(int page, bool focus)
{
   if (page < 0 || page >= MONO_MAX_PAGES)
      return FALSE;
   g_previous_page = g_page;
   g_page = page;
   if (focus) g_focus = page;
   return TRUE;
}

int mono_getpage(void) { return g_page; }
bool mono_flip(int page) { return mono_setpage(page, TRUE); }

bool mono_unflip(void)
{
   int previous = g_previous_page;
   return previous >= 0 && mono_setpage(previous, TRUE);
}

int mono_setfocus(int page)
{
   int previous = g_focus;
   if (page >= 0 && page < MONO_MAX_PAGES) g_focus = page;
   return previous;
}

int mono_getfocus(void) { return g_focus; }
void mono_scr_disable(void) {}
void mono_scr_enable(void) {}
void mono_set_flags(int flags, char *message) { (void)flags; (void)message; }

void mono_cursor(bool current, int start, int stop)
{
   (void)current; (void)start; (void)stop;
}

int mono_split(int axis, int location)
{
   (void)axis; (void)location;
   g_window = MONO_WIN_ONE;
   return g_window;
}

bool mono_unsplit(void) { g_window = -1; return TRUE; }

bool mono_setwin(int which)
{
   if (which < MONO_WIN_ONE || which > MONO_WIN_MAX)
      return FALSE;
   g_window = which;
   return TRUE;
}

int mono_getwin(void) { return g_window; }

bool mono_dump(char *filename, bool erase_it, bool readable)
{
   FILE *output;
   (void)erase_it; (void)readable;
   if (!filename || !(output = fopen(filename, "wb")))
      return FALSE;
   fclose(output);
   return TRUE;
}
