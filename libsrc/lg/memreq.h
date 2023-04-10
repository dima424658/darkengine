///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/memreq.h $
// $Author: TOML $
// $Date: 1997/01/20 15:24:11 $
// $Revision: 1.1 $
//

#ifndef __MEMREQ_H
#define __MEMREQ_H

typedef
struct sMemoryRequirements
{
   ulong physicalMemory;
   ulong swapSpace;
   ulong freeMemory;

} sMemoryRequirements;

EXTERN BOOL LGAPI VerifyRequirements(const sMemoryRequirements *, BOOL fQuiet);

#endif /* !__MEMREQ_H */
