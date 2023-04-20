/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_csg.cpp,v 1.53 2000/02/19 13:10:43 toml Exp $
// layout: interface to brushes during csg, then support calls, then portalization calls themselves

#include <string.h>

#include <lg.h>
#include <appagg.h>
#include <lgassert.h>
#include <matrixs.h>
#include <config.h>
#include <prof.h>
#include <mprintf.h>

#include <editbr.h>
#include <brlist.h>
#include <brinfo.h>
#include <brrend.h>

#include <csg.h>
#include <portal.h>
#include <media.h>
#include <mediaop.h>
#include <cvtbrush.h>
#include <gedit.h>
#include <ged_csg.h>
#include <csgbrush.h>
#include <objlight.h>
#include <animlit.h>
#include <animlgt.h>
#include <hilight.h>
#include <missrend.h>
#include <roomtowr.h>
#include <editobj.h>  // for now, for full deref/reref
#include <primal.h>
#include <phref.h>
#include <prcniter.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <property.h>
#include <propbase.h>

#include <litprop.h>
#include <textarch.h>
#include <rendprop.h>
#include <status.h>
#include <command.h>
#include <appsfx.h>
#include <undoredo.h>
#include <wrmem.h>
#include <dbfile.h>
#include <dbasemsg.h>
#include <gedmedmo.h>
#include <ged_rmap.h>
#include <doorblok.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//////////////////////////
// interface to csg process itself

// for csg
struct _editBrush *csg_brush[MAX_CSG_BRUSHES];

// interface to CSG, should probably go somewhere else, really
int gedcsgGetFaceTexture(int x, int y)
{
   editBrush *curBrush;
   short tx_id;

   curBrush=csg_brush[x];
   tx_id=curBrush->txs[y].tx_id;
   if (tx_id==-1)
      tx_id=curBrush->tx_id;
   return tx_id;
}

TexInfo *gedcsgGetFaceTexInfo(int x, int y)
{
   TexInfo *this_face;
   editBrush *curBrush;

   curBrush=csg_brush[x];
   this_face=&curBrush->txs[y];
   return this_face;
}

EXTERN BOOL get_uv_align(editBrush *br, int face, mxs_vector *u, mxs_vector *v);

BOOL gedcsgGetFaceTextureAligned(int x, int y, mxs_vector *u, mxs_vector *v)
{
   editBrush *curBrush = csg_brush[x];
   if (curBrush->txs[y].tx_rot == TEXINFO_HACK_ALIGN)
      return get_uv_align(curBrush, y, u, v);
   else
      return FALSE;
}

// for now only on a per texture type basis, i guess
BOOL gedcsgGetFaceTextureSelfLit(int x, int y)
{
   editBrush *curBrush = csg_brush[x];

   short tx_id;
   ObjID text_obj; 

   curBrush=csg_brush[x];
   tx_id=curBrush->txs[y].tx_id;
   if (tx_id==-1)
      tx_id=curBrush->tx_id;
   
   text_obj = GetTextureObj(tx_id); 

   return ObjRenderType(text_obj) == kRenderUnlit;
}

static float tex_scale_override=1.0;
float gedcsgGetFaceScale(int x, int y)
{
   int scale = get_face(x, y)->tx_scale;

   if (scale == 0)
   {
      editBrush *curBrush = csg_brush[x];
      Warning(("0 texture scale for brush %d, face %d.  Overriding to 1.\n", x, y));
      Warning(("  Type %d, br_id %d\n",brushGetType(curBrush),curBrush->br_id));
      return 1.0;
   }
   return scale_pow2int_to_float(scale)*tex_scale_override;
}

/////////////////////////
// tools for csg to inform game systems about problems

static BOOL csg_auto_hilight=TRUE;
static int  csg_hilight_cnt=0;

EXTERN void gedcsgHilightBr(int br_id)
{
   hilight_array[br_id]|=PORTAL_HILIGHT_BIT;
   csg_hilight_cnt++;
}

// setup highlights for portal usage
EXTERN void gedcsgClrHilights(void)
{
   csg_hilight_cnt=0;   
   hilightClearBits(PORTAL_HILIGHT_BIT);
}

