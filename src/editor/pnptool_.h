/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/pnptool_.h,v 1.4 2000/01/29 13:12:53 adurant Exp $
#pragma once

#ifndef __PNPTOOL__H
#define __PNPTOOL__H

extern _PnP_GadgData *curPnP;

// register codes, for freeing
#define PnP_REGTEXTBOX          0
#define PnP_REGTOGGLE           1
#define PnP_REGONESHOT          2
#define PnP_REGSLIDERfloat      3
#define PnP_REGSLIDERint        4
#define PnP_REGSLIDERshort      5
#define PnP_REGSLIDERfixang     6
#define PnP_REGPICTURE          7
#define PnP_REGSLIDERString     8
#define PnP_REGVSLIDER          9 

EXTERN void _PnP_Register(void* gadg, int type);

#endif // __PNPTOOL__H
