/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodutl.h,v 1.4 2000/01/29 13:32:58 adurant Exp $
//
// Model utility functions
//
#pragma once

#ifndef __PHMODUTL_H
#define __PHMODUTL_H

///////////////////////////////////////////////////////////////////////////////

EXTERN BOOL PhysHasActiveModel(ObjID objID, cPhysModel ** pModel);

///////////////////////////////////////

EXTERN mxs_real PhysGetObjsNorm(cPhysModel * pModel1, tPhysSubModId subModId1,
                                cPhysModel * pModel2, tPhysSubModId subModId2,
                                mxs_vector & normal);

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PHMODUTL_H */
