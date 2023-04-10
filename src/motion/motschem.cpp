/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motschem.cpp,v 1.8 1999/10/25 15:54:30 BODISAFA Exp $

#include <motschem.h>
#include <objedit.h>
#include <stdlib.h>
#include <rand.h>
#include <motmngr.h>

#include <dbmem.h> // must be last included header


cMotionSchema::cMotionSchema()
{
   m_ArchObj=OBJ_NULL;
   m_ArchIndex=kNameMapIndexNotFound;
   m_Flags=NULL;
   m_TimeModifier=0;
   m_DistModifier=0;
}

cMotionSchema::~cMotionSchema()
{
}

BOOL cMotionSchema::GetRandomMotion(int *pMotID) const
{
   int size=m_MotRunHandleList.Size();
   if(size>0)
   {
      int id=RandRange(0,size-1);
#ifdef OLD_WAY
      int id= (size*Rand())/(RAND_MAX+1);
      if(id>=size)
         id=size-1;
#endif
      *pMotID = m_MotRunHandleList[id];
      return TRUE;
   }
   return FALSE;
}

BOOL cMotionSchema::GetRandomMotionInRange(int min, int max, int *pMotID) const
{
   int size=m_MotRunHandleList.Size();
   Assert_(max<=size);
   if(min>max)
      return FALSE;
   int id=RandRange(min,max);
#ifdef OLD_WAY
   int id= min + (((max-min+1)*Rand())/(RAND_MAX+1));
#endif
   Assert_(id<=max);
   *pMotID = m_MotRunHandleList[id];
   return TRUE;
}

// @TODO: how to deal when not all motions in loco schema get successfully loaded?
BOOL cMotionSchema::GetMotion(int offset, int *pMotID) const
{
   if(m_MotRunHandleList.Size()!=m_MotIndexList.Size())
   {
      Warning(("Not all motions in schema could be loaded\n"));
   }

   int size=m_MotRunHandleList.Size();
   if(size<=offset)
      return FALSE;

   *pMotID = m_MotRunHandleList[offset];
   return TRUE;
}

BOOL cMotionSchema::Save(ITagFile *pFile)
{
   // write out name
   ITagFile_Write(pFile,(char *)&m_ArchIndex,sizeof(m_ArchIndex));

   // write out schema index
   ITagFile_Write(pFile,(char *)&m_SchemaID,sizeof(m_ArchIndex));

   // write out flags
   ITagFile_Write(pFile,(char *)&m_Flags,sizeof(m_Flags));

   // write out params
   ITagFile_Write(pFile,(char *)&m_TimeModifier,sizeof(m_TimeModifier));
   ITagFile_Write(pFile,(char *)&m_DistModifier,sizeof(m_DistModifier));

   // write out index list
   ulong size=m_MotIndexList.Size();
   int i;
   ITagFile_Write(pFile,(char *)&size,sizeof(size));
   for(i=0;i<size;i++)
      ITagFile_Write(pFile,(char *)&m_MotIndexList[i],sizeof(m_MotIndexList[i]));

   return FALSE;
}

BOOL cMotionSchema::Load(ITagFile *pFile, cNameMap *pNameMap)
{
   // read in name
   ITagFile_Read(pFile,(char *)&m_ArchIndex,sizeof(m_ArchIndex));

   // read in schema index
   ITagFile_Read(pFile,(char *)&m_SchemaID,sizeof(m_ArchIndex));

   // read in flags
   ITagFile_Read(pFile,(char *)&m_Flags,sizeof(m_Flags));

   // read in params
   ITagFile_Read(pFile,(char *)&m_TimeModifier,sizeof(m_TimeModifier));
   ITagFile_Read(pFile,(char *)&m_DistModifier,sizeof(m_DistModifier));

   // read in index list
   ulong size,index;
   int i;
   ITagFile_Read(pFile,(char *)&size,sizeof(size));
   m_MotIndexList.SetSize(size);
   for(i=0;i<size;i++)
   {
      ITagFile_Read(pFile,(char *)&index,sizeof(index));
      m_MotIndexList.SetItem((int *)&index,i);
   }

   SetupRunTimeData(pNameMap);
   return TRUE;
}

void cMotionSchema::AddMotion(const Label *name, const sMotStuff *pStuff)
{
   int index;

   Assert_(g_pMotionSet);
   if(!g_pMotionSet->AddMotion(name,pStuff,&index))
      return;

   m_MotIndexList.Append(index);
}

void cMotionSchema::SetArchetype(int index)
{
   m_ArchIndex=index;
}

void cMotionSchema::SetupRunTimeData(cNameMap *pNameMap)
{
   int i,size,index,rh;

   // build run handle list
   m_MotRunHandleList.SetSize(0);
   size=m_MotIndexList.Size();
   for(i=0;i<size;i++)
   {
      index=m_MotIndexList[i];            
      if(g_pMotionSet->GetRunHandle(index,&rh))
         m_MotRunHandleList.Append(rh);
   }

   // set archetype objid
   if(m_Flags&kMSchFlag_ArchObjSwizzle)
   {
      Label *name=pNameMap->NameFromID(m_ArchIndex);
      Assert_(name);
      m_ArchObj=EditGetObjNamed(name->text);
      if(m_ArchObj==OBJ_NULL)
      {
         Warning(("SetupRunTimeData: archetype obj %s not found\n",name->text));
      }
   }
}


