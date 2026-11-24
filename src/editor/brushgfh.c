/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brushgfh.c,v 1.45 2000/02/19 12:27:50 toml Exp $

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <lg.h>
#include <res.h>
#include <2d.h>
#include <rect.h>
#include <resrect.h>
#include <gadget.h>
#include <config.h>
#include <darklimit.h>
#include <uiapp.h>
#include <brushgfh.h>
#include <brlist.h>
#include <brundo.h>
#include <command.h>
#include <contexts.h>
#include <editbr.h>
#include <editbr_.h>
#include <editor.h>
#include <gedit.h>
#include <gfhfrobs.h>
#include <imgui_pnp.h>
#include <pnptools.h>
#include <status.h>
#include <swappnp.h>
#include <vbrush.h>
#include <viewmgr.h>

#define NUM_COORDS 9

static struct
{
   Rect bounds;
   Rect pnp;
   Rect align;
   Rect swap;
   editBrush *brush;
   int active_pnp;
   int active_swap;
   int coords[NUM_COORDS];
   int labels[NUM_COORDS];
   float values[NUM_COORDS];
   BOOL dual;
   BOOL created;
   BOOL selection_changed;
   _PnP_GadgData controls;
} BrushGFH;

#define NUM_SPATIAL_DIMS 3 // This will not change unless the universe does
// convert axis to brush structs 0-2 = xyz, 3-5 = dwh, 6-8 = phb

enum 
{
   TRANS_AXES,
   SIZE_AXES,
   ROT_AXES
};

#define ANGLE_UNITS 360

void stuff_brush_axis(editBrush* brush, int axis, float val)
{
   int mod3 = axis %NUM_SPATIAL_DIMS; // har
   int div3 = axis /NUM_SPATIAL_DIMS; // har
   if (brush == NULL)
      return;

   switch(div3)
   {
      case TRANS_AXES:
         brush->pos.el[mod3] = val;
         break;      
      case SIZE_AXES:
         brush->sz.el[mod3] = val/2;
         break;
      case ROT_AXES:
         brush->ang.el[mod3] = (fixang)(val*2*FIXANG_PI/ANGLE_UNITS);
         break;   
   }
}

////////////////////////////////////////

void grab_brush_axis(editBrush* brush, int axis, float* val)
{
   int mod3 = axis %NUM_SPATIAL_DIMS; // har
   int div3 = axis /NUM_SPATIAL_DIMS; // har

   if (brush == NULL)
   {
      *val = 0.0;
      return;
   }

   switch(div3)
   {
      case TRANS_AXES:
         *val = brush->pos.el[mod3];
         break;      
      case SIZE_AXES:
         *val = 2*brush->sz.el[mod3];
         break;
      case ROT_AXES:
         // here we are, do the angles
         *val = brush->ang.el[mod3]*1.0f*ANGLE_UNITS/(2*FIXANG_PI);
         break;   
   }
}

////////////////////////////////////////

typedef struct _CoordDef
{
   char* name;      
   Ref ref;
} CoordDef;

CoordDef coord_defs[] = 
{
   { "X", REF_RECT_GFHBrushTopL,},
   { "Y", REF_RECT_GFHBrushMidL,},
   { "Z", REF_RECT_GFHBrushBotL,},
   { "D", REF_RECT_GFHBrushTopR,},
   { "W", REF_RECT_GFHBrushMidR,},
   { "H", REF_RECT_GFHBrushBotR,},
   { "B", REF_RECT_GFHBrushBotM,},
   { "P", REF_RECT_GFHBrushMidM,},
   { "H", REF_RECT_GFHBrushTopM,},
};

