/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phpos.h,v 1.7 2000/01/31 09:52:03 adurant Exp $
//
//
//
#pragma once

#ifndef __PHPOS_H
#define __PHPOS_H

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysPos
//
// Physics location, including ref and back pointer to model
//

// @TBD (toml 08-04-97): remove m_ ?
// @TBD (toml 08-04-97): Naming convention on position and ref?

class cPhysModel;

class cPhysPos
{
public:
   Position     m_position;
   Position     m_endposition;
   Position     m_targetposition;
   cPhysModel * m_pModel;                        // owner model
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__PHPOS_H */
