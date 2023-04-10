//		Llist.H		Double-linked list header file
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/llist.h 1.11 1997/02/11 20:14:24 TOML Exp $
* $Log: llist.h $
 * Revision 1.11  1997/02/11  20:14:24  TOML
 * padded queue to DWORD align
 * 
 * Revision 1.10  1997/01/29  16:12:45  TOML
 * c++ happy queue
 * 
 * Revision 1.9  1997/01/29  10:12:29  TOML
 * corrected bad extern "C" matching
 * 
 * Revision 1.8  1996/12/04  12:36:59  DFAN
 * Added cast to forallinlist() to make C++ happy
 * 
 * Revision 1.7  1996/10/21  09:35:08  TOML
 * msvc
 * 
 * Revision 1.6  1996/01/22  15:39:08  DAVET
 *
 * Added cplusplus stuff.
 *
 * Revision 1.5  1993/04/19  11:36:13  rex
 * Arggh! More void* to make llist work!
 *
 * Revision 1.4  1993/04/19  09:35:40  rex
 * Fixed llist macros again with void* so work with queues too (arrggh)
 *
 * Revision 1.3  1993/04/19  09:26:41  rex
 * Fixed llist macros to do casting, so as to not get ptr warnings
 *
 * Revision 1.2  1993/04/16  12:02:57  rex
 * Added llist_insert_before() and llist_insert_after(), dropped head arg
 * from llist_remove()
 *
 * Revision 1.1  1993/04/16  11:04:26  rex
 * Initial revision
 *
 * Revision 1.3  1993/04/15  16:25:40  rex
 * Made basic llist, derived Llist from it.
 *
 * Revision 1.2  1993/01/12  17:54:37  rex
 * Modified includes in preparation for turning into library
 *
 * Revision 1.1  1992/08/31  17:01:28  unknown
 * Initial revision
 *
*/

#ifndef LLIST_H
#define LLIST_H

#include <types.h>

#ifdef __cplusplus
extern "C"  {
#endif


//	--------------------------------------------------------------
//		LOW-LEVEL LINKED LIST
//	--------------------------------------------------------------

//	List node (data must follow)
#if !defined(__SC__) && !defined(__cplusplus)

typedef struct _llist {
	struct _llist *pnext;	// ptr to next node or NULL if at tail
	struct _llist *pprev;	// ptr to prev node or NULL if at head
									// real data follows, here
} llist;

#else

typedef struct __llist {
	struct __llist *pnext;	// ptr to next node or NULL if at tail
	struct __llist *pprev;	// ptr to prev node or NULL if at head
									// real data follows, here
} llist;
#define _llist \
    { \
    	struct __llist *pnext;	\
    	struct __llist *pprev;	\
    }

#endif

//	Queue node (sorted list, 1st item is short priority)
#if !defined(__SC__) && !defined(__cplusplus)

typedef struct _queue {
	struct _queue *pnext;	// ptr to next node or NULL if at tail
	struct _queue *pprev;	// ptr to prev node or NULL if at head
	short priority;			// higher numbers go to head of list
	short _padTo12Bytes;
									// real data follows, here
} queue;

#else

typedef struct __queue {
	struct __queue *pnext;	// ptr to next node or NULL if at tail
	struct __queue *pprev;	// ptr to prev node or NULL if at head
	short priority;			// higher numbers go to head of list
	short _padTo12Bytes; 
									// real data follows, here
} queue;

#define _queue \
    { \
    	struct __queue *pnext; \
    	struct __queue *pprev; \
    	short priority; \
    	short _padTo12Bytes; \
    }

#endif

//	List header
#if !defined(__SC__) && !defined(__cplusplus)

typedef struct _llist_head {
	llist head;					// head list item (not really in list)
	llist tail;					// tail list item (not really in list)
} llist_head;

#else

typedef struct __llist_head {
	llist head;					// head list item (not really in list)
	llist tail;					// tail list item (not really in list)
} llist_head;

#define _llist_head \
    { \
	llist head; \
	llist tail; \
    }

#endif

//	Initialize a list header (must be done before use)

#define llist_init(plh) { \
	(plh)->head.pnext = llist_end(plh); \
	(plh)->tail.pprev = llist_beg(plh); \
	}

//	Add a new list node to head of list

#define llist_add_head(plh,pll) { \
	(pll)->pnext = llist_head(plh); \
	(pll)->pprev = llist_beg(plh); \
	((plh)->head.pnext)->pprev = (void *) pll; \
	(plh)->head.pnext = (void *) pll; \
	}

//	Add a new list node to tail of list

