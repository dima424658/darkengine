///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/stktrace.h $
// $Author: TOML $
// $Date: 1996/06/27 14:20:32 $
// $Revision: 1.2 $
//

#ifndef __STKTRACE_H
#define __STKTRACE_H

EXTERN int LGAPI FillStackArray(int Skip, int MaxFrames, void **p);
EXTERN int LGAPI FillThreadStackArray(HANDLE hThread, int Skip, int MaxFrames, void **p);

#endif /* !__STKTRACE_H */