void get_scaled_rect(Ref ref, Rect* bounds,Rect* out, Rect* anchor)
{
   Rect scale;
   RectElem element;
   Rect source, ancr;
   memcpy(&scale, RefGet(REF_RECT_GFHbounds), sizeof(scale));
   memcpy(&element, RefGet(ref), sizeof(element));
   source=element.area; ancr=element.anchor;
   short sw = RectWidth(&scale);
   short sh = RectHeight(&scale);
   short bw = RectWidth(bounds);
   short bh = RectHeight(bounds);
   
   // relativize the source rect
   source.ul.x -= scale.ul.x;
   source.ul.y -= scale.ul.y;
   source.lr.x -= scale.ul.x;
   source.lr.y -= scale.ul.y;

   // grow the rect 1 in all directions
   source.ul.x--;
   source.ul.y--;
   source.lr.x++;
   source.lr.y++;

   // scale source and anchor from scale to bounds
   source.ul.x = source.ul.x*bw/sw;
   source.lr.x = source.lr.x*bw/sw;
   source.ul.y = source.ul.y*bh/sh;
   source.lr.y = source.lr.y*bh/sh;

   ancr.ul.x = ancr.ul.x*bw/sw;
   ancr.lr.x = ancr.lr.x*bw/sw;
   ancr.ul.y = ancr.ul.y*bh/sh;
   ancr.lr.y = ancr.lr.y*bh/sh;

   *out = source;
   if (anchor != NULL)
      *anchor = ancr;
}


static void handle_coord(PnP_SliderOp op, Rect *rect, float value, intptr_t axis)
{
   editBrush *brush = BrushGFH.brush;
   if (!brush || op != PnP_SliderUpdateOp)
      return;
   editUndoStoreStart(brush);
   stuff_brush_axis(brush, (int)axis, value);
   i_changed_the_brush(brush, FALSE, FALSE, FALSE);
   editUndoStoreBrush(brush);
}

static void create_coord(int axis, const Rect *height_reference)
{
   Rect field, label;
   const BOOL angular = axis / NUM_SPATIAL_DIMS == ROT_AXES;
   const float lo = angular ? 0 : -DARK_LIMIT_RUNTIME_WORLD_EXTENT;
   const float hi = angular ? 360 : DARK_LIMIT_RUNTIME_WORLD_EXTENT;
   const float step = angular ? 1.0f : 0.01f;
   const Ref ref = coord_defs[axis].ref;
   get_scaled_rect(ref, &BrushGFH.bounds, &field, NULL);
   get_scaled_rect(MKREF(RES_GFHcoord, ref - REF_RECT_GFHBrushTopL), &BrushGFH.bounds, &label, NULL);
   label.ul.y = field.ul.y;
   BrushGFH.labels[axis] = PnP_DragLabel(&label, coord_defs[axis].name, lo, hi, angular ? 1.0f : 0.25f,
      &BrushGFH.values[axis], handle_coord, axis, angular ? PNP_SLIDER_WRAP : 0);
   PnP_MatchControlHeight(BrushGFH.labels[axis], height_reference);
   BrushGFH.coords[axis] = PnP_SliderFloat(&field, NULL, lo, hi, step, &BrushGFH.values[axis],
      handle_coord, axis, angular ? PNP_SLIDER_WRAP : 0);
   PnP_MatchControlHeight(BrushGFH.coords[axis], height_reference);
}

BOOL brushGFH_AlignNTextureUp(void)
{
   return BrushGFH.dual && BrushGFH.active_pnp == PNP_TEXTURE;
}

const Rect *GFHGetAlignLayoutBounds(void)
{
   return BrushGFH.created && BrushGFH.dual ? &BrushGFH.align : NULL;
}

static void close_pnp(void)
{
   if (brushGFH_AlignNTextureUp())
      GFHClose_a_PNP(1);
   GFHClose_a_PNP(0);
   BrushGFH.active_pnp = PNP_NULL;
}

static void open_pnp(int mode)
{
   if (!BrushGFH.brush && mode != PNP_USER)
      mode = PNP_NULL;
   BrushGFH.active_pnp = mode;
   if (brushGFH_AlignNTextureUp())
      GFHOpen_a_PNP(1, PNP_ALIGN, &BrushGFH.align, BrushGFH.brush);
   GFHOpen_a_PNP(0, mode, &BrushGFH.pnp, BrushGFH.brush);
}