EXTERN void gedcsgCheckHilights(void)
{
   if (csg_hilight_cnt && (csg_auto_hilight))
      hilight_active=PORTAL_HILIGHT_BIT;
}

//////////////////////////////////
//////////////////////////////////
// PORTALIZATION
//////////////////////////////////
//////////////////////////////////

#define DONE_PORTAL_SND "portalized_sound"
#define DONE_LIT_SND    "lit_sound"

//////////////////////
// light animation system stuck here for now

#define ON_NOW()             '1', 0
#define OFF_NOW()            '0', 0
#define ON(x)                '1', x, ON_NOW()
#define OFF(x)               '0', x, OFF_NOW()
#define BRIGHT(x)            'b', x
#define WAIT(x)              'w', x
#define CONTINUE()           'c', 0
#define SKIP(x)              's', (x)*255/100
#define STOP()               '.', 0
#define RANDOM_WAIT(x,y)     'r', x, 'w', y
#define RANDOM_BRIGHT(x,y)   'r', x, 'b', y, '1', 0
#define RANDOM_FADE_ON(x,y)  'r', x, '1', y, '1', 0
#define RANDOM_FADE_OFF(x,y) 'r', x, '0', y, '0', 0
// jump is signed, relative to current location (JUMP(0) is infinite loop)
#define JUMP(x)              'j', x

static uchar pulse[] =
{
   ON(40),
   OFF(40),
   CONTINUE()
};

static uchar flicker[] =
{
   RANDOM_BRIGHT(0,240),
   WAIT(5),  // 20 fps
   CONTINUE()
};

uchar *light_script[] =
{
   0,
   pulse,
   flicker
};

#define MAX_LIGHT_SCRIPTS  (sizeof(light_script)/sizeof(light_script[0])+1)

int max_light_scripts = MAX_LIGHT_SCRIPTS;

///////////////////////
// brush media ops stuck here for now

int num_media_ops = 10;
#define NUM_MEDIA num_media_ops

const char *mediaop_names[] =
{
   "fill solid",
   "fill air",
   "fill water",
   "flood",
   "evaporate",
   "solid->water",
   "solid->air",
   "air->solid",
   "water->solid",
   "blockable"
};

MediaOp media_op[MAX_OP] =
{
   // solid
   { MEDIA_SOLID, MEDIA_SOLID, MEDIA_SOLID,
     MEDIA_SOLID, MEDIA_SOLID, MEDIA_SOLID },

   // air
   { MEDIA_AIR, MEDIA_AIR, MEDIA_AIR,
     MEDIA_AIR_PERSIST, MEDIA_AIR_PERSIST, MEDIA_AIR_PERSIST },

   // water
   { MEDIA_WATER, MEDIA_WATER, MEDIA_WATER,
     MEDIA_WATER_PERSIST, MEDIA_WATER_PERSIST, MEDIA_WATER_PERSIST },

   // flood
   { MEDIA_SOLID, MEDIA_WATER, MEDIA_WATER,
     MEDIA_SOLID_PERSIST, MEDIA_WATER_PERSIST, MEDIA_WATER_PERSIST },

   // evaporate
   { MEDIA_SOLID, MEDIA_AIR, MEDIA_AIR,
     MEDIA_SOLID_PERSIST, MEDIA_AIR_PERSIST, MEDIA_AIR_PERSIST },

   // solid->water
   { MEDIA_WATER, MEDIA_AIR, MEDIA_WATER,
     MEDIA_WATER_PERSIST, MEDIA_AIR_PERSIST, MEDIA_WATER_PERSIST },

   // solid->air
   { MEDIA_AIR, MEDIA_AIR, MEDIA_WATER,
     MEDIA_AIR_PERSIST, MEDIA_AIR_PERSIST, MEDIA_WATER_PERSIST },

   // air->solid
   { MEDIA_SOLID, MEDIA_SOLID, MEDIA_WATER,
     MEDIA_SOLID_PERSIST, MEDIA_SOLID_PERSIST, MEDIA_WATER_PERSIST },

   // water->solid
   { MEDIA_SOLID, MEDIA_AIR, MEDIA_SOLID,
     MEDIA_SOLID_PERSIST, MEDIA_AIR_PERSIST, MEDIA_SOLID_PERSIST },

   // blockable -- Notice that we can't get persistant solids from
   // blockable brushes.  This lets us clip doorway blockers by simply
   // adding masonry and so forth and whichwhat around them.
   { MEDIA_SOLID, MEDIA_AIR_PERSIST, MEDIA_WATER_PERSIST,
     MEDIA_SOLID, MEDIA_AIR_PERSIST, MEDIA_WATER_PERSIST }
};

