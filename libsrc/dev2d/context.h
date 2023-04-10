// $Header: x:/prj/tech/libsrc/dev2d/RCS/context.h 1.8 1997/01/27 11:20:25 KEVIN Exp $

#ifndef __CONTEXT_H
#define __CONTEXT_H
#include <grd.h>
#include <bitmap.h>

#ifdef __cplusplus
extern "C" {
#endif

#define gr_min(x,y) ((x) < (y) ? (x) : (y))
#define gr_max(x,y) ((x) > (y) ? (x) : (y))

#define gr_init_gc(c) \
do {                                               \
   grs_canvas *__c = (c);                          \
   __c->gc=grd_defgc;                              \
   __c->gc.safe_clip.f.right=fix_make(__c->bm.w,0);\
   __c->gc.safe_clip.f.bot  =fix_make(__c->bm.h,0);\
   __c->gc.clip = __c->gc.safe_clip;               \
} while (0)

/* macros for setting the clipping region of a specified canvas. */
extern void gr_cset_fix_cliprect(grs_canvas* c, fix l, fix t, fix r, fix b);

#define gr_cset_cliprect(c, l, t, r, b) \
      gr_cset_fix_cliprect(c,fix_make(l,0),fix_make(t,0),fix_make(r,0),fix_make(b,0))

#define gr_set_cliprect(l, t, r, b) \
         gr_cset_cliprect(grd_canvas,l,t,r,b)

#define gr_set_fix_cliprect(l, t, r, b) \
         gr_cset_fix_cliprect(grd_canvas,l,t,r,b)

extern void gr_safe_cset_fix_cliprect(grs_canvas* c, fix l, fix t, fix r, fix b);

#define gr_safe_cset_cliprect(canv, l, t, r, b)        \
      gr_safe_cset_fix_cliprect(canv,fix_make(l,0),fix_make(t,0),fix_make(r,0),fix_make(b,0))

#define gr_safe_set_cliprect(l, t, r, b) \
      gr_safe_cset_cliprect(grd_canvas,l,t,r,b)

#define gr_safe_set_fix_cliprect(l, t, r, b) \
      gr_safe_cset_fix_cliprect(grd_canvas,l,t,r,b)

#define gr_init_fix_clip(clip, l, t, r, b)                \
   (clip)->f.sten=NULL, \
   (clip)->f.left=(l), (clip)->f.top=(t), \
   (clip)->f.right=(r),(clip)->f.bot=(b)

#define gr_set_clipmask(t,b,mask) \
   grd_clip.top=(t), grd_clip.bot=(b), \
   grd_clip.sten = (mask)

/* macros for getting parts of the graphic context of the current canvas. */
#define gr_set_fcolor(color) (grd_canvas->gc.fcolor=color)
#define gr_set_fcolor8(col8) \
do {                               \
   switch (grd_bm.type) {          \
   case BMT_FLAT16:                \
   case BMT_BANK16:                \
      gr_set_fcolor(               \
         ((ushort *)pixpal)[col8]);\
      break;                       \
   default:                        \
      gr_set_fcolor(col8);         \
      break;                       \
   }                               \
} while (0)
#define gr_get_fcolor() (grd_canvas->gc.fcolor)
#define gr_set_bcolor(color) (grd_canvas->gc.bcolor=color)
#define gr_get_bcolor() (grd_canvas->gc.bcolor)
#define gr_set_text_attr(attr) (grd_canvas->gc.text_attr=attr)
#define gr_get_text_attr() (grd_canvas->gc.text_attr)

#define gr_set_fill_type(_ft) (grd_canvas->gc.fill_type = _ft)
#define gr_get_fill_type() (grd_canvas->gc.fill_type)

#define gr_set_fill_parm(parm) \
   (grd_canvas->gc.fill_parm=(long)(parm))
#define gr_get_fill_parm() (grd_canvas->gc.fill_parm)

#define gr_cset_clipmask(canvas,t,b,mask) \
   (canvas)->gc.clip.i.top=(t), (canvas)->gc.clip.i.bot=(b), \
   (canvas)->gc.clip.i.sten = (mask)
#define gr_cset_fcolor(canvas,color) ((canvas)->gc.fcolor=color)
#define gr_cget_fcolor(canvas) ((canvas)->gc.fcolor)
#define gr_cset_bcolor(canvas,color) ((canvas)->gc.bcolor=color)
#define gr_cget_bcolor(canvas) ((canvas)->gc.bcolor)
#define gr_cset_font(canvas,fnt) ((canvas)->gc.font=fnt)
#define gr_cget_font(canvas) ((canvas)->gc.font)

/* macros for getting part of the clipping region of the current canvas. */
#define gr_get_cliprect(l,t,r,b) (*(l)=grd_clip.left,*(t)=grd_clip.top, \
   *(r)=grd_clip.right,*(b)=grd_clip.bot)
#define gr_get_fix_cliprect(l,t,r,b) (*(l)=grd_fix_clip.left, \
   *(t)=grd_fix_clip.top,*(r)=grd_fix_clip.right,*(b)=grd_fix_clip.bot)
#define gr_get_clip_l() (grd_clip.left)
#define gr_get_clip_t() (grd_clip.top)
#define gr_get_clip_r() (grd_clip.right)
#define gr_get_clip_b() (grd_clip.bot)
#define gr_get_fclip_l() (grd_fix_clip.left)
#define gr_get_fclip_t() (grd_fix_clip.top)
#define gr_get_fclip_r() (grd_fix_clip.right)
#define gr_get_fclip_b() (grd_fix_clip.bot)

/* How about some handy macros to save and restore a cliprect */
#define gr_save_cliprect(save)  *(save) = grd_clip
#define gr_restore_cliprect(save)  grd_clip = *(save)

/* macros for getting part of the clipping region of a specified canvas. */
#define gr_cget_cliprect(c,l,t,r,b) (\
   *(l)=(c)->gc.clip.i.left,*(t)=(c)->gc.clip.i.top,\
   *(r)=(c)->gc.clip.i.right,*(b)=(c)->gc.clip.i.bot)
#define gr_cget_fix_cliprect(l,t,r,b) (\
   *(l)=(c)->gc.clip.f.left,*(t)=(c)->gc.clip.f.top,\
   *(r)=(c)->gc.clip.f.right,*(b)=(c)->gc.clip.f.bot)
#define gr_cget_clip_l(c) ((c)->gc.clip.i.left)
#define gr_cget_clip_t(c) ((c)->gc.clip.i.top)
#define gr_cget_clip_r(c) ((c)->gc.clip.i.right)
#define gr_cget_clip_b(c) ((c)->gc.clip.i.bot)
#define gr_cget_fclip_l(c) ((c)->gc.clip.f.left)
#define gr_cget_fclip_t(c) ((c)->gc.clip.f.top)
#define gr_cget_fclip_r(c) ((c)->gc.clip.f.right)
#define gr_cget_fclip_b(c) ((c)->gc.clip.f.bot)

#define gr_set_unpack_buf(buf) (grd_unpack_buf = (buf))
#define gr_get_unpack_buf() (grd_unpack_buf)
#define gr_set_unpack_buf_size(buf_size) (grd_unpack_buf_size = (buf_size))
#define gr_get_unpack_buf_size() (grd_unpack_buf_size)
#define gr_set_light_tab(ltab) (grd_light_table = (ltab))
#define gr_get_light_tab() (grd_light_table)
#define gr_set_light_tab_size(ltab_size) (grd_light_table_size = (ltab_size))
#define gr_get_light_tab_size() (grd_light_table_size)

#ifdef __cplusplus
};
#endif
#endif /* !__CTXMAC */
