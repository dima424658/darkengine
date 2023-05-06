/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/pnpslide.cpp,v 1.19 1997/12/22 18:24:19 mahk Exp $

#include <string.h>

#include <lg.h>
#include <gadget.h>
#include <float.h>
#include <keydefs.h>

#include <pnptools.h>
#include <cyclegad.h>
#include <vslider.h>
#include <pnpvslid.h>

extern "C" {
#include <pnptool_.h>
}

// Must be last header
#include <dbmem.h>

EXTERN DrawElement stupid_arrows[];
#pragma off(unreferenced)

#define _set_min_max(tcd,lo,hi,scale,minval,maxval) \
   if (lo==hi) \
   { \
      (tcd)->min  =  minval; \
      (tcd)->max  =  maxval; \
      (tcd)->wrap = TRUE; \
   } \
   else \
   { \
      (tcd)->min  = lo; \
      (tcd)->max  = hi; \
      (tcd)->wrap = FALSE; \
   } \
   (tcd)->delta=scale

#define _unsigned_set_min_max(tcd,lo,hi,scale,maxval) \
   _set_min_max(tcd,lo,hi,scale,0,maxval)

#define _signed_set_min_max(tcd,lo,hi,scale,maxval) \
   _set_min_max(tcd,lo,hi,scale,-(maxval),maxval)

static int _slider_build_name(Rect *space, const char *name, short pad, pnp_vslider_data* data)
{
   DrawElement draw;   
   Rect r = *space;
   short w,h;
   
   // build the name box
   ElementClear(&draw);
   draw.draw_type = DRAWTYPE_TEXT;
   draw.draw_data = (void*)name;   
   ElementSize(&draw,&w,&h);
   w+=pad;
   r.lr.x = r.ul.x;
   r.lr.x += w;
   _pnp_vslider(&r,name,data);
   return w;
}

#define PNP_CYCLE_FLAGS (CYCLE_FOCUS_FLAG|CYCLE_EDIT_FLAG|CYCLE_NOSPACE_FLAG)

////////////////////////////////////////////////////////////
// PNP SLIDER TEMPLATES 
//
// the real generic hacks are here

template <class type> struct CycleData
{
   void (*update)(PnP_SliderOp op, Rect* where, type val, int data); 
   int data; 
   BOOL send_updates;
};

//
// Dumb interface class
//