EXTERN void write_cell(FILE *f, PortalCell *p);

BOOL gedcsg_save_world_rep(const char *fname)
{
   FILE *f = fopen(fname, "wb");
   int i;
   if (!f) return FALSE;

   fwrite(&wr_num_cells, sizeof(wr_num_cells), 1, f);
   for (i=0; i < wr_num_cells; ++i)
      write_cell(f, WR_CELL(i));
   fclose(f);
   return TRUE;
}

//////////////////////
// actual calls to build/modify/maintain world rep

// first, some externs which should be in a header file somewhere
EXTERN bool merge_nodes;
EXTERN bool optimize_bsp;
EXTERN void portal_shine_omni_light(int light_index, Location *loc, uchar dynamic);
#ifdef RGB_LIGHTING
EXTERN void portal_set_normalized_color(int rm, int gm, int bm);
#endif

BOOL gedcsg_light_objcast = FALSE;
BOOL gedcsg_light_raycast = FALSE;
BOOL gedcsg_light_quad; // controls all terrain lights, but not object lights

// publicly visible state of the database
BOOL gedcsg_level_valid=TRUE, gedcsg_lighting_ok=FALSE;

// parse a single light brush, can be a sort of auto-auto-portalize, as we add it as we go
BOOL gedcsg_parse_light_brush(editBrush *us, uchar dynamic)
{
   int lt, lt_index;
   float bright = brLight_Bright(us);
#ifdef RGB_LIGHTING
   int r,g,b;

   portal_convert_hsb_to_rgb(&r, &g, &b, brLight_Hue(us), brLight_Saturation(us));
   portal_set_normalized_color(r,g,b);
#endif

   if (brushGetType(us) != brType_LIGHT)
      return FALSE;

   if (brLight_Handle(us) != 0)
      if (is_object_light_defined(brLight_Handle(us) - 1))
         return FALSE;

   clear_surface_cache();

#ifdef RGB_LIGHTING
   {
      rgb_vector temp;
      temp.x = bright * r / 255.0;
      temp.y = bright * g / 255.0;
      temp.z = bright * b / 255.0;
      lt_index = add_object_light(&us->pos, &temp, 0);
   }
#else
   lt_index = add_object_light(&us->pos, bright, 0);
#endif
   brLight_Handle(us) = lt_index + 1;

#if 0
   if (brLight_isSpot(us)) {
      Position pos;
      MakePositionFromVectors(&pos, &us->pos, &us->ang);
      lt = portal_add_spotlight(bright, &pos, brLight_Focal(us), 0);
   } else
#endif
   {
      Location loc;
      MakeLocationFromVector(&loc, &us->pos);

      lt = portal_add_omni_light(bright, 0, &loc, dynamic, 0, 0);
      if (lt > 0)
         portal_shine_omni_light(lt_index, &loc, dynamic);
   }

#if 0
   // Huh?  What's this do? -- sb
   if (bright==255.0)
      LightSetBrightness(lt, brLight_Bright(us));
#endif

   return TRUE;
}

static void gedcsg_clear_light_brush(editBrush *us)
{
   brLight_Handle(us)=0;
}

ObjID *g_pObjCastList = NULL;
int    g_ObjCastListSize;

EXTERN void reset_object_lighting(void);

