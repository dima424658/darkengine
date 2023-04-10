/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mskltree.cpp,v 1.3 1997/12/22 19:48:56 mahk Exp $

#include <mskltree.h>
#include <lg.h>

// Must be last header
#include <dbmem.h>

#define kValueAny 3 // XXX need to invent this for real

typedef tMSkillKey tMSkillMask;

class cMSkillMask
{
public:
   cMSkillMask(const tMSkillKey key=0, const int bitShift=0, const int nValues=0);
//   cMSkillMask(const cMSkillMask& mask);

   BOOL GetValue(tMSkillKey *pValue, const int index) const;

   // Builds a key with current mask's values and value at index set to value argument
//   tMSkillKey MakeChildKey(const int index, const int value);

private:
   const tMSkillKey m_Key;
   const ushort m_BitShift;
   ushort m_BitMask;
   const int m_nValues;
};

class cMSkillTreeNode
{
public:

   cMSkillTreeNode(const int depth, const int maxChildren, const cMSkillTree *pTree);
   ~cMSkillTreeNode();

   cMSkillTreeNode *InsertChild(const int iChild);

   cMSkillTreeNode *GetChild(const int iChild) { \
      AssertMsg1(iChild<m_MaxChildren,"cMSkillTreeNode::GetChild. index %d too big",iChild); \
      return m_ppChildren[iChild]; }

   void MakeLeaf(void *pData);

   BOOL IsLeaf() { return m_IsLeaf; }

   void *GetData(const cMSkillMask& mask) { return LinearGetData(mask); }

private:

   void *RecursiveGetData(const cMSkillMask &mask);
   void *LinearGetData(const cMSkillMask &mask);

