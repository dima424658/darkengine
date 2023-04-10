/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/fcytagdb.h,v 1.4 2000/01/29 13:20:49 adurant Exp $
#pragma once

#ifndef __FCYTAGDB_H
#define __FCYTAGDB_H

typedef struct sTagInfo
{
   BOOL isMandatory;
   float weight;
} sTagInfo;

#ifdef __cplusplus

#include <tagfile.h>
#include <tagdbt.h>

class cTagRemapper;     // save us a dependency

class IFancyTagDatabase
{
public:
   virtual ~IFancyTagDatabase() {}

   // associate a piece of data with an unordered set of keys
   virtual void InsertData(int category, ITagDBKeySet *pSet,
                           const sTagDBData Data) = 0;

   // returns handle for tag
   virtual ulong RegisterTag(const sTagInfo *pTagInfo) = 0;
   virtual const sTagInfo *GetTagInfo(ulong handle) const = 0;

   virtual BOOL Load(ITagFile *pFile) = 0;
   virtual BOOL Save(ITagFile *pFile) = 0;

   // This is where we swizzle between the global tag tokenization and
   // our own peristent tokenization.
   virtual void Swizzle(void Func(cTagDBKey *)) = 0;

   virtual void SwizzleRequiredFlags(cTagRemapper *pRemapper) = 0;

   // find all data which matches the input set.  Tag input should only use
   // previously registered tag keys.
   // Only data of the specified category is considered.
   // Output sorted by tag weight
   virtual BOOL GetMatches(int category, cTagDBInput *pInput,
                           cTagDBOutput *pOutput) = 0;
};

// factory function
IFancyTagDatabase *NewIFancyTagDatabase(int nCategories);

#endif // cplusplus

#endif // FCYTAGDB_H
