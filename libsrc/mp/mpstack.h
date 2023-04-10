//
// mpstack.h -- motion stack stuff. Memory management for multiped motion list
// and stack used to evaluate it.
//

#ifndef MPSTACK_H
#define MPSTACK_H

//

#include <motion.h>

//
// The motions being applied to a multiped form a stack, each element of which
// can be an individual motion or a transition between two motions. Think of 
// it as a Reverse Polish Notation evaluator, where a motion is atomic and a
// transition is an operator with two arguments, each of which might be 
// another transition. So evaluating the motion stack is an inherently 
// recursive process, just like evaluating an RPN stack.
//

typedef enum
{
   MN_MOTION,
   MN_TRANSITION
} MotionNodeType;

//
// To use the following structures: starting with a pointer to
// mps_stack_node, look at the type. If it's MN_MOTION, cast the pointer
// to mps_motion_node. If it's MN_TRANSITION, cast to mps_transition_node.
// Should properly be done through inheritance. O HOW I MISS C++.
//

typedef struct mps_stack_node
{
   struct mps_stack_node *	prev;
   struct mps_stack_node *	next;
   MotionNodeType			type;
} mps_stack_node;

//

#define MP_KILL_FLAG	0x80000000
#define MP_END_FLAG		0x40000000

#define MP_MOTNUM_FROM_NODE_HANDLE(handle) ((handle)&~(MP_KILL_FLAG|MP_END_FLAG))

//

typedef struct mps_motion_node
{
   mps_stack_node *prev;
   mps_stack_node *next;
   MotionNodeType type;
   float time;
   int handle;
   int start_frame;
   float frame;
   mps_motion_param params;
   mxs_vector base_pos;
   quat base_orient;

   // Offset is used when not starting at the beginning of the motion.
   mxs_vector offset;
   motion_callback callback;
   int callback_num;
   int callback_frame;

   quat *targets;

   // It gets bigger and bigger...
   int flag_index;

   // And bigger.
} mps_motion_node;

//

typedef struct mps_transition_node
{
   mps_stack_node *prev;
   mps_stack_node *next;
   MotionNodeType	type;
   float			time;
   float			duration;
} mps_transition_node;

//
// This can probably be safely reduced to 8 or even less. It's all a matter of
// how the app deals with starting motions. As long as nobody ever gets into a
// tight loop starting new motions on a multiped, you'll probably have a
// maximum of maybe 4 motions and 3 transitions going at once.
//
#define MAX_STACK_SIZE	16

//

typedef struct mps_motion_stack
{
   mps_stack_node *list[MAX_STACK_SIZE];
   int				index;
} mps_motion_stack;

//

void mp_stack_init(mps_motion_stack * stack);
void mp_stack_push(mps_motion_stack * stack, mps_stack_node * node);
mps_stack_node * mp_stack_pop(mps_motion_stack * stack);

void mp_dump_stack(mps_motion_stack * stack);

//

#endif


