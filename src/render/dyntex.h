/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/dyntex.h,v 1.3 2000/01/29 13:38:43 adurant Exp $
#pragma once

#include <objtype.h>
#include <osystype.h>
#include <label.h>
#include <propface.h>

F_DECLARE_INTERFACE(IDynTexture); 

#undef INTERFACE
#define INTERFACE IDynTexture

struct Location;

DECLARE_INTERFACE_(IDynTexture,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD(ChangeTexture)(ObjID objid, int id1, int id2) PURE;
   STDMETHOD(ChangeTexture)(Location *loc, float rad, int id1, int id2) PURE;
};

void DynTextureCreate();

