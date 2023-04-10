/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrloc.h,v 1.5 1999/04/16 17:03:01 JON Exp $
//
// Ranged combat locations
//

#ifndef __AICBRLOC_H
#define __AICBRLOC_H

#include <aicbrtyp.h>

#include <aipthtyp.h>

#include <comtools.h>
#include <objtype.h>
#include <simtime.h>
#include <wrtype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

typedef struct sAIRangedLocationScorecard
{
   float m_LOFPlus;
   float m_LOFPartialPlus;
   float m_rangePlus[kAIRC_NumRanges];
   float m_damageMax;
   float m_damageMinus;
   float m_coverPlus;
} sAIRangedLocationScorecard;

///////////////////////////////////////////////////////////////////////////////

const float kAIRangedCombatMoveThreshold = 10.; // @TODO: propertize

///////////////////////////////////////////////////////////////////////////////

typedef int eAIRangedCombatCover;

///////////////////////////////////////////////////////////////////////////////

typedef struct sAIRangedLocation
{
   // who am i
   ObjID m_ownerID;  

   // location/obj
   Location m_loc;
   ObjID m_objID;    

   BOOL m_pathcast;
   tAIPathCellID m_cellID;

   // LOF
   eAIRangedCombatProjTestFlags m_LOFFlags;   
   int m_targetSubModel;

   // range
   float m_rangeSq;
   eAIRangedCombatRange m_rangeClass;

   // misc
   int m_damageTaken;
   eAIRangedCombatCover m_cover; 

   // score
   float m_score;
} sAIRangedLocation;

///////////////////////////////////////

class cAIRangedLocation: public sAIRangedLocation
{
public:
   cAIRangedLocation(cAINewRangedSubcombat* pOwner);
   cAIRangedLocation(cAINewRangedSubcombat* pOwner, const Location& loc);
   ~cAIRangedLocation(void);

   void SetLocation(const Location& loc);
   const Location& GetLocation(void) const;

   BOOL TestPathcast(void);
   BOOL GetPathcast(void) const; 
   tAIPathCellID GetCellID(void) const;

   BOOL TestLOF(void);
   BOOL GetLOF(void) const;   // simple test, excludes destroyables & door triggers
   void SetLOF(void);
   int GetTargetSubModel(void) const;

   eAIRangedCombatRange TestRangeClass(void);
   eAIRangedCombatRange GetRangeClass(void) const;
   float GetRangeSq(void) const;

   void Score(void);
   float GetScore(void) const;

protected:
   cAINewRangedSubcombat* m_pOwner;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedLocation::~cAIRangedLocation(void) {SafeRelease(m_pOwner);} 

inline const Location& cAIRangedLocation::GetLocation(void) const {return m_loc;}
inline BOOL cAIRangedLocation::GetPathcast(void) const {return m_pathcast;}
inline tAIPathCellID cAIRangedLocation::GetCellID(void) const {return m_cellID;}
inline BOOL cAIRangedLocation::GetLOF(void) const {return !(m_LOFFlags&kAIRC_ProjBlocked);}
inline int cAIRangedLocation::GetTargetSubModel(void) const {return m_targetSubModel;}
inline eAIRangedCombatRange cAIRangedLocation::GetRangeClass(void) const {return m_rangeClass;}
inline float cAIRangedLocation::GetRangeSq(void) const {return m_rangeSq;}
inline float cAIRangedLocation::GetScore(void) const {return m_score;}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRLOC_H */


