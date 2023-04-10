/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#include <csg.h>
#include <bspdata.h>

extern bool post_edge_merge;

extern void dump_edge(PortalPolyEdge *edge);
extern void dump_portal_poly(PortalPolygon *poly);
extern void dump_portal_polyhedron(PortalPolyhedron *ph);

extern void CheckEdge(PortalPolyEdge *edge);
extern void CheckPoly(PortalPolygon *poly, char *str);
extern void CheckPoly2(PortalPolygon *poly, char *str);
extern void CheckPolygonPlane(PortalPolygon *poly, char *str);
extern void CheckPolyInPolyhedron(PortalPolygon *poly, PortalPolyhedron *ph, char *str);
extern void CheckPolyEdgeCount(PortalPolygon *poly, PortalPolyhedron *ph, char *str);
extern void CheckPolyhedron(PortalPolyhedron *ph, char *str);
extern void CheckPolyhedronQuick(PortalPolyhedron *ph, char *str);