void gedcsg_relight_level(void)
{
   int light_count=0, obj_light_count, anim_obj_light_count;
   int total_light_count;
   int hIter;
   uchar lighting_type = 0;
   char lit_snd[64];
   editBrush *us;

   if (gedcsg_light_objcast)
      lighting_type = LIGHT_OBJCAST;
   else
   if (!gedcsg_light_raycast)
      lighting_type = LIGHT_QUICK;
   else
      // Weirdly, we only really apply quad lighting if objcast
      // lighting is also on.
      if (gedcsg_light_quad && gedcsg_light_objcast)
         lighting_type |= LIGHT_QUAD;

   mprintf("...let\'s light, eh? (type %x)",lighting_type);
   Status("Lighting....");
   AnimLightClear();
   clear_surface_cache();
   reset_lighting();   // really dont need to do this, if just portalized
   reset_object_lighting();

   if (gedcsg_light_objcast)
   {
      IObjectSystem *pObjSys;
      IProperty *pImmobileProp;
      ObjID obj;

      // Build list of objects to cast against
      Assert_(g_pObjCastList == NULL);

      g_pObjCastList = (ObjID *)malloc(gMaxObjID * sizeof(ObjID));
      g_ObjCastListSize = 0;

      pObjSys = AppGetObj(IObjectSystem);
      pImmobileProp = pObjSys->GetCoreProperty(kImmobilityProperty);

      cConcreteIter iter(pImmobileProp);

      iter.Start();
      while (iter.Next(&obj))
      {
         if (OBJ_IS_CONCRETE(obj))
         {
            if (pObjSys->ObjIsImmobile(obj))
            {
               g_pObjCastList[g_ObjCastListSize] = obj;
               g_ObjCastListSize++;
               Assert_(g_ObjCastListSize < gMaxObjID);
            }
         }
      }
      iter.Stop();

      SafeRelease(pImmobileProp);
      SafeRelease(pObjSys);
   }

   DrBlkUnblockAll();
   brushConvertStart();
   brushConvertEnd();   // ok, this is ultra - dorky, but might work...

   // Is there sunlight?
   const sMissionRenderParams *pMRP = GetMissionRenderParams();
   if (pMRP->use_sun && (gedcsg_light_raycast || gedcsg_light_objcast)) {
      Status("Applying sunlight...");

      int r, g, b;
      int type = 0;
      if (pMRP->sunlight_quad && gedcsg_light_objcast)
         type |= LIGHT_QUAD | LIGHT_OBJCAST;

      portal_convert_hsb_to_rgb(&r, &g, &b, pMRP->sun_h, pMRP->sun_s);
      portal_set_normalized_color(r, g, b);
      portal_shine_sunlight(&pMRP->sunlight_vector, pMRP->sun_b, 0, type);
   }

   Status("Applying brush lights...");
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if (brushGetType(us) == brType_LIGHT)
         if (brushConvertTest(us))
         {
            gedcsg_clear_light_brush(us);  // since we reset object lighting right above this
            if (gedcsg_parse_light_brush(us, lighting_type))
               light_count++;
         }
      us=blistIterNext(hIter);
   }

   DrBlkReblockAll();

   anim_obj_light_count = ObjAnimLightLightLevel(lighting_type);
   obj_light_count = ObjLightLightLevel(lighting_type);
   total_light_count = light_count + obj_light_count + anim_obj_light_count;

   mprintf("...we inserted %d light%s\n", total_light_count,
            total_light_count!=1?"s":"");
   mprintf("   including %d regular from objects, %d animated\n",
           obj_light_count, anim_obj_light_count);

   Status("Done");
   gedit_redraw_3d();
   gedcsg_lighting_ok=TRUE;
   if (config_get_raw(DONE_LIT_SND,lit_snd,64))
      SFX_Play_Raw(SFX_STATIC,NULL,lit_snd);

   if (gedcsg_light_objcast)
   {
      Assert_(g_pObjCastList != NULL);

      free(g_pObjCastList);

      g_pObjCastList = NULL;
      g_ObjCastListSize = 0;
   }
}

EXTERN bool optimize_bsp;
EXTERN void cid_register_brush_planes(void *);
   
void register_brush_planes(void)
{
   editBrush *us;
   void *phedra;
   int hIter;

   brushConvertStart();
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if ((phedra=brushConvertPortal(us))!=NULL)
         cid_register_brush_planes(phedra);
      us=blistIterNext(hIter);
   }
   brushConvertEnd();
}

bool cow_autosaves=TRUE;
static BOOL doing_opt=FALSE;
EXTERN void cid_register_start(void);
EXTERN void fix_tjoints(void);
static bool fixup_tjoints=TRUE;
BOOL doing_opt_pass1=FALSE;

