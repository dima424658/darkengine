///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/cpptools/RCS/packflag.h 1.2 1997/09/17 23:24:28 mahk Exp $
//
//
//

#ifndef __PACKFLAG_H
#define __PACKFLAG_H

#include <string.h>

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
    
///////////////////////////////////////////////////////////////////////////////

#endif /* !__PACKFLAG_H */



