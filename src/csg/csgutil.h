/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgutil.h,v 1.2 2000/01/29 12:58:29 adurant Exp $
#pragma once

extern void ChangePh(PortalPolygon *poly, PortalPolyhedron *ph, PortalPolyhedron *dest);
extern PortalPolygon *GetPhNext(PortalPolygon *poly, PortalPolyhedron *ph);
extern void SetPhNext(PortalPolygon *poly, PortalPolyhedron *ph, PortalPolygon *next);
extern PortalPolygon *AddPolyToList(PortalPolygon *list, PortalPolygon *poly, PortalPolyhedron *ph);
extern PortalPolyEdge *AddEdgeToList(PortalPolyEdge *list, PortalPolyEdge *edge);

extern void *PortalMakeEdge(BspVertex *a, BspVertex *b);
extern void *PortalMakePolyhedron(void);
extern void *PortalMakePolygon(void *ph_);
extern void *PortalAddPolygonEdge(void *poly_, void *edge_, void *pe2_);
