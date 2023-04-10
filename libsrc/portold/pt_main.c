/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 *   $Header: r:/t2repos/thief2/libsrc/portold/pt_main.c,v 1.12 1996/12/20 17:53:31 buzzard Exp $
 *
 *  PORTAL texture mappers
 *
 *  Hooks and outer loops 
 */

#include <lg.h>
#include <fix.h>

#include <pt_asm.h>
#include <ptmap.h>
#include <pt.h>

fix last_u, last_v;
uchar *pt_tmap_ptr;
uchar *pt_clut, *pt_light_table, *pt_tluc_table;
ulong pt_tmap_mask, pt_tmap_row;

uchar pt_arb_size;

int dither;
int pt_preload;
fix pt_light, pt_dlight, pt_toggle;
uchar pt_buffer[16], pt_lit_buffer[16];
fix pt_step_table[2];
int pt_row_table[65];

void (*pt_unlit_8_chain)(void);
void (*pt_pallit_8_chain)(void);
void (*pt_palflat_8_chain)(void);
void (*pt_lit_8_chain)(void);
void (*pt_clut_8_chain)(void);
void (*pt_transp_8_chain)(void);
void (*pt_tluc_8_chain)(void);
void (*pt_generic_8_chain)(void);
void (*pt_generic_8_2_chain)(void);

void (*pt_clut_n_chain)(void);
void (*pt_transp_n_chain)(void);
void (*pt_tluc_n_chain)(void);

void (*pt_func_8)(void);
void (*pt_func_n)(void);
void (*pt_func_8_flat)(void);
void (*pt_func_n_flat)(void);
void (*pt_func_perspective_core)(void);
void (*pt_func_perspective_run)(void);

void ptmap_unlit(grs_bitmap *bm)
{
   if (bm->row == 256) {
      if (bm->w == 256 && bm->h == 256) {
         pt_func_n = pt_unlit_256_n_asm;
         pt_unlit_256_n_setup();
         pt_func_perspective_core = pt_unlit_perspective_unrolled_asm;
      } else {
         pt_func_n = pt_unlit_n_asm;
         pt_unlit_n_setup();
         pt_func_perspective_core = pt_unlit_perspective_core_asm;
      }
      pt_func_8 = pt_unlit_8_asm;
      pt_unlit_8_setup();
      pt_arb_size = 0;
   } else {
      pt_func_n = pt_unlit_arb_n_asm;
      pt_func_8 = pt_unlit_arb_8_asm;
      pt_tmap_row = bm->row;
      pt_arb_size = 1;
      {
         int i;
         int row = bm->row, cur=-(row*16);
         for (i=32-16; i <= 32+16; ++i, cur+=row) {
            pt_row_table[i] = cur;
         }
      }
      pt_func_perspective_core = pt_unlit_perspective_core_asm;
   }

   if (pt_clut) {
      pt_clut_n_chain = pt_func_n;
      pt_func_n = pt_clut_n_asm;
      pt_unlit_8_chain = pt_clut_8_asm;
      pt_clut_8_chain = pt_generic_8_asm;
      pt_func_perspective_core = pt_unlit_perspective_core_asm;
   } else
      pt_unlit_8_chain = pt_preload?pt_generic_preload_8_asm:pt_generic_8_asm;

   pt_func_perspective_run  = pt_unlit_perspective_run_asm;
}