static editBrush hotregion_hack_brush;

void build_level(void)
{
   char portal_snd[64];
   int i=1, hIter;
   editBrush *us;
   void *ph;

   if (!doing_opt)
      gedcsgClrHilights();

   if (!config_get_raw("portalized_sound",portal_snd,64))
      portal_snd[0]='\0';

   FreeWR();     // TODO: ahh, this is a horrible hack - would be nice to put it somewhere real

   if (cow_autosaves)
   {
      Status("Saving out p_portal");
      gedit_editted=TRUE;                  // hack to make sure it doesnt bother saving out the 
      dbSave("p_portal.cow",kFiletypeAll); //   about to be destroyed world rep data
   }

   gedcsg_level_valid = TRUE;
   gedit_editted=FALSE;
   StatusField(SF_FILESTATE," ");
   Status("Converting Brushes....");

   editobjFullDeref();

   cid_register_start();
   // if optimizing, supply CSG with all the brush planes up front,
   // so it can match up the bsp planes from the optimizer
   if (optimize_bsp)
      register_brush_planes();

   init_csg_internal_database();
   csg_num_brushes=0;
   memset(csg_brush,0,sizeof(struct _editBrush *)*MAX_CSG_BRUSHES);

   DrBlkGenerateBrushes();
   brushConvertStart();
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if ((ph=brushConvertPortal(us))!=NULL)
      {
         us->timestamp=i++;
         if (brushGetType(us)==brType_TERRAIN)
            csg_brush[us->br_id] = us;
         else
            csg_brush[us->br_id] = &hotregion_hack_brush;
         cid_insert_brush(us->br_id,ph);
         if (us->br_id>csg_num_brushes)
            csg_num_brushes=us->br_id;
      }
      us=blistIterNext(hIter);      
   }
   brushConvertEnd();
   mprintf("Setup %d brush level (%d brushes total)...\n",i,blistCount());
   csg_num_brushes++;         // it is now max brush id + 1, for looping in csg
   Status("Portalizing....");
   portalize_csg_internal_database();
   if (optimize_bsp && fixup_tjoints) {
      Status("Tjoint Fix....");
      fix_tjoints();
   }

   DrBlkDestroyBrushes();
   ConvertPersistantCells(CELL_CAN_BLOCK_VISION);
   mprintf("Portalized...");
   if ((portal_snd[0]!='\0')&&(config_is_defined(DONE_LIT_SND)))
     SFX_Play_Raw(SFX_STATIC,NULL,portal_snd);
   Status("Lighting...");
   editobjFullReref();
   if (!doing_opt_pass1) 
      gedcsg_relight_level();
   PhysRefSystemClear(); 
   PhysRefSystemResize(); 
   PhysRefSystemRebuild();
   Status("Setting up world database from brushes (water flow, etc.)...");
   GEdMedMoMarkWaterAllBrushes();
   RoomToWRProcess();
   Status("Done");
   gedit_redraw_3d();
   undoClearUndoStack();   // punt any undo info during portalization
   if ((portal_snd[0]!='\0')&&(!config_is_defined(DONE_LIT_SND)))
      SFX_Play_Raw(SFX_STATIC,NULL,portal_snd);
   if (!doing_opt)
      gedcsgCheckHilights();
}

EXTERN bool coplanar;
EXTERN float REAL_EPSILON;

void optimize_level(void)
{
   bool old_merge = merge_nodes;
   bool old_coplanar = coplanar;
   bool old_raycast = gedcsg_light_raycast;

   gedcsgClrHilights();
   doing_opt=TRUE;

   // rebuild level with correct polygons

   coplanar = TRUE;
   optimize_bsp = FALSE;
   gedcsg_light_raycast = FALSE;
   doing_opt_pass1 = TRUE;
   build_level();
   doing_opt_pass1 = FALSE;

   // save world rep to temp file
   gedcsg_save_world_rep("temp.wr");

   mprintf("Computing optimized BSP... this may take a while.\n");

   // shell to csgmerge
   {
      char buf[64];
      sprintf(buf, "csgmerge -e%lg temp.wr", REAL_EPSILON);
      system(buf);
   }

   // turn on optimizations
   optimize_bsp = TRUE;
   merge_nodes = old_merge;
   coplanar = old_coplanar;
   gedcsg_light_raycast = old_raycast;

   build_level();

   mprintf("All future level builds will be \"optimized\", "
        "until you disable 'optimize_bsp'\n");
   mprintf("Rerun 'optimize' for a fully optimized one.\n");

   gedcsgCheckHilights();
   doing_opt=FALSE;
}

