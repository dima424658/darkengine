/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/medmenu.cpp,v 1.7 2000/02/19 13:11:01 toml Exp $

// menu stuff for saving and loading.

#include <gadget.h>
#include <config.h>
#include <medmenu.h>
#include <motedit_.h>
#include <medplay.h>
#include <gedit.h>
#include <viewmgr.h>
#include <status.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>

#include <prompts.h>

#define EDITMODE_NONE      -1
#define EDITMODE_SAVELOAD  0
#define EDITMODE_PLAYOPT   1
#define EDITMODE_FRAME     2
#define EDITMODE_HEADER    3
#define EDITMODE_PLAY      4

// active menu stuff, for menus inside rendering area
#define MAX_ACTIVE_MENUS 4
LGadBox *activeMenus[MAX_ACTIVE_MENUS];
int numActiveMenus=0;


int current_edit_mode = EDITMODE_NONE;

LGadMenu saveloadmenu_gad;
LGadEditMenu playoptmenu_gad;
LGadMenu headermenu_gad;
LGadMenu framemenu_gad;

bool EditorEraseMinorMode(int new_mode)
{
   if (current_edit_mode == new_mode)
      return(FALSE);
   numActiveMenus=0;

   if(current_edit_mode==EDITMODE_PLAY) // stop playing motion
   {
      PlayMotionStop();
   }
   current_edit_mode = new_mode;

   return(TRUE);
}

// ******* SAVE LOAD *******

LGadEditMenu loadname_menu;
LGadEditMenu saveas_menu;

DrawElement saveloadmenu_elems[] = {
   { DRAWTYPE_TEXT, "Load Name", }, 
   { DRAWTYPE_TEXT, "Save ", },
   { DRAWTYPE_TEXT, "Quit MotEditor", },
   { DRAWTYPE_TEXT, "Done", },
};

//extern Id mainMotionID,mainCompMotionID;
extern char mainMotionName[], mainCompMotionName[];

#pragma off(unreferenced)
bool SaveLoadMenuFunc(int sel, LGadMenu *vm)
{
   switch(sel)
   {
   case 0:
      char buf[32], *res;
      res=prompt_string(g_MotEditMotionName,buf);
      if (res)
         MotEditSetEditMotion(res);
      break;
   case 1:
      MotEditFinishMotionEdit();
      break;
   case 2:
      MotEditClose();
      break;
   case 3:
      EditorEraseMinorMode(EDITMODE_NONE);
      return TRUE;
   }
   return FALSE;
}
#pragma on(unreferenced)


#pragma off(unreferenced)
bool SaveLoadButtonFunc(short action,void *data, LGadBox *b)
{
   if(!(action&MOUSE_LUP))
      return FALSE;

   if (!EditorEraseMinorMode(EDITMODE_SAVELOAD))
      return(TRUE);
   LGadCreateMenuArgs(&saveloadmenu_gad, main_root, STANDARD_ME_MENU_MARGIN, MOTEDIT_BUTTON_HEIGHT+STANDARD_ME_MENU_MARGIN, -2, -2, 
      sizeof(saveloadmenu_elems)/sizeof(DrawElement), 0, saveloadmenu_elems, SaveLoadMenuFunc,
      NULL, STANDARD_ME_MENU_FLAGS, STANDARD_ME_MENU_BORDER, NULL, 0);
   ActiveMenuAdd(VB(&saveloadmenu_gad));
   return(0);
}
#pragma on(unreferenced)


// ******* PLAY OPTIONS *******

char *playopt_loop_names[] = { "PLAY ONCE","PLAY LOOP" };
//char *playopt_which_names[] = { "MAIN ONLY", "WITH ALL MOTIONS" };
char *playopt_blend_names[] = { "NO BLENDING", "WITH BLENDING" };
char *playopt_freq_names[] = { "TIME BASED", "FRAME BASED" };

#define NUM_STRINGS(name) (sizeof(name)/sizeof(name[0]))

#define EnumStuff(name) kFieldFlagUnsigned, 0, NUM_STRINGS(name), NUM_STRINGS(name), name,

static sFieldDesc playOptions_fields[] =
{
   { "Trans Millisec", kFieldTypeFloat, FieldLocation(PlayOptions,trans_duration), },
   { "", kFieldTypeEnum, FieldLocation(PlayOptions,loop), EnumStuff(playopt_loop_names) },
   { "", kFieldTypeEnum, FieldLocation(PlayOptions,blend), EnumStuff(playopt_blend_names) },
   { "", kFieldTypeEnum, FieldLocation(PlayOptions,frame_based), EnumStuff(playopt_freq_names) },
   { "Timewarp", kFieldTypeFloat, FieldLocation(PlayOptions,timewarp), },
   { "Stretch", kFieldTypeFloat, FieldLocation(PlayOptions,stretch), },
};

