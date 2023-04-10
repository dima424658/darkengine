/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/engscrpt.cpp,v 1.8 1998/11/02 06:21:11 dc Exp $
//
//
//

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>

#include <command.h>

#include <engscrpt.h>

// must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// Message implementations
//

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDebugScrSrv
//

#ifdef PLAYTEST
EXTERN void hello_debugger(void);
#endif

DECLARE_SCRIPT_SERVICE_IMPL(cDebugScrSrv, Debug)
{
public:

   STDMETHOD (MPrint)(const string ref s1,
                      const string ref s2,
                      const string ref s3,
                      const string ref s4,
                      const string ref s5,
                      const string ref s6,
                      const string ref s7,
                      const string ref s8)
   {
#ifdef PLAYTEST
      mprintf("%s%s%s%s%s%s%s%s\n",
              (const char *)s1,
              (const char *)s2,
              (const char *)s3,
              (const char *)s4,
              (const char *)s5,
              (const char *)s6,
              (const char *)s7,
              (const char *)s8);
#endif
      return S_OK;
   }

   STDMETHOD (Command)(const string ref s1,
                      const string ref s2,
                      const string ref s3,
                      const string ref s4,
                      const string ref s5,
                      const string ref s6,
                      const string ref s7,
                      const string ref s8)
   {
      char buf[1024];
      sprintf(buf, "%s %s%s%s%s%s%s%s",
              (const char *)s1,
              (const char *)s2,
              (const char *)s3,
              (const char *)s4,
              (const char *)s5,
              (const char *)s6,
              (const char *)s7,
              (const char *)s8);
      CommandExecute(buf);
      return S_OK;
   }
   
   STDMETHOD (Break)()
   {
#ifdef PLAYTEST
      hello_debugger();
#endif
      return S_OK;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDebugScrSrv, Debug);

///////////////////////////////////////////////////////////////////////////////
