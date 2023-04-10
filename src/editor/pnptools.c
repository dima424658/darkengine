/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/pnptools.c,v 1.26 2000/02/19 13:11:12 toml Exp $
// tools for PnP maintenance

#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <rect.h>
#include <config.h>
#include <gadblist.h>
#include <lgsprntf.h>
#include <cyclegad.h>
#include <command.h>
#include <guistyle.h>
#include <uiedit.h>

#include <brushGFH.h>
#include <PnPtools.h>
#include <pnptool_.h>
#include <pnpvslid.h>
#include <vslider.h>
#include <viewmgr.h>

#include <mprintf.h>
#include <editor.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// this is the easy one, for rectangle building
// note that bx is really on both sides, where by is only on the top
void buildYLineRect(Rect *fill, Rect *src, int vline, float hsplit, int side, int bx, int by)
{
   // use vline for Y
   fill->ul.y=src->ul.y+(RectHeight(src)*vline/NUM_LINES)+by;
   fill->lr.y=src->ul.y+(RectHeight(src)*(vline+1)/NUM_LINES)-1;
   if (side)
   {
      fill->ul.x=src->ul.x+(int)(RectWidth(src)*hsplit);
      fill->lr.x=src->lr.x;
   }
   else
   {
      fill->ul.x=src->ul.x;
      fill->lr.x=src->ul.x+(int)(RectWidth(src)*hsplit);
   }
   fill->ul.x+=bx;
   fill->lr.x-=bx;
}

void PnP_ExecCommandInt(char *cmd, int val)
{
   char pbuf[16];
   itoa(val,pbuf,10);
   if (!CommandExecuteParam(cmd,pbuf))
      Warning(("Couldnt find command <%s> to execute (w/%d)",cmd,val));
}

void PnP_ExecCommandFloat(char *cmd, float val)
{
   char pbuf[24];
   gcvt(val,10,pbuf);
   if (!CommandExecuteParam(cmd,pbuf))
      Warning(("Couldnt find command <%s> to execute (w/%s)",cmd,pbuf));
}

////////////////////////////////////////////////
// from here on down are the hideous shell layer
// on top of the real gadget stuff

_PnP_GadgData *curPnP;

// redraw functions are down at the bottom of the file

// void PnP_GadgFree(void) is down at the bottom of the file

void PnP_SetCurrent(_PnP_GadgData *thePnP)
{
   curPnP=thePnP;
}

void PnP_GadgStart(_PnP_GadgData *thePnP, LGadRoot *root)
{
   if (thePnP==NULL)
   {
      Warning(("Hey, start called with no curPnP!"));
      return;
   }
   if (thePnP->GadgIdx>0)      
   {
      Warning(("Hey, start called while active PnP gadgs are around!"));
      PnP_GadgFree(thePnP);
   }
   thePnP->GadgIdx=thePnP->GadgOwner=0;
   thePnP->_root=root;
   PnP_SetCurrent(thePnP);
}

// for now, we ignore type, since we only do boxes
void _PnP_Register(void *gadg, int type)
{
   if (curPnP->GadgIdx==MAX_PNP_GADG)
      Warning(("Out of PnP Gadget Space"));
   curPnP->GadgType[curPnP->GadgIdx] = type;
   curPnP->GadgSrc[curPnP->GadgIdx]=curPnP->GadgOwner;
   curPnP->GadgPtr[curPnP->GadgIdx++]=gadg;
}

////////////////////////
// picture box

typedef struct {
   void (*draw_me)(int data);
   int data;
} pictureInfo;

static void _pnp_picture_box_draw(void *data, LGadBox *box)
{
   pictureInfo *our_pi=LGadBoxGetUserData(VB(box));
   if (our_pi->draw_me!=NULL)
      (*our_pi->draw_me)(our_pi->data);
}

// installs a picture frame at space, with an update/render callback of draw
int PnP_PictureBox(Rect *space, void (*draw)(int data), int data)
{
   pictureInfo *our_pi=(pictureInfo *)Malloc(sizeof(pictureInfo));
   LGadBox *gadg;
   
   gadg=LGadCreateBox(NULL,curPnP->_root,
      space->ul.x,space->ul.y,(short)RectWidth(space),(short)RectHeight(space),
      NULL,NULL,_pnp_picture_box_draw,0);
   our_pi->draw_me=draw;
   our_pi->data=data;
   LGadBoxSetUserData(gadg,our_pi);
   _PnP_Register(gadg,PnP_REGPICTURE);
   return curPnP->GadgOwner++;
}

static void _PnP_PictureDestroy(void *g)
{
   void *ptr = LGadBoxGetUserData(VB(g));
   LGadDestroyBox(VB(g),TRUE);
   Free(ptr);
}

