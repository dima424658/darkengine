//
//
//

#ifndef MPLIST_H
#define MPLIST_H

//

#include <mpstack.h>

//

typedef struct mps_motion_list
{
	uint			signature;
	int				count;
	mps_stack_node *head;
	mps_stack_node *tail;
	uint			flags;
} mps_motion_list;

//

EXTERN void mp_list_init(mps_motion_list * list);
EXTERN void mp_list_free(mps_motion_list * list);
EXTERN BOOL mp_list_clone(mps_motion_list *dst, mps_motion_list *src);

EXTERN void mp_list_add(mps_motion_list * list, mps_stack_node * node);
EXTERN void mp_list_remove(mps_motion_list * list, mps_stack_node * node);

#define mp_list_empty(list)	((list)->count == 0)
#define mp_list_count(list)	((list)->count)

EXTERN mps_stack_node * mp_list_traverse(mps_motion_list * list, mps_stack_node * node);

//

#endif

