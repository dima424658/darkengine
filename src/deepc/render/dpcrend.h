/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCREND_H
#define __DPCREND_H

EXTERN ObjID g_distPickObj;

EXTERN void DPC_init_object_rend(void);
EXTERN void DPC_term_object_rend(void);
EXTERN void DPC_init_renderer(void);
EXTERN void DPC_term_renderer(void);
EXTERN void DPC_pick_reset(void);

#endif // __DPCREND_H
