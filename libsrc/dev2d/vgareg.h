/*
 * $RCSfile: vgareg.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:02:21 $
 * 
 * Constants for vga registers.
 *
 * This file is part of the dev2d library.
 */
 
#ifndef __VGAREGS_H
#define __VGAREGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* VGA base video address */
#define VGA_BASE   0xa0000

/* VGA miscellaneous registers */
#define GEN_STAT0  0x3c2
#define GEN_STAT1  0x3da
#define GEN_MSCOUT_R    0x3cc
#define GEN_MSCOUT_W    0x3c2
#define GEN_FEAT_R      0x3ca
#define GEN_FEAT_W      0x3da
#define MR_STAT    0x3da			/* vga status reg */
#define MR_VSYNC   8					/* vertical retrace bit */
#define MR_HSYNC   1             /* horizontal retrace bit */

/* VGA sequencer register stuff */
#define SRX_ADR    0x3c4			/* sequencer address port */
#define SRX_DATA   0x3c5			/* sequencer data port */
#define SR_MAP     2					/* seq map mask reg */
#define SR_MEMORY  4					/* seq memory mode reg */
#define SR4_C4     8					/* chain4 bit */
#define SR4_OE		 4					/* odd/even bit */

/* VGA CRT controller registers */
#define CRX_ADR    0x3d4			/* crt controller address port */
#define CRX_DATA   0x3d5			/* crt controller data */
#define CR_MAX     0x9           /* maximum scan line register. */
#define CR_SAH     0xc				/* screen source address high */
#define CR_SAL     0xd				/* low */
#define CR_OFFSET  0x13				/* crtc offset reg */
#define CR_ULINE   0x14				/* crtc underline register */
#define CR14_DWORD 64				/* dword mode bit */
#define CR_MODE    0x17				/* crtc mode register */
#define CR17_BYTE  64				/* byte mode bit */

/* VGA graphics controller registers */
#define GRX_ADR    0x3ce			/* graphics controller address port */
#define GRX_DATA   0x3cf			/* gc data port */
#define GR_MAP     4					/* gc read map select */
#define GR_MODE    5					/* gc mode register index */
#define GR5_OE     16				/* odd/even bit */
#define GR_MISC    6					/* gc misc register index */
#define GR6_CHAIN  2					/* chain bit */
#define GR6_MODE   1             /* mode bit */

/* VGA attribute controller */
#define ARX_ADR    0x3c0			/* attibute controller address port */
#define ARX_WRITE  0x3c0			/* ar data port */
#define ARX_READ	 0x3c1
#define AR_HPAN    0x13				/* horizontal panning register */
#define AR_PAS     0x20

/* VGA PEL registers. */
#define PEL_RADR   0x3c7         /* PEL address for reads */
#define PEL_WADR   0x3c8         /* PEL address for writes */
#define PEL_DATA   0x3c9         /* PEL data read/write register */

#ifdef __cplusplus
};
#endif
#endif /* !__VGAREGS_H */
