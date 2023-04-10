/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brushgfh.c,v 1.45 2000/02/19 12:27:50 toml Exp $

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <comtools.h>
#include <res.h>
#include <rect.h>
#include <resrect.h>
#include <drawelem.h>
#include <gadget.h>
#include <region.h>
#include <util2d.h>
#include <mprintf.h>
#include <config.h>
#include <cfgdbg.h>
#include <lgsprntf.h>
#include <gcompose.h>
#include <guistyle.h>

#include <uiapp.h>
#include <brushgfh.h>
#include <swappnp.h>
#include <reg.h>
#include <editbr_.h>
#include <editbr.h>
#include <brlist.h>
#include <brinfo.h>
#include <vbrush.h>
#include <cyclegad.h>
#include <brundo.h>
#include <gedit.h>
#include <vslider.h>
#include <status.h>
#include <viewmgr.h>
#include <dragbox.h>
#include <cmdbutts.h>
#include <command.h>
#include <contexts.h>
#include <gfhfrobs.h>

#include <editor.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define NUM_SPATIAL_DIMS 3 // This will not change unless the universe does
#define NUM_COORDS 9

////////////////////////////////////////

static struct _BrushGFH
{
   LGadRoot root;
   CycleGadg coords[NUM_COORDS];
   float vals[NUM_COORDS];
   LGadButton labels[NUM_COORDS];
   
   LGadButtonList menu1_buttons;
   CmdButtonListDesc menu1_desc;

   LGadButtonList menu2_buttons;
   CmdButtonListDesc menu2_desc;

   Rect pnp1area;
   BOOL dualpnps;     // this is hideous

   BOOL pnp2isup;
   Rect pnp2area;     // area for pnp 2
   LGadRoot pnp2root; // its own special root, cause were crazy

   Rect swaparea;
   editBrush* brush;
   BOOL created; 
   ulong coordmask;
} BrushGFH;

////////////////////////////////////////////////////////////
// The Nine Coordinate editors
//////////////////////////////////////////////////////////// 

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
   ConfigSpew("grabstuff",("stuffing %f to axis %d\n",(fix)(val*FIX_UNIT),axis));
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
         *val = brush->ang.el[mod3]*1.0*ANGLE_UNITS/(2*FIXANG_PI);
         break;   
   }
   //   ConfigSpew("grabstuff",("grabbing %f from axis %d\n",(fix)((*val)*FIX_UNIT),axis));
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

#define COORD_DELTA (0.01)
#define ANG_DELTA (1.00)
#define AXIS_IS_ANG(axis) (axis>=6)

////////////////////////////////////////

static bool coord_cycle_call(CycleGadg* gadg, ulong action, eCyclePart part, void* var, CycleGadgDesc* desc)
{
   float* pval = var; 
   int axis = (int)desc->user_data;

   if (CYCLE_ACTION_KIND(action) == kCycleActionUpdate)
   {
      grab_brush_axis(BrushGFH.brush,axis,pval);
      return FALSE;
   }
   
   editUndoStoreStart(BrushGFH.brush);
   stuff_brush_axis(BrushGFH.brush,axis,*pval);
   i_changed_the_brush(BrushGFH.brush,FALSE,FALSE,FALSE);
   editUndoStoreBrush(BrushGFH.brush);
   
   return TRUE;
}

////////////////////////////////////////

#ifdef BITMAP_ARROWS
DrawElement stupid_arrows[] = 
{
   { DRAWTYPE_RES, REF_IMG_LeftArrow,},
   { DRAWTYPE_RES, REF_IMG_RightArrow,},
};
#else
DrawElement stupid_arrows[] = 
{
   { DRAWTYPE_TEXT, "<", 0, 0, 0, BORDER(DRAWFLAG_BORDER_OUTLINE)|FORMAT(DRAWFLAG_FORMAT_RIGHT), },
   { DRAWTYPE_TEXT, ">", 0, 0, 0, BORDER(DRAWFLAG_BORDER_OUTLINE)|FORMAT(DRAWFLAG_FORMAT_LEFT), },
};
#endif 


