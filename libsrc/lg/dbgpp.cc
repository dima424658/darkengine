// $Header: x:/prj/tech/libsrc/lg/RCS/dbgpp.cc 1.2 1995/03/15 22:26:04 ept Exp $
// $Log: dbgpp.cc $
// Revision 1.2  1995/03/15  22:26:04  ept
// Added externs to DbgHandle, dbgTags, errErrCode so that some
// things can still be used even if DBG_ON is not set.
// 
// Revision 1.1  1994/08/12  17:17:44  jak
// Initial revision
// 


#include <dbgpp.h>
#include <mout.h>

extern "C" {
void DbgHandle(int reportType, ulong src, char *buff);
extern char *dbgTags[];
extern int errErrCode;
}

char _buf[1024];
dbgostrstream dbgppStream(_buf,1024);

dbgostrstream &dbgostrstream::prefix( int reportType )
{
   if (reportType<0 || reportType>=4)
   { Warning(( "Dbgpp: bad report type: %d\n", reportType )); return *this; }

   *this << dbgTags[reportType];
   return *this;
}

void dbgostrstream::flush_to_dbg( int reportType, int errCode, int src )
{
   if (reportType<0 || reportType>=4)
   { Warning(( "Dbgpp: bad report type: %d\n", reportType )); return; }

   if (reportType==DBG_ERROR) errErrCode = errCode;

   // Have to null-terminate the stuff.
   (*this) << ((char)0);

   src = reportType==DBG_SPEW ? src : DBG_SLOT_MASK;
   DbgHandle( reportType, src, rdbuf()->str() );  // yeeee-haw

   // Flush strstream
   rdbuf()->pubseekoff( 0, std::ios::beg, std::ios::out );
}

