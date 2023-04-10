/*
 *   $Header: x:/prj/tech/libsrc/g2/RCS/pt_main.c 1.8 1998/04/06 15:42:12 KEVIN Exp $
 *
 *  PORTAL texture mappers
 *
 *  Hooks and outer loops 
 */

#include <lg.h>
#include <fix.h>

#include <tmapd.h>

#include <pt_asm.h>
#include <ptmap.h>
#include <pt.h>

uchar *g2pt_tmap_ptr;
uchar *g2pt_clut, *g2pt_tluc_table;
ulong g2pt_tmap_mask, g2pt_tmap_row;

uchar g2pt_arb_size;
int g2pt16_mask=0; // are we 16 bit?

int g2pt_dither=0;
int g2pt_preload;
fix g2pt_light, g2pt_dlight, g2pt_toggle;
uchar g2pt_buffer[16], g2pt_lit_buffer[16];
fix g2pt_step_table[2];
int g2pt_row_table[65];

void (*g2pt_unlit_8_chain)(void);
void (*g2pt_pallit_8_chain)(void);
void (*g2pt_palflat_8_chain)(void);
void (*g2pt_lit_8_chain)(void);
void (*g2pt_clut_8_chain)(void);
void (*g2pt_transp_8_chain)(void);
void (*g2pt_tluc_8_chain)(void);
void (*g2pt_generic_8_chain)(void);
void (*g2pt_generic_8_2_chain)(void);

void (*g2pt_clut_n_chain)(void);
void (*g2pt_transp_n_chain)(void);
void (*g2pt_tluc_n_chain)(void);

void (*g2pt_func_8)(void);
void (*g2pt_func_n)(void);
void (*g2pt_func_8_flat)(void);
void (*g2pt_func_n_flat)(void);
void (*g2pt_func_perspective_core)(void);
void (*g2pt_func_perspective_run)(void);


#define RECIP_TABLE_SIZE 2048
fix g2pt_reciprocal_table_24[RECIP_TABLE_SIZE+1];
float g2pt_int_table[32];


bool g2pt_span_clip=FALSE;
bool g2pt_project_space=TRUE;

void g2pt_init(void)
{
   int i;
   extern float g2pt_two_to_52_power;
   extern double g2pt_u_offset;
   extern double g2pt_v_offset;

   g2pt_u_offset = g2pt_two_to_52_power;
   g2pt_v_offset = g2pt_two_to_52_power;

   g2pt_reciprocal_table_24[0] = 0x7fffffff;

   for (i=1; i <= RECIP_TABLE_SIZE; ++i)
      g2pt_reciprocal_table_24[i] = fix_make(256,0) / i;

   for (i=0; i < 32; ++i)
      g2pt_int_table[i] = i;

}


static void g2ptmap_unlit(grs_bitmap *bm)
{
   if (POW2(bm)) {
      g2pt_func_n = g2pt_unlit_n_asm;
      g2pt_unlit_n_setup(bm);
      g2pt_func_8 = g2pt_unlit_8_asm;
      g2pt_unlit_8_setup(bm);
      g2pt_func_perspective_core = g2pt_unlit_perspective_core_asm;
      g2pt_arb_size = 0;
   }
#ifdef ARB_SIZE_OK
   else
   {
      g2pt_func_n = g2pt_unlit_arb_n_asm;
      g2pt_func_8 = g2pt_unlit_arb_8_asm;
      g2pt_tmap_row = bm->row;
      g2pt_arb_size = 1;
      {
         int i;
         int row = bm->row, cur=-(row*16);
         for (i=32-16; i <= 32+16; ++i, cur+=row) {
            g2pt_row_table[i] = cur;
         }
      }
      g2pt_func_perspective_core = g2pt_unlit_perspective_core_asm;
   }
#else
   else
      CriticalMsg("Can't perspective map using non power of 2 texture.\n");
#endif

   if (g2pt_clut) {
      g2pt_clut_n_chain = g2pt_func_n;
      g2pt_func_n = g2pt_clut_n_asm;
      g2pt_unlit_8_chain = g2pt_clut_8_asm;
      g2pt_clut_8_chain = g2pt_generic_8_asm;
      g2pt_func_perspective_core = g2pt_unlit_perspective_core_asm;
   } else
      g2pt_unlit_8_chain = g2pt_preload?g2pt_generic_preload_8_asm:g2pt_generic_8_asm;

   g2pt_func_perspective_run  = g2pt_unlit_perspective_run_asm;
}

static void g2ptmap_lit(grs_bitmap *bm)
{
   if (POW2(bm)) {
      g2pt_arb_size = 0;

      g2pt_func_n = g2pt_lit_n_asm;

      g2pt_func_8 = g2pt_lit_8_asm;
      g2pt_lit_8_chain = g2pt_preload ? g2pt_generic_preload_8_asm : g2pt_generic_8_asm;

      g2pt_lit_n_setup(bm);
      g2pt_lit_8_setup(bm);
   }
   else
      CriticalMsg("Can't perspective map using non power of 2 texture.\n");

   g2pt_func_perspective_run = g2pt_lit_perspective_run_asm;
   g2pt_func_perspective_core = g2pt_lit_perspective_core_asm;
}


