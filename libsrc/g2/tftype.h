// $Header: x:/prj/tech/libsrc/g2/RCS/tftype.h 1.2 1997/05/16 09:38:47 KEVIN Exp $

#ifndef __TFTYPE_H
#define __TFTYPE_H

#include <dev2d.h>
typedef void (tmap_setup_func)(grs_bitmap *bm, void (*)(grs_bitmap *));
typedef tmap_setup_func *ptmap_setup_func;
#endif

