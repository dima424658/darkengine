//		Slist.H		Singly-linked list header file
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/slist.h 1.3 1996/10/21 09:35:01 TOML Exp $
* $Log: slist.h $
 * Revision 1.3  1996/10/21  09:35:01  TOML
 * msvc
 * 
 * Revision 1.2  1996/01/22  15:39:12  DAVET
 *
 * Added cplusplus stuff.
 *
 * Revision 1.1  1993/05/03  10:53:29  rex
 * Initial revision
 *
*/

#ifndef SLIST_H
#define SLIST_H

#include "types.h"

#ifdef __cplusplus
extern "C"  {
#endif


//	--------------------------------------------------------------

//	Slist node
#if !defined(__SC__) && !defined(__cplusplus)

typedef struct _slist {
	struct _slist *psnext;	// ptr to next node or NULL if last one
									// real data follows, here
} slist;

#else

typedef struct __slist {
	struct _slist *psnext;	// ptr to next node or NULL if last one
									// real data follows, here
} slist;

#define _slist \
    { \
    	struct __slist *psnext; \
    }

#endif

//	Slist header
#if !defined(__SC__) && !defined(__cplusplus)

typedef struct _slist_head {
	struct _slist;				// ptr to 1st item in list
} slist_head;

#else

typedef struct __slist_head {
	struct _slist;
} slist_head;

#define _slist_head  \
    { \
	struct _slist; \
    }

#endif

//	Initialize an slist header (must be done before use)

#define slist_init(pslh) { (pslh)->psnext = NULL; }

//	Add a new slist node to head of list

#define slist_add_head(pslh,psl) { \
	(psl)->psnext = slist_head(pslh); \
	(pslh)->psnext = (void *) psl; \
	}

//	Insert after specified node

#define slist_insert_after(psl,pnode) { \
	(psl)->psnext = (pnode)->psnext; \
	(pnode)->psnext = (void *) psl; \
	}

//	Remove node (must specify prior node)

#define slist_remove(psl,pslbefore) { (pslbefore)->psnext = (psl)->psnext; }

//	Get ptr to head slist node

#define slist_head(pslh) (void*)((pslh)->psnext)

//	Determine if list empty

#define slist_empty(pslh) (slist_head(pslh) == NULL)

//	Get next node

#define slist_next(psl) (void*)((psl)->psnext)		// get ptr to next node

//	Iterate across all items

#define forallinslist(listtype,pslh,psl) for (psl = \
	(listtype *) slist_head(pslh); psl != NULL; psl = slist_next(psl))


#ifdef __cplusplus
}
#endif

#endif

