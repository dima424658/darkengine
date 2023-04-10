/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthedg.h,v 1.3 1998/10/16 14:42:14 TOML Exp $
//

#ifndef __AIPTHEDG_H
#define __AIPTHEDG_H

#include <aitype.h>
#include <aipthtyp.h>

#pragma pack(4)
#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPathEdge
//

class cAIPathEdge
{
public:
   tAIPathCellID   cell;             // the cell the AI should be in when heading towards this edge
   tAIPathVertexID vertexA, vertexB; // vertices of the edge we should be heading towards

   void GetPointA(mxs_vector * p) const;
   void GetPointB(mxs_vector * p) const;
   void GetCenter(mxs_vector * p) const;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPTHEDG_H */
