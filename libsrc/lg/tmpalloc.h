/*
 * $Source: x:/prj/tech/libsrc/lg/RCS/tmpalloc.h $
 * $Revision: 1.7 $
 * $Author: PATMAC $
 * $Date: 1998/07/02 13:39:09 $
 *
 * Header for routines for controlling temporary stacks of big_buffer
 *
 * This file is part of the 2d library.
 */

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

extern MemStack *temp_mem_get_stack(void);
extern int temp_mem_init(MemStack *ms);
extern int temp_mem_uninit(void);
extern void *temp_malloc(long n);
extern void *temp_realloc(void *p,long n);
extern int temp_free(void *p);

#ifdef DBG_ON
extern int temp_spew_mem_init(MemStack *ms,char *file,int line);
extern int temp_spew_mem_uninit(char *file,int line);
extern void *temp_spew_malloc(long n,char *file,int line);
extern void *temp_spew_realloc(void *p,long n,char *file,int line);
extern int temp_spew_free(void *p,char *file,int line);

#define TempMemInit(ms) temp_spew_mem_init(ms,__FILE__,__LINE__)
#define TempMemUninit() temp_spew_mem_uninit(__FILE__,__LINE__)
#define TempMalloc(n) temp_spew_malloc(n,__FILE__,__LINE__)
#define TempRealloc(p,n) temp_spew_realloc(p,n,__FILE__,__LINE__)
#define TempFree(p) temp_spew_free(p,__FILE__,__LINE__)
#else /* !DBG_ON */

#define TempMemInit temp_mem_init
#define TempMemUninit temp_mem_uninit
#define TempMalloc temp_malloc
#define TempRealloc temp_realloc
#define TempFree temp_free

#endif /* DBG_ON */
#ifdef __cplusplus
}
#endif  // cplusplus