////////////////////////////////////////

extern void get_scaled_rect(Ref ref, Rect* bounds,Rect* out, Rect* anchor);

void create_coord_cycler(LGadRoot* root, Rect* bounds, CycleGadg* gadg, int axis, float* var, 
                         CycleGadg *next, CycleGadg *prev)
{
   floatCycleGadgDesc fdesc;
   CycleGadgDesc desc;
   BOOL is_ang=(axis / NUM_SPATIAL_DIMS) == ROT_AXES;
   
   memset(&desc,0,sizeof(desc));

   get_scaled_rect(coord_defs[axis].ref,bounds,&desc.bounds,NULL);
   desc.var = var;
   desc.arrow_drawelems = stupid_arrows;
   desc.cb = coord_cycle_call;
   desc.format_string = "%3.2f"; 
   desc.flags = CYCLE_FOCUS_FLAG|CYCLE_EDIT_FLAG|CYCLE_NOSPACE_FLAG;
   desc.next = next;
   desc.prev = prev;
   desc.user_data = (void*)axis;
   if (is_ang)
   {
      fdesc.delta = ANG_DELTA;
      fdesc.min   = 0.0;
      fdesc.max   = 360.0;
      fdesc.wrap  = TRUE;
   }
   else
   {
      fdesc.delta =  COORD_DELTA;
      fdesc.min   = -1000.00;
      fdesc.max   = +1000.00;
      fdesc.wrap  =  FALSE;
   }
   CreatefloatCycleGadg(gadg,root,&desc,&fdesc);
}

////////////////////////////////////////

void destroy_coord_cycler(CycleGadg* gadg)
{
   DestroyfloatCycleGadg(gadg);
}

////////////////////////////////////////////////////////////
// Label Buttons w/ virtual slider mojo
//

static int vs_update_axis = 0;

void label_vs_update(void)
{
   grab_brush_axis(BrushGFH.brush,vs_update_axis,&BrushGFH.vals[vs_update_axis]);
   UpdateCycleGadg(&BrushGFH.coords[vs_update_axis],TRUE);
   gedit_raw_change_brush(BrushGFH.brush,FALSE,FALSE,FALSE);
   vm_render_cameras();
}

////////////////////////////////////////

bool label_button_call(short action, void* data, LGadBox* vb)
{
   if (action & MOUSE_LDOWN)
   {
      editBrush* br = BrushGFH.brush;
      int varnum = (int)LGadBoxGetUserData(vb);
      int axis = varnum%NUM_SPATIAL_DIMS;
      fixangSlider angslider;
      floatSlider fslider;
      void* slidah;
      
      switch (varnum/NUM_SPATIAL_DIMS)
      {
         case ROT_AXES:
            Status("Drag to rotate brush");
            slidah = brushVSsetupAng(&angslider,br,axis);
            break;
         case TRANS_AXES:
            Status("Drag to move brush");
            slidah = brushVSsetupPos(&fslider,br,axis,1.0);
            break;
         case SIZE_AXES:
            Status("Drag to size brush");
            slidah = brushVSsetupSz(&fslider,br,axis,1.0);
            break;
      }
      editUndoStoreStart(br);
      vs_update_axis = varnum;
      gedit_setup_check_lazy_drag();
      if (virtual_slider(1,&slidah,label_vs_update)==VSLIDER_CHANGED)
         i_changed_the_brush(br,FALSE,FALSE,FALSE);   // this will do brush updates as it sees fit
      gedit_finish_check_lazy_drag();
      editUndoStoreBrush(br);
      Status("");
      return TRUE;
   }
   return FALSE;
   
}

////////////////////////////////////////

