/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrpttyp.h,v 1.20 2000/02/22 19:49:48 toml Exp $
//
//
//

#ifndef __SCRPTTYP_H
#define __SCRPTTYP_H

#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// Base types
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <matrixs.h>
#include <scrptobj.h>
#include <multparm.h>
#include <lg.h> // for assertmsg

// because scrstrs can pass memory across script boundaries, they must be 
// allocated directly from the master heap
#include <allocapi.h>

#ifndef SCRIPT
#include <dbmem.h>
#define ScrAlloc(n)      malloc(n)
#define ScrRealloc(p, n) realloc(p, n)
#define ScrFree(p)       free(p)
#ifdef SHIP
#define ScrStrDup(p)     strdup(p)
#else
inline char * ScrStrDup(const char * pszSource)
{
   char * p = (char *)ScrAlloc(strlen(pszSource) + 1);
   strcpy(p, pszSource);
   return p;
}
#endif
#else
// @Q (toml 02-22-00): is there a way to get allocator credit here? unfortunately, string literal not 
// permanent in memory. Could if changed allocator to copy strings.
EXTERN IMalloc * g_pAppAlloc;
#define ScrAlloc(n)     g_pAppAlloc->Alloc(n)
#define ScrRealloc(p, n)   g_pAppAlloc->Realloc(p, n)
#define ScrFree(p)      g_pAppAlloc->Free(p)
inline char * ScrStrDup(const char * pszSource)
{
   char * p = (char *)ScrAlloc(strlen(pszSource) + 1);
   strcpy(p, pszSource);
   return p;
}
#endif

typedef int (*tScriptPrintFunc)(const char *pszFormatString, ...);

#define string       cScrStr
#define real         float
#define vector       cScrVec
#define integer      int
#define boolean      true_bool
#define timer_handle int

#ifdef __cplusplus

//
// Simple vector wrapper for scripts
//

class cScrVec : public mxs_vector
{
public:
   cScrVec() { x = 0.0; y = 0.0; z = 0.0; }
   cScrVec(real i, real j, real k) { x = i; y = j; z = k; }; 
   cScrVec(const mxs_vector& v) { x = v.x; y = v.y; z = v.z;}; 
   
   const cScrVec& operator =(const mxs_vector& v) { x = v.x; y = v.y; z = v.z; return *this;}; 
   cScrVec operator +(const mxs_vector& v) const { return cScrVec(x+v.x, y+v.y, z+v.z); }; 
   cScrVec operator -(const mxs_vector& v) const { return cScrVec(x-v.x, y-v.y, z-v.z); }; 
   cScrVec operator /(float s) const { return cScrVec(x/s, y/s, z/s); }; 
   void operator *=(float s) { x*=s; y*=s; z*=s; }; 

   float MagSquared(void) {return x*x+y*y+z*z;};
   float Magnitude(void) {return sqrt(MagSquared());};
   void Normalize(void) {float mag = Magnitude(); x=x/mag, y=y/mag, z=z/mag;};
};

////////////////////////////////////////////////////////////


//
// Simple (inefficient) string class for scripts. Add operations only as required!
//
class cScrStr
{
public:
   // Constructors/Destructors
   cScrStr()                  { m_pszData = (char *)ScrAlloc(1); m_pszData[0] = 0; }
   cScrStr(char ch)           { m_pszData = (char *)ScrAlloc(2); m_pszData[0] = ch; m_pszData[1] = 0; }
   cScrStr(int i)             { m_pszData = (char *)ScrAlloc(12); itoa(i, m_pszData, 10); }
   cScrStr(float f)           { m_pszData = (char *)ScrAlloc(50); sprintf(m_pszData, "%f", f); }
   cScrStr(double d)          { m_pszData = (char *)ScrAlloc(50); sprintf(m_pszData, "%f", d); }
   cScrStr(const vector& v)   { m_pszData = (char *)ScrAlloc(150); sprintf(m_pszData, "%f, %f, %f", v.x, v.y, v.z); }
   cScrStr(const char * s)    { m_pszData = ScrStrDup((s) ? s : ""); }
   cScrStr(boolean b)         { m_pszData = ScrStrDup((b) ? "TRUE" : "FALSE"); };
   cScrStr(const cScrStr & s) { m_pszData = ScrStrDup(s.m_pszData); }
   cScrStr(const sMultiParm & m);

   cScrStr(const char* s1, const char* s2);  // concatenation constructor 

   ~cScrStr() { ScrFree(m_pszData); }; 

   operator const char *() const { return m_pszData; };

   cScrStr operator +(const char* s) const { return cScrStr(m_pszData,s); }; 
   cScrStr& operator +=(const char* s);

   char &operator[](int nIndex) const;

