// $Header: x:/prj/tech/libsrc/dev2d/RCS/canvas.h 1.4 1998/04/28 13:46:38 KEVIN Exp $

#ifndef __CANVAS_H
#define __CANVAS_H
#include <grs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*set_canvas_func)(grs_canvas *f);
extern void gr_set_canvas (grs_canvas *c);
extern int gr_install_set_canvas_callback(set_canvas_func f);
extern void gr_uninstall_set_canvas_callback(int cb_id);
extern void gr_make_canvas (grs_bitmap *bm, grs_canvas *c);
extern void gr_init_canvas (grs_canvas *c, uchar *p, uint type,
                            int w, int h);
extern void gr_init_sub_canvas (grs_canvas *sc, grs_canvas *dc,
                                int x, int y, int w, int h);
extern void gr_close_canvas(grs_canvas *c);
#define gr_close_sub_canvas gr_close_canvas
extern grs_canvas *gr_alloc_canvas (int id, int w, int h);
extern void gr_free_canvas (grs_canvas *c);
extern grs_canvas *gr_alloc_sub_canvas (grs_canvas *c, int x, int y,
                                        int w, int h);
extern void gr_free_sub_canvas (grs_canvas *c);

extern int gr_push_canvas (grs_canvas *c);
extern grs_canvas *gr_pop_canvas (void);

extern void gr_alloc_ytab(grs_canvas *c);
extern void gr_free_ytab(grs_canvas *c);

#ifdef __cplusplus
};
#endif
#endif /* !__CANVAS_H */
