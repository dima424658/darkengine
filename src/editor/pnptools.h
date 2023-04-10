/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/pnptools.h,v 1.15 2000/01/29 13:12:52 adurant Exp $
// header for PnP maintenance tools
#pragma once

// the PnP has 7 virtual lines

#define NUM_LINES 7

////////////////////////////////////////
// misc usefulness
// note that bx is really on both sides, where by is only on the top
EXTERN void buildYLineRect(Rect *fill, Rect *src, int vline, float hsplit, int side, int bx, int by);

// these are sort of randomly here, but convienient
EXTERN void PnP_ExecCommandInt(char *cmd, int val);
EXTERN void PnP_ExecCommandFloat(char *cmd, float val);

////////////////////////////////////////
// here come all the actual PnP tools

// what a PnP looks like, really
// this is only so you can Malloc them outside
// i suppose i could have a getPnPSize call.
// anyway, dont screw with these yourself, sheesh
// hmmmmm

#define MAX_PNP_GADG 48

typedef struct {
   void     *GadgPtr[MAX_PNP_GADG];
   int       GadgType[MAX_PNP_GADG];
   int       GadgSrc[MAX_PNP_GADG];
   int       GadgIdx;
   int       GadgOwner;
   LGadRoot *_root;
   void     *userData;
} _PnP_GadgData;

EXTERN void PnP_SetCurrent(_PnP_GadgData *thePnP);

// redraw tools for updating the gadgets
EXTERN void PnP_FullRedraw(_PnP_GadgData *thePnP);
EXTERN void PnP_Redraw(int Owner, void *data);
EXTERN void PnP_Hide(int Owner, bool hide_or_show); // true is hide
   
// call when starting or freeing a gadget list
// note start implicitly calls SetCurrent, ie it SetCurrent(thePnP)'s at end
EXTERN void PnP_GadgStart(_PnP_GadgData *thePnP, LGadRoot *root);
EXTERN void PnP_GadgFree(_PnP_GadgData *thePnP);

// installs a picture frame at space, with an update/render callback of draw
EXTERN int PnP_PictureBox(Rect *space, void (*draw)(int data), int data);
EXTERN int PnP_TextBox(Rect *space, char *text);

// a 0-1 toggle which is a bool, at space, calls update if non null w/data
EXTERN int PnP_ButtonToggle(Rect *space, char *off, char *on, bool *var,
                  void (*update)(Rect *where, bool val, int data), int data);
EXTERN int PnP_ButtonOneShot(Rect *space, char *name, void (*shoot)(int data), int data);

// ranged slider, calls update w/data, scale is set, 
typedef enum _PnP_SliderOp
{
   PnP_SliderUpdateOp, // value has been updated
   PnP_SliderClickOp,  // we've been clicked.
} PnP_SliderOp;

#define PNP_SLIDER_WRAP   0x1
#define PNP_SLIDER_VSLIDE 0x2 
#define PNP_SLIDER_MENU   0x4 // strings only
#define PNP_SLIDER_CLICKS 0x8 // get click events


EXTERN int PnP_SliderFloat(Rect *space, char *name, float lo, float hi, float scale, float *var,
                     void (*update)(PnP_SliderOp op, Rect *where, float val, int data), int data, ulong flags);
EXTERN int PnP_SliderInt(Rect *space, char *name, int lo, int hi, int scale, int *var,
                   void (*update)(PnP_SliderOp op, Rect *where, int val, int data), int data, ulong flags);
EXTERN int PnP_SliderFixang(Rect *space, char *name, ushort lo, ushort hi, ushort scale,
                            ushort *var, void (*update)(PnP_SliderOp op, Rect *where, ushort val, int data), 
                            int data, ulong flags);
EXTERN int PnP_SliderShort(Rect *space, char *name, short lo, short hi, short scale, 
                           short *var, void (*update)(PnP_SliderOp op, Rect *where, short val, int data), 
                           int data, ulong flags);

EXTERN int PnP_SliderString(Rect *space, char *name,  int num, char** vals, int *var, 
                     void (*update)(PnP_SliderOp op, Rect *where, int val, int data), int data, ulong flags);

// dynamic string length resetting code
EXTERN void PnP_StringSliderSetStrings(void* g, char** strings, int num);
EXTERN void *PnP_GetStringSubGadget(int owner);

// this hateful macro is a utility for synching variables

// this should be a function, but its a macro since it wants to work for all types
// note it changes changed, and glob and loc and such
#define _varforPnP_synch(Glob,Loc,update,owner,changed) \
   if (Glob!=Loc)                                    \
   {                                                 \
      if (update)                                    \
         Glob=Loc;                                   \
      else                                           \
         Loc=Glob;                                   \
      PnP_Redraw(owner,NULL);                        \
      changed=TRUE;                                  \
   }

// these are to use in watcom, so you dont get always false warnings
#define _varforPnP_synch_FALSE(Glob,Loc,owner,changed) \
   if (Glob!=Loc)                                    \
   {                                                 \
      Loc=Glob;                                      \
      PnP_Redraw(owner,NULL);                        \
      changed=TRUE;                                  \
   }

#define _varforPnP_synch_TRUE(Glob,Loc,owner,changed) \
   if (Glob!=Loc)                                    \
   {                                                 \
      Glob=Loc;                                      \
      PnP_Redraw(owner,NULL);                        \
      changed=TRUE;                                  \
   }
