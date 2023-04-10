/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

extern void PortalMergeCells(PortalPolyhedron *ph, PortalPolyhedron *ph2);
#if 0
extern void PortalMergePolys(PortalPolygon *dest, PortalPolygon *src);
extern bool try_edge_merge(PortalPolygon *p, PortalPolyhedron *ph,
    PortalPolyEdge *e1, PortalPolyEdge *e2);
extern bool combination_is_convex(PortalPolygon *dest, PortalPolygon *src);
#endif

extern void emit_merge_cell_polys(PortalPolyhedron *ph);
extern void emit_merge_cell_edges(PortalPolyhedron *ph);
