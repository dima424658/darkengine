/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phterr.h,v 1.7 2000/01/31 09:52:10 adurant Exp $
//
//
//
#pragma once

#ifndef __PHTERR_H
#define __PHTERR_H

#include <dlist.h>
#include <dynarray.h>

#include <matrixs.h>

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrPoly
//
// A terrain polygon, identified by cell and plane ID
//

class cPhysTerrPoly;
typedef cDList<cPhysTerrPoly, 0>     cPhysTerrPolyListBase;
typedef cDListNode<cPhysTerrPoly, 0> cPhysTerrPolyBase;

///////////////////////////////////////

class cPhysTerrPoly : public cPhysTerrPolyBase
{
public:
   cPhysTerrPoly(cPhysTerrPoly *pTerrPoly);
   cPhysTerrPoly(int cellID, int polyID);

   int GetCellID(void) const;
   int GetPolyID(void) const;
   int GetPlaneID(void) const;

   const mxs_vector & GetNormal(void) const;
   mxs_real           GetPlaneConstant(void) const;

private:
   int m_cellID;
   int m_polyID;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrVertex
//
// A terrain polygon and vertex info
//
// @Q (toml 08-05-97): is this a good name for this class?

class cPhysTerrVertex : public cPhysTerrPoly
{
public:
   cPhysTerrVertex(cPhysTerrVertex *pTerrVertex);
   cPhysTerrVertex(int cellID, int polyID, int vertexOffset,
                   int vertexInPoly);

   int GetVertexOffset() const;
   int GetVertexInPoly() const;

private:

   int m_vertexOffset;
   int m_vertexInPoly;
};

typedef class cPhysTerrPoly  cPhysTerrPoly;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrPolyList
//
// List of polygon IDs
//

class cPhysTerrPolyList : public cPhysTerrPolyListBase
{
public:
   ~cPhysTerrPolyList(void);
   BOOL Find(int cellID, int polyID, cPhysTerrPoly ** ppTerrPoly) const;
   BOOL GetNormal(mxs_vector & normal, const mxs_vector &sphere_loc, mxs_real sphere_radius) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrPolyArray
//
// Dynamic array of terrain contact lists
//

typedef cDynZeroArray<cPhysTerrPolyList *> cPhysTerrPolyArrayBase;

class cPhysTerrPolyArray : public cPhysTerrPolyArrayBase
{
public:
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrVertex, inline functions
//

inline int cPhysTerrVertex::GetVertexOffset() const
{
   return m_vertexOffset;
}

///////////////////////////////////////

inline int cPhysTerrVertex::GetVertexInPoly() const
{
   return m_vertexInPoly;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrPoly, inline functions
//

inline cPhysTerrPoly::cPhysTerrPoly(cPhysTerrPoly *pTerrPoly)
 : m_cellID(pTerrPoly->GetCellID()),
   m_polyID(pTerrPoly->GetPolyID())
{
}

////////////////////////////////////////   

inline cPhysTerrPoly::cPhysTerrPoly(int cellID, int polyID)
 : m_cellID(cellID),
   m_polyID(polyID)
{
}

///////////////////////////////////////

inline int cPhysTerrPoly::GetCellID(void) const
{
   return m_cellID;
}

///////////////////////////////////////

inline int cPhysTerrPoly::GetPolyID(void) const
{
   return m_polyID;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysTerrVertex
//

inline cPhysTerrVertex::cPhysTerrVertex(int cellID, int polyID, int vertexOffset, int vertexInPoly)
 : cPhysTerrPoly(cellID, polyID),
   m_vertexOffset(vertexOffset),
   m_vertexInPoly(vertexInPoly)
{
}

////////////////////////////////////////

inline cPhysTerrVertex::cPhysTerrVertex(cPhysTerrVertex *pTerrVertex)
 : cPhysTerrPoly(pTerrVertex->GetCellID(), pTerrVertex->GetPolyID()),
   m_vertexOffset(pTerrVertex->GetVertexOffset()),
   m_vertexInPoly(pTerrVertex->GetVertexInPoly())
{
}

////////////////////////////////////////

#pragma pack()

#endif /* !__PHTERR_H */
