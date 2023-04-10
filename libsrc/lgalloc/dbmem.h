///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/dbmem.h $
// $Author: TOML $
// $Date: 1997/10/15 11:51:02 $
// $Revision: 1.2 $
//

// THIS MUST BE THE FINAL INCLUDE!
#ifdef DEBUG
#ifdef __cplusplus
#define new MEMALL_DEBUG_NEW
#endif
#define malloc(s)       ((*f_malloc_db)(s, __FILE__, __LINE__))
#define realloc(p, s)   ((*f_realloc_db)(p, s, __FILE__, __LINE__))
#endif