static void _PnP_PictureUpdate(void *g, void *data)
{
   pictureInfo *our_pi=LGadBoxGetUserData(VB(g));
   if (data!=NULL)
   {
      our_pi->data=(int)data;
      LGadBoxSetUserData(VB(g),our_pi);
   }
   LGadDrawBox(VB(g),NULL);
}

/////////////////////////////////////////////
// text boxes

void _pnp_textbox(Rect *space, char *text)
{
   LGadButton *gadg;
   DrawElement draw; 
   ElementClear(&draw); 
   draw.draw_type = DRAWTYPE_TEXT; 
   draw.draw_data = text;
   // attempt to make textboxes clearly non-interactive
   //   draw.draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
   draw.bcolor = guiStyleGetColor(NULL,StyleColorBG2);
   
   gadg=LGadCreateButtonArgs(NULL,curPnP->_root,
      space->ul.x,space->ul.y,(short)RectWidth(space),(short)RectHeight(space),&draw,NULL,0);
   // watch while we turn this ordinary button into an inert, listless textbox...
   LGadButtonSetFlags(gadg,LGADBUTT_FLAG_STATELESS); 

   _PnP_Register(gadg,PnP_REGTEXTBOX);
}

int PnP_TextBox(Rect *space, char *text)
{
   _pnp_textbox(space,text);
   return curPnP->GadgOwner++;
}

void PnP_TextBoxDestroy(void *g)
{
   LGadDestroyBox(VB(g),TRUE);
}

void PnP_TextBoxUpdate(void *g, void *data)
{
   if (data!=NULL)
      LGadButtonDrawElem(VB(g)).draw_data=data;
   LGadDrawBox(VB(g),NULL);
}

////////////////////////////////////////////////////////////
// TOGGLE TOOL

typedef struct _toggle_data
{
   char* strings[2];
   bool* var;
   int realvar;
   void (*update)(Rect* , bool, int);
   int data;
} toggle_data;

static void toggle_update_elem(LGadToggle* gadg)
{
   toggle_data* tog = (toggle_data*)LGadBoxGetUserData(VB(gadg));
   DrawElement* elem = &BUTTON_DRAWELEM(gadg);
   char* newstring = tog->strings[tog->realvar];
   if (newstring != NULL)
   {
      elem->draw_data = newstring;
      elem->fcolor = 0;
   }
   else 
   {
      elem->fcolor = guiStyleGetColor(NULL,StyleColorHilite);
   }
}

static bool toggle_call(short action, void* data, LGadBox* vb)
{
   toggle_data* tog = (toggle_data*)LGadBoxGetUserData(vb);
   if (action & (MOUSE_LUP|MOUSE_RUP))
   {
      LGadToggle* gadg = (LGadToggle*)vb;
      toggle_update_elem(gadg);

      LGadDrawBox(vb,NULL);
      *tog->var = tog->realvar;
      if (tog->update)
         tog->update(BOX_RECT(vb),*tog->var,tog->data);
      return TRUE;
   }
   return FALSE;
}

// a 0-1 toggle which is a bool, at space, calls update if non null w/data

int PnP_ButtonToggle(Rect *space, char *off, char *on, bool *var,
                  void (*update)(Rect *where, bool val, int data), int data)
{
   toggle_data* tog = Malloc(sizeof(toggle_data));
   LGadToggle* gadg;
   DrawElement draw;
   
                                           
   tog->strings[0] = off;
   tog->strings[1] = on;
   if (*var != 0) *var = TRUE;  // lets range check
   tog->var = var;
   tog->realvar = *var;
   tog->update = update;
   tog->data = data;
   ElementClear(&draw);
   draw.draw_type = DRAWTYPE_TEXT;
   draw.draw_data = tog->strings[*var];
   if (draw.draw_data == NULL)
   {
      draw.draw_data = tog->strings[!*var];
      draw.fcolor = guiStyleGetColor(NULL,StyleColorHilite);
   }
   draw.draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
   
   gadg = LGadCreateToggleArgs(NULL,curPnP->_root,space->ul.x,space->ul.y,
                               (short)RectWidth(space),(short)RectHeight(space),
                               &draw,toggle_call,&tog->realvar,2,1,0);
   LGadBoxSetUserData(VB(gadg),tog);
   _PnP_Register(gadg,PnP_REGTOGGLE);
   return curPnP->GadgOwner++;
}

void PnP_ButtonToggleDestroy(void* g)
{
   void *ptr = LGadBoxGetUserData(VB(g));
   LGadDestroyBox(VB(g),TRUE);
   Free(ptr);
}