static void set_pnp_mode(int mode)
{
   if (!BrushGFH.created || mode < PNP_TEXTURE || mode > PNP_NULL)
      return;
   close_pnp();
   open_pnp(mode);
}

static void set_swap_mode(int mode)
{
   if (!BrushGFH.created || mode < GFH_SWAP_CREATE || mode > GFH_SWAP_TEST)
      return;
   GFHCloseSwappable();
   BrushGFH.active_swap = mode;
   GFHOpenSwappable(mode, &BrushGFH.swap, BrushGFH.brush);
}

static void update_pnp(GFHUpdateOp op, editBrush *brush)
{
   GFHUpdate_a_PNP(0, op, brush);
   if (brushGFH_AlignNTextureUp())
      GFHUpdate_a_PNP(1, op, brush);
}

void GFHUpdate(GFHUpdateOp op)
{
   int i;
   if (!BrushGFH.created)
      return;

   for (i = 0; i < NUM_COORDS; ++i)
      grab_brush_axis(BrushGFH.brush, i, &BrushGFH.values[i]);

   if (BrushGFH.selection_changed && op == GFH_FRAME)
   {
      set_pnp_mode(PnP_ForBrush(BrushGFH.brush));
      BrushGFH.selection_changed = FALSE;
      op = GFH_FORCE;
   }
   if (!BrushGFH.selection_changed)
      update_pnp(op, BrushGFH.brush);
   Update_GFHFrobs(op, BrushGFH.brush);
   GFHUpdateSwappable(op, BrushGFH.brush);
}

void GFHRedrawPnPnSuch(void)
{
   BrushGFH.selection_changed = TRUE;
}

void GFHSetCurrentBrush(editBrush *brush)
{
   if (!BrushGFH.created || BrushGFH.brush == brush)
      return;
   if (!BrushGFH.selection_changed)
      update_pnp(GFH_MINIMAL, NULL);
   BrushGFH.selection_changed = TRUE;
   BrushGFH.brush = brush;
}

static Rect absolute_area(Ref reference, Rect *bounds)
{
   Rect rect;
   get_scaled_rect(reference, bounds, &rect, NULL);
   rect.ul.x += bounds->ul.x;
   rect.lr.x += bounds->ul.x;
   rect.ul.y += bounds->ul.y;
   rect.lr.y += bounds->ul.y;
   return rect;
}

void CreateBrushGFH(Rect *bounds)
{
   Rect undo, del, time, top, bottom, coord_height;
   int i, mode, swap = GFH_SWAP_CREATE;
   BrushGFH.bounds = *bounds;
   BrushGFH.brush = blistGet();
   BrushGFH.pnp = absolute_area(REF_RECT_GFHModalArea, bounds);
   BrushGFH.swap = absolute_area(REF_RECT_GFHSwapArea, bounds);
   BrushGFH.align = BrushGFH.pnp;
   BrushGFH.align.ul.x = bounds->lr.x + 4;
   BrushGFH.align.lr.x = BrushGFH.align.ul.x + RectWidth(&BrushGFH.pnp);
   BrushGFH.dual = grd_visible_canvas->bm.w > 640;
   BrushGFH.created = TRUE;
   BrushGFH.selection_changed = FALSE;
   vBrush_Change_Callback = GFHSetCurrentBrush;

   PnP_SetOrigin(bounds->ul);
   PnP_GadgStart(&BrushGFH.controls);
   get_scaled_rect(REF_RECT_GFHBrushMidL, bounds, &coord_height, NULL);
   for (i = 0; i < NUM_COORDS; ++i)
   {
      grab_brush_axis(BrushGFH.brush, i, &BrushGFH.values[i]);
      create_coord(i, &coord_height);
   }
   get_scaled_rect(REF_RECT_GFH_UndoArea, bounds, &undo, NULL);
   get_scaled_rect(REF_RECT_GFH_DeleteArea, bounds, &del, NULL);
   get_scaled_rect(REF_RECT_GFH_TimeArea, bounds, &time, NULL);
   PnP_CommandButtons(&undo, "gfh_menu1", 2, MakePoint(1, 2));
   PnP_CommandButtons(&del, "gfh_menu2", 2, MakePoint(1, 2));

   mode = PnP_ForBrush(BrushGFH.brush);
   config_get_int("pnp", &mode);
   open_pnp(mode >= PNP_TEXTURE && mode <= PNP_NULL ? mode : PNP_NULL);
   config_get_int("swap", &swap);
   BrushGFH.active_swap = swap >= GFH_SWAP_CREATE && swap <= GFH_SWAP_TEST ? swap : GFH_SWAP_CREATE;
   GFHOpenSwappable(BrushGFH.active_swap, &BrushGFH.swap, BrushGFH.brush);

   PnP_SetOrigin(bounds->ul);
   top.ul = MakePoint(undo.lr.x + 8, undo.ul.y);
   top.lr = MakePoint(del.ul.x - 8, del.lr.y);
   bottom.ul = MakePoint(undo.ul.x, time.ul.y + 1);
   bottom.lr = MakePoint(del.lr.x, time.lr.y);
   Create_GFHFrobs(&top, &bottom, BrushGFH.brush);
}