static void g2ptmap_tluc8(grs_bitmap *bm)
{
   g2pt_arb_size = 0;

   g2pt_func_n = g2pt_tluc_n_asm;
   g2pt_tluc_n_chain = g2pt_unlit_n_asm;

   g2pt_func_8 = g2pt_unlit_8_asm;
   g2pt_unlit_8_chain = g2pt_tluc_8_asm;

   g2pt_unlit_8_setup(bm);
   g2pt_unlit_n_setup(bm);

   g2pt_func_perspective_run = g2pt_unlit_perspective_run_asm;
   g2pt_func_perspective_core = g2pt_unlit_perspective_core_asm;
}

static void g2ptmap_transp(grs_bitmap *bm)
{
   bool clut = FALSE;

   g2pt_arb_size = 0;

   g2pt_func_n = g2pt_transp_n_asm;
   g2pt_transp_n_chain = clut ? g2pt_clut_n_asm : g2pt_unlit_n_asm;
   g2pt_clut_n_chain = g2pt_unlit_n_asm;
 
   g2pt_func_8 = g2pt_unlit_8_asm;
   if (clut) {
      g2pt_unlit_8_chain = g2pt_clut_8_asm;
      g2pt_clut_8_chain = g2pt_transp_8_asm;
   } else {
      g2pt_unlit_8_chain = g2pt_transp_8_asm;
   }

   g2pt_unlit_8_setup(bm);
   g2pt_unlit_n_setup(bm);

   g2pt_func_perspective_run = g2pt_unlit_perspective_run_asm;
   g2pt_func_perspective_core = g2pt_unlit_perspective_core_asm;
}

#if 0
void g2ptmap_pallit(void)
{
   g2pt_arb_size = 0;
   if (!g2pt_clut)
   {
         g2pt_func_n = g2pt_pallit_n_asm;
         g2pt_func_n_flat = g2pt_palflat_n_asm;

         g2pt_func_8 = g2pt_unlit_8_asm;
         g2pt_unlit_8_chain = g2pt_pallit_store_8_asm;

         g2pt_func_8_flat = g2pt_palflat_8_asm;
         g2pt_palflat_8_chain = g2pt_preload ? g2pt_generic_preload_8_asm : g2pt_generic_8_asm;

         if (g2pt_dither)
            g2pt_func_perspective_core = g2pt_lit_perspective_core_asm;
         else
            g2pt_func_perspective_core = g2pt_pallit_perspective_core_asm;
         g2pt_func_perspective_run = g2pt_lit_perspective_run_asm;
   }
   else
   {
      g2pt_func_n = g2pt_clut_n_asm;
      g2pt_clut_n_chain = g2pt_pallit_n_asm;

      g2pt_func_8 = g2pt_unlit_8_asm;
#if 0
      g2pt_unlit_8_chain = g2pt_pallit_8_asm;
      g2pt_pallit_8_chain = g2pt_clut_8_asm;
      g2pt_clut_8_chain = g2pt_generic_8_asm;
#else
      g2pt_unlit_8_chain = g2pt_pallit_clut_store_8_asm;
#endif

      g2pt_func_perspective_core = g2pt_lit_perspective_core_asm;
      g2pt_func_perspective_run = g2pt_lit_perspective_run_asm;
   }

   g2pt_pallit_n_setup();
   g2pt_palflat_n_setup();

   g2pt_unlit_8_setup();
   g2pt_palflat_8_setup();
}

void g2ptmap_pallit_tluc8(void)
{
   g2pt_arb_size = 0;

   g2pt_func_n = g2pt_tluc_n_asm;
   g2pt_tluc_n_chain = g2pt_pallit_n_asm;

   g2pt_func_8 = g2pt_unlit_8_asm;
   g2pt_unlit_8_chain = g2pt_pallit_8_asm;
   g2pt_pallit_8_chain = g2pt_tluc_8_asm;

   g2pt_unlit_8_setup();
   g2pt_pallit_n_setup();

   g2pt_func_perspective_run  = g2pt_unlit_perspective_run_asm;
   g2pt_func_perspective_core = g2pt_unlit_perspective_core_asm;
}
#endif

