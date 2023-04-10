// $Header: x:/prj/tech/libsrc/lg/RCS/dbgpp.h 1.5 1996/10/10 12:11:38 TOML Exp $
// $Log: dbgpp.h $
// Revision 1.5  1996/10/10  12:11:38  TOML
// msvc
// 
// Revision 1.4  1996/01/24  10:48:59  DAVET
// Added cplusplus stuff
// 
// Revision 1.3  1995/03/15  22:27:15  ept
// Added Errorpp and WarnUserpp to things defined when DBG_ON not set.
// 
// Revision 1.2  1994/12/06  22:39:46  lmfeeney
// correct args for Errorpp
// 
// Revision 1.1  1994/08/12  17:18:41  jak
// Initial revision
// 

#ifndef _DBGPP_H
#define _DBGPP_H

#include <iostream.h>
#include <strstrea.h>
extern "C" {
#include <dbg.h>
}
#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


class dbgostrstream : public ostrstream
{
public:
   dbgostrstream() {}
#ifdef __WATCOMC__
   dbgostrstream( char *str, int len, ios::openmode mode = ios::out ) :
#else
   dbgostrstream( char *str, int len, ios::open_mode mode = ios::out ) :
#endif
      ostrstream(str,len,mode) {}
   void flush_to_dbg( int reportType, int errCode, int src );
   dbgostrstream &prefix( int reportType );
   ~dbgostrstream() {}
} ;

// The debug stream object itself
extern dbgostrstream dbgppStream;

#ifdef DBG_ON

#define Errorpp(n,msg) ((dbgppStream.prefix(DBG_ERROR) << msg),dbgppStream.flush_to_dbg(DBG_ERROR,n,0))
#define WarnUserpp(msg) ((dbgppStream.prefix(DBG_WARNUSER) << msg),dbgppStream.flush_to_dbg(DBG_WARNUSER,0,0))

#ifdef WARN_ON
#define Warningpp(msg) ((dbgppStream.prefix(DBG_WARNING) << msg),dbgppStream.flush_to_dbg(DBG_WARNING,0,0))
#define Assrtpp(expr,msg) if (!(expr)) Warningpp(msg) ; else
#else
#define Warningpp(msg)          do {} while (0)
#define Assertpp(expr,msg)      do {} while (0)
#endif

#ifdef SPEW_ON
#define Spewpp(src,msg) do { if (DbgSpewTest(src)) ((dbgppStream.prefix(DBG_SPEW) << msg),dbgppStream.flush_to_dbg(DBG_SPEW,0,(src))); } while (0)
#else
#define Spewpp(src,msg)         do {} while (0)
#endif

#else

//	If DBG_ON not defined, most macros and functions are macro'ed to
//	nothing or (0).  A few functions remain

#define Errorpp(n,msg) ((dbgppStream.prefix(DBG_ERROR) << msg),dbgppStream.flush_to_dbg(DBG_ERROR,n,0))
#define WarnUserpp(msg) ((dbgppStream.prefix(DBG_WARNUSER) << msg),dbgppStream.flush_to_dbg(DBG_WARNUSER,0,0))
#define Warningpp(msg)
#define Assertpp(expr,msg)
#define Spewpp(src,msg)

#endif // DBG_ON

#ifdef __cplusplus
}
#endif  // cplusplus

#endif // _DBGPP_H

