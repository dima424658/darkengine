/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motschem.h,v 1.6 2000/01/31 09:51:03 adurant Exp $
#pragma once

//
// This header contains class interface for motion schemas and 
// motion schema sets.
//
// They are the return values from queries to the motion database, and can
// be used to generate motion plans.
//

#ifndef __MOTSCHEM_H
#define __MOTSCHEM_H

#include <dynarray.h>
#include <tagfile.h>
#include <objtype.h>
#include <label.h>
#include <motset.h> // for sMotStuff
#include <namemap.h>
#include <motdbtyp.h>


#define kMotSchemaID_Invalid -1

#define kMSchFlag_ArchObjSwizzle 0x1
#define kMSchFlag_FixedDuration  0x2
#define kMSchFlag_TimeWarp       0x4
#define kMSchFlag_Stretch        0x8
#define kMSchFlag_FixedDist      0x10

// these are the atomic units of the motion database, and the thing from
// which the motion system builds its plans.
// It's up to the app to associate data with the schema's archetype, and
// to know how to access that data

class cMotionSchema
{
public:
   cMotionSchema();

   ~cMotionSchema();

   // returns motion handle for a random motion in schema
   BOOL GetRandomMotion(int *pMotID) const;
   BOOL GetRandomMotionInRange(int min, int max, int *pMotID) const;
   BOOL GetMotion(int offset, int *pMotID) const;

   void SetDuration(float duration);
   void SetTimeWarp(float timeWarp);
   void SetDistance(float distance);
   void SetStretch(float timeWarp);
   BOOL GetDuration(float *pDuration) const;
   BOOL GetTimeWarp(float *pTimeWarp) const;
   BOOL GetDistance(float *pDist) const;
   BOOL GetStretch(float *pStretch) const;

   int NumMotions() const { return m_MotRunHandleList.Size(); }

   ObjID GetArchetype() const { return m_ArchObj; }

   int GetIndex() const { return m_ArchIndex; }

   int GetSchemaID() const { return m_SchemaID; }
   void SetSchemaID(int id) { m_SchemaID = id; }

   ulong GetFlags() const { return m_Flags; }
   void SetFlags(ulong flags) { m_Flags=flags; }

   // save/load
   BOOL Save(ITagFile *pFile);
   BOOL Load(ITagFile *pFile, cNameMap *pNameMap);
   void SetupRunTimeData(cNameMap *pNameMap);

   // db building
   void AddMotion(const Label *name, const sMotStuff *pStuff);
   void SetArchetype(int index); // index into name map provided by EndBuild and Load

private:
   ulong m_Flags;
   int m_ArchIndex;
   ObjID m_ArchObj;
   int m_SchemaID;
   float m_TimeModifier;
   float m_DistModifier;
   cDynArray<int> m_MotIndexList;
   cDynArray<int> m_MotRunHandleList;
};

inline void cMotionSchema::SetDuration(float duration)
{
   if(m_Flags&kMSchFlag_TimeWarp)
      m_Flags&=(~kMSchFlag_TimeWarp);
   m_Flags|=kMSchFlag_FixedDuration;
   m_TimeModifier=duration;
}

inline void cMotionSchema::SetTimeWarp(float timeWarp)
{
   if(m_Flags&kMSchFlag_FixedDuration)
      m_Flags&=(~kMSchFlag_FixedDuration);
   m_Flags|=kMSchFlag_TimeWarp;
   m_TimeModifier=timeWarp;
}

inline void cMotionSchema::SetDistance(float dist)
{
   if(m_Flags&kMSchFlag_Stretch)
      m_Flags&=(~kMSchFlag_Stretch);
   m_Flags|=kMSchFlag_FixedDist;
   m_DistModifier=dist;
}

inline void cMotionSchema::SetStretch(float stretch)
{
   if(m_Flags&kMSchFlag_FixedDist)
      m_Flags&=(~kMSchFlag_FixedDist);
   m_Flags|=kMSchFlag_Stretch;
   m_DistModifier=stretch;
}

inline BOOL cMotionSchema::GetDuration(float *pDuration) const
{
   if(m_Flags&kMSchFlag_FixedDuration)
   {
      *pDuration=m_TimeModifier;
      return TRUE;
   }
   return FALSE;
}

inline BOOL cMotionSchema::GetTimeWarp(float *pTimeWarp) const
{
   if(m_Flags&kMSchFlag_TimeWarp && !(m_Flags&kMSchFlag_FixedDuration))
   {
      *pTimeWarp=m_TimeModifier;
      return TRUE;
   }
   return FALSE;
}

inline BOOL cMotionSchema::GetDistance(float *pDist) const
{
   if(m_Flags&kMSchFlag_FixedDist)
   {
      *pDist=m_DistModifier;
      return TRUE;
   }
   return FALSE;
}

inline BOOL cMotionSchema::GetStretch(float *pStretch) const
{
   if(m_Flags&kMSchFlag_Stretch && !(m_Flags&kMSchFlag_FixedDist))
   {
      *pStretch=m_DistModifier;
      return TRUE;
   }
   return FALSE;
}

typedef cDynClassArray<cMotionSchema> cMotionSchemaSet;

typedef cDynArray<cMotionSchema *> cMotionSchemaPtrList;

#endif       // motschem_h
