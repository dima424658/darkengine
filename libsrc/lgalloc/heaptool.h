///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/heaptool.h $
// $Author: TOML $
// $Date: 1997/07/15 21:28:30 $
// $Revision: 1.1 $
//

#ifndef __HEAPTOOL_H
#define __HEAPTOOL_H

///////////////////////////////////////////////////////////////////////////////

const int kByteShift           = 0;
const int kWordShift           = 1;
const int kDoubleWordShift     = 2;
const int kQuadWordShift       = 3;
const int kDoubleQuadWordShift = 4;

const int kPageSize            = 4096;

#define SizeAlign(x, a)        ((1 + ((x - 1) >> a)) << a)

///////////////////////////////////////////////////////////////////////////////

#endif /* !__HEAPTOOL_H */