void DestroyBrushGFH(void)
{
   close_pnp();
   GFHCloseSwappable();
   Destroy_GFHFrobs();
   PnP_GadgFree(&BrushGFH.controls);
   BrushGFH.brush = NULL;
   BrushGFH.created = FALSE;
   vBrush_Change_Callback = NULL;
}

void GFHSetCoordMask(ulong mask)
{
   int i;
   PnP_SetCurrent(&BrushGFH.controls);
   for (i = 0; i < NUM_COORDS; ++i)
   {
      PnP_Hide(BrushGFH.coords[i], (mask & (1 << i)) != 0);
      PnP_Hide(BrushGFH.labels[i], (mask & (1 << i)) != 0);
   }
}

static void deferred_set_pnp_mode(void *data)
{
   set_pnp_mode((int)(intptr_t)data);
}

static void deferred_set_swap_mode(void *data)
{
   set_swap_mode((int)(intptr_t)data);
}

static void do_pnp_mode(int n)
{
   uiDefer(deferred_set_pnp_mode,(void *)(intptr_t)n);
}

static void do_swap_mode(int n)
{
   uiDefer(deferred_set_swap_mode,(void *)(intptr_t)n);
}

static void set_coord_mask(int n)
{
   GFHSetCoordMask(n);
}

static void coord_edit(char *str)
{
   int n=-1, i;

   if (str==NULL) return;
   if (isdigit(str[0]))
   {
      n=atoi(str);
      if ((n>=0)&&(n<NUM_COORDS))
      {
         for (i=0; i<NUM_COORDS; i++)
         {
            int idx=coord_defs[i].ref - REF_RECT_GFHBrushTopL;
            if (idx==n)
               break;
         }
         n=i;  // if i==NUM_COORDS, will fail < in if below
      }
   }
   else
   {
      int i;
      for (i=0; i<NUM_COORDS; i++)
         if (strnicmp(coord_defs[i].name,str,1)==0)
         {
            n=i;
            break;
         }
   }
   if (n >= 0 && n < NUM_COORDS)
   {
      PnP_SetCurrent(&BrushGFH.controls);
      PnP_FocusControl(BrushGFH.coords[n]);
   }
   else
      Status("Invalid Coordinate");
}

static Command gfh_commands[]=
{
   { "start_pnp",FUNC_INT,do_pnp_mode,"Change the plug-n-play gadget\n",},
   { "start_swap",FUNC_INT,do_swap_mode,"Change the swap gadget\n",},
   { "coord_mask",FUNC_INT,set_coord_mask,"Hide/show some GFH coords\n",},
   { "gfh_coord_edit",FUNC_STRING,coord_edit,"Text edit coord N\n" },
};

void setup_gfh_commands(void)
{
   COMMANDS(gfh_commands,HK_EDITOR);
}
