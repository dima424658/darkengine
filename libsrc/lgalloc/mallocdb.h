///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/mallocdb.h $
// $Author: TOML $
// $Date: 1997/07/15 21:28:30 $
// $Revision: 1.1 $
//

#ifndef __MALLOCDB_H
#define __MALLOCDB_H

#ifndef SHIP
EXTERN void * malloc_db(size_t size, const char * file, int line);
EXTERN void free_db(void * p, const char * file, int line);
EXTERN void * realloc_db(void * p, size_t size, const char * file, int line);
#endif

#endif /* !__MALLOCDB_H */
