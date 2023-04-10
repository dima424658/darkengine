// $Header: x:/prj/tech/libsrc/font/RCS/setup.c 1.1 1997/01/23 14:05:38 KEVIN Exp $
// font information setup function.

#include <fonts.h>

extern gdubm_func flat16_flat8_tluc16_ubitmap;

void font_setup(grs_font_info *fi, grs_font *f)
{
   int type;

   switch (f->id) {
   case FONTT_FLAT8:
      fi->blitter = NULL;
      fi->align_mask = 0;
      fi->align_base = f->pal_id;
      type = BMT_FLAT8;
      break;

   case FONTT_ANTIALIAS16:
      fi->blitter = flat16_flat8_tluc16_ubitmap;
      fi->align_mask = 0;
      fi->align_base = 0;
      type = BMT_FLAT8;
      break;

   case FONTT_MONO:
   default:
      fi->blitter = NULL;
      fi->align_mask = 7;
      fi->align_base = 0;
      type = BMT_MONO;
      break;

   }
   gr_init_bm(&fi->bm, NULL, type, BMF_TRANS, 0, f->h);
   if (fi->blitter==NULL)
      fi->blitter = gd_ubitmap_expose(&fi->bm, 0, 0);
   fi->bm.row = f->w;
   fi->bits = ((uchar *)f) + f->buf;
   fi->off_tab = &(f->off_tab);
   fi->range = f->max - f->min;
}