static void create_label_button(LGadButton* butt, LGadRoot* root, Rect* bounds, int axis)
{
   int idx = coord_defs[axis].ref - REF_RECT_GFHBrushTopL;
   Rect area;
   DrawElement draw;

   get_scaled_rect(MKREF(RES_GFHcoord,idx),bounds,&area,NULL);

   ElementClear(&draw); 
   draw.draw_type = DRAWTYPE_TEXT;
   draw.draw_data = coord_defs[axis].name;
   draw.draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);

   
   LGadCreateButtonArgs(butt,root,area.ul.x,area.ul.y,
                 (short)RectWidth(&area), (short)RectHeight(&area),
                 &draw, label_button_call,0);
   LGadButtonSetFlags(butt,LGADBUTT_FLAG_GETDOWNS);
   LGadBoxSetUserData(VB(butt),(void*)axis);
}

////////////////////////////////////////
// Root draw call. Does nothing.
//

#ifdef ROOT_DRAW
#pragma off(unrefereneced)
static void root_draw_call(void* data, LGadBox* vb)
{
   gr_clear(guiStyleGetColor(NULL,StyleColorBG));
}
#pragma on(unreferenced)
#endif 

////////////////////////////////////////////////////////////
// GFH API FUNCTIONS
////////////////////////////////////////////////////////////

void brushGFH_OpenPnPs(int n);
void brushGFH_ClosePnPs(void);
void brushGFH_UpdatePnPs(GFHUpdateOp op, editBrush *brush);
void set_pnp_mode(int n);

static bool New_Brush_Hack=FALSE;     // so setcur sets it, update does it

void GFHUpdate(GFHUpdateOp op)
{
   int i;
   for (i = 0; i < NUM_COORDS; i++)
   {
      float val = BrushGFH.vals[i];
      bool update = op == GFH_FORCE;
      grab_brush_axis(BrushGFH.brush,i,&BrushGFH.vals[i]);
      if (update || BrushGFH.vals[i] != val)
         UpdateCycleGadg(&BrushGFH.coords[i],TRUE);
   }
   if ((New_Brush_Hack)&&(op==GFH_FRAME))
   {
      set_pnp_mode(PnP_ForBrush(BrushGFH.brush));
      Update_GFHFrobs(GFH_FORCE,BrushGFH.brush);
      New_Brush_Hack=FALSE;
   }
   else
   {
      brushGFH_UpdatePnPs(op,BrushGFH.brush);
      Update_GFHFrobs(op,BrushGFH.brush);
   }
   GFHUpdateSwappable(op,BrushGFH.brush);
}

void GFHRedrawPnPnSuch(void)
{
   New_Brush_Hack=TRUE;
}

////////////////////////////////////////


void GFHSetCurrentBrush(editBrush* brush)
{
   int i;
   
   ConfigSpew("brushset",("Setting gfx brush to %x of type %d\n",brush,brush?brushGetType(brush):666));
   if (!BrushGFH.created)
   {
      Warning(("Setting GFH current brush, but GFH hasn't been created\n"));
      return;
   }

   if (BrushGFH.brush!=brush)
   {
      New_Brush_Hack=TRUE;
      brushGFH_UpdatePnPs(GFH_MINIMAL,NULL);  // to indicate this one is dead
   }
   BrushGFH.brush = brush;

   for (i = 0; i < NUM_COORDS; i++)
   {
      grab_brush_axis(brush,i,&BrushGFH.vals[i]);
      UpdateCycleGadg(&BrushGFH.coords[i],TRUE);
   }
}

////////////////////////////////////////

// some zany little tables to make the next/prev thing
// correspond to actual screen layout instead of internal
// ordering
static int next_table[NUM_COORDS] = { 
   1, 2, 8,
   4, 5, 0,
   3, 6, 7,
};
static int prev_table[NUM_COORDS] = { 
   5, 0, 1,
   6, 3, 4,
   7, 8, 2, 
};

