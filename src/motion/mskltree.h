/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mskltree.h,v 1.2 2000/01/29 13:22:21 adurant Exp $
#pragma once

#ifndef __MSKLTREE_H
#define __MSKLTREE_H

typedef class cMSkillTree cMSkillTree;
typedef class cMSkillTreeNode cMSkillTreeNode;

// XXX can add different node type if want bigger key

typedef ulong tMSkillKey;
typedef struct sMSkillKeyDesc sMSkillKeyDesc;

struct sMSkillKeyDesc
{
   int nValues;
   uchar *pValues;
};

class cMSkillTree
{
public:

   cMSkillTree(const int bitsPerLevel, const int maxDepth); // XXX Assert if > sizeof(long)
   cMSkillTree(const cMSkillTree& atree); // copy constructor
   ~cMSkillTree();

   tMSkillKey BuildKey(const sMSkillKeyDesc& keyDesc);
   void InsertData(const void *dataPtr, const tMSkillKey key);
   void *GetData(const tMSkillKey key);

   int GetWildValue() const { return m_MaxEntriesPerLevel; } // the value to pass in to
                                                    // keydesc when
                                                    // any value is fine.

private:

   const int m_BitShift;
   const int m_MaxEntriesPerLevel;
   const int m_MaxDepth;
   cMSkillTreeNode *m_pHead;

};

#endif
