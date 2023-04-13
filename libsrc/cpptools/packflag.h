/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/cpptools/RCS/packflag.h 1.5 1970/01/01 00:00:00 mahk Exp $
//
//
//

#ifndef INCLUDED_PACKFLAG_H
#define INCLUDED_PACKFLAG_H

#include <memall.h>
#include <string.h>

#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////

class cPackedBoolSet
{
public:
   cPackedBoolSet(int size);
   ~cPackedBoolSet();

   BOOL IsSet(int) const;

   void Set(int);
   void Clear(int);

   // val must be ONE or ZERO!
   void AssignBit(int, int);
   // more traditional non-zero bool
   void Assign(int, BOOL);

   void SetAll(void);
   void ClearAll(void);
   int Size(void);

   void Resize(int newSize);

   void CopyFrom(const cPackedBoolSet& from);

private: 
   DWORD *      m_pBits;
   int m_nBits;
   
   static DWORD gm_fMasks[32];
};

///////////////////////////////////////////////////////////////////////////////

inline cPackedBoolSet::cPackedBoolSet(int size)
   : m_nBits(size)
{
   m_pBits = new DWORD [(size >> 5) + 1];
   ClearAll();
}

///////////////////////////////////////

inline cPackedBoolSet::~cPackedBoolSet()
{
   delete [] m_pBits;
}

///////////////////////////////////////


inline void cPackedBoolSet::SetAll(void)
{
   memset(m_pBits, 0xFF, ((m_nBits >> 5) + 1) * sizeof(DWORD));
}


////////////////////////////////////////

inline void cPackedBoolSet::ClearAll(void)
{
   memset(m_pBits, 0, ((m_nBits >> 5) + 1) * sizeof(DWORD));
}

////////////////////////////////////////

inline BOOL cPackedBoolSet::IsSet(int i) const 
{
   int iBit = i & 0x1f;
   return ((m_pBits[i >> 5] & gm_fMasks[iBit]) >> iBit); 
}

///////////////////////////////////////

inline void cPackedBoolSet::Set(int i)
{
   int iBit = i & 0x1f;
   m_pBits[i >> 5] |= gm_fMasks[iBit];
}

///////////////////////////////////////

inline void cPackedBoolSet::Clear(int i)
{
   int iBit = i & 0x1f;
   m_pBits[i >> 5] &= ~gm_fMasks[iBit];
}

////////////////////////////////////////

//
// I think this is faster than doing a compare to 
// decided whether to test or set
//

inline void cPackedBoolSet::AssignBit(int i, int bit)
{
   int iBit = i & 0x1f;
   DWORD word = m_pBits[i >> 5];
   word &= ~gm_fMasks[iBit]; 
   word |= bit << iBit;
   m_pBits[i >> 5] = word;
}

////////////////////////////////////////

inline void cPackedBoolSet::Assign(int i, BOOL val)
{
   // look ma, no tests
   long bit = (long)val;
   bit = (bit|-bit) >> 31;  // only zero and -zero have no sign bit

   AssignBit(i,bit & 1);
}

inline int cPackedBoolSet::Size(void)
{
    return m_nBits;
}

//////////////////////////////////////////

inline void cPackedBoolSet::Resize(int size)
{
    int oldnBits = m_nBits;
    int vecSize = (size >> 5) + 1;
    int oldVecSize = (m_nBits >> 5) + 1;

    m_nBits = size;

    if (vecSize != oldVecSize)
    {
        DWORD* oldBits = m_pBits;
        m_pBits = new DWORD[vecSize];

        if (vecSize < oldVecSize)
        {
            // copy the bits you can
            memcpy(m_pBits, oldBits, sizeof(DWORD) * vecSize);
        }
        else
        {
            // Clear the new bits and copy the lower bits
            memset(m_pBits + oldVecSize, 0, sizeof(DWORD) * (vecSize - oldVecSize));
            memcpy(m_pBits, oldBits, sizeof(DWORD) * oldVecSize);
        }


        delete[] oldBits;
    }
}

//////////////////////////////////////////

inline void cPackedBoolSet::CopyFrom(const cPackedBoolSet& from)
{
    Resize(from.m_nBits);
    int copySize = (m_nBits >> 5) + 1;
    memcpy(m_pBits, from.m_pBits, copySize * sizeof(DWORD));
}



///////////////////////////////////////////////////////////////////////////////

#include <undbmem.h>

#endif /* !INCLUDED_PACKFLAG_H */