void CreateBrushGFHInternal(Rect* bounds, editBrush* brush)
{
   Rect area, anchor;
   Rect top_reg, bot_reg;
   int pnp = PnP_ForBrush(brush), swap=0;   
   int i;
   struct _BrushGFH* gfh = &BrushGFH;

   AssertMsg(!gfh->created,"Hey, brush GFH was already created");

   LGadSetupSubRoot(&gfh->root,LGadCurrentRoot(),
                    bounds->ul.x,bounds->ul.y,
                    (short)RectWidth(bounds),(short)RectHeight(bounds));
   gfh->coordmask = 0;
   gfh->brush = brush;

   // MAIN PNP AREAS
   config_get_int("pnp",&pnp);
   get_scaled_rect(REF_RECT_GFHModalArea,bounds,&area,&anchor);
   gfh->pnp1area = area;
   if (grd_visible_canvas->bm.w>640)
   {
      gfh->pnp2area.ul.x=0; gfh->pnp2area.ul.y=area.ul.y;
      gfh->pnp2area.lr.x=RectWidth(&area);
      gfh->pnp2area.lr.y=area.ul.y+RectHeight(&area);
      LGadSetupSubRoot(&gfh->pnp2root,LGadCurrentRoot(),
                       bounds->lr.x+4,bounds->ul.y,
                       (short)RectWidth(&area),(short)RectHeight(&area));
      gfh->pnp2isup = FALSE;
      gfh->dualpnps = TRUE;
   }
   else
      gfh->pnp2isup = gfh->dualpnps = FALSE;
   brushGFH_OpenPnPs(pnp);

   // SWAPPABLE AREA
   config_get_int("swap",&swap);
   get_scaled_rect(REF_RECT_GFHSwapArea,bounds,&area,&anchor);
   gfh->swaparea = area;
   GFHOpenSwappable(swap,&gfh->root,&area,brush);

   for (i = 0; i < NUM_COORDS; i++)
   {
      grab_brush_axis(brush,i,&gfh->vals[i]);
      create_coord_cycler(&gfh->root,bounds,&gfh->coords[i],i,&gfh->vals[i],
         &gfh->coords[next_table[i]],&gfh->coords[prev_table[i]]);
      create_label_button(&gfh->labels[i],&gfh->root,bounds,i);
   }
      
   gfh->created = TRUE;
   vBrush_Change_Callback = GFHSetCurrentBrush; 

   get_scaled_rect(REF_RECT_GFH_UndoArea,bounds,&area,&anchor);
   CreateConfigButtonDesc(&gfh->menu1_desc,2,&area,"gfh_menu1",MakePoint(1,2),MakePoint(1,1));
   CreateCmdButtonList(&gfh->menu1_buttons,&gfh->root,&gfh->menu1_desc);
   bot_reg.ul.x=area.ul.x; top_reg.ul.x=area.lr.x+8; top_reg.ul.y=area.ul.y;

   get_scaled_rect(REF_RECT_GFH_DeleteArea,bounds,&area,&anchor);
   CreateConfigButtonDesc(&gfh->menu2_desc,2,&area,"gfh_menu2",MakePoint(1,2),MakePoint(1,1));
   CreateCmdButtonList(&gfh->menu2_buttons,&gfh->root,&gfh->menu2_desc);
   bot_reg.lr.x=area.lr.x; top_reg.lr.x=area.ul.x-8; top_reg.lr.y=area.lr.y;
   
   get_scaled_rect(REF_RECT_GFH_TimeArea,bounds,&area,&anchor);
   bot_reg.ul.y=area.ul.y+1;   bot_reg.lr.y=area.lr.y;
   Create_GFHFrobs(&gfh->root,&top_reg,&bot_reg,brush);
   
   LGadDrawBox(VB(&BrushGFH),NULL);
}
   

////////////////////////////////////////

void CreateBrushGFH(Rect* bounds)
{
   CreateBrushGFHInternal(bounds, blistGet());
}

////////////////////////////////////////

void DestroyBrushGFH(void)
{
   int i;
   struct _BrushGFH* gfh = &BrushGFH;

   brushGFH_ClosePnPs();
   GFHCloseSwappable();
   for (i = 0; i < NUM_COORDS; i++)
   {
      destroy_coord_cycler(&BrushGFH.coords[i]);
      LGadDestroyBox((LGadBox*)&BrushGFH.labels[i],FALSE);
   }
   LGadDestroyButtonList(&gfh->menu1_buttons);
   DestroyConfigButtonDesc(&gfh->menu1_desc);

   LGadDestroyButtonList(&gfh->menu2_buttons);
   DestroyConfigButtonDesc(&gfh->menu2_desc);

   Destroy_GFHFrobs();

   LGadDestroyRoot(&BrushGFH.root);
   if (BrushGFH.dualpnps)
      LGadDestroyRoot(&BrushGFH.pnp2root);
    
   BrushGFH.brush = NULL;
   BrushGFH.created = FALSE;
   vBrush_Change_Callback = NULL;
}           

