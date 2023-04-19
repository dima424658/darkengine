// $Header: x:/prj/tech/libsrc/g2/RCS/tmaps.h 1.3 1997/05/16 09:38:49 KEVIN Exp $

#ifndef __TMAPS_H
#define __TMAPS_H

#include <dev2d.h>
#include <primtab.h>

typedef void (*g2_il_func)(int x, int xf, fix u, fix v);

typedef struct {
	grs_bitmap* bm;
	uchar* p_dest;
	uchar* p_src;

	fix dsrc[2];

	fix du_frac, dv_frac;

	int color;
	int y;
	int mask;
	ushort drow;
	fix i;
	g2_il_func il_func;
	uchar * ltab;

	union {
		gdupix_func* pix_func;
		//gdulin_func* hline_func;
	};
	gdlpix_func* lpix_func;
	union { // coordinate gradients wrt canvas x (dc/dx)
		fix dcx[G2C_NUM_POLY_COORD];
		// first five coords are intensity, texture u, texture v, haze, dryness
		struct {
			fix dix, dux, dvx;
		};
	};
	union { // coordinate gradients wrt canvas y (dc/dy)
		fix dcy[G2C_NUM_POLY_COORD];
		struct {
			fix diy, duy, dvy;
		};
	};
} g2s_tmap_info;

#endif

