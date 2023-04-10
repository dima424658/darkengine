/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiutils.h,v 1.20 2000/02/28 19:35:19 bfarquha Exp $
//
// Utility functions and wrappers to the outside world
//

#ifndef __AIUTILS_H
#define __AIUTILS_H

#pragma once

#include <dynarray.h>

#include <aitype.h>
#include <aidist.h>
#include <rand.h>
#include <simtime.h>

F_DECLARE_INTERFACE(ITagFile);

///////////////////////////////////////////////////////////////////////////////

inline void ProjectFromLocationOnZPlane(const mxs_vector & startLoc, float distance, floatang angle, mxs_vector * pResult)
{
   pResult->x = startLoc.x + cos(angle.value) * distance;
   pResult->y = startLoc.y + sin(angle.value) * distance;
   pResult->z = startLoc.z;
}

///////////////////////////////////////////////////////////////////////////////

inline float AICompueZAngle(const mxs_vector & fromLoc, const mxs_vector & toLoc)
{
   floatang result(0, 0, AIXYDistance(fromLoc, toLoc), ffabsf(fromLoc.z - toLoc.z));
   return ffabsf(result.value);
}

///////////////////////////////////////////////////////////////////////////////

#define kLineIntersect   0
#define kLineNoIntersect 1
#define kLineIdent       2

int GetXYIntersection(const mxs_vector & line1a, const mxs_vector & line1b,
                      const mxs_vector & line2a, const mxs_vector & line2b,
                      mxs_vector * pResult);

int GetXYSegmentIntersect(const mxs_vector & seg1a, const mxs_vector & seg1b,
                          const mxs_vector & seg2a, const mxs_vector & seg2b,
                          mxs_vector * pResult);

int GetXYLineSegIntersect(const mxs_vector & line1a, const mxs_vector & line1b,
                          const mxs_vector & seg2a, const mxs_vector & seg2b,
                          mxs_vector * pResult);

///////////////////////////////////////////////////////////////////////////////
//
// Object manipulation
//

typedef struct Position Position;

BOOL GetObjPosition(ObjID obj, Position * p);
BOOL GetObjLocation(ObjID obj, mxs_vector * p);
const mxs_vector * GetObjLocation(ObjID obj);
BOOL GetObjFacing(ObjID obj, floatang * pFacing);
void GetObjVelocity(ObjID obj, mxs_vector * p);
void GetObjAngVelocity(ObjID obj, mxs_vector * p);
void SetObjImpulse(ObjID obj, float x, float y, float z, float facing, BOOL rotating = TRUE);

typedef mxs_vector tAIFloorBBox[4];
BOOL AIGetObjFloorBBox(ObjID object, tAIFloorBBox * pBBox, const mxs_vector * pAltLoc = NULL, const mxs_angvec * pAltFac = NULL);

BOOL AIGetPhysSphereZMinMax(ObjID object, float *pMinZ, float *pMaxZ);

///////////////////////////////////////////////////////////////////////////////
//
// Door manipulation
//

enum eAIDoorFlags
{
   kAIDF_BlocksVision = 0x01,
   kAIDF_LargeDoor = 0x02
};

struct sAIDoorInfo
{
   ObjID    obj;
   unsigned flags;
};

const cDynArray<sAIDoorInfo> & AIGetDoors();
void AIUpdateDoors();

///////////////////

BOOL AICanOpenDoor(ObjID ai, ObjID obj);
BOOL AIDoorIsOpen(ObjID obj);
void AIOpenDoor(ObjID ai, ObjID door);
void AICloseDoor(ObjID ai, ObjID door, BOOL all=TRUE);

///////////////////

struct sAIDoorBBox
{
   ObjID        object;
   tAIFloorBBox bbox;
};

void AIGetDoorFloorBBoxes(cDynArray<sAIDoorBBox> *);

///////////////////////////////////////////////////////////////////////////////

#define AIGetTime()       GetSimTime()
#define AIGetFrameTime()  GetSimFrameTime()

///////////////////////////////////////////////////////////////////////////////

#define AIRandom(low, high) \
   RandRange(low, high)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: AIDecide
//
// Most Dark AI decisions are intended to be based on simple comparisons
// of choices with clear tie-breaking rules. In the case of a perfect tie,
// the choice is random. This template function standardizes this process
// for anyone in the AI who needs to make such decisions. To use it, you'll
// simply need to write a sort function.
//
// Note the macro use and voids are here only to allow both Microsoft and
// Watcom to both compile.
//
// The compare function prototype is:
//  int MyCompare(tOption **, tOption **)

typedef int (*tAICompareFunc)(const void *, const void *);

///////////////////////////////////////

#define AIDecide(ppOptions, nOptions, pfnCompare, pTieBreak) \
   _AIDecide(ppOptions, nOptions, ((tAICompareFunc)pfnCompare), (void *)(pTieBreak))

///////////////////////////////////////