void GFHSetCoordMask(ulong mask)
{
   //   ulong changes = BrushGFH.coordmask ^ mask;
   int i;
   if (!BrushGFH.created)
    { BrushGFH.coordmask = mask; return; }
   for (i = 0; i < NUM_COORDS; i++)
   {
      bool invis = (mask & (1 << i)) != 0;
      LGadHideBox(VB(&BrushGFH.coords[i]),invis);
      LGadHideBox(VB(&BrushGFH.labels[i]),invis);
   }   
   BrushGFH.coordmask = mask;
}

////////////////////////////////////////

void get_scaled_rect(Ref ref, Rect* bounds,Rect* out, Rect* anchor)
{
   Rect scale = *(Rect*)RefGet(REF_RECT_GFHbounds);
   Rect source = ((RectElem*)RefGet(ref))->area;
   Rect ancr   = ((RectElem*)RefGet(ref))->anchor;
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

////////////////////////////////////////////////////////////
// GFH Commands
////////////////////////////////////////////////////////////

BOOL brushGFH_AlignNTextureUp(void)
{
   return BrushGFH.pnp2isup;
}

// abstract over the pnp system, since we dont have the same root or area
static void brushGFH_OpenPnPs(int n)
{
   if ((n==PNP_TEXTURE)&&BrushGFH.dualpnps)
   {  // do things with PnP 2 here
      BrushGFH.pnp2isup=TRUE;
      GFHOpen_a_PNP(1,PNP_ALIGN,&BrushGFH.pnp2root,&BrushGFH.pnp2area,BrushGFH.brush);
   }
   GFHOpen_a_PNP(0,n,&BrushGFH.root,&BrushGFH.pnp1area,BrushGFH.brush);
}

static void brushGFH_ClosePnPs(void)
{
   if (BrushGFH.pnp2isup)
      GFHClose_a_PNP(1);
   GFHClose_a_PNP(0);
   BrushGFH.pnp2isup=FALSE;
}

static void brushGFH_UpdatePnPs(GFHUpdateOp op, editBrush *brush)
{
   if (BrushGFH.pnp2isup)
      GFHUpdate_a_PNP(1,op,brush);
   GFHUpdate_a_PNP(0,op,brush);
}

// high level call
static void set_pnp_mode(int n)
{
   brushGFH_ClosePnPs();
   brushGFH_OpenPnPs(n);
   //   GFHOpenPNP(n,&BrushGFH.root,&BrushGFH.pnp1area,BrushGFH.brush);
   LGadDrawBox(VB(&BrushGFH.root),NULL);
   if (BrushGFH.pnp2isup)
      LGadDrawBox(VB(&BrushGFH.pnp2root),NULL);
}

static void set_swap_mode(int n)
{
   GFHCloseSwappable();
   GFHOpenSwappable(n,&BrushGFH.root,&BrushGFH.swaparea,BrushGFH.brush);
   LGadDrawBox(VB(&BrushGFH.root),NULL);
}

static void do_pnp_mode(int n)
{
   uiDefer((deferfunc)set_pnp_mode,(void *)n);
}

static void do_swap_mode(int n)
{
   uiDefer((deferfunc)set_swap_mode,(void *)n);
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
   if ((n>=0)&&(n<NUM_COORDS))
   {
      LGadTextBox *tb=&BrushGFH.coords[n].text;
      LGadTextBoxSetCursor(tb,0);
      LGadFocusTextBox(tb);
      LGadTextBoxSetFlag(tb,TEXTBOX_EDIT_BRANDNEW);
      LGadDrawBox(VB(tb),NULL);
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
