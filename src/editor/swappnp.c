/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/swappnp.c,v 1.16 2000/02/19 13:11:33 toml Exp $
#include <gcompose.h>

#include <swappnp.h>
#include <userpnp.h>

#include <editbr.h>
#include <brinfo.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// gadget creation/destruction/update methods
////////////////////////////////////////////////////////////

typedef struct methods 
{
   void (*open)(LGadRoot* root, Rect* bounds, editBrush* brush);
   void (*update)(GFHUpdateOp op, editBrush* brush);
   void (*close)(void);
} Methods;

typedef struct methodizer
{
   char* name;
   int max;
   Methods* methods;
   LGadRoot root;
   int which;
} Methodizer;

////////////////////////////////////////////////////////////
// Generic methodizer stuff
////////////////////////////////////////////////////////////

void MethodOpen(Methodizer* m, int  kind, LGadRoot* root, Rect* bounds, editBrush* brush)
{
   if (m->which > 0)
   {
      Warning(("opening %s %d when %d is already open!\n",m->name,kind,m->which));
   }
   
   if (kind >= m->max)
   {
      Warning(("There is no %s #%d\n",m->name,kind));
      return;
   }

   if (m->methods[kind].open != NULL)
   {
      Rect r = { { 0,0},}; 
      
      r.lr = MakePoint(RectWidth(bounds),RectHeight(bounds));
      LGadSetupSubRoot(&m->root,root,bounds->ul.x,bounds->ul.y,
                       (short)RectWidth(bounds),(short)RectHeight(bounds));
      m->methods[kind].open(&m->root,&r,brush);
   }
   m->which = kind;
}

////////////////////////////////////////

void MethodUpdate(Methodizer* m, GFHUpdateOp op,editBrush* brush)
{
   if (m->which < 0)
   {
      Warning(("Updating the null %s\n",m->name));
      return;
   }
   Assrt(m->which < m->max);
   if (m->methods[m->which].update != NULL)
      m->methods[m->which].update(op,brush);
}

////////////////////////////////////////

void MethodClose(Methodizer* m)
{
   if (m->which < 0)
   {
      Warning(("Closing the null %s\n",m->name));
      return;
   }
   Assrt(m->which < m->max);
   if (m->methods[m->which].close != NULL)
   {
      Rect r;
      GUIcompose c;
      m->methods[m->which].close();
      region_abs_rect(&VB(&m->root)->r,BOX_RECT(VB(&m->root)),&r);
      GUIsetup(&c,&r,ComposeFlagClear,GUI_CANV_ANY);
      GUIdone(&c);
      LGadDestroyRoot(&m->root);
   }
   m->which = -1;
}


////////////////////////////////////////////////////////////
// PNP methods
//////////////////////////////////////////////////////////////

#define PnPFunctionList(x) Create_##x##PnP, Update_##x##PnP, Destroy_##x##PnP, 
#define PnPProto(x) \
   EXTERN void Create_##x##PnP(LGadRoot *root, Rect *bounds, editBrush *br); \
   EXTERN void Destroy_##x##PnP(void); \
   EXTERN void Update_##x##PnP(GFHUpdateOp op, editBrush *br)

PnPProto(txt);
PnPProto(grid);
PnPProto(align);
PnPProto(light);
PnPProto(area);
PnPProto(obj);
PnPProto(flow);
PnPProto(room);

Methods pnp_methods[] = 
{
   { PnPFunctionList(txt) },
   { PnPFunctionList(grid) },
   { PnPFunctionList(align) },
   { PnPFunctionList(light) },
   { PnPFunctionList(area) },
   { PnPFunctionList(obj) },
   { PnPFunctionList(flow) },
   { PnPFunctionList(room) },
   { CreateUserPNP, UpdateUserPNP, DestroyUserPNP,}, // user
   { NULL, NULL, NULL }
};

#define NUM_PNPS (sizeof(pnp_methods)/sizeof(pnp_methods[0]))

static Methodizer PNPizer[2] =
{ { "PNP 1", NUM_PNPS, pnp_methods, NULL, -1}, { "PNP 2", NUM_PNPS, pnp_methods, NULL, -1} };

int PnP_ForBrush(editBrush *brush)
{
   if (brush!=NULL)
      switch (brushGetType(brush))
      {
         case brType_TERRAIN:      return PNP_TEXTURE;
         case brType_LIGHT:        return PNP_LIGHT;
         case brType_HOTREGION:    return PNP_AREA;
         case brType_OBJECT:       return PNP_OBJ;
         case brType_FLOW:         return PNP_FLOW;
         case brType_ROOM:         return PNP_ROOM;
      }
   return PNP_NULL;
}

////////////////////////////////////////

void GFHOpen_a_PNP(int which, PNPkind kind, LGadRoot* root, Rect* bounds, editBrush* brush)
{
   MethodOpen(&PNPizer[which],kind,root,bounds,brush);
}

////////////////////////////////////////

void GFHUpdate_a_PNP(int which, GFHUpdateOp op,editBrush* brush)
{
   MethodUpdate(&PNPizer[which],op,brush);
}

////////////////////////////////////////

void GFHClose_a_PNP(int which)
{
   MethodClose(&PNPizer[which]);
}

/// check for am i one of two up
int GFH_PNP_Control_2Up(int me)
{
   int v1=PNPizer[0].which, v2=PNPizer[1].which;
   if ((v1<0)||(v2<0)) return -1;
   if (v1==me) return v2;
   else        return v1;
}

////////////////////////////////////////////////////////////
// SWAPPABLE methods
//////////////////////////////////////////////////////////////

// since swaps are really just PnPs in a different place, for all practical purposes
#define SwapFunctionList(x) Create_##x##Swap, Update_##x##Swap, Destroy_##x##Swap, 
#define SwapProto(x) \
   extern void Create_##x##Swap(LGadRoot *root, Rect *bounds, editBrush *br); \
   extern void Destroy_##x##Swap(void); \
   extern void Update_##x##Swap(GFHUpdateOp op, editBrush *br)

SwapProto(cr);
SwapProto(layout);
SwapProto(scroll);
SwapProto(filter);
SwapProto(test);

Methods swappable_methods[] = 
{
   { SwapFunctionList(cr) },     // cr eate
   { SwapFunctionList(layout) }, // layout
   { SwapFunctionList(scroll) }, // scroll
   { SwapFunctionList(filter) }, // filter
   { SwapFunctionList(test) },   // test
   { NULL, NULL, NULL,},         // user
};

#define NUM_SWAPPABLES (sizeof(swappable_methods)/sizeof(swappable_methods[0]))

static Methodizer Swapizer = { "Swappable", NUM_SWAPPABLES, swappable_methods,};

void GFHOpenSwappable(GFHSwapKind kind, LGadRoot* root, Rect* bounds, editBrush *brush)
{
   MethodOpen(&Swapizer, kind,root,bounds,brush);
}

////////////////////////////////////////

void GFHUpdateSwappable(GFHUpdateOp op, editBrush* brush)
{
   MethodUpdate(&Swapizer,op,brush);
}

////////////////////////////////////////

void GFHCloseSwappable(void)
{
   MethodClose(&Swapizer);
}

////////////////////////////////////////

GFHSwapKind GFHCurrentSwappable(void)
{
   return Swapizer.which;
}