#define BuildMethods(type) \
class type##Methods { \
public: \
static void* Build(type##Slider* arg) { return VSlider##type##Build(arg);};  \
static CycleGadg* GadgCreate(CycleGadg* g, LGadRoot* r, CycleGadgDesc* desc, type##CycleGadgDesc* typedesc) { return Create##type##CycleGadg(g,r,desc,typedesc); }; \
static void GadgDestroy(CycleGadg* g) { Destroy##type##CycleGadg(g);};\
static int registry_num(void) { return PnP_REGSLIDER##type; } ; \
}; 

//
// Stupid class just to hold my functions
//

#define SLIDER_TEMPLATE template <class type, class DESC, class SLIDER, class METHODS >

#define SLIDER_INSTANCE(type) pnpSlider<type,type##CycleGadgDesc,type##Slider,type##Methods>

SLIDER_TEMPLATE 
struct pnpSlider
{
   CycleGadg _gadg;  // must be castable to a box
   CycleData<type> _data;
   pnp_vslider_data* vdata; 

   int Register(int num = -1);  // register your pnp
   static bool cycle_cb(CycleGadg* gadg, ulong action, eCyclePart part, void* var, CycleGadgDesc* desc); 
   static pnp_vslider_data* build_vslider(void* gadg, type* var, type lo, type hi, float scale);
   static void Update(void* g, void* arg);   

   pnpSlider(Rect* space, const char* name, type lo, type hi, float scale, type* var,
               void(*update)(PnP_SliderOp op, Rect* where, type val, int data), int data, ulong flags, const char* format); 
   ~pnpSlider();
};

////////////////////////////////////////

SLIDER_TEMPLATE bool pnpSlider<type,DESC,SLIDER,METHODS>::cycle_cb(CycleGadg* gadg, ulong action, eCyclePart part,
 void* var, CycleGadgDesc* desc)
{ 
   BOOL retval = TRUE;
   BOOL potent;

   CycleData<type>* cyc = (CycleData<type>*)desc->user_data; 

   if (CYCLE_ACTION_KIND(action) == kCycleActionUpdate)
   {
      if (cyc->send_updates)
      {
         potent = TRUE;
         retval = FALSE;
      }
      else return FALSE;
   }
   else
      potent = part != kCycleCenterPart || CYCLE_ACTION_KIND(action) == kCycleActionKey;
   PnP_SliderOp op = (potent) ? PnP_SliderUpdateOp : PnP_SliderClickOp; 

   if (cyc->update) 
      cyc->update(op,BOX_RECT(VB(gadg)),*(type *)var,cyc->data); 
   return retval; 
}


////////////////////////////////////////

SLIDER_TEMPLATE
 void pnpSlider<type,DESC,SLIDER,METHODS>::Update(void* g, void* arg)
{
   pnpSlider<type,DESC,SLIDER,METHODS>* slide = (pnpSlider<type,DESC,SLIDER,METHODS>*)g;
   CycleGadg *gadg = &slide->_gadg; 
   UpdateCycleGadg(gadg,TRUE); 
   if (arg != NULL)
   {
      TriggerCycleGadg(gadg,CYCLE_MAKE_ACTION(kCycleActionKey,KB_FLAG_DOWN|KEY_ENTER),kCycleCenterPart);
   }
}

#define BuildCycleUpdate(type) \
EXTERN void PnP_##type##SliderUpdate(void *g, void *arg) \
{ \
   SLIDER_INSTANCE(type)::Update(g,arg); \
}

////////////////////////////////////////

SLIDER_TEMPLATE
 pnp_vslider_data* pnpSlider<type,DESC,SLIDER,METHODS>
::build_vslider(void* gadg, type* var, type lo, type hi, float scale)
{
   pnp_vslider_data* vdata = (pnp_vslider_data*)Malloc(sizeof(*vdata)); 
   SLIDER* slider  = (SLIDER*)Malloc(sizeof(*slider));   
   vdata->slider = (anonSlider*)METHODS::Build(slider);
   slider->val = var;                                    
   slider->lo = lo;                                      
   slider->hi = hi;                                      
   slider->scale = scale;                                
   vdata->update_gadg = gadg;                            
   vdata->update_func = Update;
   vdata->update_data = (void*)1;          
   return vdata;                                         
}


////////////////////////////////////////////////////////////

SLIDER_TEMPLATE pnpSlider<type,DESC,SLIDER,METHODS>:: 
pnpSlider(Rect* space, const char* name, type lo, type hi, float scale, type* var,
       void(*update)(PnP_SliderOp op, Rect* where, type val, int data), 
       int data, ulong flags, const char* format)
{
   CycleGadgDesc cdesc = { {0,}, NULL, stupid_arrows, cycle_cb, format};
   DESC fdesc;
   CycleData<type>* cyc = &_data;
   vdata = NULL; 
   int w;   
   CycleGadg* gadg = &_gadg;

   memset(gadg,0,sizeof(*gadg));

   if (flags & PNP_SLIDER_VSLIDE)
   {
      vdata = build_vslider(gadg,var,lo,hi,1.0);
   }
   w=_slider_build_name(space,name,4,vdata);

   // now build the cyclegadg
   cdesc.bounds = *space;
   cdesc.bounds.ul.x += w;
   cdesc.var = var;
   cdesc.user_data = cyc;
   cdesc.flags = PNP_CYCLE_FLAGS;

   if (flags & PNP_SLIDER_CLICKS)
      cdesc.flags |= CYCLE_NONPOTENT_FLAG;
      
   cyc->update = update;
   cyc->data = data;
   cyc->send_updates = FALSE;

   memset(&fdesc,0,sizeof(fdesc));
   _signed_set_min_max(&fdesc,lo,hi,scale,FLT_MAX);
   if (flags & PNP_SLIDER_WRAP)
      fdesc.wrap = TRUE;
   

   gadg = METHODS::GadgCreate(gadg,curPnP->_root,&cdesc,&fdesc);
}


SLIDER_TEMPLATE int pnpSlider<type,DESC,SLIDER,METHODS>::Register(int num)
{ 
   if (num < 0) num = METHODS::registry_num();
   _PnP_Register(&_gadg,num);
   return curPnP->GadgOwner++;
}

#define BuildCycle(type,Type,format) \
EXTERN int PnP_Slider##Type(Rect* space, const char* name, type lo, type hi, type scale, type *var, \
                     void (*update)(PnP_SliderOp op, Rect* where, type val, int data), int data, ulong flags) \
{ \
   SLIDER_INSTANCE(type)* slider; \
   slider = new SLIDER_INSTANCE(type)(space,name,lo,hi,scale,var,update,data,flags,format); \
   return slider->Register();\
}

////////////////////////////////////////

SLIDER_TEMPLATE pnpSlider<type,DESC,SLIDER,METHODS>::~pnpSlider()
{  // Um, this is somewhat of an abstraction violation
   // but theres no current api for ripping the userdata back out of cyclegadgs
   METHODS::GadgDestroy(&_gadg);
   if (vdata) Free(vdata); 

}

#define BuildCycleDestroy(type) \
EXTERN void PnP_Slider##type##Destroy(void* p) \
{ \
     delete (SLIDER_INSTANCE(type)*)p; \
}

//////////////////////////////////
// Instantiate

BuildMethods(float)
BuildCycle(float,Float,"%3.2f")
BuildCycleUpdate(float)
BuildCycleDestroy(float)

BuildMethods(short)
BuildCycle(short,Short,"%5hd")
BuildCycleUpdate(short)
BuildCycleDestroy(short)



BuildMethods(int)
BuildCycle(int,Int,"%d")
BuildCycleUpdate(int)
BuildCycleDestroy(int)

////////////////////////////////////////////////////////////
// String slider
// 

static bool string_cycle_cb(CycleGadg* gadg, ulong action, eCyclePart part,
 void* var, CycleGadgDesc* desc)
{ 
   if (CYCLE_ACTION_KIND(action) == kCycleActionUpdate)
      return FALSE;
   PnP_SliderOp op = PnP_SliderUpdateOp;
   CycleData<int>* cyc = (CycleData<int>*)desc->user_data; 
   if (part == kCycleCenterPart 
       && CYCLE_ACTION_KIND(action) == kCycleActionMouse)
       op = PnP_SliderClickOp;
   if (cyc->update) 
   {
      cyc->update(op,BOX_RECT(VB(gadg)),*(int *)var,cyc->data); 
   }
   return TRUE; 
}

EXTERN int PnP_SliderString(Rect *space, const char *name, int num, char** vals, int *var,
                   void (*update)(PnP_SliderOp op, Rect *where, int val, int data), int data, ulong flags)
{
   CycleGadgDesc cdesc = { {0,}, NULL, stupid_arrows, string_cycle_cb, };
   StringCycleGadgDesc sdesc;
   CycleData<int>* cyc = (CycleData<int>*)Malloc(sizeof(*cyc));
   
   CycleGadg* gadg;
   int w=_slider_build_name(space,name,4,NULL);

   // now build the cyclegadg
   cdesc.bounds = *space;
   cdesc.bounds.ul.x += w;
   cdesc.var = var;
   cdesc.user_data = cyc;
   cdesc.flags = PNP_CYCLE_FLAGS; 
   if (flags & (PNP_SLIDER_CLICKS|PNP_SLIDER_MENU))
      cdesc.flags |= CYCLE_NONPOTENT_FLAG;
   
   memset(&sdesc,0,sizeof(sdesc));
   sdesc.strings = vals;
   sdesc.num = num;
   if (flags & PNP_SLIDER_WRAP)
      sdesc.flags |= STRING_CYCLE_WRAP;
   if (flags & PNP_SLIDER_MENU)
      sdesc.flags |= STRING_CYCLE_MENU;

   cyc->update = update;
   cyc->data = data;

   gadg = CreateStringCycleGadg(NULL,curPnP->_root,&cdesc,&sdesc);
   _PnP_Register(gadg,PnP_REGSLIDERString);
   return curPnP->GadgOwner++;
}

EXTERN void PnP_SliderStringDestroy(void* p)
{
   CycleGadg* gadg = (CycleGadg*)p;
   void* data = StringCycleGadgData(gadg);
   DestroyStringCycleGadg(gadg);
   Free(data);
}

EXTERN void PnP_StringSliderUpdate(void *g, void *data) 
{ 
   CycleGadg *gadg = (CycleGadg *)g;
   UpdateCycleGadg(gadg,TRUE);
}

EXTERN void PnP_StringSliderSetStrings(void* g, char** strings, int num)
{
   CycleGadg* gadg = (CycleGadg *)g;
   RedescribeStringCycleGadg(gadg,strings,num);
}



////////////////////////////////////////////////////////////
// SPECIAL FIXANG GADGET
// 

#define TOFIXANG(x)  (fixang)((x)*FIXANG_PI/180.0)
#define TOFLOAT(x)   ((float)(x)*180.0/FIXANG_PI)

//
// What we want is a float slider that secretly converts to fixang on the way out. 
//

typedef SLIDER_INSTANCE(float) fSlider;

struct DegreeSlider : public fSlider
{
   float fakevar;
   void (*real_update)(PnP_SliderOp op, Rect* where, fixang val, int data); 
   fixang* realvar;
   int realdata;
      
   DegreeSlider(Rect* space, const char* name, fixang lo, fixang hi, float scale, fixang* var,
       void(*update)(PnP_SliderOp op, Rect* where, fixang val, int data), 
       int data, ulong flags);
   static void fake_update(PnP_SliderOp op, Rect* where, float val, int data); 
};

DegreeSlider::DegreeSlider(Rect* space, const char* name, fixang lo, fixang hi, float scale, fixang* var,
       void(*update)(PnP_SliderOp op, Rect* where, fixang val, int data), 
       int data, ulong flags) 
: fakevar(TOFLOAT(*var)),
  realvar(var),
  real_update(update),
  realdata(data),
  fSlider(space,name,TOFLOAT(lo),TOFLOAT(hi),TOFLOAT(scale),&fakevar,fake_update,(int)this,flags,"%3.2f")
{
   // send update messages even when others wouldn't want them.
   //   _data.send_updates = TRUE;
   Update(this,NULL);
}

void DegreeSlider::fake_update(PnP_SliderOp op, Rect* where, float val, int data)
{
   DegreeSlider* s = (DegreeSlider*)data;


   *s->realvar = TOFIXANG(val);

   if (s->real_update != NULL)
      s->real_update(op,where,TOFIXANG(val),s->realdata);
}

EXTERN int PnP_SliderFixang(Rect* space, const char* name, fixang lo, fixang hi, fixang scale, fixang *var, \
                     void (*update)(PnP_SliderOp op, Rect* where, fixang val, int data), int data, ulong flags) 
{ 
   DegreeSlider* slider; 
   slider = new DegreeSlider(space,name,lo,hi,scale,var,update,data,flags); 
   return slider->Register(PnP_REGSLIDERfixang);
}

EXTERN void PnP_SliderfixangDestroy(void* p) 
{  
   delete (DegreeSlider*)p; 
}

EXTERN void PnP_fixangSliderUpdate(void *g, void *arg) 
{ 
   DegreeSlider* s = (DegreeSlider*)g;
   s->fakevar = TOFLOAT(*s->realvar);
   DegreeSlider::Update(g,arg); 
}