static void g2ptmap_unlit16(grs_bitmap *bm)
{
   g2pt_func_perspective_core = g2pt_unlit_perspective_core_asm;
   g2pt_func_perspective_run  = g2pt_unlit_perspective_run_asm;
   g2pt_arb_size = 0;
   g2pt16_mask = 0xffffffff;

   if (bm->type == BMT_FLAT8) {
      pixpal = (void *)grd_pal16_list[bm->align];

      g2pt_func_8 = g2pt_unlit_8_asm;
      g2pt_unlit_8_setup(bm);

      if (bm->flags & BMF_TRANS) {
         g2pt_unlit_8_chain = g2pt816_transp_8_asm;
         g2pt_func_n = g2pt816_transp_n_asm;
         g2pt_unlit_n_setup(bm);
         g2pt_transp_n_chain = g2pt_unlit_n_asm;
      } else {
         g2pt_unlit_8_chain = g2pt_pal816_8_asm;
         g2pt_func_n = g2pt816_unlit_n_asm;
         g2pt816_unlit_n_setup(bm);
      }
   } else {
      g2pt_func_n = g2pt16_unlit_n_asm;
      g2pt_func_8 = g2pt16_unlit_8_asm;
      g2pt_unlit_8_chain = g2pt16_generic_8_asm;

      g2pt16_unlit_n_setup(bm);
      g2pt16_unlit_8_setup(bm);
   }
}

static void g2ptmap_lit16(grs_bitmap *bm)
{
   g2pt_func_n = g2pt16_lit_n_asm;
   g2pt_func_8 = g2pt16_lit_8_asm;
   g2pt_func_perspective_core = g2pt_lit_perspective_core_asm;
   g2pt_lit_8_chain = g2pt16_generic_8_asm;

   g2pt16_lit_n_setup(bm);
   g2pt16_lit_8_setup(bm);

   g2pt_func_perspective_run  = g2pt_lit_perspective_run_asm;
   g2pt_arb_size = 0;
   g2pt16_mask = 0xffffffff;
}

// The following are the "portal" interface to the perspective mappers.
// These funcs are called when the client has already set up the gradients
// for the poly and we just need to rasterize

extern BOOL g2pt_poly_lit;
static int g2ptmap_setup8(grs_bitmap *bm)
{
   if (bm->type == BMT_TLUC8)
      g2ptmap_tluc8(bm);
   else if (bm->flags & BMF_TRANS)
      g2ptmap_transp(bm);
   else if (g2pt_poly_lit)
      g2ptmap_lit(bm);
   else {
      g2ptmap_unlit(bm);
   }
   g2pt16_mask = 0;
   return G2PTC_OK;
}

static int g2ptmap_setup16(grs_bitmap *bm)
{
   if (g2pt_poly_lit) {
      if (bm->type == BMT_FLAT8)
         g2ptmap_lit16(bm);
      else
         return G2PTC_FAIL;
   } else
      g2ptmap_unlit16(bm);

   return G2PTC_OK;
}


int g2ptmap_setup(grs_bitmap *bm)
{
   switch (grd_bm.type) {
   case BMT_FLAT8:
      return g2ptmap_setup8(bm);
   case BMT_FLAT16:
      return g2ptmap_setup16(bm);
   default:
      Warning(("portal mappers called with unsupported canvas_type"));
   }
   return G2PTC_FAIL;
}



// These are the regular g2 entry points.

extern int g2pt_unlit_perspective_poly(int n, g2s_point **vp);
extern int g2pt_lit_perspective_poly(int n, g2s_point **vp);

void g2ptmap_setup_unlit(grs_bitmap *bm)
{
   g2d_pp.u_scale = bm->w * 65536.0;
   g2d_pp.v_scale = bm->h * 65536.0;
   g2d_pp.poly_func = g2pt_unlit_perspective_poly;

   if (gr_get_fill_type()==FILL_CLUT)
      g2pt_clut = (uchar *)gr_get_fill_parm();
   else
      g2pt_clut = NULL;

   g2ptmap_unlit(bm);
   g2pt16_mask = 0;
}

void g2ptmap_setup_lit(grs_bitmap *bm)
{
   g2d_pp.u_scale = bm->w * 65536.0;
   g2d_pp.v_scale = bm->h * 65536.0;
   g2d_pp.poly_func = g2pt_lit_perspective_poly;

//   if (gr_get_fill_type()==FILL_CLUT)
//      g2pt_clut = (uchar *)gr_get_fill_parm();

   g2ptmap_lit(bm);
   g2pt16_mask = 0;
}


void g2ptmap_setup_unlit16(grs_bitmap *bm)
{
   g2d_pp.u_scale = bm->w * 65536.0;
   g2d_pp.v_scale = bm->h * 65536.0;
   g2d_pp.poly_func = g2pt_unlit_perspective_poly;

   g2pt_clut = NULL;

   g2ptmap_unlit16(bm);
}


void g2ptmap_setup_lit16(grs_bitmap *bm)
{
   g2d_pp.u_scale = bm->w * 65536.0;
   g2d_pp.v_scale = bm->h * 65536.0;
   g2d_pp.poly_func = g2pt_lit_perspective_poly;

   g2pt_clut = NULL;

   g2ptmap_lit16(bm);
}
