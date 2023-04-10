// $Header: x:/prj/tech/libsrc/dev2d/RCS/rsdcfg.h 1.1 1997/03/11 10:44:14 KEVIN Exp $
#ifndef __RSDCFG_H
#define __RSDCFG_H

#ifdef __cplusplus
extern "C" {
#endif

#define RCF_ALLOCATE 1

typedef struct gds_rsd_config {
   uchar *buf;
   int size, cap, flags;
} gds_rsd_config;

extern void gd_set_rsd_config(gds_rsd_config *cfg);
extern void gd_get_rsd_config(gds_rsd_config *cfg);

#ifdef __cplusplus
};
#endif

#endif
