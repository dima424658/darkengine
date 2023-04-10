// $Header: x:/prj/tech/libsrc/g2/RCS/fl8sf.h 1.2 1997/05/16 09:38:32 KEVIN Exp $

#ifndef __FL8SF_H
#define __FL8SF_H

extern void trans_8to8_uscale();
extern void trans_8to8_scale();
extern void trans_clut_8to8_uscale();
extern void trans_clut_8to8_scale();
extern void trans_xor_8to8_uscale();
extern void trans_xor_8to8_scale();
extern void trans_solid_8to8_uscale();
extern void trans_solid_8to8_scale();
extern void trans_tluc_8to8_uscale();
extern void trans_tluc_8to8_scale();

extern void opaque_8to8_uscale();
extern void opaque_8to8_scale();
extern void opaque_clut_8to8_uscale();
extern void opaque_clut_8to8_scale();
extern void opaque_xor_8to8_uscale();
extern void opaque_xor_8to8_scale();
extern void opaque_solid_8to8_uscale();
extern void opaque_solid_8to8_scale();
extern void opaque_tluc_8to8_uscale();
extern void opaque_tluc_8to8_scale();

#endif
