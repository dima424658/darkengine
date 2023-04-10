/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/r3app.h,v 1.3 2000/01/29 13:21:33 adurant Exp $
#pragma once

#ifndef __R3APP_H
#define __R3APP_H

typedef struct _r3s_context r3s_context;
EXTERN r3s_context *sim_context;

EXTERN void LGAPI r3SysCreate(void);
DEFINE_LG_GUID(IID_Gr3d, 0x26);

#endif // __R3APP_H
