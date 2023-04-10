/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptmsg.cpp,v 1.10 1998/03/05 08:19:53 MAT Exp $
//
//
//

#include <lg.h>
#include <comtools.h>

#include <scrptapi.h>
#include <scrptmsg.h>

#if defined(__WATCOMC__)
#pragma initialize library
#else
#pragma init_seg (lib)
#endif




///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sScrMsg
//

IMPLEMENT_SCRMSG_PERSISTENT(sScrMsg)
{
   // If this were not the root class, we'd call BASE::Persistence()

   if (PersistentVersion(kScrMsgVer) != kScrMsgVer)
      return FALSE;

   Persistent(from);
   Persistent(to);
   Persistent(message);
   Persistent(time);
   Persistent(flags);
   Persistent(data);
   Persistent(data2);
   Persistent(data3);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SCRMSG_PERSISTENT(sScrTimerMsg)
{
   PersistenceHeader(sScrMsg, kScrTimerMsgVer);
   
   Persistent(name);

   return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
