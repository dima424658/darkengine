/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthedg.cpp,v 1.3 1998/10/14 22:53:52 TOML Exp $
//
//
//

#include <aitype.h>
#include <aipathdb.h>
#include <aipthedg.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPathEdge
//

void cAIPathEdge::GetPointA(mxs_vector * p) const
{
   *p = g_AIPathDB.GetVertex(vertexA);
}

///////////////////////////////////////

void cAIPathEdge::GetPointB(mxs_vector * p) const
{
   *p = g_AIPathDB.GetVertex(vertexB);
}

///////////////////////////////////////

void cAIPathEdge::GetCenter(mxs_vector * p) const
{
   const mxs_vector & vA = g_AIPathDB.GetVertex(vertexA);
   const mxs_vector & vB = g_AIPathDB.GetVertex(vertexB);
   p->x = (vA.x + vB.x) / 2;
   p->y = (vA.y + vB.y) / 2;
   p->z = (vA.z + vB.z) / 2;
}

///////////////////////////////////////////////////////////////////////////////
