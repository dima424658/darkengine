// $Header: x:/prj/tech/libsrc/cpptools/RCS/hashfunc.h 1.4 1997/08/12 12:12:19 TOML Exp $
#ifndef HASHFUNC_H
#define HASHFUNC_H

#include <string.h>
#include <hashfns.h>

#define __CPPTOOLSAPI


//
// This class contains the default implementations of the hash functions
//
class __CPPTOOLSAPI cHashFunctions
{
public:
   static unsigned Hash(const char * p)
   {
      return HashString(p);
   }

   static BOOL IsEqual(const char *p1, const char *p2)
   {
      return !strcmp(p1, p2);
   }

   static unsigned Hash(const long p)
   {
      return HashIntegerValue(p);
   }

   static BOOL IsEqual(const long p1, const long p2)
   {
      return p1 == p2;
   }

   static unsigned Hash(const unsigned long p)
   {
      return HashIntegerValue(p);
   }

   static BOOL IsEqual(const unsigned long p1, const unsigned long p2)
   {
      return p1 == p2;
   }

   static unsigned Hash(const int p)
   {
      return HashIntegerValue(p);
   }

   static BOOL IsEqual(const int p1, const int p2)
   {
      return p1 == p2;
   }

   static unsigned Hash(const unsigned int p)
   {
      return HashIntegerValue(p);
   }

   static BOOL IsEqual(const unsigned int p1, const unsigned int p2)
   {
      return p1 == p2;
   }

   static unsigned Hash(const short p)
   {
      return HashIntegerValue(((int)p) | (((int)p) << 16));
   }

   static BOOL IsEqual(const short p1, const short p2)
   {
      return p1 == p2;
   }

   static unsigned Hash(const unsigned short p)
   {
      return HashIntegerValue(((int)p) | (((int)p) << 16));
   }

   static BOOL IsEqual(const unsigned short p1, const unsigned short p2)
   {
      return p1 == p2;
   }

   static unsigned Hash(const GUID *p)
   {
      return HashGUID(p);
   }

   static BOOL IsEqual(const GUID *p1, const GUID *p2)
   {
      return !memcmp(p1, p2, 16); // sizeof(GUID)
   }

   static unsigned Hash(const void *p)
   {
      return HashPtr(p);
   }

   static BOOL IsEqual(const void *p1, const void *p2)
   {
      return p1 == p2; 
   }

   static unsigned Hash(const long *p)
   {
      return HashIntegerValue(*p);
   }

   static BOOL IsEqual(const long *p1, const long *p2)
   {
      return *p1 == *p2;
   }

   static unsigned Hash(const unsigned long *p)
   {
      return HashIntegerValue(*p);
   }

   static BOOL IsEqual(const unsigned long *p1, const unsigned long *p2)
   {
      return *p1 == *p2;
   }

   static unsigned Hash(const int *p)
   {
      return HashIntegerValue(*p);
   }

   static BOOL IsEqual(const int *p1, const int *p2)
   {
      return *p1 == *p2;
   }

   static unsigned Hash(const unsigned int *p)
   {
      return HashIntegerValue(*p);
   }

   static BOOL IsEqual(const unsigned int *p1, const unsigned int *p2)
   {
      return *p1 == *p2;
   }

   static unsigned Hash(const short *p)
   {
      return HashIntegerValue(*p);
   }

   static BOOL IsEqual(const short *p1, const short *p2)
   {
      return *p1 == *p2;
   }

   static unsigned Hash(const unsigned short *p)
   {
      return HashIntegerValue(*p);
   }

   static BOOL IsEqual(const unsigned short *p1, const unsigned short *p2)
   {
      return *p1 == *p2;
   }
};

class __CPPTOOLSAPI cCaselessStringHashFuncs
{
public:
   static unsigned Hash(const char* s)
   {
      return HashStringCaseless(s);
   }

   static unsigned IsEqual(const char* s1, const char* s2)
   {
      return stricmp(s1,s2) == 0;
   }
};

#endif // HASHFUNC_H















