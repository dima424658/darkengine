/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

extern void *PortalPolyhedronAlloc(void);
extern void  PortalPolyhedronFree(void *p);

extern void *PortalPolygonAlloc(void);
extern void  PortalPolygonFree(void *p);

extern void *PortalPolyEdgeAlloc(void);
extern void  PortalPolyEdgeFree(void *p);

extern void *PortalEdgeAlloc(void);
extern void  PortalEdgeFree(void *p);

extern void portalize_mem_init(void);
extern void portalize_mem_reset(void);
