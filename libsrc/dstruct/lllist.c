//		lllist.C		Basic doubly-linked list
//		Rex E. Bradford (REX)
//
//		This module contains the few routines associated with the simple
//		llist structure which are not macros.  These include:
//
//		llist_insert_queue() - insert a node into a sorted list
//		llist_move_queue()   - move a node within a sorted list
//		llist_num_nodes()    - count # nodes in a list
//
//		See llist.h for the rest of the functionality, or llist.txt
//		for documentation on how to use llist's.
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/lllist.c 1.3 1996/10/21 09:35:11 TOML Exp $
* $log$
*/

#include <llist.h>

//	----------------------------------------------------------
//		LITTLE LINKED LIST ROUTINES (SEE MACROS IN LLIST.H TOO)
//	----------------------------------------------------------
//
//	llist_insert_queue() inserts a new queue item into list.

void llist_insert_queue(llist_head *plh, queue *plq)
{
	queue *pxx;

//	Point to element past new one in priority

	pxx = (queue *) llist_head(plh);
	while ((pxx != llist_end(plh)) && (plq->priority <= pxx->priority))
		pxx = pxx->pnext;

//	Patch us in before this element

	llist_insert_before(plq,pxx);
}

//	---------------------------------------------------------
//
//	llist_move_queue() moves a queue item after inserting new priority.

bool llist_move_queue(llist_head *plh, queue *plq)
{
	bool moved;

//	See if priority warrants moving the queue node

	moved = FALSE;
	if ((plq->pprev != llist_beg(plh)) && (plq->priority > plq->pprev->priority))
		moved = TRUE;
	else if ((plq->pnext != llist_end(plh)) && (plq->priority < plq->pnext->priority))
		moved = TRUE;

//	Yes, detach from queue and re-insert

	if (moved)
		{
		llist_remove(plq);
		llist_insert_queue(plh, plq);
		}

//	Report whether item had to be moved or not

	return(moved);
}

//	----------------------------------------------------------
//
//	llist_num_nodes() counts # nodes in list.
//
//		plh = ptr to list header
//
//	Returns: # nodes in list

int llist_num_nodes(llist_head *plh)
{
	llist *pll;
	int num;

	num = 0;
	forallinlist(llist,plh,pll)
		++num;

	return(num);
}