static sStructDesc playOptions_desc = StructDescBuild(PlayOptions,kStructFlagNone,playOptions_fields);

#pragma off(unreferenced)
bool PlayOptButtonFunc(short action,void *data, LGadBox *b)
{
   if(!(action&MOUSE_LUP))
      return FALSE;

   if (!EditorEraseMinorMode(EDITMODE_PLAYOPT))
      return(TRUE);

   sStructEditorDesc editdesc =
   {
      "Play Options",
   };

   IStructEditor *ed = CreateStructEditor(&editdesc,&playOptions_desc,&playOptions);

   if(ed->Go(kStructEdModal))
   {
      PlayOptApply();
      EditorEraseMinorMode(EDITMODE_NONE);
   }
   SafeRelease(ed);

   return(0);
}
#pragma on(unreferenced)


// ******** Header Menus ***********

#include <motstruc.h>
#include <string.h>

mps_motion_info edit_motinfo;

LGadEditMenu headbasic_menu;

DrawElement headermenu_elems[] = {
   { DRAWTYPE_TEXT, "Basic Info" }, 
   { DRAWTYPE_TEXT, "Done"},
};

DrawElement headbasic_elems[] = {
   { DRAWTYPE_TEXT, "Name", 0, 0, 0, FORMAT(DRAWFLAG_FORMAT_LEFT) },
   { DRAWTYPE_TEXT, "Num Frames", 0, 0, 0, FORMAT(DRAWFLAG_FORMAT_LEFT) },
   { DRAWTYPE_TEXT, "Capture Frequency", 0, 0, 0, FORMAT(DRAWFLAG_FORMAT_LEFT) },
   { DRAWTYPE_TEXT, "Done", 0, 0, 0, FORMAT(DRAWFLAG_FORMAT_LEFT) },
};

DrawElement headbasic_title = { DRAWTYPE_TEXT, "Basic Motion Header", 0, 1 };

VarElem headbasic_varelems[] = {
   { (void *)(edit_motinfo.name), EDITTYPE_STRING, EDITFLAG_READONLY },
   { (void *)&(edit_motinfo.num_frames), EDITTYPE_FLOAT, EDITFLAG_READONLY },
   { (void *)&(edit_motinfo.freq), EDITTYPE_INT, EDITFLAG_READONLY },
   { (void *)ActiveMenuKill, EDITTYPE_CANCEL, },
};

// XXX to do.  Get menu "close" selection to work for header descriptor menu
// needs to copy back the motappdata array.
#pragma off(unreferenced)
bool HeaderMenuFunc(int sel, LGadMenu *vm)
{
   switch(sel)
   {
   case 0:
      if(g_MotEditMotionNum==MP_MOT_INVALID)
      {
         Warning(("No motion loaded\n"));
         break;
      }
//      memcpy(&edit_motinfo,&(mp_motion_list[g_MotEditMotionNum].info),sizeof(mps_motion_info));
      StandardEditMenu(headbasic_menu, -2, 0, headbasic_elems, &headbasic_title, headbasic_varelems);
      break;
   case 1:
      EditorEraseMinorMode(EDITMODE_NONE);
      return TRUE;
   }
   return FALSE;
}
#pragma on(unreferenced)

#pragma off(unreferenced)
bool HeaderButtonFunc(short action,void *data, LGadBox *b)
{
   if(!(action&MOUSE_LUP))
      return FALSE;

   if(!g_MotEditMotion)
   {
      Status("no motion loaded");
      return TRUE;
   }
   if (!EditorEraseMinorMode(EDITMODE_HEADER))
      return(TRUE);
   memcpy(&edit_motinfo,&g_MotEditMotion->info,sizeof(edit_motinfo));
   LGadCreateMenuArgs(&headermenu_gad, main_root, STANDARD_ME_MENU_MARGIN, MOTEDIT_BUTTON_HEIGHT+STANDARD_ME_MENU_MARGIN, -2, -2, 
      sizeof(headermenu_elems)/sizeof(DrawElement), 0, headermenu_elems, HeaderMenuFunc,
      NULL, STANDARD_ME_MENU_FLAGS, STANDARD_ME_MENU_BORDER, NULL, 0);
   ActiveMenuAdd(VB(&headermenu_gad));
   return FALSE;
}
#pragma on(unreferenced)

// ******************* FRAME MENU **************

#include <medflbox.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

EXTERN LGadBox *flag_box;
LGadEditMenu flag_menu;

DrawElement framemenu_elems[] = {
   { DRAWTYPE_TEXT, "Set Flag" }, 
   { DRAWTYPE_TEXT, "Mark Left" }, 
   { DRAWTYPE_TEXT, "Mark Right" }, 
   { DRAWTYPE_TEXT, "Clear Marks" }, 
   { DRAWTYPE_TEXT, "Done"},
};