void PnP_ButtonToggleUpdate(void* g, void *data)
{
   toggle_data *tog=LGadBoxGetUserData(VB(g));
   LGadToggle* gadg = (LGadToggle*)VB(g);
   if (data!=NULL)
      tog->var=(bool *)data;
   tog->realvar=*tog->var;
   toggle_update_elem(gadg);
   LGadDrawBox(VB(g),NULL);
}

////////////////////////////////////////////////////////////
// ONE SHOT TOOL 

typedef struct _oneshotdata
{
   void (*shoot)(int);
   int data;
} oneshotdata;

bool pnp_oneshot_cb(short action, void* data, LGadBox* vb)
{
   oneshotdata *shooter = (oneshotdata*)LGadBoxGetUserData(vb);
   
   if (action & (MOUSE_LUP|MOUSE_RUP))
   {
      shooter->shoot(shooter->data);
      return TRUE;
   }
   return FALSE;
}

void _PnP_ButtonOneShot(Rect *space, char *name, void (*shoot)(int data), int data)
{
   oneshotdata* shooter = Malloc(sizeof(oneshotdata));
   DrawElement draw; 
   LGadButton* gadg;
   
   shooter->shoot = shoot;
   shooter->data = data;
   
   ElementClear(&draw);
   draw.draw_type = DRAWTYPE_TEXT;
   draw.draw_data = name;
   draw.draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);

   gadg = LGadCreateButtonArgs(NULL,curPnP->_root,space->ul.x,space->ul.y,
                               (short)RectWidth(space),(short)RectHeight(space),
                               &draw,pnp_oneshot_cb,0);
   LGadBoxSetUserData(VB(gadg),shooter);
   _PnP_Register(gadg,PnP_REGONESHOT);
}


int PnP_ButtonOneShot(Rect *space, char *name, void (*shoot)(int data), int data)
{
   _PnP_ButtonOneShot(space,name,shoot,data);
   return curPnP->GadgOwner++;
}

void PnP_ButtonOneShotDestroy(void* g)
{
   void *ptr = LGadBoxGetUserData(VB(g));
   LGadDestroyBox(VB(g),TRUE);
   Free(ptr);
}

void PnP_ButtonOneShotUpdate(void* g, void *data)
{
   // not sure what to do here with data
   LGadDrawBox(VB(g),NULL);
}

////////////////////////////////////////////////////////////
// virtual slider sub-gadget
//



// I'd like to apologize in advance to anyone who has to port this code.

static pnp_vslider_data* the_slider_data;

static void pnp_vslider_func(void)
{
   pnp_vslider_data* data = the_slider_data;
   vm_render_cameras(); 
   if (data->update_func != NULL)
      data->update_func(data->update_gadg,data->update_data);
}

static bool vslider_cb(short action, void* arg, LGadBox* vb)
{
   pnp_vslider_data* data = (pnp_vslider_data*)LGadBoxGetUserData(vb);
   if (data != NULL && action & (MOUSE_LDOWN|MOUSE_RDOWN|MOUSE_CDOWN))
   {
      the_slider_data = data;
      virtual_slider(1,&data->slider,pnp_vslider_func);
      return TRUE;
   }
   return FALSE;
}

void _pnp_vslider(Rect* area, char* title, pnp_vslider_data* _data)
{
   pnp_vslider_data* data = _data;
   DrawElement draw;
   void* gadg;
   
   if (_data != NULL)
   {
      data = Malloc(sizeof(*data)); 
      *data = *_data;
   }

   ElementClear(&draw);
   draw.draw_type = DRAWTYPE_TEXT;
   draw.draw_data = title;
   draw.draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);

   gadg=LGadCreateButtonArgs(NULL,curPnP->_root,
        area->ul.x,area->ul.y,(short)RectWidth(area),(short)RectHeight(area),&draw,
        vslider_cb,0);                     
   if (data != NULL)
      LGadButtonSetFlags(gadg,LGADBUTT_FLAG_GETDOWNS);
   else
      LGadButtonSetFlags(gadg,LGADBUTT_FLAG_STATELESS);
   LGadBoxSetUserData(VB(gadg),data);

   _PnP_Register(gadg,PnP_REGVSLIDER);
}

void _pnp_vslider_update(void* g, void* arg)
{
   pnp_vslider_data* data = (pnp_vslider_data*)LGadBoxGetUserData(VB(g));
   if (data != NULL)
   {
      data->slider->parse(0,data->slider,SLIDER_UPDATE);
   }
}

void _pnp_vslider_destroy(void* g)
{
   pnp_vslider_data* data = (pnp_vslider_data*)LGadBoxGetUserData(VB(g));
   if (data != NULL)
   {
      Free(data->slider);
      Free(data);
   }
   LGadDestroyBox(VB(g),TRUE);
}

////////////////////////////////////////////////////////////
// Slider generics
////////////////////////////////////////////////////////////

