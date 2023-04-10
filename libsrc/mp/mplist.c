//
// mplist.c - Multiped motion list management.
//

#include <multiped.h>


extern mps_motion_state_callback mp_motion_start_callback;
extern mps_motion_state_callback mp_motion_end_callback;

//

void mp_list_init(mps_motion_list * list)
{
	list->count = 0;
	list->head = NULL;
	list->tail = NULL;
	list->flags = 0;
}

//

void mp_list_free(mps_motion_list * list)
{
	mps_stack_node * node = list->head, * temp;

	while (node)
	{
		temp = node->next;
		if (node->type == MN_MOTION)
		{
			mps_motion_node * m = (mps_motion_node *) node;
			if (m->targets)
			{
				mp_free(m->targets);
				m->targets = NULL;
			}
         if(mp_motion_end_callback)
         {
            int mnum=MP_MOTNUM_FROM_NODE_HANDLE(m->handle);

            // check motion is not a pose
            if(mnum!=MP_MOTNUM_FROM_NODE_HANDLE(-1))
               mp_motion_end_callback(mnum);
         }
		}

		mp_free(node);
		node = temp;
	}

	list->head = NULL;
	list->tail = NULL;
	list->count = 0;
}

//

void mp_list_add(mps_motion_list * list, mps_stack_node * node)
{
   if(node->type==MN_MOTION && mp_motion_start_callback)
   {
      mps_motion_node *m=(mps_motion_node *)node;
      int mnum=MP_MOTNUM_FROM_NODE_HANDLE(m->handle);

      // check motion is not a pose
      if(mnum!=MP_MOTNUM_FROM_NODE_HANDLE(-1))
         mp_motion_start_callback(mnum);
   }
	if (list->tail)
	{
		list->tail->next = node;
		node->prev = list->tail;
		node->next = NULL;
		list->tail = node;
	}
	else
	{
	// Empty list.
		list->head = 
		list->tail = node;
		node->prev = 
		node->next = NULL;
	}

	list->count++;
}

//

void mp_list_remove(mps_motion_list * list, mps_stack_node * node)
{
	mps_stack_node * p = node->prev;
	mps_stack_node * n = node->next;

   if(node->type==MN_MOTION && mp_motion_end_callback)
   {
      mps_motion_node *m=(mps_motion_node *)node;
      int mnum=MP_MOTNUM_FROM_NODE_HANDLE(m->handle);

      // check motion is not a pose
      if(mnum!=MP_MOTNUM_FROM_NODE_HANDLE(-1))
         mp_motion_end_callback(mnum);
   }
	if (p)
	{
		p->next = n;
	}
	else
	{
	// Node is head of list.		
		list->head = n;
	}

	if (n)
	{
		n->prev = p;
	}
	else
	{
	// Node is tail of list.
		list->tail = p;
	}

	if (node->type == MN_MOTION)
	{
		mps_motion_node * m = (mps_motion_node *) node;
		if (m->targets)
		{
			mp_free(m->targets);
			m->targets = NULL;
		}
	}

	mp_free(node);

	list->count--;
}

//

mps_stack_node * mp_list_traverse(mps_motion_list * list, mps_stack_node * node)
{
	mps_stack_node * result;
	if (node)
	{
		result = node->next;
	}
	else
	{
		result = list->head;
	}
	return result;
}

//

BOOL mp_list_clone(mps_motion_list *dst, mps_motion_list *src)
{
   mps_stack_node *node=NULL,*newnode;

   mp_list_init(dst);

   while(node=mp_list_traverse(src,node))
   {
      if(node->type==MN_MOTION)
      {
         if(!(newnode=mp_alloc(sizeof(mps_motion_node),__FILE__, __LINE__)))
         {
            mp_list_free(dst);
            return FALSE;
         }
         memcpy(newnode,node,sizeof(mps_motion_node));
         if(((mps_motion_node *)node)->targets)
         {
            ((mps_motion_node *)newnode)->targets=Malloc(MSize(((mps_motion_node *)node)->targets)); // XXX this may copy more than necessary
            memcpy(((mps_motion_node *)newnode)->targets,((mps_motion_node *)node)->targets,MSize(((mps_motion_node *)node)->targets));
         }
      } else
      {
         if(!(newnode=mp_alloc(sizeof(mps_transition_node),__FILE__, __LINE__)))
         {
            mp_list_free(dst);
            return FALSE;
         }
         memcpy(newnode,node,sizeof(mps_transition_node));
      }
      mp_list_add(dst,newnode);
   }
   return TRUE;
}
