/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// This module deals with how the major menus are layed out

#include <mediface.h>
#include <gadget.h>
#include <lgslider.h>
#include <util2d.h>
#include <gcompose.h>
#include <guistyle.h>
#include <event.h>
#include <motedit_.h>
#include <medflbox.h>
#include <medmenu.h>

#include <vumanui.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

LGadRoot *main_root;
LGadBox *main_box;

// XXX todo:  currently task exception in mouse render when switch from hires
// to lowres mode.

// The basic interface

// these bring up the major menus, and are the play button and slider
//LGadScale control_slider;
LGadSlider control_slider;
BOOL control_slider_active = FALSE;
LGadBox *flag_box;

typedef int (*ControlFunc)(void);

typedef struct {
   DrawElement draw;
   Point size;
   Point coord;
   LGadButtonCallback bfunc;
} ControlInfo;

#define STANDARD_MOTEDIT_FLAGS BORDER(DRAWFLAG_BORDER_OUTLINE)

ControlInfo control_info[] = { \
   { { DRAWTYPE_TEXT, (void *)"SAVE/LOAD",NULL,0,0,STANDARD_MOTEDIT_FLAGS }, {106, MOTEDIT_BUTTON_HEIGHT}, {0, 0}, SaveLoadButtonFunc },
   { { DRAWTYPE_TEXT, (void *)"PLAY OPTIONS",NULL,0,0,STANDARD_MOTEDIT_FLAGS }, {106, MOTEDIT_BUTTON_HEIGHT}, {106, 0}, PlayOptButtonFunc },
   { { DRAWTYPE_TEXT, (void *)"FRAME",NULL,0,0,STANDARD_MOTEDIT_FLAGS }, {106, MOTEDIT_BUTTON_HEIGHT}, {318, 0}, FrameButtonFunc },
   { { DRAWTYPE_TEXT, (void *)"HEADER",NULL,0,0,STANDARD_MOTEDIT_FLAGS }, {106, MOTEDIT_BUTTON_HEIGHT}, {530, 0}, HeaderButtonFunc },
};

ControlInfo play_button_control_info = { \
   { DRAWTYPE_TEXT, (void *)"PLAY",NULL,0,0,STANDARD_MOTEDIT_FLAGS },\
   {80, MOTEDIT_BUTTON_HEIGHT}, {0, 140}, PlayButtonFunc };

#define NUM_CONTROL_BUTTONS ((sizeof(control_info)/sizeof(control_info[0]))+1)

LGadButton *control_buttons[NUM_CONTROL_BUTTONS];

extern void LGadInitSlider(LGadSlider *vs);

bool MainMouseHandler(short x, short y, short action, short wheel, LGadBox *vb)
{
   if (!(action & (MOUSE_LUP|MOUSE_RUP)))
      return(TRUE);

   return(TRUE);
}

int minor_mode=-1;

typedef struct {
   grs_bitmap gui_bm;
   uchar *gui_bits;
} GuiInfo;
 
DrawElement control_slider_elem={DRAWTYPE_NONE,NULL,NULL,0,0,STANDARD_MOTEDIT_FLAGS};

void MainInterfaceResetBotSlider(int num_frames)
{
   Rect r;

   // store off dimensions of old slider, since want to reuse these
   r=*(BOX_RECT((LGadBox *)(&control_slider)));

   LGadDestroyBox((LGadBox *)&control_slider,FALSE);
   control_slider_active = FALSE;
   g_CurMotEditFrame=0;
   g_LastMotEditFrame=0;

#if 0
   LGadCreateScaleArgs(&control_slider,main_root,80,460,560,20,\
      &control_slider_elem,NULL,&g_CurMotEditFrame,num_frames,1,255,26,(560/num_frames)-1,20,1,SCALE_NORMAL,0,0);
#endif
   if (num_frames>1)
   {
      LGadInitSlider(&control_slider);
      LGadCreateSliderArgs(&control_slider,main_root,r.ul.x,r.ul.y,r.lr.x-r.ul.x,r.lr.y-r.ul.y,
                           &control_slider_elem,NULL,
                           &g_CurMotEditFrame,num_frames-1,1,r.lr.x-r.ul.x,LGSLIDER_HORIZONTAL,0);
      LGadDrawBox(VB(&control_slider),NULL);
      control_slider_active = TRUE;
   }    
}


void MainInterfaceRefreshFlagBox()
{
   LGadDrawBox(flag_box,NULL);
}

void CreateMotionEditorInterface()
{
   int i; // iterator
   int w,h,bot,right;
   Rect *r;

   main_root=vmGrabViews(vmGrabTwoWide);
   
   main_box=VB(main_root);
   r=LGadBoxRect(main_box);
   w=(r->lr.x-r->ul.x)/(NUM_CONTROL_BUTTONS-1);
   h=MOTEDIT_BUTTON_HEIGHT;
   bot=r->lr.y-r->ul.y;
   right=r->lr.x-r->ul.x;

   // top menu buttons
   for (i=0; i < NUM_CONTROL_BUTTONS-1; i++)
   {
      control_info[i].draw.fcolor=guiStyleGetColor(NULL,StyleColorText);
      control_buttons[i]=LGadCreateButtonArgs(NULL,main_root,\
         w*i, 0, w, h,\
         &((control_info[i]).draw),control_info[i].bfunc,0);
   }
   // play button
   play_button_control_info.draw.fcolor=guiStyleGetColor(NULL,StyleColorText);
   control_buttons[i]=LGadCreateButtonArgs(NULL,main_root,\
      0, bot-h, w, h,\
      &(play_button_control_info.draw),play_button_control_info.bfunc,0);

   // motion slider
   LGadInitSlider(&control_slider);
   control_slider_elem.fcolor=guiStyleGetColor(NULL,StyleColorText);
   control_slider_elem.bcolor=guiStyleGetColor(NULL,StyleColorDim);
   LGadCreateSliderArgs(&control_slider,main_root,w,bot-(h/2),right-w,h/2,&control_slider_elem,NULL,\
      &g_CurMotEditFrame,100,1,right-w,LGSLIDER_HORIZONTAL,0);
   control_slider_active = TRUE;
   // mark/flag view box
   flag_box=LGadCreateBox(NULL,main_root,w,bot-h,right-w,h/2,MainMouseHandler,NULL,FlagBoxDrawCallback,0);
   FlagBoxResetMarks(100);
   LGadDrawBox(main_box,NULL);
}

void DestroyMotionEditorInterface()
{
   int i; // iterator

   for (i=0; i < NUM_CONTROL_BUTTONS; i++)
   {
      LGadDestroyBox((LGadBox *)control_buttons[i],TRUE);
   }
   if (control_slider_active)
      LGadDestroyBox((LGadBox *)&control_slider,FALSE);
   LGadDestroyBox((LGadBox *)flag_box,TRUE);
   DestroyActiveMenus();

   vmReleaseViews(main_root);
}

void MainInterfaceReset()
{
   DestroyMotionEditorInterface();
   CreateMotionEditorInterface();
}