#define DeclareCycleUpdate(type) \
EXTERN void PnP_##type##SliderUpdate(void *g, void *arg) 

#define DeclareCycleDestroy(type) \
EXTERN void PnP_Slider##type##Destroy(void* p)




//////////////////////////////////
// float Sliders

DeclareCycleUpdate(float);
DeclareCycleDestroy(float);


////////////////////////////////////////////////////////////
// int sliders

DeclareCycleUpdate(int); 
DeclareCycleDestroy(int);


//////////////////////////////////
// short Sliders

DeclareCycleUpdate(short); 
DeclareCycleDestroy(short);

//////////////////////////////////
// fixang??? Sliders

DeclareCycleUpdate(fixang); 
DeclareCycleDestroy(fixang);


////////////////////////////////////////////////////////////
// String Slider
//


DeclareCycleUpdate(String);
DeclareCycleDestroy(String);

//
// Destroy funcs
//


void (*PnP_GadgDestroy[])(void *g)=
{
   PnP_TextBoxDestroy,
   PnP_ButtonToggleDestroy,
   PnP_ButtonOneShotDestroy,
   PnP_SliderfloatDestroy,
   PnP_SliderintDestroy,   
   PnP_SlidershortDestroy,
   PnP_SliderfixangDestroy,
   _PnP_PictureDestroy,
   PnP_SliderStringDestroy,
   _pnp_vslider_destroy,
};


#define NUM_GADGDESTROYS (sizeof(PnP_GadgDestroy)/sizeof(PnP_GadgDestroy[0]))

///////////////////////////////////
// Free all current gadgets
void PnP_GadgFree(_PnP_GadgData *thePnP)
{
   int cur;
   while (--thePnP->GadgIdx>=0)
   {
      cur=thePnP->GadgType[thePnP->GadgIdx];
      if ((cur>=0)&&(cur<NUM_GADGDESTROYS))
         (*PnP_GadgDestroy[cur])(thePnP->GadgPtr[thePnP->GadgIdx]);
      else
         Free(thePnP->GadgPtr[thePnP->GadgIdx]);
   }
}

void (*PnP_GadgUpdate[])(void *g, void *data)=
{
   PnP_TextBoxUpdate,
   PnP_ButtonToggleUpdate,
   PnP_ButtonOneShotUpdate,
   PnP_floatSliderUpdate,
   PnP_intSliderUpdate,
   PnP_shortSliderUpdate,
   PnP_fixangSliderUpdate,
   _PnP_PictureUpdate,
   PnP_StringSliderUpdate,
   _pnp_vslider_update,
};

#define NUM_GADGUPDATES (sizeof(PnP_GadgUpdate)/sizeof(PnP_GadgUpdate[0]))

static void _pnp_updateidx(_PnP_GadgData *thePnP, int idx, void *data)
{
   int cur=thePnP->GadgType[idx];
   if ((cur>=0)&&(cur<NUM_GADGUPDATES))
      (*PnP_GadgUpdate[cur])(thePnP->GadgPtr[idx],data);
   else
      Warning(("Hey, trying to update invalid Gadget type %d at %d\n",cur,idx));
}

void PnP_Redraw(int Owner, void *data)
{
   int i;
   if (Owner>=curPnP->GadgOwner) return;
   for (i=0; i<curPnP->GadgIdx; i++)
      if (curPnP->GadgSrc[i]==Owner)
         _pnp_updateidx(curPnP,i,data);
      else if (curPnP->GadgSrc[i]>Owner)
         break; // they are sorted, so we are done
   return;
}

void PnP_FullRedraw(_PnP_GadgData *thePnP)
{
   int i;
   for (i=0; i<thePnP->GadgIdx; i++)
      _pnp_updateidx(thePnP,i,NULL);
}

void PnP_Hide(int Owner, bool which)
{
   int i;
   if (Owner>=curPnP->GadgOwner) return;
   for (i=0; i<curPnP->GadgIdx; i++)
      if (curPnP->GadgSrc[i]==Owner)
         LGadHideBox(VB(curPnP->GadgPtr[i]),which);
      else if (curPnP->GadgSrc[i]>Owner)
         break; // they are sorted, so we are done
   return;
}

void *PnP_GetStringSubGadget(int Owner)
{
   int i;
   if (Owner>=curPnP->GadgOwner) return NULL;
   for (i=0; i<curPnP->GadgIdx; i++)
   {
      if (curPnP->GadgSrc[i]==Owner)
         if (curPnP->GadgType[i]==PnP_REGSLIDERString)
            return (void *)curPnP->GadgPtr[i];
      if (curPnP->GadgSrc[i]>Owner)
         break;
   }
   return NULL;
}