#pragma off(unreferenced)
bool FrameMenuFunc(int sel, LGadMenu *vm)
{
   switch(sel)
   {
   case 0:
   {
      sStructEditorDesc editdesc =
      {
         "Set Flag",
      };

      // set up initial values
      sFlagValue flagValue;
      flagValue.value=FlagBoxGetCurrentFlag();

      IStructEditor *ed = CreateStructEditor(&editdesc,&frame_flag_desc,&flagValue);

      if(ed->Go(kStructEdModal))
      {
         FlagBoxAddFlag(flagValue.value);
      }
      SafeRelease(ed);

      break;
   }
   case 1:
      FlagBoxSetMark(g_CurMotEditFrame,0);
      LGadDrawBox(flag_box,NULL);
      break;
   case 2:
      FlagBoxSetMark(g_CurMotEditFrame,1);
      LGadDrawBox(flag_box,NULL);
      break;
   case 3:
      FlagBoxResetMarks(-1);
      LGadDrawBox(flag_box,NULL);
      break;
   case 4:
      EditorEraseMinorMode(EDITMODE_NONE);
      return TRUE;
   }
   return FALSE;
}
#pragma on(unreferenced)

#pragma off(unreferenced)
bool FrameButtonFunc(short action,void *data, LGadBox *b)
{
   if(!(action&MOUSE_LUP))
      return FALSE;

   if (!EditorEraseMinorMode(EDITMODE_FRAME))
      return(TRUE);

   LGadCreateMenuArgs(&framemenu_gad, main_root, STANDARD_ME_MENU_MARGIN, \
      MOTEDIT_BUTTON_HEIGHT+STANDARD_ME_MENU_MARGIN, -2, -2, \
      sizeof(framemenu_elems)/sizeof(DrawElement), 0, framemenu_elems, FrameMenuFunc, \
      NULL, STANDARD_ME_MENU_FLAGS, STANDARD_ME_MENU_BORDER, NULL, 0);
   ActiveMenuAdd(VB(&framemenu_gad));
   return(0);
}
#pragma on(unreferenced)

// ********* PLAY BUTTON ******

#pragma off(unreferenced)
bool PlayButtonFunc(short action,void *data, LGadBox *b)
{
   if(!(action&MOUSE_LUP))
      return FALSE;

   if(current_edit_mode==EDITMODE_PLAY)
   {
      PlayMotionStop();
      if(!EditorEraseMinorMode(EDITMODE_NONE))
         return(TRUE);
      return FALSE;
   }
   if (!EditorEraseMinorMode(EDITMODE_PLAY))
      return(TRUE);
   PlayMotionStart();
   return(FALSE);
}
#pragma on(unreferenced)

// ************ FRAME SLIDER **********************

// returns TRUE if slider value changed
BOOL FrameSliderUpdate()
{
   if(g_LastMotEditFrame!=g_CurMotEditFrame)
   {
      char spew[20];

      sprintf(spew,"%d\n",g_CurMotEditFrame);
      Status(spew);
      g_LastMotEditFrame=g_CurMotEditFrame;

      if(current_edit_mode!=EDITMODE_FRAME) // too annoying to have this go away
         EditorEraseMinorMode(EDITMODE_NONE);

      return TRUE;
   }
   return FALSE;
}

// ****** DRAWING STUFF FOR MENUS THAT APPEAR OVER RENDERING AREA ******

void ActiveMenuAdd(LGadBox *b)
{
   if(numActiveMenus>MAX_ACTIVE_MENUS-1)
   {
      Warning(("ActiveMenuAdd: too many active menus!\n"));
      return;
   }
   activeMenus[numActiveMenus]=b;
   numActiveMenus++;
}

#pragma off(unreferenced)
bool ActiveMenuKill(short action, void *data, LGadBox *b)
{
   int i,j;

   for(i=0;i<numActiveMenus;i++)
   {
      if(b==activeMenus[i])
         break;
   }
   if(i==numActiveMenus)
      return FALSE;
   for(j=i;j<numActiveMenus-1;j++)
   {
      activeMenus[j]=activeMenus[j+1];
   }
   activeMenus[j]=NULL;
   numActiveMenus--;
   return TRUE;
}
#pragma on(unreferenced)

// this is bad because voyGuiMethods render to screen canvas, and these
// should really render to render canvas.  Should probably make GUI methods
// work off of current canvas or something, but then how to interpret x,y
// values if current canvas is a subcanvas?
// HACK: for now, ActiveMenusDraw is called after render canvas has been blitted.
// since GDI reblits anyway, we only get flickering in direct draw mode.  Should
// fix this for real, though.
void DestroyActiveMenus()
{
   int i;

   for(i=0;i<numActiveMenus;i++)
   {
      LGadDestroyBox(activeMenus[i],FALSE);
      activeMenus[i]=NULL;
   }
   numActiveMenus=0;
   EditorEraseMinorMode(EDITMODE_NONE);
}