   // Assignment
   const cScrStr & operator=(char ch)        { m_pszData = (char *)ScrRealloc(m_pszData, 2); m_pszData[0] = ch; m_pszData[1] = 0; return *this; }
   const cScrStr & operator=(int i)          { m_pszData = (char *)ScrRealloc(m_pszData, 12); itoa(i, m_pszData, 10); return *this; }
   const cScrStr & operator=(float f)        { m_pszData = (char *)ScrRealloc(m_pszData, 50); sprintf(m_pszData, "%f", f); return *this; }
   const cScrStr & operator=(double d)       { m_pszData = (char *)ScrRealloc(m_pszData, 50); sprintf(m_pszData, "%f", d); return *this; }
   const cScrStr & operator=(const char * s) { ScrFree(m_pszData); m_pszData = ScrStrDup((s) ? s : ""); return *this; }
   const cScrStr & operator=(boolean b)      { ScrFree(m_pszData); m_pszData = ScrStrDup((b) ? "TRUE" : "FALSE"); return *this; }
   const cScrStr & operator=(const cScrStr & s) { ScrFree(m_pszData); m_pszData = ScrStrDup(s.m_pszData); return *this; }
   const cScrStr & operator=(const sMultiParm & parm);

private:
   char * m_pszData;
};

#ifndef SCRIPT
extern cScrStr NULL_STRING;
#else
const cScrStr NULL_STRING;
#endif

inline const cScrStr & cScrStr::operator=(const sMultiParm & parm)
{
   switch (parm.type)
   {
      case kMT_Int:
         operator=(parm.i);
         break;
      case kMT_Float:
         operator=(parm.f);
         break;
      case kMT_String:
         operator=(parm.psz);
         break;
      case kMT_Vector:
         operator=((vector)*parm.pVec);
         break;
   }
   return *this;
}


inline cScrStr::cScrStr(const char* s1, const char* s2)
   :m_pszData(NULL) 
{
   int len1 = strlen(s1);
   int len2 = strlen(s2); 
   m_pszData = (char*)ScrAlloc(len1+len2+1);
   strcpy(m_pszData,s1);
   strcpy(m_pszData+len1,s2); 
}

inline cScrStr& cScrStr::operator +=(const char* s2)
{
   char* s1 = m_pszData; 
   int len1 = strlen(s1);
   int len2 = strlen(s2); 
   m_pszData = (char*)ScrAlloc(len1+len2+1);
   strcpy(m_pszData,s1);
   strcpy(m_pszData+len1,s2); 
   ScrFree(s1); 
   return *this; 
}

//
// Return a reference to the character at the specified index
//
inline char &cScrStr::operator[](int nIndex) const
{
   AssertMsg(nIndex >= 0 && nIndex <= strlen(m_pszData), "Invalid index");
   return m_pszData[nIndex];
}

inline cScrStr::cScrStr(const sMultiParm & parm)
   : m_pszData(ScrStrDup(""))
{
   operator=(parm);
}

// Compare helpers
inline BOOL operator==(const cScrStr &s1, const cScrStr &s2)
{
   return strcmp(s1, s2) == 0;
}

inline BOOL operator==(const cScrStr &s1, const char *s2)
{
   return strcmp(s1, s2) == 0;
}

inline BOOL operator==(const char *s1, const cScrStr &s2)
{
   return strcmp(s2, s1) == 0;
}

inline BOOL operator!=(const cScrStr& s1, const cScrStr& s2)
{
   return strcmp(s1, s2) != 0;
}

inline BOOL operator!=(const cScrStr& s1, const char* s2)
{
   return strcmp(s1, s2) != 0;
}

inline BOOL operator!=(const char* s1, const cScrStr& s2)
{
   return strcmp(s2, s1) != 0;
}

inline BOOL operator<(const cScrStr& s1, const cScrStr& s2)
{
   return strcmp(s1, s2) < 0;
}

inline BOOL operator<(const cScrStr& s1, const char* s2)
{
   return strcmp(s1, s2) < 0;
}

inline BOOL operator<(const char* s1, const cScrStr& s2)
{
   return strcmp(s2, s1) > 0;
}

inline BOOL operator>(const cScrStr& s1, const cScrStr& s2)
{
   return strcmp(s1, s2) > 0;
}

inline BOOL operator>(const cScrStr& s1, const char* s2)
{
   return strcmp(s1, s2) > 0;
}

inline BOOL operator>(const char* s1, const cScrStr& s2)
{
   return strcmp(s2, s1) < 0;
}

inline BOOL operator<=(const cScrStr& s1, const cScrStr& s2)
{
   return strcmp(s1, s2) <= 0;
}

inline BOOL operator<=(const cScrStr& s1, const char* s2)
{
   return strcmp(s1, s2) <= 0;
}

inline BOOL operator<=(const char* s1, const cScrStr& s2)
{
   return strcmp(s2, s1) >= 0;
}

inline BOOL operator>=(const cScrStr& s1, const cScrStr& s2)
{
   return strcmp(s1, s2) >= 0;
}

inline BOOL operator>=(const cScrStr& s1, const char* s2)
{
   return strcmp(s1, s2) >= 0;
}

inline BOOL operator>=(const char* s1, const cScrStr& s2)
{
   return strcmp(s2, s1) <= 0;
}


#else
typedef struct cScrStr cScrStr;
#endif
///////////////////////////////////////////////////////////////////////////////

#ifndef SCRIPT
#include <undbmem.h>
#endif

#endif /* !__SCRPTTYP_H */