void ptmap_pallit(void)
{
   pt_arb_size = 0;
   if (!pt_clut)
   {
         pt_func_n = pt_pallit_n_asm;
         pt_func_n_flat = pt_palflat_n_asm;

         pt_func_8 = pt_unlit_8_asm;
         pt_unlit_8_chain = pt_pallit_store_8_asm;

         pt_func_8_flat = pt_palflat_8_asm;
         pt_palflat_8_chain = pt_preload ? pt_generic_preload_8_asm : pt_generic_8_asm;

         if (dither)
            pt_func_perspective_core = pt_lit_perspective_core_asm;
         else
            pt_func_perspective_core = pt_pallit_perspective_core_asm;
         pt_func_perspective_run = pt_lit_perspective_run_asm;
   }
   else
   {
      pt_func_n = pt_clut_n_asm;
      pt_clut_n_chain = pt_pallit_n_asm;

      pt_func_8 = pt_unlit_8_asm;
#if 0
      pt_unlit_8_chain = pt_pallit_8_asm;
      pt_pallit_8_chain = pt_clut_8_asm;
      pt_clut_8_chain = pt_generic_8_asm;
#else
      pt_unlit_8_chain = pt_pallit_clut_store_8_asm;
#endif

      pt_func_perspective_core = pt_lit_perspective_core_asm;
      pt_func_perspective_run = pt_lit_perspective_run_asm;
   }

   pt_pallit_n_setup();
   pt_palflat_n_setup();

   pt_unlit_8_setup();
   pt_palflat_8_setup();
}

void ptmap_lit(void)
{
   pt_arb_size = 0;

   pt_func_n = pt_lit_n_asm;

   pt_func_8 = pt_lit_8_asm;
   pt_lit_8_chain = pt_preload ? pt_generic_preload_8_asm : pt_generic_8_asm;

   pt_lit_n_setup();
   pt_lit_8_setup();

   pt_func_perspective_run = pt_lit_perspective_run_asm;
   pt_func_perspective_core = pt_lit_perspective_core_asm;
}

void ptmap_tluc8(void)
{
   pt_arb_size = 0;

   pt_func_n = pt_tluc_n_asm;
   pt_tluc_n_chain = pt_unlit_n_asm;

   pt_func_8 = pt_unlit_8_asm;
   pt_unlit_8_chain = pt_tluc_8_asm;

   pt_unlit_8_setup();
   pt_unlit_n_setup();

   pt_func_perspective_run = pt_unlit_perspective_run_asm;
   pt_func_perspective_core = pt_unlit_perspective_core_asm;
}

void ptmap_transp(void)
{
   bool clut = FALSE;

   pt_arb_size = 0;

   pt_func_n = pt_transp_n_asm;
   pt_transp_n_chain = clut ? pt_clut_n_asm : pt_unlit_n_asm;
   pt_clut_n_chain = pt_unlit_n_asm;
 
   pt_func_8 = pt_unlit_8_asm;
   if (clut) {
      pt_unlit_8_chain = pt_clut_8_asm;
      pt_clut_8_chain = pt_transp_8_asm;
   } else {
      pt_unlit_8_chain = pt_transp_8_asm;
   }

   pt_unlit_8_setup();
   pt_unlit_n_setup();

   pt_func_perspective_run = pt_unlit_perspective_run_asm;
   pt_func_perspective_core = pt_unlit_perspective_core_asm;
}

void ptmap_pallit_tluc8(void)
{
   pt_arb_size = 0;

   pt_func_n = pt_tluc_n_asm;
   pt_tluc_n_chain = pt_pallit_n_asm;

   pt_func_8 = pt_unlit_8_asm;
   pt_unlit_8_chain = pt_pallit_8_asm;
   pt_pallit_8_chain = pt_tluc_8_asm;

   pt_unlit_8_setup();
   pt_pallit_n_setup();

   pt_func_perspective_run  = pt_unlit_perspective_run_asm;
   pt_func_perspective_core = pt_unlit_perspective_core_asm;
}

extern bool poly_lit;
void ptmap_setup(grs_bitmap *bm)
{
   if (grd_canvas->bm.type != BMT_FLAT8)
      Error(1, "Tried to run portal texture mappers in non-flat 8 mode!\n");
   if (bm->type == BMT_TLUC8)
      ptmap_tluc8();
   else if (bm->flags & BMF_TRANS)
      ptmap_transp();
   else if (poly_lit)
      ptmap_lit();
   else {
      ptmap_unlit(bm);
   }
}

