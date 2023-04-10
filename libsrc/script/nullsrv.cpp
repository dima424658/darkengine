/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/nullsrv.cpp,v 1.1 1997/12/28 14:26:10 TOML Exp $
//
//
//


#include <lg.h>

#include <scrptapi.h>
#include <scrptsrv.h>

#include <nullsrv.h>


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cNullScrSrv
//

DECLARE_SCRIPT_SERVICE_IMPL(cNullScrSrv, Null)
{
   STDMETHOD_(void, Null1)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null2)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null3)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null4)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null5)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null6)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null7)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null8)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null9)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null10)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null11)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null12)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null13)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null14)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null15)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null16)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null17)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null18)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null19)()
   {
      NullCall();
   }

   STDMETHOD_(void, Null20)()
   {
      NullCall();
   }

   void NullCall()
   {
      CriticalMsg("Script service is not supported!");
   }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cNullScrSrv, Null);

///////////////////////////////////////////////////////////////////////////////
