///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/codewrit.h $
// $Author: TOML $
// $Date: 1996/11/02 15:26:59 $
// $Revision: 1.1 $
//

#ifndef __CODEWRIT_H
#define __CODEWRIT_H

#ifdef _WIN32
EXTERN BOOL LGAPI MakeAllCodeWritable(void);
EXTERN BOOL LGAPI MakeFunctionWritable(void * pfnFunction, unsigned sizeFunction);
#else
#define MakeAllCodeWritable()
#define MakeFunctionWritable(pfnFunction, sizeFunction)
#endif

#endif /* !__CODEWRIT_H */