#define llist_add_tail(plh,pll) { \
	(pll)->pnext = llist_end(plh); \
	(pll)->pprev = llist_tail(plh); \
	((plh)->tail.pprev)->pnext = (void *) pll; \
	(plh)->tail.pprev = (void *) pll; \
	}

//	Insert before specified node

#define llist_insert_before(pll,pnode) { \
	(pll)->pprev = (void *) (pnode)->pprev; \
	(pll)->pnext = (void *) pnode; \
	(pnode)->pprev->pnext = (void *) pll; \
	(pnode)->pprev = (void *) pll; \
	}

//	Insert after specified node

#define llist_insert_after(pll,pnode) { \
	(pll)->pprev = (void *) pnode; \
	(pll)->pnext = (void *)(pnode)->pnext; \
	(pnode)->pnext->pprev = (void *) pll; \
	(pnode)->pnext = (void *) pll; \
	}

//	Add in priority order

void llist_insert_queue(llist_head *plh, queue *plq);

//	Move to new spot in queue (after inserting new priority)

bool llist_move_queue(llist_head *plh, queue *plq);

//	Remove node

#define llist_remove(pll) { \
	((pll)->pprev)->pnext = (pll)->pnext; \
	((pll)->pnext)->pprev = (pll)->pprev; \
	}

//	Get ptr to head or tail list nodes

#define llist_head(plh) (void*)((plh)->head.pnext)
#define llist_tail(plh) (void*)((plh)->tail.pprev)

//	Determine if list empty

#define llist_empty(plh) (llist_head(plh)==llist_end(plh))

//	Get # nodes in list

int llist_num_nodes(llist_head *plh);

//	Get next & prev nodes from a node

#define llist_next(pll) (void*)((pll)->pnext)		// get ptr to next node
#define llist_prev(pll) (void*)((pll)->pprev)		// get ptr to prev node

//	Get beginning and end items (used to check when traversing)

#define llist_beg(plh) (void*)(&((plh)->head))
#define llist_end(plh) (void*)(&((plh)->tail))

//	Iterate across all items from head to tail

#define forallinlist(listtype,plh,pll) \
   for (pll = (listtype *) llist_head(plh); \
        pll != llist_end(plh); \
        pll = (listtype *) llist_next(pll))

//	Iterate across all items from tail to head

#define forallinlistrev(listtype,plh,pll) \
   for (pll = (listtype *) llist_tail(plh); \
        pll != llist_beg(plh); \
        pll = (listtype *) llist_prev(pll))

//	--------------------------------------------------------------
//		HIGH-LEVEL LINKED LIST OF FIXED SIZE ITEMS (MANAGES STORAGE)
//	--------------------------------------------------------------

//	List header

typedef struct {
	struct _llist_head;		// llist header (head, tail, numnodes)
	llist *pfree;				// ptr to next free element or NULL if no more
	llist *pNodeStore;		// ptr to first node store block, they're linked
									// (node store is list ptrs followed by data block)
	ushort nodeSize;			// size of each node
	short numNodesPerBlock;	// # nodes in list storage (including free ones)
} LlistHead;

//	Forgive the void pointers, C-- sucks

void LlistInit(LlistHead *plh, ushort nodeSize, short numNodesPerBlock);
void *LlistAddHead(LlistHead *plh);			// add 1st free to head, return ptr
void *LlistAddTail(LlistHead *plh);			// add 1st free to tail, return ptr
void *LlistAddQueue(LlistHead *plh, short prior);	// add in priority order
bool LlistMoveQueue(LlistHead *plh, void *pnode, short newprior);	// move pri
void LlistFree(LlistHead *plh, void *pnode);		// free node
void LlistFreeAll(LlistHead *plh);					// free all nodes
void LlistDestroy(LlistHead *plh);			// destroy list, reclaim storage

#define LlistHead(plh) (llist_head(plh))				// get ptr to head
#define LlistTail(plh) (llist_tail(plh))				// get ptr to tail
#define LlistFirstFree(plh) ((plh)->pfree)			// get ptr to first free

#define LlistEmpty(plh) (llist_empty(plh))			// determine if list empty
#define LlistNumNodes(plh) (llist_num_nodes((llist_head *) plh))	// # active

#define LlistNext(pll) (llist_next(pll))		// get ptr to next node
#define LlistPrev(pll) (llist_prev(pll))		// get ptr to prev node

#define LlistBeg(plh) (llist_beg(plh))			// beginning of list
#define LlistEnd(plh) (llist_end(plh))			// end of list

#define FORALLINLIST(listtype,plh,pll) forallinlist(listtype,plh,pll)
#define FORALLINLISTREV(listtype,plh,pll) forallinlistrev(listtype,plh,pll)


#ifdef __cplusplus
}
#endif

#endif