   bool m_IsLeaf;
   uchar m_Depth;
   uchar m_nChildren;
   const uchar m_MaxChildren;
   void *m_pData;
   cMSkillTreeNode **m_ppChildren;
   const cMSkillTree *m_pTree; // the tree this node belongs to
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///
///  ATree implementation
///
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

cMSkillTree::cMSkillTree(const int bitsPerLevel, const int maxDepth)
 : m_BitShift(bitsPerLevel),
   m_MaxDepth(maxDepth),
   m_MaxEntriesPerLevel((1<<bitsPerLevel)-1)  // subtract off for wild value
{
   AssertMsg2((bitsPerLevel*maxDepth)/8 <= sizeof(tMSkillKey),\
      "ATree cannot support key size of %d*%d bits\n",bitsPerLevel,maxDepth);

   m_pHead=NULL;
}

////////////////////////////////////////////////////////////////////////////////

cMSkillTree::cMSkillTree(const cMSkillTree& atree)
 : m_BitShift(atree.m_BitShift),
   m_MaxEntriesPerLevel(atree.m_MaxEntriesPerLevel),
   m_MaxDepth(atree.m_MaxDepth)
{
   m_pHead=atree.m_pHead;
}

////////////////////////////////////////////////////////////////////////////////

cMSkillTree::~cMSkillTree()
{
   if(m_pHead)
      delete m_pHead;
}

////////////////////////////////////////////////////////////////////////////////

// XXX should make this inline, I think
tMSkillKey cMSkillTree::BuildKey(const sMSkillKeyDesc &keyDesc)
{
   int i;
   uchar *pVal=keyDesc.pValues;
   tMSkillKey key=0;

   AssertMsg1(keyDesc.nValues<=m_MaxDepth,"cMSkillTree::BuildKey. Request has %d values!",keyDesc.nValues);
   pVal=keyDesc.pValues;
   for(i=0;i<keyDesc.nValues;i++,pVal++)
   {
      if(*pVal>m_MaxEntriesPerLevel)
      {
         AssertMsg2(*pVal<=m_MaxEntriesPerLevel,"cMSkillTree::BuildKey. Value %d at entry %d is too large to use\n",*pVal, i);
      }
      key|=(*pVal)<<(m_BitShift*i);
   }
   return key;
}

////////////////////////////////////////////////////////////////////////////////

void cMSkillTree::InsertData(const void *pData, const tMSkillKey key)
{
   if(!m_pHead)
   {
      cMSkillMask mask(key, m_BitShift, m_MaxDepth);

      m_pHead=new cMSkillTreeNode(0, m_MaxEntriesPerLevel, this);

      if(!m_pHead)
      {
         AssertMsg(m_pHead,"Unable to create ATree node!\n");
         return;
      }
   }
   int depth=0;
   tMSkillKey val=0;
   cMSkillMask mask(key,m_BitShift,m_MaxDepth);
   cMSkillTreeNode *pNode=m_pHead;
   cMSkillTreeNode *pChild;

   while(mask.GetValue(&val,depth++))
   {
      AssertMsg(val!=GetWildValue(),"cMSkillTree::InsertData. key must be fully specified. 'Any' is not allowed\n");
      if(!(pChild=pNode->GetChild(val)))
      {
         pChild=pNode->InsertChild(val);
      }
      pNode=pChild;
   }
   // insert leaf
   pNode->MakeLeaf((void *)pData); // secretly know that we don't change it
}

////////////////////////////////////////////////////////////////////////////////

void *cMSkillTree::GetData(const tMSkillKey key)
{
   if(!m_pHead)
      return NULL;

   cMSkillMask mask(key,m_BitShift,m_MaxDepth);
   return m_pHead->GetData(mask);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///
///  cMSkillMask implementation
///
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

cMSkillMask::cMSkillMask(const tMSkillKey mask, const int bitShift, const int nValues)
 : m_BitShift(bitShift),
   m_nValues(nValues),
   m_Key(mask)
{
   int i=bitShift;
   ulong bit=0x1;

   m_BitMask=0;
   while(i--)
   {
      m_BitMask|=bit;
      bit=bit<<1;
   }
}

inline BOOL cMSkillMask::GetValue(tMSkillKey *pValue, const int index) const
{
   tMSkillKey val=m_Key;

   if(index>=m_nValues)
      return FALSE;

   val=val>>(m_BitShift*index);
   *pValue=val&m_BitMask;
   return TRUE;
}

#if 0
cMSkillMask::cMSkillMask(const cMSkillMask& mask)
 : m_Key(mask.m_Key),
   m_BitShift(mask.m_BitShift),
   m_BitMask(mask.m_BitMask),
   m_nValues(mask.m_nValues)
{
}

tMSkillKey cMSkillMask::MakeChildKey(const int index, const int value)
{
   tMSkillKey key=m_Key;
   int i=index;
   int val=value&m_BitMask;
   int mask=m_BitMask;

   val=val<<(m_BitShift*index);
   mask=mask<<(m_BitShift*index);

   tMSkillKey MakeChildKey(const int index, const int value);
   key=key&(~mask); // in case value at index was previously set, this will wipe it
   key=key|val;
   return key;
}
#endif


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///
///  cMSkillTreeNode implementation
///
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

cMSkillTreeNode::cMSkillTreeNode(const int depth, const int maxChildren, const cMSkillTree *pTree)
 : m_MaxChildren(maxChildren),
   m_pTree(pTree)
{
   m_IsLeaf=FALSE;
   m_Depth=depth;
   m_nChildren=0;
   m_pData=NULL;

   m_ppChildren=new cMSkillTreeNode *[m_MaxChildren];
   // NULL it out
   int i;
   for(i=0;i<m_MaxChildren;i++)
   {
      m_ppChildren[i]=NULL;
   }
}

cMSkillTreeNode::~cMSkillTreeNode()
{
   int i;
   cMSkillTreeNode **ppChild=m_ppChildren;

   for(i=0;i<m_nChildren;i++,ppChild++)
   {
      if(*ppChild)
         delete *ppChild;
   }
   delete [] m_ppChildren; 
}

cMSkillTreeNode *cMSkillTreeNode::InsertChild(const int iChild)
{
   cMSkillTreeNode *pChild;

   pChild=m_ppChildren[iChild];
   if(pChild)
   {
      AssertMsg1(FALSE,"cMSkillTreeNode::InsertChild.  Child already exists at %d\n",iChild);
      return pChild;
   }
   m_ppChildren[iChild]=new cMSkillTreeNode(m_Depth+1,m_MaxChildren,m_pTree);
   if(m_nChildren<=iChild)
   {
      m_nChildren=iChild+1;
   }
   return m_ppChildren[iChild];
}

void cMSkillTreeNode::MakeLeaf(void *pData)
{
   if(m_nChildren)
   {
      AssertMsg1(FALSE,"Cannot make node a leaf since already has %d children!\n",m_nChildren);
      return;
   }
   if(m_IsLeaf)
   {
      AssertMsg(FALSE,"Cannot make node a leaf since it already is one");
      return;
   }
   m_IsLeaf=TRUE;
   m_pData=pData;
}

// to depth-first recursion on tree
void *cMSkillTreeNode::RecursiveGetData(const cMSkillMask &mask)
{
   int i=0;
   void *pData;
   cMSkillTreeNode *pChild;
   tMSkillKey val;

   while(i<m_nChildren)
   {
      pChild=GetChild(i);
      if(!pChild) // dead end.  data not to be found here, so check other kids
         continue;
      if(pChild->IsLeaf()) // found the data.  get outta here.
         return pChild->m_pData;
      // sanity check
      AssertMsg1(mask.GetValue(&val,m_Depth+1),"No value for mask at depth %d",m_Depth+1);
      mask.GetValue(&val,m_Depth+1);

      if(val!=m_pTree->GetWildValue()) // revert to linear search
      {
         if(NULL!=(pData=pChild->LinearGetData(mask)))
         {
            // found the data.   get outta here
            return pData;
         }
      } else // continue with recursive search
      {
         if(NULL!=(pData=pChild->RecursiveGetData(mask)))
         {
            // found the data.   get outta here
            return pData;
         }
      }
      // haven't found the data yet, check next child
      i++;
   }
   // checked all children and failed to find data
   return NULL;
}

void *cMSkillTreeNode::LinearGetData(const cMSkillMask &mask)
{
   int depth=m_Depth;
   tMSkillKey val;
   cMSkillTreeNode *pNode=this;
   cMSkillTreeNode *pChild;

   while(mask.GetValue(&val,depth++))
   {
      if(val==m_pTree->GetWildValue()) // need to check recursively
      {
         return pNode->RecursiveGetData(mask);
      }
      AssertMsg(!pNode->IsLeaf(),"cMSkillTreeNode::LinearGetData. tree badly constructed");

      pChild=pNode->GetChild(val);
      if(!pChild) // no entry for key
         return NULL;

      pNode=pChild;
   }
   // if got this far, then node HAS to be leaf
   AssertMsg(pNode->IsLeaf(),"cMSkillTreeNode::LinearGetData. tree badly constructed");
   return pNode->m_pData;
}