template <class tOption, class tCount>
inline
tOption * _AIDecide(tOption **     ppOptions,
                    tCount         nOptions,
                    tAICompareFunc pfnCompare,
                    void  *        pTieBreakVoid)
{
   tOption ** ppFirstBest;
   tOption ** ppLastBest;
   tOption *  pTieBreak = (tOption *) pTieBreakVoid;

   // Quick outs...
   if (!nOptions)
      return NULL;

   if (nOptions == 1)
      return *ppOptions;

   // First, sort the array. The best options will be at the end...
   qsort(ppOptions, nOptions, sizeof(tOption *), pfnCompare);

   ppLastBest = ppOptions + (nOptions - 1);

   // If the current activity is of equal value as the best, just keep on with that
   if (pTieBreak && (*pfnCompare)((const void *)(&pTieBreak), (const void *)(ppLastBest)) == 0)
      return pTieBreak;

   // Now, check if we have equal options...
   ppFirstBest = ppLastBest;

   while (ppFirstBest > ppOptions)
   {
      if ((*pfnCompare)(ppFirstBest - 1, ppLastBest) == 0)
         ppFirstBest--;
      else
         break;
   }

   // If we have equal options, pick one at random
   int range = ppLastBest - ppFirstBest;

   if (range == 0)
      return *ppLastBest;

   return *(ppFirstBest + AIRandom(0, range));
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITimer; consolodation of AI's simple scheduling mechanism
//
// Times are set out as an enum to reduce phase shifting issues somewhat.
//

enum eAITimerPeriod
{
   kAIT_Min = 1,

   kAIT_40Hz   = 25,
   kAIT_20Hz   = 50,
   kAIT_10Hz   = 100,
   kAIT_2Hz    = 500,

   kAIT_1Sec   = 1000,
   kAIT_2Sec   = 2000,
   kAIT_10Sec  = 10000,
   kAIT_20Sec  = 20000,

   kAIT_Infinite = 0x7fffffff,

   // AI elements run 20 by default
   kAIT_Default = kAIT_20Hz,
   kAIT_Max = 0xffffffff,
};

//
// This is a type violation, but a mild one. If you really
// want a nonstandard time, use this to make it
//
#define AICustomTime(t) ((eAITimerPeriod)(t))

class cAITimer
{
public:
   cAITimer(eAITimerPeriod period = kAIT_Default);

   void Set(eAITimerPeriod period);
   void Reset();

   BOOL Expired() const;

   unsigned Remaining() const;

   void Force();
   void Delay(unsigned delay);

   void Save(ITagFile *);
   void Load(ITagFile *);

private:
   unsigned m_Expiration;
   unsigned m_Period;
};

///////////////////////////////////////////////////////////////////////////////
//
// Variant of timer used for "random" activity
//

class cAIRandomTimer
{
public:
   cAIRandomTimer(unsigned low, unsigned high);

   void Set(unsigned low, unsigned high);
   void Reset();

   BOOL Expired() const;

   unsigned Remaining() const;

   void Force();
   void Delay(unsigned delay);

   void Save(ITagFile *);
   void Load(ITagFile *);

private:
   unsigned m_Expiration;
   ushort   m_Low;
   ushort   m_High;
};

///////////////////////////////////////////////////////////////////////////////
//
// cAITimer inline functions
//

inline cAITimer::cAITimer(eAITimerPeriod period)
 : m_Expiration(0),
   m_Period(period)
{
}

///////////////////////////////////////

inline BOOL cAITimer::Expired() const
{
  return (!m_Expiration || AIGetTime() > m_Expiration);
}

///////////////////////////////////////

inline unsigned cAITimer::Remaining() const
{
   if (m_Expiration == kAIT_Infinite)
      return kAIT_Infinite;

   unsigned time = AIGetTime();

   if (!m_Expiration || time > m_Expiration)
      return 0;

   return m_Expiration - time;
}

///////////////////////////////////////

inline void cAITimer::Reset()
{
   m_Expiration = AIGetTime() + m_Period;
}

///////////////////////////////////////

inline void cAITimer::Force()
{
   m_Expiration = 0;
}

///////////////////////////////////////

inline void cAITimer::Set(eAITimerPeriod period)
{
   m_Period     = period;
   m_Expiration = AIGetTime() + m_Period;
}

///////////////////////////////////////

inline void cAITimer::Delay(unsigned delay)
{
   m_Expiration = (delay == kAIT_Infinite) ? kAIT_Infinite : AIGetTime() + delay;
}

///////////////////////////////////////////////////////////////////////////////
//
// cAIRandomTimer inline functions
//

inline cAIRandomTimer::cAIRandomTimer(unsigned low, unsigned high)
 : m_Expiration(0),
   m_Low((ushort)low),
   m_High((ushort)high)
{
}

///////////////////////////////////////

inline BOOL cAIRandomTimer::Expired() const
{
  return (m_High && (!m_Expiration || AIGetTime() > m_Expiration));
}

///////////////////////////////////////

inline unsigned cAIRandomTimer::Remaining() const
{
   if (m_Expiration == kAIT_Infinite)
      return kAIT_Infinite;

   unsigned time = AIGetTime();

   if (!m_Expiration || time > m_Expiration)
      return 0;

   return m_Expiration - time;
}

///////////////////////////////////////

inline void cAIRandomTimer::Reset()
{
   m_Expiration = AIGetTime() + AIRandom(m_Low, m_High);
}

///////////////////////////////////////

inline void cAIRandomTimer::Force()
{
   m_Expiration = 0;
}

///////////////////////////////////////

inline void cAIRandomTimer::Set(unsigned low, unsigned high)
{
   m_Low = (ushort)low;
   m_High = (ushort)high;
   m_Expiration = AIGetTime() + AIRandom(m_Low, m_High);
}

///////////////////////////////////////

inline void cAIRandomTimer::Delay(unsigned delay)
{
   m_Expiration = (delay == kAIT_Infinite) ? kAIT_Infinite : AIGetTime() + delay;
}

///////////////////////////////////////////////////////////////////////////////

void CalcArc(mxs_vector* pDir, float initVelocity, float gravity);

///////////////////////////////////////////////////////////////////////////////

BOOL DoorIsBig(ObjID door);

#endif /* !__AIUTILS_H */
