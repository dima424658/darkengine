/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/



#ifndef __MOUT_H
#define __MOUT_H

#include <streambuf>
#include <ostream>

class mstreambuf : public std::streambuf
{
public:
   mstreambuf();
   ~mstreambuf();
   mstreambuf( std::streambuf & );
   mstreambuf( mstreambuf & );

   virtual int overflow( int = EOF );
   virtual int underflow() { return EOF; }
   virtual int	sync()      { return EOF; }

private:
   void init_mstreambuf( void );
} ;

extern mstreambuf __mout_mstreambuf;
extern std::ostream mout;

#endif /* !__MOUT_H */
