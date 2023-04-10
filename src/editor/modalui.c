/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/modalui.c,v 1.5 2000/02/19 13:11:05 toml Exp $

#include <lg.h>
#include <mouse.h>

#include <stupidui.h>
#include <modalui.h>
#include <scrnman.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// doug is being lame, but im also reducing header interdependancy
extern void vBrush_FrameCallback(void);

  // assuming we start inside a ScrnStartFrame(), this
  // works fine.  If not, then start should do a ScrnStartFrame(),
  // and end should do a ScrnEndFrame()
BOOL modal_ui_start(int btn_check)
{
   if (btn_check)
   {
      bool resl=TRUE, resr=TRUE;  // assume ok
      if (btn_check&MODAL_CHECK_LEFT)  mouse_check_btn(MOUSE_LBUTTON,&resl);
      if (btn_check&MODAL_CHECK_RIGHT) mouse_check_btn(MOUSE_RBUTTON,&resr);
      if (!(resl || resr))
      {
         Warning(("neither button down in modal_ui_start\n"));
         return FALSE;
      }
   }
   stupid_ui_start();
   return TRUE;
}

void modal_ui_update(void)
{
   vBrush_FrameCallback();
   ScrnForceUpdate();
}

void modal_ui_end(void)
{
   stupid_ui_end();
}