extern "C" void UpdateMenuCheckmarks (void);

void set_lighting_mode(int mode)
{
   switch (mode)
   {
      case 0:
      {
         gedcsg_light_raycast = FALSE;
         gedcsg_light_objcast = FALSE;
         mprintf("Lighting mode: Quick\n");
         break;
      }

      case 1:
      {
         gedcsg_light_raycast = TRUE;
         gedcsg_light_objcast = FALSE;
         mprintf("Lighting mode: Raycast\n");
         break;
      }

      case 2:
      {
         gedcsg_light_raycast = FALSE;
         gedcsg_light_objcast = TRUE;
         mprintf("Lighting mode: Objcast\n");
         break;
      }
      
      default:
      {
         Warning(("Unknown lighting mode: %d\n", mode));
         break;
      }
   }
   
   UpdateMenuCheckmarks();
}

///////////////////////////////
// commands

// various csg things which need to be commanded from somewhere, i guess
EXTERN bool split_polys;
EXTERN bool merge_polys;
EXTERN int show_debug;
//EXTERN bool decal_hack;
EXTERN int _watch_light;

Command gedcsg_keys[] =
{
   { "watch_light", VAR_INT, &_watch_light },

   { "optimize", FUNC_VOID, optimize_level, "build optimized portalization" },
   { "portalize", FUNC_VOID, build_level, "recompute world terrain" },
   { "relight_level", FUNC_VOID, gedcsg_relight_level, "relight the level" },
   { "save_wr", FUNC_STRING, gedcsg_save_world_rep,"save world rep to file"},
   { "compress_br_ids", FUNC_VOID, ged_remap_brushes, "compact brush ids" },
   { "merge_node", TOGGLE_BOOL, &merge_nodes, "build faster imperfect level" },
   { "optimize_bsp", TOGGLE_BOOL, &optimize_bsp },
   { "set_lighting_mode", FUNC_INT, set_lighting_mode, "Set lighting mode (0, 1, 2)", },
   { "quad_lighting", TOGGLE_BOOL, &gedcsg_light_quad, "oversampled raycast lighting" },
   { "auto_hilight", TOGGLE_BOOL, &csg_auto_hilight, "automatically hilight any bad brushes after portalization" },
   { "split_polys", TOGGLE_BOOL, &split_polys, "force coplanar poly splits" },
   { "merge_polys", TOGGLE_BOOL, &merge_polys, "merge coplanar polys" },
//   { "decal_hack", TOGGLE_BOOL, &decal_hack },
   { "coplanar", TOGGLE_BOOL, &coplanar, "correctly handle coplanar polys" },
   { "csg_epsilon", VAR_FLOAT, &REAL_EPSILON, "set voodoo epsilon value" },
   { "show_debug", VAR_INT, &show_debug, "set split# to debug, -1 for all #s" },
   { "tex_scale_override", VAR_FLOAT, &tex_scale_override, "set texture scale override factor" },
   { "profile", FUNC_STRING, writeProfile, "write out sProf profile" },
   { "autosaves", TOGGLE_BOOL, &cow_autosaves, "do we autosave exit and p_portal" },
   { "tjoint", TOGGLE_BOOL, &fixup_tjoints, "fix tjoints when portalizing" },
};
   
void gedcsgCommandRegister(void)
{
   COMMANDS(gedcsg_keys, HK_BRUSH_EDIT);
   // horrible hack for hotregion custom media strangeness
   brushZero(&hotregion_hack_brush,PRIMAL_CUBE_IDX);
   brushSetType(&hotregion_hack_brush,brType_HOTREGION);
   if (config_is_defined("no_pportal"))
      cow_autosaves=FALSE;
}
